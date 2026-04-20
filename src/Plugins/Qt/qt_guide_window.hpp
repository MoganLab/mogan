/******************************************************************************
 * MODULE     : qt_guide_window.hpp
 * DESCRIPTION: IntelliJ IDEA style startup login dialog
 * COPYRIGHT  : (C) 2025 yinyu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef STARTUP_LOGIN_DIALOG_H
#define STARTUP_LOGIN_DIALOG_H

#include <QEvent>
#include <QPoint>
#include <QPointer>
#include <QPropertyAnimation>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_WIN)
#include <QWKWidgets/widgetwindowagent.h>
#endif

namespace QWK {

class StartupLoginDialog : public QDialog {
  Q_OBJECT

public:
  enum Result {
    LoginClicked,
    SkipClicked,
    DialogRejected,
    InitializationComplete
  };

  explicit StartupLoginDialog (QWidget* parent= nullptr);
  ~StartupLoginDialog ();

  static bool shouldShow ();

  void   setAsyncStartupMode (bool enabled);
  Result execWithResult ();
  void   startInitialization ();
  bool   isInitializationComplete () const { return initializationComplete; }

signals:
  void loginRequested ();
  void skipRequested ();
  void initializationStarted ();
  void initializationFinished (bool success);
  void windowReadyForTransition ();

protected:
  void showEvent (QShowEvent* event) override;
  void closeEvent (QCloseEvent* event) override;
  bool eventFilter (QObject* watched, QEvent* event) override;

private:
  void    setupUi ();
  void    setupFramelessWindow ();
  void    setupSignalConnections ();
  QString styleSheet () const;
  void    initializeProgressUi ();
  void    startBackgroundInitialization ();
  void    fadeOutAndClose ();

  // 按钮点击处理
  void handleLoginButtonClick ();
  void handleSkipButtonClick ();

  // 初始化任务信号处理
  void handleProgressUpdate (int step, const QString& message, int percentage);
  void handleTimeEstimationUpdate (qint64 elapsedMs, qint64 estimatedTotalMs);
  void handleInitializationComplete (bool success);
  void handleErrorOccurred (const QString& error);

  // UI状态管理
  void showProgressUI ();
  void hideProgressUI ();
  void updateProgressUI (int percentage, const QString& status,
                         const QString& timeEstimation);
  void enableButtons (bool enabled);
  void installDragHandler (QWidget* widget);
  void resetDragCursor ();
  void ensureMainWindowOverlay ();
  void clearMainWindowOverlay ();
  void syncMainWindowOverlay ();
  void centerOverMainWindow ();

  // autobackup
  bool getAutoBackup ();
  void setAutoBackup (bool autobackup);

  // UI elements
  QLabel*      titleLabel;
  QLabel*      iconLabel;
  QLabel*      subtitleLabel;
  QLabel*      featureLabel1;
  QLabel*      featureLabel2;
  QLabel*      featureLabel3;
  QLabel*      featureLabel4;
  QCheckBox*   autoBackupCheckBox;
  QPushButton* loginButton;
  QPushButton* skipButton;
  QVBoxLayout* mainLayout;
  QVBoxLayout* featureLayout;
  QHBoxLayout* buttonLayout;

  // Progress UI elements
  QProgressBar* progressBar;
  QLabel*       statusLabel;
  QLabel*       timeEstimationLabel;

  // Window management for frameless window
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_WIN)
  QWK::WidgetWindowAgent* windowAgent;
#endif

  // Animation
  QPropertyAnimation* fadeAnimation;

  // Dialog result
  Result result;

  // Initialization state
  bool initializationInProgress;
  bool initializationComplete;
  bool userChoiceMade;

  // Manual dragging for the frameless startup dialog
  bool              dragInProgress;
  QPoint            dragOffset;
  bool              asyncStartupMode;
  QPointer<QWidget> mainWindowOverlayHost;
  QPointer<QWidget> mainWindowOverlay;
};

} // namespace QWK

#endif // STARTUP_LOGIN_DIALOG_H
