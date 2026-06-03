
/******************************************************************************
 * MODULE     : qt_tm_widget.hpp
 * DESCRIPTION: The main TeXmacs input widget and its embedded counterpart.
 * COPYRIGHT  : (C) 2008  Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_TM_WIDGET_HPP
#define QT_TM_WIDGET_HPP

#include "list.hpp"

#include "qt_simple_widget.hpp"
#include "qt_widget.hpp"
#include "qt_window_widget.hpp"

#include "QTMAuxiliaryWidget.hpp"
#include "QTMInteractiveInputHelper.hpp"
#include "QTMScrollView.hpp"
#include "QTMTabPage.hpp"
#include "QTMWidget.hpp"
#include "qt_chat_tab_widget.hpp"

#include <QLayout>
#include <QMainWindow>
#include <QSettings>
#include <QStackedWidget>

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_WIN)
#include "../QWindowKit/loginbutton.hpp"
#include "../QWindowKit/logindialog.hpp"
#include "../QWindowKit/notificationbar.hpp"
#include "../QWindowKit/windowbar.hpp"
#include "../QWindowKit/windowbutton.hpp"
#include <QWKWidgets/widgetwindowagent.h>
#endif

class QLabel;
class QToolBar;
class QTMInteractivePrompt;
class PDFReaderWidget;
class PdfToolBar;

/*! Models one main window with toolbars, an associated view, etc.

 The underlying QWidget is a QTMWindow, whose central widget is a QWidget
 holding the extra toolbars and the canvas for the open buffer. Each canvas
 is of type QTMWidget and belongs to one qt_simple_widget_rep.
 */
class qt_tm_widget_rep : public qt_window_widget_rep {

  /*
   enum {
   header_visibility        =  1, // all toolbars
   main_toolbar_visibility  =  2,
   mode_toolbar_visibility  =  4,
   focus_toolbar_visibility =  8,
   user_toolbar_visibility  = 16,
   footer_visibility        = 32,
   side_tools_0_visibility  = 64,
   side_tools_1_visibility  = 128,
   bottom_tools_visibility  = 256,
   extra_tools_visibility   = 512,
   tab_tools_visibility     = 1024
   } visibility_t;
   */
  QLabel*                 rightLabel;
  QLabel*                 leftLabel;
  QLabel*                 middleLabel;
  QToolBar*               menuToolBar;
  QToolBar*               mainToolBar;
  QToolBar*               modeToolBar;
  QToolBar*               focusToolBar;
  QToolBar*               userToolBar;
  PdfToolBar*             pdfToolBar; ///< PDF 阅读器工具栏
  QDockWidget*            sideTools;
  QDockWidget*            leftTools;
  QDockWidget*            bottomTools;
  QDockWidget*            extraTools;
  QDockWidget*            chatSideDock; ///< AI 聊天侧边栏 Dock
  QTMTabPageContainer*    tabPageContainer;
  QTMAuxiliaryWidget*     auxiliaryWidget;
  QWK::WidgetWindowAgent* windowAgent;
  QWK::NotificationBar*   scmNotificationBar; // SCM 提示条
  QWK::LoginButton*       loginButton;
  QPushButton*            vipButton;
  QWK::LoginDialog*       m_loginDialog;
  QLabel*                 avatarLabel;
  QLabel*                 nameLabel;
  QLabel*                 accountIdLabel;
  QLabel*                 membershipPeriodLabel;
  QLabel*                 membershipTitleLabel;
  QPushButton*            loginActionButton;
  QPushButton*            logoutButton;
  QPushButton* chatSidebarToggleBtn; ///< 文档区域右上角的新建对话浮动按钮

  // 更新提示区域控件
  QWidget*     m_updateSection     = nullptr;
  QLabel*      m_updateTitleLabel  = nullptr;
  QPushButton* m_updateNowButton   = nullptr;
  QPushButton* m_snoozeButton      = nullptr;
  bool         m_hasUpdateAvailable= false;

#ifdef Q_OS_MAC
  QToolBar* dumbToolBar;
  QAction*  modeToolBarAction;
  QAction*  mainToolBarAction;
  QWidget*  rulerWidget;
#endif

  QTMInteractiveInputHelper helper;
  QTMInteractivePrompt*     prompt;
  qt_widget                 int_prompt;
  qt_widget                 int_input;

  bool    visibility[12];
  bool    full_screen;
  bool    is_presentation;
  bool    menuToolBarVisibleCache;
  bool    titleBarVisibleCache;
  QString m_userId;
  QString m_memberType;
  QString m_currentScmNotificationItem;

private:
  void onAddTabRequested ();
  void setupLoginDialog (QWK::LoginDialog* loginDialog);
  void checkLocalTokenAndLogin ();
  void fetchUserInfo (const QString& token, bool showDialog= true);
  void refreshLoginDialogPlacement ();
  bool shouldShowLoginDialogUpdateSection ();
  void setLoginDialogUpdateSectionVisible (bool visible);
  void refreshMembershipInfoInBackground ();
  void refreshScmNotificationBar ();
  void syncScmUpdateNotification (bool           updateAvailable,
                                  const QString& remoteVersion= QString ());
  void syncScmGuestNotification (bool visible);
  void
       syncScmMembershipNotification (bool           hasData,
                                      const QString& memberType = QString (),
                                      const QString& periodLabel= QString (),
                                      const QString& periodLabelColor= QString (),
                                      const QString& productType= QString ());
  void triggerOAuth2 ();
  void updateLoginButtonState (bool           isLoggedIn,
                               const QString& displayName= QString ());
  void updateDialogContent (bool isLoggedIn, const QString& username,
                            const QString& email, const QString& avatarText,
                            const QString& memberType,
                            const QString& periodLabel,
                            const QString& periodLabelColor,
                            const QString& productType);
  void showNotLoggedInDialog (const QString& errorMessage);
  void updateVipButtonVisibility (bool isLoggedIn, const QString& memberType);
  void logout ();
  void sync_chat_sidebar_mode ();
  void position_chat_sidebar_button ();
  void set_central_widget_updates_frozen (bool frozen);

  // Version update notification
  void    checkVersionUpdate ();
  QString parseVersionFromTM (const QByteArray& data);
  bool    isVersionNewer (const QString& remote, const QString& local);

  qt_widget main_widget;
  qt_widget main_menu_widget;
  qt_widget waiting_main_menu_widget;
  qt_widget main_icons_widget;
  qt_widget mode_icons_widget;
  qt_widget focus_icons_widget;
  qt_widget user_icons_widget;
  qt_widget side_tools_widget;
  qt_widget left_tools_widget;
  qt_widget bottom_tools_widget;
  qt_widget extra_tools_widget;
  qt_widget tab_bar_widget;
  qt_widget notification_bar_widget;
  qt_widget auxiliary_widget;
  qt_widget dock_window_widget;   // trick to return correct widget position
  QWidget*  startupContentWidget; ///\< 启动标签页模式下显示的控件。
  QWidget*
       chatContentWidget; ///\< 聊天标签页模式下显示的控件（QTChatTabWidget）。
  bool startupTabMode;    ///\< 启动标签页视图是否激活。
  PDFReaderWidget* pdfViewerWidget;   ///\< PDF 标签页模式下的阅读器控件。
  bool             pdfTabMode;        ///\< PDF 阅读器标签页是否激活。
  QString          currentPdfPath;    ///\< 当前显示的 PDF 路径。
  QString          lastLoadedPdfPath; ///\< 上次加载的 PDF 路径。
  bool             chatTabMode;       ///\< 聊天标签页视图是否激活。
  bool             chatSidebarMode;   ///\< AI 聊天侧边栏模式是否激活。
  bool   chatSidebarModeMemory_;      ///\< 记忆用户主动设置的侧边栏模式状态。
  bool   centralWidgetUpdatesFrozen_; ///\< 标签切换期间冻结编辑区更新。
  string currentEditorFile;           ///\< 当前编辑器打开的文件路径。

public:
  qt_tm_widget_rep (int mask, command _quit);
  ~qt_tm_widget_rep ();

  /**
   * @brief 判断新建标签页前是否需要把 current view 切回主窗口默认 view。
   *
   * 当焦点位于 AI Chat 输入框等非默认 view 时，顶部标签栏 “+” 的新建命令
   * 需要先恢复到所属主窗口的默认 view，否则 `(new-document)` 可能在错误的
   * view 上执行或直接失败。此逻辑提取为静态方法，便于单元测试。
   *
   * @param currentView   当前全局 current view
   * @param currentWindow 当前 view 关联的 window（可为空）
   * @param ownerWindow   触发新建操作的主窗口
   * @return 需要切回主窗口默认 view 时返回 true
   */
  static bool shouldResetCurrentViewForNewTab (url currentView,
                                               url currentWindow,
                                               url ownerWindow);

  virtual widget plain_window_widget (string name, command quit, int b);

  virtual void     send (slot s, blackbox val);
  virtual blackbox query (slot s, int type_id);
  virtual widget   read (slot s, blackbox index);
  virtual void     write (slot s, blackbox index, widget w);

  void        set_full_screen (bool flag);
  void        update_visibility ();
  void        install_main_menu ();
  static void tweak_iconbar_size (QSize& sz);
  void        openRenewalPage ();
  void        checkNetworkAvailable ();
  void        sync_startup_tab_mode ();
  /**
   * @brief 同步聊天标签页控件的可见性。
   *
   * 当 \ref chatTabMode 为 true 时，隐藏编辑器并显示
   * \ref chatContentWidget（按需创建）。
   * 否则隐藏聊天控件并恢复编辑器。
   */
  void sync_chat_tab_mode ();

  friend class QTMInteractiveInputHelper;

protected:
  ////// Convenience methods to access our QWidgets

  QMainWindow*   mainwindow () { return qobject_cast<QMainWindow*> (qwid); }
  QWidget*       centralwidget () { return mainwindow ()->centralWidget (); }
  QTMScrollView* scrollarea () {
    return qobject_cast<QTMScrollView*> (main_widget->qwid);
  }
  QTMWidget* canvas () { return qobject_cast<QTMWidget*> (main_widget->qwid); }
};

//! List of widgets wanting to install their menu bar
extern list<qt_tm_widget_rep*> waiting_widgets;

//! Positive means the menu is busy.
extern int menu_count;

/*! A simple texmacs input widget.

 This is a stripped down version of qt_tm_widget_rep, whose underlying widget
 isn't a QTMWindow anymore, but a regular QTMWidget because it is intended to be
 embedded somewhere else.

*/
class qt_tm_embedded_widget_rep : public qt_widget_rep {
  widget main_widget;

public:
  command quit;

  qt_tm_embedded_widget_rep (command _quit);

  virtual void     send (slot s, blackbox val);
  virtual blackbox query (slot s, int type_id);
  virtual widget   read (slot s, blackbox index);
  virtual void     write (slot s, blackbox index, widget w);

  virtual QWidget*     as_qwidget ();
  virtual QLayoutItem* as_qlayoutitem ();
};

#endif // QT_TM_WIDGET_HPP
