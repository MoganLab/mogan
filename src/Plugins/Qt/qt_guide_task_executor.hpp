/******************************************************************************
 * MODULE     : qt_guide_task_executor.hpp
 * DESCRIPTION: Single-threaded initialization task executor for Mogan bootstrap
 * COPYRIGHT  : (C) 2025 yinyu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef BOOTSTRAP_TASK_EXECUTOR_H
#define BOOTSTRAP_TASK_EXECUTOR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>

/**
 * @class BootstrapTaskExecutor
 * @brief Single-threaded task executor for performing Mogan initialization
 * tasks
 *
 * This executor performs initialization tasks in the main thread while
 * keeping the UI responsive through task segmentation. It provides
 * progress updates and handles initialization failures gracefully.
 */
class BootstrapTaskExecutor : public QObject {
  Q_OBJECT

public:
  enum class TaskStep {
    NotStarted,
    FileSystemCheck,
    ConfigurationLoad,
    FontInitialization,
    StdDrdInitialization,
    Complete
  };

  explicit BootstrapTaskExecutor (QObject* parent     = nullptr,
                                  bool     chineseMode= true);
  ~BootstrapTaskExecutor ();

  void    setChineseMode (bool enabled) { m_chineseMode= enabled; }
  bool    isChineseMode () const { return m_chineseMode; }
  QString trText (const QString& en, const QString& zh) const;

  /**
   * @brief Start the initialization tasks
   *
   * This method starts the initialization process in the main thread.
   * Tasks are executed in small segments to keep the UI responsive.
   */
  void start ();

  /**
   * @brief Request cancellation of initialization
   *
   * This method signals the executor to stop gracefully.
   * The executor will finish its current task segment and then stop.
   */
  void cancel ();

  /**
   * @brief Get the current progress percentage
   * @return Progress percentage from 0 to 100
   */
  int progress () const { return m_progress; }

  /**
   * @brief Check if initialization is currently running
   * @return true if initialization is in progress
   */
  bool isRunning () const { return m_running; }

  /**
   * @brief Check if initialization was completed (successfully or not)
   * @return true if initialization completed
   */
  bool isCompleted () const { return m_completed; }

  /**
   * @brief Check if initialization was successful
   * @return true if initialization completed successfully
   */
  bool isSuccessful () const { return m_success; }

  /**
   * @brief Get the last error message
   * @return Error message if initialization failed, empty string otherwise
   */
  QString errorMessage () const { return m_errorMessage; }

  /**
   * @brief Get the current status message
   * @return Descriptive message of current task
   */
  QString currentStatus () const { return m_currentStatus; }

signals:
  /**
   * @brief Signal emitted when progress updates
   * @param step Current initialization step
   * @param message Descriptive message of current task
   * @param percentage Progress percentage (0-100)
   */
  void progressUpdated (int step, const QString& message, int percentage);

  /**
   * @brief Signal emitted when initialization completes
   * @param success true if initialization succeeded, false otherwise
   */
  void initializationComplete (bool success);

  /**
   * @brief Signal emitted when an error occurs
   * @param error Error message describing the problem
   */
  void errorOccurred (const QString& error);

  /**
   * @brief Signal emitted for time estimation updates
   * @param elapsedMs Time elapsed in milliseconds
   * @param estimatedTotalMs Estimated total time in milliseconds
   */
  void timeEstimationUpdated (qint64 elapsedMs, qint64 estimatedTotalMs);

private slots:
  /**
   * @brief Execute the next segment of initialization tasks
   *
   * This slot is called repeatedly to execute tasks in small segments,
   * allowing the UI to remain responsive.
   */
  void executeNextSegment ();

private:
  // Individual initialization steps
  bool performFileSystemCheck ();
  bool performConfigurationLoad ();
  bool performFontInitialization ();
  bool performStdDrdInitialization ();

  // Progress tracking
  void updateProgress (TaskStep step, const QString& message);
  void updateTimeEstimation ();

  // Time estimation
  qint64 estimateStepTime (TaskStep step) const;

  // Helper methods for error handling and state management
  void handleExecutionStopped ();
  void completeInitialization ();
  void handleUnexpectedStep ();
  void handleStepFailure (const QString& stepMessage);
  void handleException (const QString& error);
  void handleUnknownException ();
  void scheduleNextExecution ();

  // Exception handling wrapper
  bool executeWithExceptionHandling (const std::function<bool ()>& func,
                                     const QString&                errorPrefix);

  // State tracking
  bool     m_running;
  bool     m_cancelled;
  bool     m_completed;
  bool     m_success;
  bool     m_chineseMode;
  int      m_progress;
  QString  m_errorMessage;
  QString  m_currentStatus;
  TaskStep m_currentStep;

  // Time tracking for progress estimation
  qint64          m_startTime;
  qint64          m_stepStartTime;
  QVector<qint64> m_stepDurations;

  // Timer for segmented execution
  QTimer* m_executionTimer;
};

#endif // BOOTSTRAP_TASK_EXECUTOR_H