/******************************************************************************
 * MODULE     : qt_guide_window.cpp
 * DESCRIPTION: IntelliJ IDEA style startup login dialog implementation
 * COPYRIGHT  : (C) 2025
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_guide_window.hpp"
#include "qlabel.h"
#include "qnamespace.h"
#include "qt_guide_task_executor.hpp"
#include "qt_utilities.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QShowEvent>

namespace QWK {

// StartupLoginDialog implementation
void
StartupLoginDialog::setupUi () {
  // 创建标题标签
  titleLabel= new QLabel (qt_translate ("欢迎使用 Liii STEM"),
                          this); // Welcome to Liii STEM
  titleLabel->setAlignment (Qt::AlignCenter);
  titleLabel->setObjectName ("titleLabel");

  // 创建图标标签
  iconLabel= new QLabel (this);
  iconLabel->setPixmap (QIcon (":/app/stem.png").pixmap (128, 128));
  iconLabel->setAlignment (Qt::AlignCenter);
  iconLabel->setObjectName ("iconLabel");

  // 为图标添加阴影效果 - 明显的蓝色光晕
  QGraphicsDropShadowEffect* shadowEffect= new QGraphicsDropShadowEffect (this);
  shadowEffect->setBlurRadius (30); // 更大的模糊半径，光晕更扩散
  shadowEffect->setColor (QColor (0, 122, 255, 180)); // 更深的蓝色，更不透明
  shadowEffect->setOffset (0, 0); // 设置为0，使阴影均匀分布在四周
  iconLabel->setGraphicsEffect (shadowEffect);

  // 创建子标题
  subtitleLabel= new QLabel (
      qt_translate ("免费畅享核心功能，付费升级AI高效体验！"), this);
  subtitleLabel->setAlignment (Qt::AlignCenter);
  subtitleLabel->setObjectName ("subtitleLabel");

  // 创建功能特性标签
  featureLabel1= new QLabel (
      "1. " + qt_translate ("Register now and receive a 7-day membership."),
      this); // Register now and receive a 7-day membership.
  featureLabel2= new QLabel (
      "2. " + qt_translate ("Log in to sync settings and access all features"),
      this); // Log in to sync settings and access all features
  featureLabel3=
      new QLabel ("3. " + qt_translate ("Log in to chat with the AI."),
                  this); // Log in to chat with the AI.
  featureLabel4= new QLabel (
      "4. " + qt_translate ("Log in and enjoy seamless Markdown import."),
      this); // Log in and enjoy seamless Markdown import.

  featureLabel1->setObjectName ("featureLabel");
  featureLabel2->setObjectName ("featureLabel");
  featureLabel3->setObjectName ("featureLabel");
  featureLabel4->setObjectName ("featureLabel");

  // 创建按钮
  loginButton= new QPushButton (qt_translate ("登录"), this); // 登录 Log In
  loginButton->setObjectName ("loginButton");
  loginButton->setDefault (true);

  skipButton=
      new QPushButton (qt_translate ("跳过登录"), this); // 跳过登录 Skip Login
  skipButton->setObjectName ("skipButton");

  // 创建布局
  featureLayout= new QVBoxLayout ();
  featureLayout->setSpacing (4);
  featureLayout->addWidget (featureLabel1);
  featureLayout->addWidget (featureLabel2);
  featureLayout->addWidget (featureLabel3);
  featureLayout->addWidget (featureLabel4);

  buttonLayout= new QHBoxLayout ();
  buttonLayout->addStretch ();
  buttonLayout->addWidget (loginButton);
  buttonLayout->addWidget (skipButton);
  buttonLayout->addStretch ();

  mainLayout= new QVBoxLayout (this);
  mainLayout->addWidget (titleLabel);
  mainLayout->addSpacing (10);
  mainLayout->addWidget (subtitleLabel);
  mainLayout->addSpacing (10);
  mainLayout->addWidget (iconLabel);
  mainLayout->addSpacing (15);
  mainLayout->addLayout (featureLayout);
  mainLayout->addStretch ();
  mainLayout->addLayout (buttonLayout);

  setLayout (mainLayout);

  // 初始化进度UI（初始隐藏）
  initializeProgressUi ();
}

void
StartupLoginDialog::setupFramelessWindow () {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_WIN)
  // 设置无边框窗口管理
  windowAgent= new QWK::WidgetWindowAgent (this);
  windowAgent->setup (this);

  // 设置标题、图标和副标题标签为可拖动区域
  if (windowAgent) {
    if (titleLabel) {
      windowAgent->setHitTestVisible (titleLabel, true);
    }
    if (iconLabel) {
      windowAgent->setHitTestVisible (iconLabel, true);
    }
    if (subtitleLabel) {
      windowAgent->setHitTestVisible (subtitleLabel, true);
    }
  }
#endif
}

void
StartupLoginDialog::setupSignalConnections () {
  // 连接登录按钮点击信号
  connect (loginButton, &QPushButton::clicked, this,
           &StartupLoginDialog::handleLoginButtonClick);

  // 连接跳过按钮点击信号
  connect (skipButton, &QPushButton::clicked, this,
           &StartupLoginDialog::handleSkipButtonClick);
}

QString
StartupLoginDialog::styleSheet () const {
  return QStringLiteral (R"(
        QDialog {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            border-radius: 12px;
        }
        QLabel {
            color: #e0e0e0;
            background-color: transparent;
        }
        QLabel#titleLabel {
            color: #ffffff;
            font-size: 28px;
            font-weight: 700;
            margin: 20px 0 16px 0;
        }
        QLabel#iconLabel {
            margin: 0;
            padding: 0;
        }
        QLabel#subtitleLabel {
            color: #ffcc00;
            font-size: 20px;
            font-weight: 600;
            margin: 0 0 8px 0;
        }
        QLabel#featureLabel {
            color: #cccccc;
            font-size: 14px;
            padding: 4px 0 4px 20px;
            margin: 0;
        }
        QPushButton {
            border: none;
            border-radius: 8px;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: 600;
            min-width: 120px;
        }
        QPushButton#loginButton {
            background-color: #007AFF;
            color: white;
            font-size: 16px;
            font-weight: 600;
            padding: 14px 32px;
        }
        QPushButton#loginButton:hover {
            background-color: #0063d1;
        }
        QPushButton#loginButton:pressed {
            background-color: #0056CC;
        }
        QPushButton#skipButton {
            background-color: transparent;
            color: #a0a0a0;
            border: none;
            font-weight: 500;
        }
        QPushButton#skipButton:hover {
            background-color: rgba(255, 255, 255, 0.05);
            color: #ffffff;
            border-color: #666666;
        }
        QPushButton#skipButton:pressed {
            background-color: rgba(255, 255, 255, 0.1);
        }
        QProgressBar {
            border: 1px solid #444444;
            border-radius: 4px;
            text-align: center;
            background-color: #2a2a2a;
            margin: 10px 40px 10px 40px;
            color: #ffffff;
        }
        QProgressBar::chunk {
            background-color: #007AFF;
            border-radius: 3px;
        }
        QLabel#statusLabel {
            color: #cccccc;
            font-size: 14px;
            margin: 10px 40px 5px 40px;
        }
        QLabel#timeEstimationLabel {
            color: #a0a0a0;
            font-size: 12px;
            margin: 0 40px 20px 40px;
        }
    )");
}

StartupLoginDialog::StartupLoginDialog (QWidget* parent)
    : QDialog (parent), titleLabel (nullptr), iconLabel (nullptr),
      subtitleLabel (nullptr), featureLabel1 (nullptr), featureLabel2 (nullptr),
      featureLabel3 (nullptr), featureLabel4 (nullptr), loginButton (nullptr),
      skipButton (nullptr), mainLayout (nullptr), featureLayout (nullptr),
      buttonLayout (nullptr), progressBar (nullptr), statusLabel (nullptr),
      timeEstimationLabel (nullptr),
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_WIN)
      windowAgent (nullptr),
#endif
      fadeAnimation (nullptr), result (DialogRejected),
      initializationInProgress (false), initializationComplete (false),
      userChoiceMade (false) {

  // 设置无边框窗口标志
  setWindowFlags ((windowFlags () | Qt::FramelessWindowHint) &
                  ~Qt::WindowContextHelpButtonHint);

  // 设置窗口图标
  setWindowIcon (QIcon (":/app/stem.png"));

  // 固定窗口大小
  setFixedSize (500, 520);
  setWindowTitle (QObject::tr (" "));

  // 设置样式表
  setStyleSheet (styleSheet ());

  // 设置UI
  setupUi ();

  // 设置无边框窗口
  setupFramelessWindow ();

  // 设置信号连接
  setupSignalConnections ();
}

StartupLoginDialog::~StartupLoginDialog () {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_WIN)
  if (windowAgent) {
    delete windowAgent;
    windowAgent= nullptr;
  }
#endif
}

StartupLoginDialog::Result
StartupLoginDialog::execWithResult () {
  result= DialogRejected;
  if (exec () == QDialog::Accepted) {
    return result;
  }
  return DialogRejected;
}

void
StartupLoginDialog::showEvent (QShowEvent* event) {
  QDialog::showEvent (event);

  // 将对话框居中显示在屏幕上
  QRect screenGeometry= QApplication::primaryScreen ()->availableGeometry ();
  move (screenGeometry.center () - rect ().center ());

  // 当对话框显示时自动开始初始化
  if (!initializationInProgress && !initializationComplete) {
    startInitialization ();
  }
}

void
StartupLoginDialog::initializeProgressUi () {
  // 创建进度条
  progressBar= new QProgressBar (this);
  progressBar->setObjectName ("progressBar");
  progressBar->setRange (0, 100);
  progressBar->setValue (0);
  progressBar->setTextVisible (true);
  progressBar->setFormat ("%p%");
  progressBar->setVisible (false); // 初始隐藏

  // 创建状态标签
  statusLabel= new QLabel (qt_translate ("准备初始化..."), this);
  statusLabel->setObjectName ("statusLabel");
  statusLabel->setAlignment (Qt::AlignCenter);
  statusLabel->setVisible (false);

  // 创建时间估算标签
  timeEstimationLabel= new QLabel ("", this);
  timeEstimationLabel->setObjectName ("timeEstimationLabel");
  timeEstimationLabel->setAlignment (Qt::AlignCenter);
  timeEstimationLabel->setVisible (false);

  // 将进度部件添加到主布局（在按钮之前）
  mainLayout->insertWidget (mainLayout->count () - 1, progressBar);
  mainLayout->insertWidget (mainLayout->count () - 1, statusLabel);
  mainLayout->insertWidget (mainLayout->count () - 1, timeEstimationLabel);
}

void
StartupLoginDialog::startInitialization () {
  // 如果初始化已经在进行中或已完成，则直接返回
  if (initializationInProgress || initializationComplete) {
    return;
  }

  initializationInProgress= true;
  initializationComplete  = false;
  userChoiceMade          = false;

  // 显示进度UI
  showProgressUI ();

  // 更新状态
  statusLabel->setText (qt_translate ("正在初始化..."));

  // 开始后台初始化
  startBackgroundInitialization ();

  emit initializationStarted ();
}

void
StartupLoginDialog::startBackgroundInitialization () {
  // 创建并配置引导任务执行器（单线程）
  BootstrapTaskExecutor* executor= new BootstrapTaskExecutor (this);

  // 连接执行器信号到处理函数
  connect (executor, &BootstrapTaskExecutor::progressUpdated, this,
           &StartupLoginDialog::handleProgressUpdate);

  connect (executor, &BootstrapTaskExecutor::timeEstimationUpdated, this,
           &StartupLoginDialog::handleTimeEstimationUpdate);

  connect (executor, &BootstrapTaskExecutor::initializationComplete, this,
           &StartupLoginDialog::handleInitializationComplete);

  connect (executor, &BootstrapTaskExecutor::errorOccurred, this,
           &StartupLoginDialog::handleErrorOccurred);

  // 初始化期间禁用按钮
  enableButtons (false);

  // 在主线程中启动执行器（不是单独的线程）
  executor->start ();
}

void
StartupLoginDialog::handleLoginButtonClick () {
  result        = StartupLoginDialog::LoginClicked;
  userChoiceMade= true;
  emit loginRequested ();

  // 根据初始化状态处理
  if (initializationComplete) {
    // 初始化已完成，淡出并关闭
    fadeOutAndClose ();
  }
  else if (!initializationInProgress) {
    // 初始化尚未开始，现在开始
    startInitialization ();
  }
  // 如果初始化正在进行中，只需等待完成
  // 完成处理程序将调用 fadeOutAndClose()
}

void
StartupLoginDialog::handleSkipButtonClick () {
  result        = StartupLoginDialog::SkipClicked;
  userChoiceMade= true;
  emit skipRequested ();

  // 根据初始化状态处理
  if (initializationComplete) {
    // 初始化已完成，淡出并关闭
    fadeOutAndClose ();
  }
  else if (!initializationInProgress) {
    // 初始化尚未开始，现在开始
    startInitialization ();
  }
  // 如果初始化正在进行中，只需等待完成
  // 完成处理程序将调用 fadeOutAndClose()
}

void
StartupLoginDialog::handleProgressUpdate (int step, const QString& message,
                                          int percentage) {
  progressBar->setValue (percentage);
  statusLabel->setText (message);
}

void
StartupLoginDialog::handleTimeEstimationUpdate (qint64 elapsedMs,
                                                qint64 estimatedTotalMs) {
  if (estimatedTotalMs > 0) {
    qint64  remainingMs = estimatedTotalMs - elapsedMs;
    int     remainingSec= static_cast<int> (remainingMs / 1000);
    QString timeText;

    if (remainingSec > 60) {
      // 分开翻译，避免 %1 被翻译系统错误处理
      QString prefix= qt_translate ("剩余时间: ");
      QString suffix= qt_translate ("分钟");
      timeText=
          prefix + QString::number ((remainingSec + 30) / 60) + " " + suffix;
    }
    else {
      // 分开翻译，避免 %1 被翻译系统错误处理
      QString prefix= qt_translate ("剩余时间: ");
      QString suffix= qt_translate ("秒");
      timeText=
          prefix + QString::number (qMax (remainingSec, 1)) + " " + suffix;
    }
    timeEstimationLabel->setText (timeText);
  }
}

void
StartupLoginDialog::handleInitializationComplete (bool success) {
  initializationInProgress= false;
  initializationComplete  = true;

  if (success) {
    // 初始化成功
    updateProgressUI (100, qt_translate ("初始化完成，注册即送7天会员！"),
                      qt_translate ("准备就绪"));

    // 如果用户已经做出选择，触发过渡
    if (userChoiceMade) {
      fadeOutAndClose ();
    }
    else {
      // 启用按钮并更新UI以供用户选择
      enableButtons (true);
      emit windowReadyForTransition ();
    }
  }
  else {
    // 初始化失败
    updateProgressUI (0, qt_translate ("初始化失败"), qt_translate ("请重试"));

    // 重新启用按钮以便重试（虽然重试功能尚未实现）
    enableButtons (true);
  }

  emit initializationFinished (success);

  // 清理执行器
  QObject* executor= sender ();
  if (executor) {
    executor->deleteLater ();
  }
}

void
StartupLoginDialog::handleErrorOccurred (const QString& error) {
  // 分开翻译，避免 %1 被翻译系统错误处理
  QString prefix= qt_translate ("错误: ");
  statusLabel->setText (prefix + error);
}

void
StartupLoginDialog::showProgressUI () {
  // 显示进度UI
  progressBar->setVisible (true);
  statusLabel->setVisible (true);
  timeEstimationLabel->setVisible (true);

  // 隐藏功能标签并调整间距
  featureLabel1->setVisible (false);
  featureLabel2->setVisible (false);
  featureLabel3->setVisible (false);
  featureLabel4->setVisible (false);
}

void
StartupLoginDialog::hideProgressUI () {
  // 隐藏进度UI
  progressBar->setVisible (false);
  statusLabel->setVisible (false);
  timeEstimationLabel->setVisible (false);

  // 显示功能标签
  featureLabel1->setVisible (true);
  featureLabel2->setVisible (true);
  featureLabel3->setVisible (true);
  featureLabel4->setVisible (true);
}

void
StartupLoginDialog::updateProgressUI (int percentage, const QString& status,
                                      const QString& timeEstimation) {
  progressBar->setValue (percentage);
  statusLabel->setText (status);
  timeEstimationLabel->setText (timeEstimation);
}

void
StartupLoginDialog::enableButtons (bool enabled) {
  loginButton->setEnabled (enabled);
  skipButton->setEnabled (enabled);
}

void
StartupLoginDialog::fadeOutAndClose () {
  // 创建淡出动画
  fadeAnimation= new QPropertyAnimation (this, "windowOpacity");
  fadeAnimation->setDuration (300);
  fadeAnimation->setStartValue (1.0);
  fadeAnimation->setEndValue (0.0);
  fadeAnimation->setEasingCurve (QEasingCurve::OutCubic);

  connect (fadeAnimation, &QPropertyAnimation::finished, this, [this] () {
    accept (); // 以接受状态关闭对话框
  });

  fadeAnimation->start ();
}

void
StartupLoginDialog::closeEvent (QCloseEvent* event) {
  // 处理窗口关闭按钮（X）
  if (initializationInProgress) {
    // 如果初始化正在进行中，阻止关闭
    event->ignore ();
    return;
  }

  result= DialogRejected;
  QDialog::closeEvent (event);
}

} // namespace QWK