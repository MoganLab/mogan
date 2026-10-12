/******************************************************************************
 * MODULE     : qt_guide_task_executor.cpp
 * DESCRIPTION: Single-threaded initialization task executor implementation
 * COPYRIGHT  : (C) 2025 yinyu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_guide_task_executor.hpp"
#include "boot.hpp"
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <functional>
#include <moebius/drd/drd_std.hpp>

// External declarations from init_texmacs.cpp
extern void init_texmacs ();
extern void init_main_paths ();
extern void init_user_dirs ();
extern bool g_startup_login_executed;
extern void acquire_boot_lock ();
extern void init_succession_status_table ();
extern void font_database_load ();
extern void load_user_preferences ();

// Using declarations for Moebius functions
using moebius::drd::init_std_drd;

BootstrapTaskExecutor::BootstrapTaskExecutor (QObject* parent, bool chineseMode)
    : QObject (parent), m_running (false), m_cancelled (false),
      m_completed (false), m_success (false), m_chineseMode (chineseMode),
      m_progress (0), m_errorMessage (""), m_currentStatus (""),
      m_currentStep (TaskStep::NotStarted), m_startTime (0),
      m_stepStartTime (0), m_executionTimer (nullptr) {

  // 初始化步骤持续时间（毫秒）
  // 这些是默认值，实际执行时会根据实际耗时更新
  m_stepDurations.resize (static_cast<int> (TaskStep::Complete) + 1);
  m_stepDurations[static_cast<int> (TaskStep::FileSystemCheck)]  = 500; // 0.5秒
  m_stepDurations[static_cast<int> (TaskStep::ConfigurationLoad)]= 500; // 1秒
  m_stepDurations[static_cast<int> (TaskStep::FontInitialization)]= 2000; // 3秒
  m_stepDurations[static_cast<int> (TaskStep::StdDrdInitialization)]=
      500; // 2秒
}

QString
BootstrapTaskExecutor::trText (const QString& en, const QString& zh) const {
  return m_chineseMode ? zh : en;
}

// Exception handling wrapper implementation
bool
BootstrapTaskExecutor::executeWithExceptionHandling (
    const std::function<bool ()>& func, const QString& errorPrefix) {
  try {
    return func ();
  } catch (const std::exception& e) {
    qWarning () << "BootstrapTaskExecutor: " << errorPrefix
                << "failed:" << e.what ();
    return false;
  } catch (...) {
    qWarning () << "BootstrapTaskExecutor: Unknown error during "
                << errorPrefix;
    return false;
  }
}

BootstrapTaskExecutor::~BootstrapTaskExecutor () {
  cancel ();
  if (m_executionTimer) {
    m_executionTimer->stop ();
    delete m_executionTimer;
  }
}

void
BootstrapTaskExecutor::start () {
  // 如果已经在运行或已完成，则直接返回
  if (m_running || m_completed) {
    return;
  }

  // 重置所有状态
  m_running  = true;
  m_cancelled= false;
  m_completed= false;
  m_success  = false;
  m_progress = 0;
  m_errorMessage.clear ();
  m_currentStatus.clear ();
  m_currentStep  = TaskStep::FileSystemCheck;
  m_startTime    = QDateTime::currentMSecsSinceEpoch ();
  m_stepStartTime= m_startTime;

  qDebug () << "BootstrapTaskExecutor: 在主线程中启动Mogan初始化...";

  // 创建执行计时器（如果不存在）
  if (!m_executionTimer) {
    m_executionTimer= new QTimer (this);
    m_executionTimer->setSingleShot (true);
    connect (m_executionTimer, &QTimer::timeout, this,
             &BootstrapTaskExecutor::executeNextSegment);
  }

  // 启动执行，使用小延迟以允许UI更新
  QTimer::singleShot (10, this, &BootstrapTaskExecutor::executeNextSegment);
}

void
BootstrapTaskExecutor::cancel () {
  if (m_running && !m_cancelled) {
    m_cancelled= true;
    qDebug () << "BootstrapTaskExecutor: Cancellation requested";
  }
}

void
BootstrapTaskExecutor::executeNextSegment () {
  // 检查是否已取消或未运行
  if (m_cancelled || !m_running) {
    handleExecutionStopped ();
    return;
  }

  try {
    // 执行当前步骤
    bool     stepSuccess= false;
    QString  stepMessage;
    TaskStep nextStep= TaskStep::NotStarted;

    // 根据当前步骤执行相应的任务
    switch (m_currentStep) {
    case TaskStep::FileSystemCheck:
      stepSuccess= performFileSystemCheck ();
      stepMessage= trText ("Checking file system...", "检查文件系统...");
      nextStep   = TaskStep::ConfigurationLoad;
      break;

    case TaskStep::ConfigurationLoad:
      stepSuccess= performConfigurationLoad ();
      stepMessage= trText ("Loading configuration...", "加载配置...");
      nextStep   = TaskStep::FontInitialization;
      break;

    case TaskStep::FontInitialization:
      stepSuccess= performFontInitialization ();
      stepMessage= trText ("Initializing fonts...", "初始化字体...");
      nextStep   = TaskStep::StdDrdInitialization;
      break;

    case TaskStep::StdDrdInitialization:
      stepSuccess= performStdDrdInitialization ();
      stepMessage= trText ("Initializing standard DRD...", "初始化标准DRD...");
      nextStep   = TaskStep::Complete;
      break;

    case TaskStep::Complete:
      // 所有步骤完成
      completeInitialization ();
      return;

    default:
      // 不应该到达这里
      handleUnexpectedStep ();
      return;
    }

    // 处理步骤执行结果
    if (stepSuccess) {
      // 步骤成功，更新进度并移动到下一步
      updateProgress (m_currentStep, stepMessage);
      m_currentStep= nextStep;

      // 如果还有下一步，调度下一次执行
      if (m_currentStep != TaskStep::Complete) {
        scheduleNextExecution ();
      }
      else {
        // 所有步骤完成
        completeInitialization ();
      }
    }
    else {
      // 步骤失败
      handleStepFailure (stepMessage);
    }

  } catch (const std::exception& e) {
    handleException (e.what ());
  } catch (...) {
    handleUnknownException ();
  }
}

// ===========================================================================
// 各个初始化步骤的实现
// init_texmacs包含acquire_boot_lock、init_succession_status_table、
// load_user_preferences、font_database_load、init_std_drd五个步骤，目前拆开
// ===========================================================================

bool
BootstrapTaskExecutor::performFileSystemCheck () {
  return executeWithExceptionHandling (
      [this] () -> bool {
        acquire_boot_lock ();            // 获取启动锁，防止重复初始化
        init_succession_status_table (); // 初始化继承状态表
        return true;
      },
      "File system check");
}

bool
BootstrapTaskExecutor::performConfigurationLoad () {
  return executeWithExceptionHandling (
      [this] () -> bool {
        load_user_preferences (); // 加载用户偏好设置
        return true;
      },
      "Perform configuration load");
}

bool
BootstrapTaskExecutor::performFontInitialization () {
  return executeWithExceptionHandling (
      [this] () -> bool {
        font_database_load (); // 初始化字体数据库
        return true;
      },
      "Perform font initialization load");
}

bool
BootstrapTaskExecutor::performStdDrdInitialization () {
  return executeWithExceptionHandling (
      [this] () -> bool {
        init_std_drd (); // 初始化标准DRD（文档关系定义）
        g_startup_login_executed=
            true; // 最后一步设置ture,防止init_texmacs重复执行
        return true;
      },
      "Perform std drd initialization load");
}

void
BootstrapTaskExecutor::updateProgress (TaskStep step, const QString& message) {
  m_currentStep  = step;
  m_currentStatus= message;

  // Calculate progress percentage based on step
  int baseProgress= 0;
  switch (step) {
  case TaskStep::FileSystemCheck:
    baseProgress= 10;
    break;
  case TaskStep::ConfigurationLoad:
    baseProgress= 30;
    break;
  case TaskStep::FontInitialization:
    baseProgress= 60;
    break;
  case TaskStep::StdDrdInitialization:
    baseProgress= 85;
    break;
  case TaskStep::Complete:
    baseProgress= 100;
    break;
  default:
    baseProgress= 0;
  }

  m_progress= baseProgress;

  // Emit progress signal
  emit progressUpdated (static_cast<int> (step), message, m_progress);

  // Update time estimation
  updateTimeEstimation ();

  qDebug () << "BootstrapTaskExecutor: Progress update - Step"
            << static_cast<int> (step) << ":" << message << "(" << m_progress
            << "%)";
}

void
BootstrapTaskExecutor::updateTimeEstimation () {
  qint64 currentTime= QDateTime::currentMSecsSinceEpoch ();
  qint64 elapsed    = currentTime - m_startTime;

  // Store actual duration for current step
  if (m_currentStep > TaskStep::NotStarted &&
      m_currentStep < TaskStep::Complete) {
    if (m_stepStartTime > 0) {
      qint64 stepDuration= currentTime - m_stepStartTime;
      m_stepDurations[static_cast<int> (m_currentStep)]= stepDuration;
    }
    m_stepStartTime= currentTime;
  }

  // Calculate estimated total time based on completed steps
  qint64 estimatedTotal= 0;
  for (int i= 0; i <= static_cast<int> (TaskStep::Complete); ++i) {
    if (i < static_cast<int> (m_currentStep)) {
      // Use actual duration for completed steps
      estimatedTotal+= m_stepDurations[i];
    }
    else {
      // Use estimated duration for remaining steps
      estimatedTotal+= estimateStepTime (static_cast<TaskStep> (i));
    }
  }

  emit timeEstimationUpdated (elapsed, estimatedTotal);
}

qint64
BootstrapTaskExecutor::estimateStepTime (TaskStep step) const {
  int index= static_cast<int> (step);
  if (index >= 0 && index < m_stepDurations.size ()) {
    return m_stepDurations[index];
  }
  return 1000; // 默认1秒
}

// ============================================================================
// 辅助方法：错误处理和状态管理
// ============================================================================

void
BootstrapTaskExecutor::handleExecutionStopped () {
  m_running  = false;
  m_completed= true;
  m_success  = false;
  if (m_cancelled) {
    m_errorMessage=
        trText ("Initialization cancelled by user", "初始化被用户取消");
  }
  emit initializationComplete (false);
}

void
BootstrapTaskExecutor::completeInitialization () {
  m_running  = false;
  m_completed= true;
  m_success  = true;
  updateProgress (TaskStep::Complete,
                  trText ("Initialization complete", "初始化完成"));
  emit initializationComplete (true);
}

void
BootstrapTaskExecutor::handleUnexpectedStep () {
  m_errorMessage= trText (
      QString ("Unexpected step: %1").arg (static_cast<int> (m_currentStep)),
      QString ("意外的步骤: %1").arg (static_cast<int> (m_currentStep)));
  m_running  = false;
  m_completed= true;
  m_success  = false;
  emit errorOccurred (m_errorMessage);
  emit initializationComplete (false);
}

void
BootstrapTaskExecutor::handleStepFailure (const QString& stepMessage) {
  m_running     = false;
  m_completed   = true;
  m_success     = false;
  m_errorMessage= trText (QString ("Step failed: %1").arg (stepMessage),
                          QString ("步骤失败: %1").arg (stepMessage));
  emit errorOccurred (m_errorMessage);
  emit initializationComplete (false);
}

void
BootstrapTaskExecutor::handleException (const QString& error) {
  m_running     = false;
  m_completed   = true;
  m_success     = false;
  m_errorMessage= trText (QString ("Exception: %1").arg (error),
                          QString ("异常: %1").arg (error));
  qWarning () << "BootstrapTaskExecutor: Exception:" << m_errorMessage;
  emit errorOccurred (m_errorMessage);
  emit initializationComplete (false);
}

void
BootstrapTaskExecutor::handleUnknownException () {
  m_running     = false;
  m_completed   = true;
  m_success     = false;
  m_errorMessage= trText ("Unknown exception occurred during initialization",
                          "初始化过程中发生未知异常");
  qWarning () << "BootstrapTaskExecutor: Unknown exception";
  emit errorOccurred (m_errorMessage);
  emit initializationComplete (false);
}

void
BootstrapTaskExecutor::scheduleNextExecution () {
  // 调度下一次执行，保持UI响应性（段之间10ms延迟）
  if (m_executionTimer) {
    m_executionTimer->start (10);
  }
  else {
    // 备用方案：使用singleShot
    QTimer::singleShot (10, this, &BootstrapTaskExecutor::executeNextSegment);
  }
}