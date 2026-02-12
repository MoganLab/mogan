
/******************************************************************************
 * MODULE     : qt_tm_widget.cpp
 * DESCRIPTION: The main TeXmacs input widget and its embedded counterpart.
 * COPYRIGHT  : (C) 2008  Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDockWidget>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMenuBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QResource>
#include <QScreen>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QWindow>

#include "analyze.hpp"
#include "config.h"
#include "scheme.hpp"

#include "qt_gui.hpp"
#include "qt_picture.hpp"
#include "qt_renderer.hpp"
#include "qt_tm_widget.hpp"
#include "qt_utilities.hpp"

#include "QTMGuiHelper.hpp" // needed to connect()
#include "QTMInteractiveInputHelper.hpp"
#include "QTMInteractivePrompt.hpp"
#include "QTMOAuth.hpp"
#include "QTMStyle.hpp" // qtstyle()
#include "QTMTabPage.hpp"
#include "QTMWindow.hpp"
#include "new_view.hpp"
#include "preferences.hpp"
#include "qt_dialogues.hpp"
#include "qt_menu.hpp"
#include "qt_simple_widget.hpp"
#include "qt_window_widget.hpp"
#include "tm_server.hpp"
#include "tm_sys_utils.hpp"
#include "tm_url.hpp"

#include <moebius/data/scheme.hpp>

using moebius::data::scm_quote;

int menu_count= 0; // zero if no menu is currently being displayed
list<qt_tm_widget_rep*> waiting_widgets;

static void
replaceActions (QWidget* dest, QList<QAction*>* src) {
  // NOTE: the parent hierarchy of the actions is not modified while installing
  //       the menu in the GUI (see qt_menu.hpp for this memory management
  //       policy)
  if (src == NULL || dest == NULL)
    TM_FAILED ("replaceActions expects valid objects");
  dest->setUpdatesEnabled (false);
  QList<QAction*> list= dest->actions ();
  for (int i= 0; i < list.count (); i++) {
    QAction* a= list[i];
    dest->removeAction (a);
  }
  for (int i= 0; i < src->count (); i++) {
    QAction* a= (*src)[i];
    dest->addAction (a);
  }
  dest->setUpdatesEnabled (true);
}

static void
replaceButtons (QToolBar* dest, QList<QAction*>* src) {
  if (src == NULL || dest == NULL)
    TM_FAILED ("replaceButtons expects valid objects");
  dest->setUpdatesEnabled (false);
  bool visible= dest->isVisible ();
  if (visible) dest->hide (); // TRICK: to avoid flicker of the dest widget
  replaceActions (dest, src);
  QList<QObject*> list= dest->children ();
  for (int i= 0; i < list.count (); ++i) {
    QToolButton* button= qobject_cast<QToolButton*> (list[i]);
    if (button) {
      button->setPopupMode (QToolButton::InstantPopup);
      if (tm_style_sheet == "") button->setStyle (qtmstyle ());
    }
  }
  if (visible) dest->show (); // TRICK: see above
  dest->setUpdatesEnabled (true);
}

void
QTMInteractiveInputHelper::commit (int result) {
  if (wid && result == QDialog::Accepted) {
    QString    item= "#f";
    QComboBox* cb  = sender ()->findChild<QComboBox*> ("input");
    if (cb) item= cb->currentText ();
    static_cast<qt_input_text_widget_rep*> (wid->int_input.rep)->input=
        from_qstring (item);
    static_cast<qt_input_text_widget_rep*> (wid->int_input.rep)->cmd ();
  }
  sender ()->deleteLater ();
}

/******************************************************************************
 * qt_tm_widget_rep
 ******************************************************************************/

qt_tm_widget_rep::qt_tm_widget_rep (int mask, command _quit)
    : qt_window_widget_rep (new QTMWindow (0), "popup", _quit), helper (this),
      prompt (NULL), full_screen (false), menuToolBarVisibleCache (false),
      titleBarVisibleCache (false), membershipTitleLabel (nullptr),
      m_userId ("") {
  type= texmacs_widget;

  main_widget= concrete (::glue_widget (true, true, 1, 1));

  // decode mask
  visibility[0] = (mask & 1) == 1;       // header
  visibility[1] = (mask & 2) == 2;       // main
  visibility[2] = (mask & 4) == 4;       // mode
  visibility[3] = (mask & 8) == 8;       // focus
  visibility[4] = (mask & 16) == 16;     // user
  visibility[5] = (mask & 32) == 32;     // footer
  visibility[6] = (mask & 64) == 64;     // right side tools
  visibility[7] = (mask & 128) == 128;   // left side tools
  visibility[8] = (mask & 256) == 256;   // bottom tools
  visibility[9] = (mask & 512) == 512;   // extra bottom tools
  visibility[10]= (mask & 1024) == 1024; // tab page bar
  visibility[11]= (mask & 2048) == 2048; // auxiliary widget

#ifdef OS_WASM
  visibility[1]= false; // main
  visibility[2]= false; // mode
  visibility[3]= false; // focus
  visibility[4]= false; // user
  visibility[8]= false;
#endif

  // general setup for main window
  QMainWindow* mw= mainwindow ();
  if (tm_style_sheet == "") mw->setStyle (qtmstyle ());
  // 复用的无边框窗口栏初始化逻辑
  auto setupWindowBar= [this, mw] (QWK::WindowBar*&         outBar,
                                   QWK::WidgetWindowAgent*& outAgent,
                                   int minHeight, bool setSafeArea) {
    outBar          = new QWK::WindowBar ();
    outAgent        = new QWK::WidgetWindowAgent (mw);
    tabPageContainer= new QTMTabPageContainer (outBar);
    // 连接新增标签页按钮信号
    QObject::connect (tabPageContainer, &QTMTabPageContainer::addTabRequested,
                      [this] () { this->onAddTabRequested (); });
    mw->setAttribute (Qt::WA_DontCreateNativeAncestors);
    if (setSafeArea)
      mw->setAttribute (Qt::WA_ContentsMarginsRespectsSafeArea, false);
    outAgent->setup (mw);
    outBar->setHostWidget (mw);
    outBar->setMinimumHeight (minHeight);
    outBar->setTitleWidget (tabPageContainer);
    outBar->adjustSize ();
    outBar->layout ()->setAlignment (tabPageContainer,
                                     Qt::AlignLeft | Qt::AlignVCenter);
    // 让标题栏子项（系统按钮等）垂直贴合高度：去掉间距与边距
    if (outBar->layout ()) {
      outBar->layout ()->setContentsMargins (0, 0, 0, 0);
      outBar->layout ()->setSpacing (0);
    }
    outBar->setObjectName ("windowbar");
    outAgent->setTitleBar (outBar);
    mw->setMenuWidget (outBar);
  };

  QScreen* screen        = QGuiApplication::primaryScreen ();
  double   dpi           = screen ? screen->logicalDotsPerInch () : 96.0;
  double   scale         = dpi / 96.0;
  int      titleBarHeight= int (32 * scale);
  int      buttonWidth   = int (46 * scale);
  int      buttonHeight  = int (32 * scale);
  int      iconBaseSize  = int (12 * scale);
  int      macosiconSize = int (20 * scale);

#if defined(Q_OS_MAC)
  // 无边框布局（macOS）- 只显示登录按钮
  Q_INIT_RESOURCE (styles);
  QWK::WindowBar* windowBar= nullptr;
  windowAgent              = nullptr;
  setupWindowBar (windowBar, windowAgent, /*minHeight*/ 20,
                  /*setSafeArea*/ true);
#elif defined(Q_OS_WIN) || defined(Q_OS_LINUX)
  // 无边框布局（Windows / Linux），并使用 /styles 资源中的图标
  Q_INIT_RESOURCE (styles);
  QWK::WindowBar* windowBar= nullptr;
  windowAgent              = nullptr;
  setupWindowBar (windowBar, windowAgent, /*minHeight*/ titleBarHeight,
                  /*setSafeArea*/ false);
  windowBar->setMinimumHeight (titleBarHeight);
  windowBar->setFixedHeight (titleBarHeight);

  // 系统按钮（图标来自 3rdparty/qwindowkitty/src/styles/styles.qrc）
  auto iconBtn= new QWK::WindowButton (windowBar);
  iconBtn->setFlat (true);
  iconBtn->setFocusPolicy (Qt::NoFocus);
  iconBtn->setIcon (QIcon (":/app/stem.png"));
  iconBtn->setObjectName ("icon-button");
  iconBtn->setIconSize (QSize (int (30 * scale), int (30 * scale)));
  windowBar->setIconButton (iconBtn);
  iconBtn->setAttribute (Qt::WA_TransparentForMouseEvents, true);
  iconBtn->setCursor (Qt::ArrowCursor);
  iconBtn->setContextMenuPolicy (Qt::NoContextMenu);

  auto pinBtn= new QWK::WindowButton (windowBar);
  pinBtn->setCheckable (true);
  pinBtn->setFlat (true);
  pinBtn->setFocusPolicy (Qt::NoFocus);
  pinBtn->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  pinBtn->setFixedSize (buttonWidth, buttonHeight);
  pinBtn->setIconSize (QSize (iconBaseSize, iconBaseSize));
  pinBtn->setIconNormal (QIcon (":/window-bar/pin.svg"));
  pinBtn->setIconChecked (QIcon (":/window-bar/pin-fill.svg"));
  pinBtn->setObjectName ("pin-button");
  pinBtn->setProperty ("system-button", true);
  windowBar->setPinButton (pinBtn);
  if (windowAgent) {
    windowAgent->setHitTestVisible (pinBtn, true);
  }

  auto minBtn= new QWK::WindowButton (windowBar);
  minBtn->setFlat (true);
  minBtn->setFocusPolicy (Qt::NoFocus);
  minBtn->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  minBtn->setFixedSize (buttonWidth, buttonHeight);
  minBtn->setIconSize (QSize (iconBaseSize, iconBaseSize));
  minBtn->setIcon (QIcon (":/window-bar/minimize.svg"));
  minBtn->setObjectName ("min-button");
  minBtn->setProperty ("system-button", true);
  windowBar->setMinButton (minBtn);
  windowAgent->setSystemButton (QWK::WindowAgentBase::Minimize, minBtn);

  auto maxBtn= new QWK::WindowButton (windowBar);
  maxBtn->setCheckable (true);
  maxBtn->setFlat (true);
  maxBtn->setFocusPolicy (Qt::NoFocus);
  maxBtn->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  maxBtn->setFixedSize (buttonWidth, buttonHeight);
  maxBtn->setIconSize (QSize (iconBaseSize, iconBaseSize));
  maxBtn->setIconNormal (QIcon (":/window-bar/maximize.svg"));
  maxBtn->setIconChecked (QIcon (":/window-bar/restore.svg"));
  maxBtn->setObjectName ("max-button");
  maxBtn->setProperty ("system-button", true);
  windowBar->setMaxButton (maxBtn);
  windowAgent->setSystemButton (QWK::WindowAgentBase::Maximize, maxBtn);

  auto closeBtn= new QWK::WindowButton (windowBar);
  closeBtn->setFlat (true);
  closeBtn->setFocusPolicy (Qt::NoFocus);
  closeBtn->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  closeBtn->setFixedSize (buttonWidth, buttonHeight);
  closeBtn->setIconSize (QSize (iconBaseSize, iconBaseSize));
  closeBtn->setIcon (QIcon (":/window-bar/close.svg"));
  closeBtn->setIconHovered (QIcon (":/window-bar/close-white.svg"));
  closeBtn->setObjectName ("close-button");
  closeBtn->setProperty ("system-button", true);
  windowBar->setCloseButton (closeBtn);
  windowAgent->setSystemButton (QWK::WindowAgentBase::Close, closeBtn);

  // 按钮信号连接到窗口行为
  QObject::connect (windowBar, &QWK::WindowBar::minimizeRequested, mw,
                    [mw] () { mw->showMinimized (); });
  QObject::connect (windowBar, &QWK::WindowBar::maximizeRequested, mw,
                    [mw] (bool max) {
                      if (max) {
                        if (mw->isFullScreen ()) {
                          mw->showNormal ();
                        }
                        mw->showMaximized ();
                      }
                      else {
                        mw->showNormal ();
                      }
                    });
  QObject::connect (windowBar, &QWK::WindowBar::closeRequested, mw,
                    &QWidget::close);
  QObject::connect (windowBar, &QWK::WindowBar::pinRequested, mw,
                    [mw, pinBtn] (bool on) {
                      mw->setWindowFlag (Qt::WindowStaysOnTopHint, on);
                      mw->show ();
                      pinBtn->setChecked (on);
                    });
#endif

  // 登录按钮 - 作为最左边的自定义按钮
  loginButton= new QWK::LoginButton (windowBar);

  loginButton->setFlat (true);
  loginButton->setFocusPolicy (Qt::NoFocus);
  loginButton->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  loginButton->setFixedSize (buttonWidth, buttonHeight);
#if defined(Q_OS_MAC)
  loginButton->setIconSize (QSize (macosiconSize, macosiconSize));
#else
  loginButton->setIconSize (QSize (iconBaseSize, iconBaseSize));
#endif
  loginButton->setIconNormal (QIcon (":/window-bar/login.svg"));
  loginButton->setObjectName ("login-button");
  loginButton->setProperty ("system-button", true);
  windowBar->setLoginButton (loginButton);
  if (windowAgent) {
    windowAgent->setHitTestVisible (loginButton, true);
  }

  m_loginDialog= new QWK::LoginDialog (mainwindow ());
  setupLoginDialog (m_loginDialog);
  QObject::connect (loginButton, &QWK::LoginButton::clicked,
                    [this] () { checkLocalTokenAndLogin (); });

  // 初始化访客提示条
  guestNotificationBar= new QWK::GuestNotificationBar (mw);

  // 连接提示条信号
  QObject::connect (guestNotificationBar,
                    &QWK::GuestNotificationBar::loginRequested,
                    [this] () { triggerOAuth2 (); });
  QObject::connect (guestNotificationBar,
                    &QWK::GuestNotificationBar::closeRequested, [this] () {
                      guestNotificationBar->hide ();
                      // 只隐藏当前会话，不保存到设置
                    });

  // 检查是否应该显示提示条
  // 1. 社区版不显示
  // 2. 商业版：用户未登录时显示，用户已登录时不显示
  if (is_community_stem ()) {
    // 社区版：不显示提示条
    guestNotificationBar->hide ();
  }
  else {
    // 商业版：检查用户登录状态（使用和OCR功能相同的判断方法）
    // 初始隐藏，等网络检查完成后再决定是否显示guestNotificationBar->hide ();
    guestNotificationBar->hide ();
    checkNetworkAvailable ();
  }

  // there is a bug in the early implementation of toolbars in Qt 4.6
  // which has been fixed in 4.6.2 (at least)
  // this is why we change dimension of icons

#if (defined(Q_OS_MAC) && (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)) &&         \
     (QT_VERSION < QT_VERSION_CHECK(4, 6, 2)))
  mw->setIconSize (QSize (22, 30));
#else
  mw->setIconSize (QSize (17, 17));
#endif
  mw->setFocusPolicy (Qt::NoFocus);

  // status bar

  QStatusBar* bar= new QStatusBar (mw);
  leftLabel      = new QLabel (qt_translate ("Welcome to TeXmacs"), bar);
  middleLabel    = new QLabel ("", bar);
  rightLabel     = new QLabel (qt_translate ("Booting"), bar);
  leftLabel->setFrameStyle (QFrame::NoFrame);
  middleLabel->setFrameStyle (QFrame::NoFrame);
  rightLabel->setFrameStyle (QFrame::NoFrame);
  leftLabel->setIndent (8);
  middleLabel->setAlignment (Qt::AlignCenter);

  // Set alignment for left and right labels
  leftLabel->setAlignment (Qt::AlignLeft | Qt::AlignVCenter);
  rightLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);

  // Add all three labels with equal stretch factors for equal width
  // distribution
  bar->addWidget (leftLabel, 1);
  bar->addWidget (middleLabel, 1);
  bar->addPermanentWidget (rightLabel, 1);
  if (tm_style_sheet == "") bar->setStyle (qtmstyle ());

  // NOTE (mg): the following setMinimumWidth command disable automatic
  // enlarging of the status bar and consequently of the main window due to
  // long messages in the left label. I found this strange solution here
  // http://www.archivum.info/qt-interest@trolltech.com/2007-05/01453/Re:-QStatusBar-size.html
  // The solution if due to Martin Petricek. He adds:
  //    The docs says: If minimumSize() is set, the minimum size hint will be
  //    ignored. Probably the minimum size hint was size of the lengthy message
  //    and internal layout was enlarging the satusbar and the main window Maybe
  //    the notice about QLayout that is at minimumSizeHint should be also at
  //    minimumSize, didn't notice it first time and spend lot of time trying to
  //    figure this out :)

  bar->setMinimumWidth (2);
#ifdef Q_OS_LINUX
  int min_h= (int) floor (28 * retina_scale);
  bar->setMinimumHeight (min_h);
#else
#if (QT_VERSION >= 0x050000)
  if (tm_style_sheet != "") {
    int min_h= (int) floor (28 * retina_scale);
    bar->setMinimumHeight (min_h);
  }
#else
  double status_scale=
      (((double) retina_icons) > retina_scale ? 1.5 : retina_scale);
  if (status_scale > 1.0) {
    int std_h= (os_mingw () ? 28 : 20);
    int min_h= (int) floor (std_h * status_scale);
    bar->setMinimumHeight (min_h);
  }
#endif
#endif
  mw->setStatusBar (bar);

  // toolbars
  menuToolBar    = new QToolBar ("menu toolbar", mw);
  mainToolBar    = new QToolBar ("main toolbar", mw);
  modeToolBar    = new QToolBar ("mode toolbar", mw);
  focusToolBar   = new QToolBar ("focus toolbar", mw);
  userToolBar    = new QToolBar ("user toolbar", mw);
  bottomTools    = new QDockWidget ("bottom tools", mw);
  extraTools     = new QDockWidget ("extra tools", mw);
  sideTools      = new QDockWidget ("side tools", 0);
  leftTools      = new QDockWidget ("left tools", 0);
  auxiliaryWidget= new QTMAuxiliaryWidget ("auxiliary widget", 0);
  // HACK: Wrap the dock in a "fake" window widget (last parameter = true) to
  // have clicks report the right position.
  static int cnt      = 0;
  string     dock_name= "dock:" * as_string (cnt++);
  dock_window_widget=
      tm_new<qt_window_widget_rep> (sideTools, dock_name, command (), true);

  if (tm_style_sheet == "") {
    menuToolBar->setStyle (qtmstyle ());
    mainToolBar->setStyle (qtmstyle ());
    modeToolBar->setStyle (qtmstyle ());
    focusToolBar->setStyle (qtmstyle ());
    userToolBar->setStyle (qtmstyle ());
    sideTools->setStyle (qtmstyle ());
    leftTools->setStyle (qtmstyle ());
    bottomTools->setStyle (qtmstyle ());
    extraTools->setStyle (qtmstyle ());
    auxiliaryWidget->setStyle (qtmstyle ());
  }

  {
    // set proper sizes for icons
    double scale= max (retina_scale, (double) retina_icons);
    QSize  sz   = QSize (int (24 * scale), int (24 * scale));
    tweak_iconbar_size (sz);
    mainToolBar->setIconSize (sz);
    sz= QSize (int (20 * scale), int (20 * scale));
    tweak_iconbar_size (sz);
    modeToolBar->setIconSize (sz);
    sz= QSize (int (16 * scale), int (16 * scale));
    tweak_iconbar_size (sz);
    focusToolBar->setIconSize (sz);
  }

  // Why we need fixed height:
  // The height of the toolbar is actually determined by the font height.
  // And the font height is not fixed. If the height of the toolbar is not
  // fixed, the stretching of it will make the document area floating and
  // triggers the re-rendering of the full document.
  //
  // NOTICE: setFixedHeight must be after setIconSize
  // TODO: the size of the toolbar should be calculated dynamically
  {
    double scale       = retina_scale;
    int    h           = (int) floor (36 * scale + 0.5);
    int    tabRowHeight= h;
#ifdef Q_OS_MAC
    tabRowHeight= (int) floor (30 * scale + 0.5);
#endif

    // 工具栏高度相等
    mainToolBar->setFixedHeight (h);
    modeToolBar->setFixedHeight (h);
    focusToolBar->setFixedHeight (h);
    tabPageContainer->setRowHeight (tabRowHeight);

    // 保持可移动行为一致
    mainToolBar->setMovable (true);
    modeToolBar->setMovable (true);
    focusToolBar->setMovable (true);
    // menu栏不允许移动
    menuToolBar->setMovable (false);
  }

  QWidget* cw= new QWidget ();
  cw->setObjectName (
      "centralWidget"); // this is important for styling toolbars.

  // The main layout

  QVBoxLayout* bl= new QVBoxLayout (cw);
  bl->setContentsMargins (0, 1, 0, 0);
  bl->setSpacing (0);
  cw->setLayout (bl);
  QWidget* q= main_widget->as_qwidget (); // force creation of QWidget
  q->setParent (
      qwid); // q->layout()->removeWidget(q) will reset the parent to this
  bl->addWidget (guestNotificationBar); // 添加访客提示条
  bl->addWidget (q);

  mw->setCentralWidget (cw);

  mainToolBar->setObjectName ("mainToolBar");
  modeToolBar->setObjectName ("modeToolBar");
  focusToolBar->setObjectName ("focusToolBar");
  userToolBar->setObjectName ("userToolBar");
  menuToolBar->setObjectName ("menuToolBar");
  bottomTools->setObjectName ("bottomTools");
  extraTools->setObjectName ("extraTools");
  sideTools->setObjectName ("sideTools");
  leftTools->setObjectName ("leftTools");
  auxiliaryWidget->setObjectName ("auxiliaryWidget");

#ifdef UNIFIED_TOOLBAR

  if (use_unified_toolbar) {
    mw->setUnifiedTitleAndToolBarOnMac (true);

    // WARNING: dumbToolBar is the toolbar installed on the top area of the
    // main widget which is  then unified in the title bar.
    // to overcome some limitations of the unified toolbar implementation we
    // install the real toolbars as widgets in this toolbar.

    dumbToolBar= mw->addToolBar ("dumb toolbar");
    dumbToolBar->setMinimumHeight (30);

    // these are the actions related to the various toolbars to be installed in
    // the dumb toolbar.

    mainToolBarAction= dumbToolBar->addWidget (mainToolBar);
    modeToolBarAction= NULL;

    // A ruler
    rulerWidget= new QWidget (cw);
    rulerWidget->setSizePolicy (QSizePolicy::Ignored, QSizePolicy::Fixed);
    rulerWidget->setMinimumHeight (1);
    rulerWidget->setBackgroundRole (QPalette::Mid);
    // FIXME: how to use 112 (active) and 146 (passive)
    rulerWidget->setVisible (false);
    rulerWidget->setAutoFillBackground (true);
    // rulerWidget = new QLabel("pippo", cw);

    // A second ruler (this one always visible) to separate from the canvas.
    QWidget* r2= new QWidget (mw);
    r2->setSizePolicy (QSizePolicy::Ignored, QSizePolicy::Fixed);
    r2->setMinimumHeight (1);
    r2->setBackgroundRole (QPalette::Mid);
    r2->setVisible (true);
    r2->setAutoFillBackground (true);

    bl->insertWidget (0, menuToolBar);
    bl->insertWidget (1, tabPageContainer);
    bl->insertWidget (2, modeToolBar);
    bl->insertWidget (3, rulerWidget);
    bl->insertWidget (4, focusToolBar);
    bl->insertWidget (5, guestNotificationBar); // 访客提示条在焦点工具栏下方
    bl->insertWidget (6, userToolBar);
    bl->insertWidget (7, r2);

    // mw->setContentsMargins (-2, -2, -2, -2);  // Why this?
    bar->setContentsMargins (0, 1, 0, 1);
  }
  else {
    mw->addToolBar (menuToolBar);
    mw->addToolBarBreak ();
    mw->addToolBar (mainToolBar);
    mw->addToolBarBreak ();
    mw->addToolBar (modeToolBar);
    mw->addToolBarBreak ();
    mw->addToolBar (focusToolBar);
    mw->addToolBarBreak ();
    mw->addToolBar (userToolBar);
    mw->addToolBarBreak ();
  }

#else
  mw->addToolBar (menuToolBar);
  mw->addToolBarBreak ();
  mw->addToolBar (mainToolBar);
  mw->addToolBarBreak ();
  mw->addToolBar (modeToolBar);
  mw->addToolBarBreak ();
  mw->addToolBar (focusToolBar);
  mw->addToolBarBreak ();
  mw->addToolBar (userToolBar);
  mw->addToolBarBreak ();
#endif

  sideTools->setAllowedAreas (Qt::AllDockWidgetAreas);
  sideTools->setFeatures (QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable);
  sideTools->setFloating (false);
  sideTools->setTitleBarWidget (new QWidget ()); // Disables title bar
  mw->addDockWidget (Qt::RightDockWidgetArea, sideTools);

  leftTools->setAllowedAreas (Qt::AllDockWidgetAreas);
  leftTools->setFeatures (QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable);
  leftTools->setFloating (false);
  leftTools->setTitleBarWidget (new QWidget ()); // Disables title bar
  mw->addDockWidget (Qt::LeftDockWidgetArea, leftTools);

  bottomTools->setAllowedAreas (Qt::BottomDockWidgetArea);
  bottomTools->setFeatures (QDockWidget::NoDockWidgetFeatures);
  bottomTools->setFloating (false);
  bottomTools->setTitleBarWidget (new QWidget ()); // Disables title bar
  // bottomTools->setMinimumHeight (10);             // Avoids warning
  bottomTools->setContentsMargins (3, 6, 3, -2); // Hacks hacks hacks... :(
  mw->addDockWidget (Qt::BottomDockWidgetArea, bottomTools);

  extraTools->setAllowedAreas (Qt::BottomDockWidgetArea);
  extraTools->setFeatures (QDockWidget::NoDockWidgetFeatures);
  extraTools->setFloating (false);
  extraTools->setTitleBarWidget (new QWidget ()); // Disables title bar
  // extraTools->setMinimumHeight (10);             // Avoids warning
  extraTools->setContentsMargins (3, 6, 3, -2); // Hacks hacks hacks... :(
  mw->addDockWidget (Qt::BottomDockWidgetArea, extraTools);

  auxiliaryWidget->setAllowedAreas (Qt::RightDockWidgetArea);
  // auxiliaryWidget->setFeatures (QDockWidget::DockWidgetMovable |
  //                        QDockWidget::DockWidgetFloatable);
  auxiliaryWidget->setFloating (false);
  // auxiliaryWidget->setTitleBarWidget (new QWidget ()); // Disables title bar
  mw->addDockWidget (Qt::RightDockWidgetArea, auxiliaryWidget);

  // FIXME? add DockWidgetClosable and connect the close signal
  // to the scheme code
  //  QObject::connect(sideDock, SIGNAL(closeEvent()),
  //                   someHelper, SLOT(call_scheme_hide_side_tools()));

  // handles visibility
  // at this point all the toolbars are empty so we avoid showing them
  // same for the menu bar if we are not on the Mac (where we do not have
  // other options)

  mainToolBar->setVisible (false);
  modeToolBar->setVisible (false);
  focusToolBar->setVisible (false);
  userToolBar->setVisible (false);
  menuToolBar->setVisible (false);
  sideTools->setVisible (false);
  leftTools->setVisible (false);
  bottomTools->setVisible (false);
  extraTools->setVisible (false);
  auxiliaryWidget->setVisible (false);
  mainwindow ()->statusBar ()->setVisible (true);
  QPalette pal;
  QColor   bgcol= to_qcolor (tm_background);
  pal.setColor (QPalette::Mid, bgcol);
  mainwindow ()->setPalette (pal);

  // 连接登录状态变化信号
  if (is_server_started ()) {
    tm_server_rep* server=
        dynamic_cast<tm_server_rep*> (get_server ().operator->());
    if (server && server->getAccount ()) {
      QTMOAuth* account= server->getAccount ();
      QObject::connect (account, &QTMOAuth::loginStateChanged,
                        [this] (bool loggedIn) {
                          // 登录状态变化时，重新检查登录状态并更新提示条
                          if (guestNotificationBar) {
                            // 社区版不显示提示条
                            if (is_community_stem ()) {
                              guestNotificationBar->hide ();
                            }
                            else {
                              // 商业版：根据登录状态决定是否显示
                              if (loggedIn) {
                                // 用户已登录，隐藏提示条
                                guestNotificationBar->hide ();
                              }
                              else {
                                // 用户未登录，显示提示条
                                guestNotificationBar->show ();
                              }
                            }
                          }
                        });
    }
  }
  else {
    std_error << "qt_tm_widget_rep: server not started, cannot connect ";
  }

  // 恢复窗口状态和几何信息
  restoreSettings ();
}

void
qt_tm_widget_rep::restoreSettings () {
  /* 恢复工具栏和窗口状态
   * mainWindowState: 恢复主窗口的工具栏、菜单栏、状态栏等UI元素的状态
   *                  包括工具栏的位置、是否可见、停靠状态等，以及菜单栏的展开状态
   * mainWindowGeometry: 恢复主窗口的几何信息
   *                     包括窗口的位置(x,y坐标)、大小(宽度、高度)、是否最大化/最小化等状态
   * restoreState() - 恢复工具栏和停靠窗口的状态
   * restoreGeometry() - 恢复窗口的几何信息（位置、大小等） */

  /* 获取用户配置路径，用于存储窗口状态设置文件 */
  string config_path= concretize (head (get_tm_preference_path ()));
  if (config_path != "") {
    /* 使用用户自定义的配置路径创建QSettings对象
     * 配置文件格式为INI格式，便于用户手动编辑 */
    QSettings settings (to_qstring (config_path * "/qtwindows.ini"),
                        QSettings::IniFormat);

    /* 从配置文件中读取窗口几何信息并恢复
     * geometry包含窗口的位置、大小、最大化/最小化状态等 */
    QByteArray geometry= settings.value ("mainWindowGeometry").toByteArray ();
    if (!geometry.isEmpty ()) mainwindow ()->restoreGeometry (geometry);

    /* 从配置文件中读取窗口状态信息并恢复
     * state包含工具栏、停靠窗口的位置、可见性、停靠状态等 */
    QByteArray state= settings.value ("mainWindowState").toByteArray ();
    if (!state.isEmpty ()) mainwindow ()->restoreState (state);
  }
  else {
    QSettings  settings ("LiiiNetwork", STEM_NAME);
    QByteArray geometry= settings.value ("mainWindowGeometry").toByteArray ();
    if (!geometry.isEmpty ()) mainwindow ()->restoreGeometry (geometry);
    QByteArray state= settings.value ("mainWindowState").toByteArray ();
    if (!state.isEmpty ()) mainwindow ()->restoreState (state);
  }
}

qt_tm_widget_rep::~qt_tm_widget_rep () {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_widget_rep::~qt_tm_widget_rep of widget "
                  << type_as_string () << LF;

  // clear any residual waiting menu installation
  waiting_widgets= remove (waiting_widgets, this);
}

void
qt_tm_widget_rep::tweak_iconbar_size (QSize& sz) {
#ifdef Q_OS_LINUX
  if (sz.height () >= 24) {
    sz.setWidth (sz.width () + 2);
    sz.setHeight (sz.height () + 8);
  }
  else if (sz.height () >= 20) {
    sz.setWidth (sz.width () + 1);
    sz.setHeight (sz.height () + 4);
  }
  else if (sz.height () >= 16) {
    sz.setHeight (sz.height () + 4);
  }
#else
  if (sz.height () >= 24) {
    sz.setWidth (sz.width () + 2);
    sz.setHeight (sz.height () + 6);
  }
  else if (sz.height () >= 20) {
    sz.setHeight (sz.height () + 2);
  }
  else if (sz.height () >= 16) {
    sz.setHeight (sz.height () + 2);
  }
#endif
  // sz.setHeight ((int) floor (sz.height () * retina_scale + 0.5));
}

/*! Return ourselves as a window widget.
 \param name A unique identifier for the window (e.g. "TeXmacs:3")
 */
widget
qt_tm_widget_rep::plain_window_widget (string name, command _quit, int b) {
  (void) b;
  (void) _quit; // The widget already has a command. Don't overwrite.
  orig_name= name;
  return this;
}

void
qt_tm_widget_rep::update_visibility () {
#define XOR(exp1, exp2) (((!exp1) && (exp2)) || ((exp1) && (!exp2)))

  bool old_mainVisibility  = mainToolBar->isVisible ();
  bool old_menuVisibility  = menuToolBar->isVisible ();
  bool old_modeVisibility  = modeToolBar->isVisible ();
  bool old_focusVisibility = focusToolBar->isVisible ();
  bool old_userVisibility  = userToolBar->isVisible ();
  bool old_sideVisibility  = sideTools->isVisible ();
  bool old_leftVisibility  = leftTools->isVisible ();
  bool old_bottomVisibility= bottomTools->isVisible ();
  bool old_extraVisibility = extraTools->isVisible ();
  bool old_auxVisibility   = auxiliaryWidget->isVisible ();
  bool old_statusVisibility= mainwindow ()->statusBar ()->isVisible ();
  bool old_titleVisibility = windowAgent->titleBar ()->isVisible ();

  bool new_mainVisibility  = visibility[1] && visibility[0];
  bool new_menuVisibility  = visibility[0];
  bool new_modeVisibility  = visibility[2] && visibility[0];
  bool new_focusVisibility = visibility[3] && visibility[0];
  bool new_userVisibility  = visibility[4] && visibility[0];
  bool new_statusVisibility= visibility[5];
  bool new_sideVisibility  = visibility[6];
  bool new_leftVisibility  = visibility[7];
  bool new_bottomVisibility= visibility[8];
  bool new_extraVisibility = visibility[9];
  bool new_tabVisibility   = visibility[10] && visibility[0];
  bool new_auxVisibility   = visibility[11];
  bool new_titleVisibility = visibility[0];

  if (XOR (old_mainVisibility, new_mainVisibility))
    mainToolBar->setVisible (new_mainVisibility);
  if (XOR (old_menuVisibility, new_menuVisibility))
    menuToolBar->setVisible (new_menuVisibility);
  if (XOR (old_modeVisibility, new_modeVisibility))
    modeToolBar->setVisible (new_modeVisibility);
  if (XOR (old_focusVisibility, new_focusVisibility))
    focusToolBar->setVisible (new_focusVisibility);
  if (XOR (old_userVisibility, new_userVisibility))
    userToolBar->setVisible (new_userVisibility);
  if (XOR (old_sideVisibility, new_sideVisibility))
    sideTools->setVisible (new_sideVisibility);
  if (XOR (old_leftVisibility, new_leftVisibility))
    leftTools->setVisible (new_leftVisibility);
  if (XOR (old_bottomVisibility, new_bottomVisibility))
    bottomTools->setVisible (new_bottomVisibility);
  if (XOR (old_extraVisibility, new_extraVisibility))
    extraTools->setVisible (new_extraVisibility);
  if (XOR (old_auxVisibility, new_auxVisibility))
    auxiliaryWidget->setVisible (new_auxVisibility);
  if (XOR (old_titleVisibility, new_titleVisibility))
    windowAgent->titleBar ()->setVisible (new_titleVisibility);
  if (XOR (old_statusVisibility, new_statusVisibility))
    mainwindow ()->statusBar ()->setVisible (new_statusVisibility);

// #if 0
#ifdef UNIFIED_TOOLBAR

  // do modifications only if needed to reduce flicker
  if (use_unified_toolbar && (XOR (old_mainVisibility, new_mainVisibility) ||
                              XOR (old_modeVisibility, new_modeVisibility))) {
    // ensure that the topmost visible toolbar is always unified on Mac
    // (actually only for main and mode toolbars, unifying focus is not
    // appropriate)

    QBoxLayout* bl=
        qobject_cast<QBoxLayout*> (mainwindow ()->centralWidget ()->layout ());

    if (modeToolBarAction)
      modeToolBarAction->setVisible (modeToolBar->isVisible ());
    mainToolBarAction->setVisible (mainToolBar->isVisible ());

    // WARNING: jugglying around bugs in Qt unified toolbar implementation
    // do not try to change the order of the following operations....

    if (mainToolBar->isVisible ()) {
      bool tmp= modeToolBar->isVisible ();
      dumbToolBar->removeAction (modeToolBarAction);
      dumbToolBar->addAction (mainToolBarAction);
      bl->insertWidget (0, rulerWidget);
      bl->insertWidget (0, modeToolBar);
      mainToolBarAction->setVisible (true);
      rulerWidget->setVisible (true);
      modeToolBar->setVisible (tmp);
      if (modeToolBarAction) modeToolBarAction->setVisible (tmp);
      dumbToolBar->setVisible (true);
    }
    else {
      dumbToolBar->removeAction (mainToolBarAction);
      if (modeToolBar->isVisible ()) {
        bl->removeWidget (rulerWidget);
        rulerWidget->setVisible (false);
        bl->removeWidget (modeToolBar);
        if (modeToolBarAction == NULL) {
          modeToolBarAction= dumbToolBar->addWidget (modeToolBar);
        }
        else {
          dumbToolBar->addAction (modeToolBarAction);
        }
        dumbToolBar->setVisible (true);
      }
      else {
        dumbToolBar->setVisible (false);
        dumbToolBar->removeAction (modeToolBarAction);
      }
    }
  }
#endif // UNIFIED_TOOLBAR
#undef XOR
  if (tm_style_sheet == "" && use_mini_bars) {
    QFont f = leftLabel->font ();
    int   fs= as_int (get_preference ("gui:mini-fontsize", QTM_MINI_FONTSIZE));
    f.setPointSize (qt_zoom (fs > 0 ? fs : QTM_MINI_FONTSIZE));
    leftLabel->setFont (f);
    rightLabel->setFont (f);
  }
}

widget
qt_tm_widget_rep::read (slot s, blackbox index) {
  widget ret;

  switch (s) {
  case SLOT_CANVAS:
    check_type_void (index, s);
    ret= abstract (main_widget);
    break;

  default:
    return qt_window_widget_rep::read (s, index);
  }

  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_widget_rep::read " << slot_name (s)
                  << "\t\tfor widget\t" << type_as_string () << LF;

  return ret;
}

void
qt_tm_widget_rep::send (slot s, blackbox val) {
  switch (s) {
  case SLOT_INVALIDATE:
  case SLOT_INVALIDATE_ALL:
  case SLOT_EXTENTS:
  case SLOT_SCROLL_POSITION:
  case SLOT_ZOOM_FACTOR:
  case SLOT_MOUSE_GRAB:
    main_widget->send (s, val);
    return;
  case SLOT_KEYBOARD_FOCUS: {
    check_type<bool> (val, s);
    bool focus= open_box<bool> (val);
    if (focus && canvas () && !canvas ()->hasFocus ())
      canvas ()->setFocus (Qt::OtherFocusReason);
  } break;
  case SLOT_HEADER_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[0]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_MAIN_ICONS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[1]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_MODE_ICONS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[2]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_FOCUS_ICONS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[3]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_USER_ICONS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[4]= open_box<bool> (val);
    update_visibility ();
  } break;

  case SLOT_FOOTER_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[5]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_SIDE_TOOLS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[6]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_LEFT_TOOLS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[7]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_BOTTOM_TOOLS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[8]= open_box<bool> (val);
    update_visibility ();
  }
  case SLOT_EXTRA_TOOLS_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[9]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_TAB_PAGES_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[10]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_AUXILIARY_WIDGET_VISIBILITY: {
    check_type<bool> (val, s);
    visibility[11]= open_box<bool> (val);
    update_visibility ();
  } break;
  case SLOT_AUXILIARY_WIDGET: {
    check_type<string> (val, s);
    auxiliaryWidget->setWindowTitle (to_qstring (open_box<string> (val)));
  } break;
  case SLOT_AUXILIARY_WIDGET_TITLE: {
    check_type<string> (val, s);
    string title= open_box<string> (val);
    auxiliaryWidget->setWindowTitle (to_qstring (title));
  } break;
  case SLOT_LEFT_FOOTER: {
    check_type<string> (val, s);
    string msg= open_box<string> (val);
    leftLabel->setText (to_qstring (msg));
    leftLabel->update ();
  } break;
  case SLOT_MIDDLE_FOOTER: {
    check_type<string> (val, s);
    string msg= open_box<string> (val);
    middleLabel->setText (to_qstring (msg));
    middleLabel->update ();
  } break;
  case SLOT_RIGHT_FOOTER: {
    check_type<string> (val, s);
    string msg= open_box<string> (val);
    rightLabel->setText (to_qstring (msg));
    rightLabel->update ();
  } break;
  case SLOT_SCROLLBARS_VISIBILITY:
    // ignore this: qt handles scrollbars independently
    //                send_int (THIS, "scrollbars", val);
    break;
  case SLOT_INTERACTIVE_MODE: {
    check_type<bool> (val, s);

    if (open_box<bool> (val) == true) {
      prompt= new QTMInteractivePrompt (int_prompt, int_input);
      mainwindow ()->statusBar ()->removeWidget (leftLabel);
      mainwindow ()->statusBar ()->removeWidget (middleLabel);
      mainwindow ()->statusBar ()->removeWidget (rightLabel);
      mainwindow ()->statusBar ()->addWidget (prompt, 1);
      prompt->start ();
    }
    else {
      if (prompt) prompt->end ();
      mainwindow ()->statusBar ()->removeWidget (prompt);
      mainwindow ()->statusBar ()->addWidget (leftLabel, 1);
      mainwindow ()->statusBar ()->addWidget (middleLabel, 1);
      mainwindow ()->statusBar ()->addPermanentWidget (rightLabel, 1);
      leftLabel->show ();
      middleLabel->show ();
      rightLabel->show ();
      prompt->deleteLater ();
      prompt= NULL;
    }
  } break;
  case SLOT_FILE: {
    check_type<string> (val, s);
    string file= open_box<string> (val);
    if (DEBUG_QT_WIDGETS) debug_widgets << "\tFile: " << file << LF;
    mainwindow ()->setWindowFilePath (utf8_to_qstring (file));
  } break;
  case SLOT_POSITION: {
    check_type<coord2> (val, s);
    coord2 p           = open_box<coord2> (val);
    QPoint pos         = to_qpoint (p);
    int    screen_count= QGuiApplication::screens ().count ();
    int    screen_w= QApplication::primaryScreen ()->availableSize ().width ();
    if ((screen_count == 1 && screen_w >= pos.x ()) || (screen_count > 1)) {
      // For only 1 screen, only move to pos.x within the screen width
      // For multiple screens, just move it
      mainwindow ()->move (pos);
    }
  } break;
  case SLOT_SIZE: {
    check_type<coord2> (val, s);
    coord2 p= open_box<coord2> (val);
    mainwindow ()->resize (to_qsize (p));
  } break;
  case SLOT_DESTROY: {
    ASSERT (is_nil (val), "type mismatch");
    if (!is_nil (quit)) quit ();
    the_gui->need_update ();
  } break;
  case SLOT_FULL_SCREEN: {
    check_type<bool> (val, s);
    set_full_screen (open_box<bool> (val));
  } break;
  default:
    qt_window_widget_rep::send (s, val);
    return;
  }

  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_widget_rep: sent " << slot_name (s)
                  << "\t\tto widget\t" << type_as_string () << LF;
}

blackbox
qt_tm_widget_rep::query (slot s, int type_id) {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_widget_rep: queried " << slot_name (s)
                  << "\t\tto widget\t" << type_as_string () << LF;

  switch (s) {
  case SLOT_SCROLL_POSITION:
  case SLOT_EXTENTS:
  case SLOT_VISIBLE_PART:
  case SLOT_ZOOM_FACTOR:
    return main_widget->query (s, type_id);

  case SLOT_HEADER_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[0]);

  case SLOT_MAIN_ICONS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[1]);

  case SLOT_MODE_ICONS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[2]);

  case SLOT_FOCUS_ICONS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[3]);

  case SLOT_USER_ICONS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[4]);

  case SLOT_FOOTER_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[5]);

  case SLOT_SIDE_TOOLS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[6]);

  case SLOT_LEFT_TOOLS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[7]);

  case SLOT_BOTTOM_TOOLS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[8]);

  case SLOT_EXTRA_TOOLS_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[9]);

  case SLOT_TAB_PAGES_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[10]);

  case SLOT_AUXILIARY_WIDGET_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (visibility[11]);

  case SLOT_INTERACTIVE_INPUT: {
    check_type_id<string> (type_id, s);
    qt_input_text_widget_rep* w=
        static_cast<qt_input_text_widget_rep*> (int_input.rep);
    if (w->ok) return close_box<string> (scm_quote (w->input));
    else return close_box<string> ("#f");
  }

  case SLOT_POSITION: {
    check_type_id<coord2> (type_id, s);
    return close_box<coord2> (from_qpoint (mainwindow ()->pos ()));
  }

  case SLOT_SIZE: {
    check_type_id<coord2> (type_id, s);
    return close_box<coord2> (from_qsize (mainwindow ()->size ()));
  }

  case SLOT_INTERACTIVE_MODE:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (prompt && prompt->isActive ());

  default:
    return qt_window_widget_rep::query (s, type_id);
  }
}

void
qt_tm_widget_rep::install_main_menu () {
  if (main_menu_widget == waiting_main_menu_widget) return;
  main_menu_widget    = waiting_main_menu_widget;
  QList<QAction*>* src= main_menu_widget->get_qactionlist ();
  if (!src) return;
  QMenuBar* dest  = new QMenuBar ();
  QScreen*  screen= QGuiApplication::primaryScreen ();
#ifdef Q_OS_WIN
  // 设置与 menuToolBar 匹配的固定高度
  // 使用 devicePixelRatio() 获取正确的屏幕缩放比
  // 获取屏幕DPI缩放比例
  double dpi  = screen ? screen->logicalDotsPerInch () : 96.0;
  double scale= dpi / 96.0;

  int h= (int) floor (72 * scale + 0.5);
#else
  double scale= screen ? screen->devicePixelRatio () : 1.0; // 正确的屏幕缩放比
  int    h    = (int) floor (108 * scale + 0.5);
#endif
  dest->setFixedHeight (h);

  if (tm_style_sheet == "") dest->setStyle (qtmstyle ());
  if (!use_native_menubar) {
    dest->setNativeMenuBar (false);
  }

  dest->clear ();
  for (int i= 0; i < src->count (); i++) {
    QAction* a= (*src)[i];
    if (a->menu ()) {
      // TRICK: Mac native QMenuBar accepts only menus which are already
      // populated
      //  this will cause a problem for us, since menus are lazy and populated
      //  only after triggering this is the reason we add a dummy action before
      //  inserting the menu
      a->menu ()->addAction ("native menubar trick");
      dest->addAction (a->menu ()->menuAction ());
      QObject::connect (a->menu (), SIGNAL (aboutToShow ()),
                        the_gui->gui_helper, SLOT (aboutToShowMainMenu ()));
      QObject::connect (a->menu (), SIGNAL (aboutToHide ()),
                        the_gui->gui_helper, SLOT (aboutToHideMainMenu ()));
    }
  }

  // 移除旧 menuBar
  QList<QWidget*> widgets= menuToolBar->findChildren<QWidget*> ();
  for (QWidget* w : widgets) {
    w->setParent (nullptr);
  }
  // 确保 menuToolBar 可见
  if (!menuToolBar->isVisible ()) {
    menuToolBar->setVisible (true);
  }

  // 确保 menuToolBar 有正确的布局策略
  menuToolBar->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Fixed);
  // 确保内容可以正确显示
  if (menuToolBar->layout ()) {
    menuToolBar->layout ()->setContentsMargins (2, 0, 2, 0);
    menuToolBar->layout ()->setSpacing (4);
  }

  // 添加新的 menuBar 到 menuToolBar
  menuToolBar->addWidget (dest);
}

void
qt_tm_widget_rep::write (slot s, blackbox index, widget w) {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_widget_rep::write " << slot_name (s) << LF;

  switch (s) {
    // Widget w is usually a qt_simple_widget_rep, with a QTMWidget as
    // underlying widget. We must discard the current main_widget and
    // display the new. But while switching buffers the widget w is a
    // glue_widget, so we may not just use canvas() everywhere.
  case SLOT_SCROLLABLE: {
    check_type_void (index, s);

    QWidget* q= main_widget->qwid;
    q->hide ();
    QLayout* l= centralwidget ()->layout ();
    l->removeWidget (q);

    q= concrete (w)->as_qwidget (); // force creation of the new QWidget
    l->addWidget (q);
    /* " When you use a layout, you do not need to pass a parent when
     constructing the child widgets. The layout will automatically reparent
     the widgets (using QWidget::setParent()) so that they are children of
     the widget on which the layout is installed " */
    main_widget= concrete (w);
    // canvas() now returns the new QTMWidget (or 0)

    if (scrollarea ()) // Fix size to draw margins around.
      scrollarea ()->surface ()->setSizePolicy (QSizePolicy::Fixed,
                                                QSizePolicy::Fixed);
    send_keyboard_focus (abstract (main_widget));
  } break;

  case SLOT_MAIN_MENU:
    check_type_void (index, s);
    {
      waiting_main_menu_widget= concrete (w);
      if (menu_count <= 0) install_main_menu ();
      else if (!contains (waiting_widgets, this))
        // menu interaction ongoing, postpone new menu installation until done
        waiting_widgets << this;
    }
    break;

  case SLOT_MAIN_ICONS:
    check_type_void (index, s);
    {
      main_icons_widget    = concrete (w);
      QList<QAction*>* list= main_icons_widget->get_qactionlist ();
      if (list) {
        replaceButtons (mainToolBar, list);
        update_visibility ();
      }
    }
    break;

  case SLOT_TAB_PAGES:
    check_type_void (index, s);
    {
      tab_bar_widget       = concrete (w);
      QList<QAction*>* list= tab_bar_widget->get_qactionlist ();
      if (list) {
        tabPageContainer->replaceTabPages (list);
        update_visibility ();
        // 为标签页设置hit test可见性
        if (windowAgent) {
          tabPageContainer->setHitTestVisibleForTabPages (windowAgent);
        }
      }
    }
    break;

  case SLOT_AUXILIARY_WIDGET:
    check_type_void (index, s);
    {
      auxiliary_widget    = concrete (w);
      QWidget* new_qwidget= auxiliary_widget->as_qwidget ();
      QWidget* old_qwidget= auxiliaryWidget->widget ();
      if (old_qwidget) old_qwidget->deleteLater ();
      new_qwidget->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);

      // 使用一层容器包装 new_qwidget，以使布局更美观（同时留出"广告位"）
      QWidget* container= new QWidget ();
      container->setObjectName ("auxiliary_container");
      QVBoxLayout* verticalLayout= new QVBoxLayout (container);
      verticalLayout->setSpacing (0);                  // 间距
      verticalLayout->setContentsMargins (0, 0, 0, 0); // 边距
      verticalLayout->setAlignment (Qt::AlignTop |
                                    Qt::AlignHCenter); // 居中对齐
      verticalLayout->addWidget (new_qwidget);

      auxiliaryWidget->setWidget (container);
      update_visibility ();
    }
    break;

  case SLOT_MODE_ICONS:
    check_type_void (index, s);
    {
      mode_icons_widget    = concrete (w);
      QList<QAction*>* list= mode_icons_widget->get_qactionlist ();
      if (list) {
        replaceButtons (modeToolBar, list);
        update_visibility ();
      }
    }
    break;

  case SLOT_FOCUS_ICONS:
    check_type_void (index, s);
    {
      bool can_update= true;
#if (QT_VERSION >= 0x050000)
      // BUG:
      // there is a problem with updateActions  which apparently
      // reset a running input method in Qt5.
      //
      // This is (probably) also relate to
      // bug #47338 [CJK] input disappears immediately
      // see http://lists.gnu.org/archive/html/texmacs-dev/2017-09/msg00000.html

      // HACK: we just disable the focus bar updating while preediting.
      // This seems enough since the other toolbars are not usually updated
      // while performing an input method keyboard sequence
      if (canvas ()) can_update= !canvas ()->isPreediting ();
#endif
      if (can_update) {
        focus_icons_widget   = concrete (w);
        QList<QAction*>* list= focus_icons_widget->get_qactionlist ();
        if (list) {
          replaceButtons (focusToolBar, list);
          update_visibility ();
        }
      }
    }
    break;

  case SLOT_USER_ICONS:
    check_type_void (index, s);
    {
      user_icons_widget    = concrete (w);
      QList<QAction*>* list= user_icons_widget->get_qactionlist ();
      if (list) {
        replaceButtons (userToolBar, list);
        update_visibility ();
      }
    }
    break;

  case SLOT_SIDE_TOOLS:
    check_type_void (index, s);
    {
      side_tools_widget   = concrete (w);
      QWidget* new_qwidget= side_tools_widget->as_qwidget ();
      QWidget* old_qwidget= sideTools->widget ();
      if (old_qwidget) old_qwidget->deleteLater ();
      sideTools->setWidget (new_qwidget);
      update_visibility ();
#if (QT_VERSION >= 0x050000)
      QList<QDockWidget*> l1;
      l1.append ((QDockWidget*) extraTools);
      QList<int> l2;
      l2.append (1);
      mainwindow ()->resizeDocks (l1, l2, Qt::Horizontal);
#endif
      new_qwidget->show ();
    }
    break;

  case SLOT_LEFT_TOOLS:
    check_type_void (index, s);
    {
      left_tools_widget   = concrete (w);
      QWidget* new_qwidget= left_tools_widget->as_qwidget ();
      QWidget* old_qwidget= leftTools->widget ();
      if (old_qwidget) old_qwidget->deleteLater ();
      leftTools->setWidget (new_qwidget);
      update_visibility ();
#if (QT_VERSION >= 0x050000)
      QList<QDockWidget*> l1;
      l1.append ((QDockWidget*) extraTools);
      QList<int> l2;
      l2.append (1);
      mainwindow ()->resizeDocks (l1, l2, Qt::Horizontal);
#endif
      new_qwidget->show ();
    }
    break;

  case SLOT_BOTTOM_TOOLS:
    check_type_void (index, s);
    {
      bottom_tools_widget = concrete (w);
      QWidget* new_qwidget= bottom_tools_widget->as_qwidget ();
      QWidget* old_qwidget= bottomTools->widget ();
      if (old_qwidget) old_qwidget->deleteLater ();
      bottomTools->setWidget (new_qwidget);
      update_visibility ();
#if (QT_VERSION >= 0x050000)
      QList<QDockWidget*> l1;
      l1.append ((QDockWidget*) extraTools);
      QList<int> l2;
      l2.append (1);
      mainwindow ()->resizeDocks (l1, l2, Qt::Vertical);
#endif
      new_qwidget->show ();
    }
    break;

  case SLOT_EXTRA_TOOLS:
    check_type_void (index, s);
    {
      extra_tools_widget  = concrete (w);
      QWidget* new_qwidget= extra_tools_widget->as_qwidget ();
      QWidget* old_qwidget= extraTools->widget ();
      if (old_qwidget) old_qwidget->deleteLater ();
      extraTools->setWidget (new_qwidget);
      update_visibility ();
#if (QT_VERSION >= 0x050000)
      QList<QDockWidget*> l1;
      l1.append ((QDockWidget*) extraTools);
      QList<int> l2;
      l2.append (1);
      mainwindow ()->resizeDocks (l1, l2, Qt::Vertical);
#endif
      new_qwidget->show ();
    }
    break;

  case SLOT_INTERACTIVE_PROMPT:
    check_type_void (index, s);
    int_prompt= concrete (w);
    break;

  case SLOT_INTERACTIVE_INPUT:
    check_type_void (index, s);
    int_input= concrete (w);
    break;

  default:
    qt_window_widget_rep::write (s, index, w);
  }
}

void set_standard_style_sheet (QWidget* w);

void
qt_tm_widget_rep::set_full_screen (bool flag) {
  full_screen = flag;
  QWidget* win= mainwindow ()->window ();
  if (win) {
    if (flag) {
      QPalette pal;
      pal.setColor (QPalette::Mid, QColor (0, 0, 0));
      mainwindow ()->setPalette (pal);
      if (mainwindow ()->centralWidget () &&
          mainwindow ()->centralWidget ()->layout ()) {
        mainwindow ()->centralWidget ()->layout ()->setContentsMargins (0, 0, 0,
                                                                        0);
      }
#ifdef UNIFIED_TOOLBAR
      if (use_unified_toolbar) {
        // HACK: we disable unified toolbar since otherwise
        //   the application will crash when we return to normal mode
        //  (bug in Qt? present at least with 4.7.1)
        mainwindow ()->setUnifiedTitleAndToolBarOnMac (false);
        mainwindow ()->centralWidget ()->layout ()->setContentsMargins (0, 0, 0,
                                                                        0);
      }
#endif
      //      mainwindow()->window()->setContentsMargins(0,0,0,0);
      // win->showFullScreen();
      win->setWindowState (win->windowState () | Qt::WindowFullScreen);
      menuToolBarVisibleCache= menuToolBar && menuToolBar->isVisible ();
      if (menuToolBar) menuToolBar->setVisible (false);
      if (windowAgent) {
        QWidget* tb         = windowAgent->titleBar ();
        titleBarVisibleCache= tb && tb->isVisible ();
        if (tb) tb->setVisible (false);
      }
      bool is_slide_style= false;
      try {
        is_slide_style= as_bool (call ("in-presentation?"));
      } catch (...) {
        is_slide_style= false;
      }
      bool           is_presentation_mode= !visibility[0];
      QTMScrollView* scrollView          = scrollarea ();
      if (scrollView && is_presentation_mode && is_slide_style) {
        QWidget* viewport= scrollView->viewport ();
        if (viewport) {
          viewport->setBackgroundRole (QPalette::Shadow);
        }
      }
    }
    else {
      QPalette pal;
      QColor   bgcol= to_qcolor (tm_background);
      pal.setColor (QPalette::Mid, bgcol);
      mainwindow ()->setPalette (pal);
      if (mainwindow ()->centralWidget () &&
          mainwindow ()->centralWidget ()->layout ()) {
        mainwindow ()->centralWidget ()->layout ()->setContentsMargins (0, 1, 0,
                                                                        0);
      }
      bool cache   = visibility[0];
      visibility[0]= false;
      update_visibility ();
      //      win->showNormal();
      win->setWindowState (win->windowState () & ~Qt::WindowFullScreen);

      visibility[0]= cache;
      update_visibility ();
      if (menuToolBar) menuToolBar->setVisible (menuToolBarVisibleCache);
      if (windowAgent) {
        QWidget* tb= windowAgent->titleBar ();
        if (tb) tb->setVisible (titleBarVisibleCache);
      }
      bool is_slide_style= false;
      try {
        is_slide_style= as_bool (call ("in-presentation?"));
      } catch (...) {
        is_slide_style= false;
      }
      QTMScrollView* scrollView= scrollarea ();
      if (scrollView && is_slide_style) {
        QWidget* viewport= scrollView->viewport ();
        if (viewport) {
          viewport->setBackgroundRole (QPalette::Mid);
        }
      }
#ifdef UNIFIED_TOOLBAR
      if (use_unified_toolbar) {
        mainwindow ()->centralWidget ()->layout ()->setContentsMargins (0, 1, 0,
                                                                        0);
        // HACK: we reenable unified toolbar (see above HACK)
        //   the application will crash when we return to normal mode
        mainwindow ()->setUnifiedTitleAndToolBarOnMac (true);
      }
#endif
    }
  }

  scrollarea ()->setHorizontalScrollBarPolicy (flag ? Qt::ScrollBarAlwaysOff
                                                    : Qt::ScrollBarAsNeeded);
  scrollarea ()->setVerticalScrollBarPolicy (flag ? Qt::ScrollBarAlwaysOff
                                                  : Qt::ScrollBarAsNeeded);
}

/******************************************************************************
 * qt_tm_embedded_widget_rep
 ******************************************************************************/

qt_tm_embedded_widget_rep::qt_tm_embedded_widget_rep (command _quit)
    : qt_widget_rep (embedded_tm_widget), quit (_quit) {
  main_widget= ::glue_widget (true, true, 1, 1);
}

void
qt_tm_embedded_widget_rep::send (slot s, blackbox val) {

  switch (s) {
  case SLOT_INVALIDATE:
  case SLOT_INVALIDATE_ALL:
  case SLOT_EXTENTS:
  case SLOT_SCROLL_POSITION:
  case SLOT_ZOOM_FACTOR:
  case SLOT_MOUSE_GRAB:
    main_widget->send (s, val);
    return;

    /// FIXME: decide what to do with these for embedded widgets
  case SLOT_HEADER_VISIBILITY:
  case SLOT_MAIN_ICONS_VISIBILITY:
  case SLOT_MODE_ICONS_VISIBILITY:
  case SLOT_FOCUS_ICONS_VISIBILITY:
  case SLOT_USER_ICONS_VISIBILITY:
  case SLOT_FOOTER_VISIBILITY:
  case SLOT_SIDE_TOOLS_VISIBILITY:
  case SLOT_LEFT_TOOLS_VISIBILITY:
  case SLOT_BOTTOM_TOOLS_VISIBILITY:
  case SLOT_EXTRA_TOOLS_VISIBILITY:
  case SLOT_TAB_PAGES_VISIBILITY:
  case SLOT_AUXILIARY_WIDGET_VISIBILITY:
  case SLOT_AUXILIARY_WIDGET:
  case SLOT_LEFT_FOOTER:
  case SLOT_RIGHT_FOOTER:
  case SLOT_MIDDLE_FOOTER:
  case SLOT_SCROLLBARS_VISIBILITY:
  case SLOT_INTERACTIVE_MODE:
  case SLOT_FILE:
    break;

  case SLOT_DESTROY: {
    ASSERT (is_nil (val), "type mismatch");
    if (!is_nil (quit)) quit ();
    the_gui->need_update ();
  } break;

  default:
    qt_widget_rep::send (s, val);
    return;
  }
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_embedded_widget_rep: sent " << slot_name (s)
                  << "\t\tto widget\t" << type_as_string () << LF;
}

blackbox
qt_tm_embedded_widget_rep::query (slot s, int type_id) {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_embedded_widget_rep::query " << slot_name (s) << LF;

  switch (s) {
  case SLOT_IDENTIFIER: {
    if (qwid) {
      widget_rep* wid= qt_window_widget_rep::widget_from_qwidget (qwid);
      if (wid) return wid->query (s, type_id);
    }
    return close_box<int> (0);
  }

  case SLOT_SCROLL_POSITION:
  case SLOT_EXTENTS:
  case SLOT_VISIBLE_PART:
  case SLOT_ZOOM_FACTOR:
  case SLOT_POSITION:
  case SLOT_SIZE:
    if (!is_nil (main_widget)) return main_widget->query (s, type_id);
    else return qt_widget_rep::query (s, type_id);
    /// FIXME: decide what to do with these for embedded widgets
  case SLOT_HEADER_VISIBILITY:
  case SLOT_MAIN_ICONS_VISIBILITY:
  case SLOT_MODE_ICONS_VISIBILITY:
  case SLOT_FOCUS_ICONS_VISIBILITY:
  case SLOT_USER_ICONS_VISIBILITY:
  case SLOT_FOOTER_VISIBILITY:
  case SLOT_SIDE_TOOLS_VISIBILITY:
  case SLOT_LEFT_TOOLS_VISIBILITY:
  case SLOT_BOTTOM_TOOLS_VISIBILITY:
  case SLOT_EXTRA_TOOLS_VISIBILITY:
  case SLOT_TAB_PAGES_VISIBILITY:
  case SLOT_AUXILIARY_WIDGET_VISIBILITY:
    check_type_id<bool> (type_id, s);
    return close_box<bool> (false);

  default:
    return qt_widget_rep::query (s, type_id);
  }
}

widget
qt_tm_embedded_widget_rep::read (slot s, blackbox index) {
  widget ret;

  switch (s) {
  case SLOT_CANVAS:
    check_type_void (index, s);
    ret= main_widget;
    break;
  default:
    return qt_widget_rep::read (s, index);
  }

  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_widget_rep::read " << slot_name (s)
                  << "\t\tfor widget\t" << type_as_string () << LF;

  return ret;
}

void
qt_tm_embedded_widget_rep::write (slot s, blackbox index, widget w) {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_tm_embedded_widget_rep::write " << slot_name (s) << LF;

  switch (s) {
    // Widget w is a qt_simple_widget_rep, with a QTMWidget as underlying
    // widget. We must discard the current QTMWidget and display the new.
    // see qt_tm_widget_rep::write()
  case SLOT_SCROLLABLE: {
    check_type_void (index, s);
    main_widget= w;
  } break;
    /// FIXME: decide what to do with these for embedded widgets
  case SLOT_MAIN_MENU:
  case SLOT_MAIN_ICONS:
  case SLOT_MODE_ICONS:
  case SLOT_FOCUS_ICONS:
  case SLOT_USER_ICONS:
  case SLOT_SIDE_TOOLS:
  case SLOT_LEFT_TOOLS:
  case SLOT_BOTTOM_TOOLS:
  case SLOT_EXTRA_TOOLS:
  case SLOT_TAB_PAGES:
  case SLOT_AUXILIARY_WIDGET:
  case SLOT_INTERACTIVE_INPUT:
  case SLOT_INTERACTIVE_PROMPT:
  default:
    qt_widget_rep::write (s, index, w);
  }
}

QWidget*
qt_tm_embedded_widget_rep::as_qwidget () {
  qwid          = new QWidget ();
  QVBoxLayout* l= new QVBoxLayout ();
  l->setContentsMargins (0, 0, 0, 0);
  qwid->setLayout (l);
  l->addWidget (concrete (main_widget)->as_qwidget ());
  return qwid;
}

QLayoutItem*
qt_tm_embedded_widget_rep::as_qlayoutitem () {
  return new QWidgetItem (as_qwidget ());
}

void
qt_tm_widget_rep::onAddTabRequested () {
  static QTime     lastCallTime;
  static const int MIN_INTERVAL_MS= 500;

  if (lastCallTime.isValid () &&
      lastCallTime.msecsTo (QTime::currentTime ()) < MIN_INTERVAL_MS) {
    return;
  }
  lastCallTime= QTime::currentTime ();

  exec_delayed (scheme_cmd ("(new-document)"));
}

// 登录相关代码
void
qt_tm_widget_rep::setupLoginDialog (QWK::LoginDialog* loginDialog) {
  // 创建登录对话框内容
  QWidget* contentWidget= new QWidget ();
  contentWidget->setObjectName ("login-dialog-content");
  auto mainLayout= new QVBoxLayout (contentWidget);
  mainLayout->setContentsMargins (16, 16, 16, 16);
  mainLayout->setSpacing (16);

  // 顶部区域：头像、名称、账户ID
  auto topSection= new QWidget ();
  auto topLayout = new QHBoxLayout (topSection);
  topLayout->setContentsMargins (0, 0, 0, 0);
  topLayout->setSpacing (12);

  // 左侧：头像
  auto avatarContainer= new QWidget ();
  auto avatarLayout   = new QVBoxLayout (avatarContainer);
  avatarLayout->setContentsMargins (0, 0, 0, 0);
  avatarLayout->setAlignment (Qt::AlignCenter);

  // 头像标签 - 后续通过API设置
  avatarLabel= new QLabel ();
  avatarLabel->setObjectName ("login-avatar-label");
  avatarLabel->setAlignment (Qt::AlignCenter);
  avatarLabel->setText ("Liii"); // 默认值
  avatarLayout->addWidget (avatarLabel);

  // 右侧：名称和账户ID
  auto infoContainer= new QWidget ();
  auto infoLayout   = new QVBoxLayout (infoContainer);
  infoLayout->setContentsMargins (0, 0, 0, 0);
  infoLayout->setSpacing (4);

  // 登出按钮 - 登录成功后显示（使用图标）
  QScreen*     logoutScreen= QGuiApplication::primaryScreen ();
  const double logoutDpi=
      logoutScreen ? logoutScreen->logicalDotsPerInch () : 96.0;
  const double logoutScale= logoutDpi / 96.0;
#if defined(Q_OS_MAC)
  const int logoutIconSize= int (30 * logoutScale);
#else
  const int logoutIconSize= int (20 * logoutScale);
#endif
  logoutButton= new QPushButton ();
  logoutButton->setObjectName ("logout-button");
  logoutButton->setIcon (QIcon (":/window-bar/logout.svg"));
  logoutButton->setToolTip (qt_translate ("Logout"));
  logoutButton->setFlat (true); // 设置为扁平按钮，看起来更像图标
  logoutButton->setIconSize (QSize (logoutIconSize, logoutIconSize));
  // 移除按钮背景色，使其看起来像纯图标
  logoutButton->setStyleSheet (
      "QPushButton { background: transparent; border: none; }");
  logoutButton->setVisible (false); // 初始状态：用户未登录，登出按钮不可见

  // 第一行：名称和登出按钮
  auto nameRowContainer= new QWidget ();
  auto nameRowLayout   = new QHBoxLayout (nameRowContainer);
  nameRowLayout->setContentsMargins (0, 0, 0, 0);
  nameRowLayout->setSpacing (8);

  // 会员名称标签 - 后续通过API设置
  nameLabel= new QLabel (qt_translate ("Not logged in"));
  nameLabel->setObjectName ("login-name-label");

  // 账户ID标签 - 后续通过API设置
  accountIdLabel= new QLabel (
      qt_translate ("Please login to view your account information."));
  accountIdLabel->setObjectName ("login-account-label");

  nameRowLayout->addWidget (nameLabel);
  nameRowLayout->addStretch ();

  infoLayout->addWidget (nameRowContainer);
  infoLayout->addWidget (accountIdLabel);

  topLayout->addWidget (avatarContainer);
  topLayout->addWidget (infoContainer);
  topLayout->addStretch ();
  topLayout->addWidget (logoutButton);

  // 底部区域：会员期限
  auto bottomSection= new QWidget ();
  bottomSection->setObjectName ("login-bottom-section");
  auto bottomLayout   = new QVBoxLayout (bottomSection);
  membershipTitleLabel= new QLabel (qt_translate ("Membership info"));
  membershipTitleLabel->setObjectName ("login-membership-title");

  // 会员期限标签 - 后续通过API设置
  membershipPeriodLabel= new QLabel (qt_translate ("Non-member"));
  membershipPeriodLabel->setObjectName ("login-membership-period");

  // 动作按钮 - 根据用户状态显示登录或注册
  loginActionButton= new QPushButton (qt_translate ("Login"));
  loginActionButton->setObjectName ("login-action-button");

  bottomLayout->addWidget (membershipTitleLabel);
  bottomLayout->addWidget (membershipPeriodLabel);
  bottomLayout->addWidget (loginActionButton);

  // 添加区域到主布局
  mainLayout->addWidget (topSection);
  mainLayout->addWidget (bottomSection);

  // 设置对话框内容
  loginDialog->setContentWidget (contentWidget);

#if defined(Q_OS_MAC)
  // 在 macOS 下将登录对话框内容整体右移 100px：
  if (contentWidget->parentWidget ()) {
    QLayout* parentLayout= contentWidget->parentWidget ()->layout ();
    if (parentLayout) {
      int left, top, right, bottom;
      parentLayout->getContentsMargins (&left, &top, &right, &bottom);
      parentLayout->setContentsMargins (left + 100, top, right, bottom);
      parentLayout->invalidate ();
      loginDialog->updateGeometry ();
      loginDialog->adjustSize ();
    }
  }
#endif

  // 连接按钮信号 - 根据文本动态处理
  QObject::connect (loginActionButton, &QPushButton::clicked, [this] () {
    if (loginActionButton->text () == qt_translate ("Login")) {
      qDebug ("Login button clicked - triggering OAuth2 flow");
      // 触发OAuth2登录流程
      triggerOAuth2 ();
    }
    else {
      // 打开会员购买/续费链接
      qDebug ("打开会员购买/续费链接");
      openRenewalPage ();
    }
  });

  // 连接登出按钮信号
  QObject::connect (logoutButton, &QPushButton::clicked, [this] () {
    qDebug ("Logout button clicked");
    logout ();
  });
}

void
qt_tm_widget_rep::checkLocalTokenAndLogin () {
  // 检查是否为社区版本，如果是则打开官方网址
  if (is_community_stem ()) {
    string pricingUrl=
        as_string (call ("account-oauth2-config", "click-return-liii-url"));
    QDesktopServices::openUrl (QUrl (to_qstring (pricingUrl)));
    return;
  }

  // 使用scheme代码获取本地token缓存
  eval ("(use-modules (liii account))");
  string  token  = as_string (call ("account-load-token"));
  QString q_token= to_qstring (token);
  qDebug ("Cached token: %s", q_token.isEmpty () ? "empty" : "found");

  if (!q_token.isEmpty ()) {
    // 有token，尝试获取用户信息
    fetchUserInfo (q_token);
  }
  else {
    // 没有token，显示登录对话框（用户需要手动点击登录按钮）
    QPoint buttonBottomCenter= loginButton->mapToGlobal (
        QPoint (loginButton->width () / 2, loginButton->height ()));
    m_loginDialog->showAtPosition (buttonBottomCenter);
  }
}

void
qt_tm_widget_rep::fetchUserInfo (const QString& token) {
  // 创建网络访问管理器
  QNetworkAccessManager* manager= new QNetworkAccessManager ();

  // 去掉token末尾的'˙'字符
  QString clean_token= token;
  if (clean_token.endsWith ("˙")) {
    clean_token= clean_token.left (clean_token.length () - 1);
  }

  // 把 "Bearer " 和 token合并成 auth_str
  QString q_auth_str= "Bearer " + clean_token;
  string  auth_str  = from_qstring (q_auth_str);

  // 创建请求
  QNetworkRequest request;
  // 从Scheme配置获取用户信息API URL
  eval ("(use-modules (liii account))");
  string userInfoUrl=
      as_string (call ("account-oauth2-config", "user-info-url"));
  request.setUrl (QUrl (to_qstring (userInfoUrl)));
  request.setRawHeader ("Authorization", to_qstring (auth_str).toUtf8 ());
  request.setRawHeader ("Content-Type", "application/json");
  request.setRawHeader ("User-Agent",
                        to_qstring (stem_user_agent ()).toUtf8 ());
  request.setRawHeader ("X-Device-Id",
                        to_qstring (stem_device_id ()).toUtf8 ());

  // 发送请求
  QNetworkReply* reply= manager->get (request);

  // 连接信号处理响应
  QObject::connect (
      reply, &QNetworkReply::finished, [this, reply, manager, token] () {
        // 定义统一的错误处理逻辑
        auto handleError= [this] (const QString& errorMessage) {
          showNotLoggedInDialog (qt_translate (from_qstring (errorMessage)));

          QPoint buttonBottomCenter= loginButton->mapToGlobal (
              QPoint (loginButton->width () / 2, loginButton->height ()));
          m_loginDialog->showAtPosition (buttonBottomCenter);
        };

        if (reply->error () == QNetworkReply::NoError) {
          // 解析响应数据
          QByteArray    responseData= reply->readAll ();
          QJsonDocument doc         = QJsonDocument::fromJson (responseData);
          QJsonObject   json        = doc.object ();

          if (json.contains ("success") && json["success"].toBool ()) {
            QJsonObject userData= json["data"].toObject ();

            m_userId          = userData["id"].toVariant ().toString ();
            QString userName  = userData["username"].toString ("liii");
            QString avatarText= userData["username"].toString ("liii").left (4);
            QString accountEmail= userData["email"].toString ("liii@lii.pro");
            QString memberType=
                userData["memberType"].toString ("Regular User");
            QString periodLabel=
                userData["periodLabel"].toString ("Non-member");
            QString periodLabelColor=
                userData["periodLabelColor"].toString ("");
            QString productType=
                userData["productType"].toString ("Subscribe Now");

            // 更新弹窗内容
            updateDialogContent (true, userName, accountEmail, avatarText,
                                 memberType, periodLabel, periodLabelColor,
                                 productType);

            // 显示弹窗
            QPoint buttonBottomCenter= loginButton->mapToGlobal (
                QPoint (loginButton->width () / 2, loginButton->height ()));
            m_loginDialog->showAtPosition (buttonBottomCenter);
          }
          else {
            // API返回错误
            handleError ("Login error, please log in again.");
          }
        }
        else {
          // 网络错误或HTTP错误
          handleError ("Network error, please log in later.");
        }

        // 清理资源
        reply->deleteLater ();
        manager->deleteLater ();
      });
}

void
qt_tm_widget_rep::triggerOAuth2 () {
  // 隐藏对话框，因为需要用户进行OAuth2认证
  if (m_loginDialog->isVisible ()) {
    m_loginDialog->hide ();
  }
  // 直接调用scheme代码触发OAuth2登录流程
  eval ("(use-modules (liii account))");
  call ("(login)");
}

void
qt_tm_widget_rep::updateDialogContent (bool isLoggedIn, const QString& username,
                                       const QString& email,
                                       const QString& avatarText,
                                       const QString& memberType,
                                       const QString& periodLabel,
                                       const QString& periodLabelColor,
                                       const QString& productType) {
  // 根据登录状态更新访客提示条可见性
  if (guestNotificationBar) {
    // 社区版不显示提示条
    if (is_community_stem ()) {
      guestNotificationBar->hide ();
    }
    else {
      // 商业版：根据登录状态决定是否显示
      if (isLoggedIn) {
        // 用户已登录，隐藏提示条
        guestNotificationBar->hide ();
      }
      else {
        // 用户未登录，显示提示条
        guestNotificationBar->show ();
      }
    }
  }

  // 更新对话框中的UI组件内容
  if (nameLabel) {
    nameLabel->setText (username);
  }
  if (accountIdLabel) {
    accountIdLabel->setText (email);
  }
  if (avatarLabel) {
    avatarLabel->setText (avatarText);
  }

  // 更新会员类型标题
  if (membershipTitleLabel) {
    membershipTitleLabel->setText (
        qt_translate (memberType.toStdString ().c_str ()));
  }

  // 更新会员期限标签
  if (membershipPeriodLabel) {
    membershipPeriodLabel->setText (
        qt_translate (periodLabel.toStdString ().c_str ()));

    // 根据periodLabelColor设置文本颜色
    if (!periodLabelColor.isEmpty () && periodLabelColor != "undefined") {
      membershipPeriodLabel->setStyleSheet (
          QString ("color: %1;").arg (periodLabelColor));
    }
    else {
      membershipPeriodLabel->setStyleSheet ("");
    }
  }

  // 根据登陆与否更新按钮
  if (!isLoggedIn) {
    loginActionButton->setVisible (true);
    logoutButton->setVisible (false);
    loginActionButton->setText (qt_translate ("Login"));
  }
  else {
    loginActionButton->setVisible (true);
    logoutButton->setVisible (true);
    // 如果productType=Renew Early,后面加上♥️
    if (productType == QStringLiteral ("Renew Early")) {
      loginActionButton->setText (
          qt_translate (productType.toStdString ().c_str ()) + " ♥️");
    }
    else {
      loginActionButton->setText (
          qt_translate (productType.toStdString ().c_str ()));
    }
  }
}

void
qt_tm_widget_rep::showNotLoggedInDialog (const QString& errorMessage) {
  updateDialogContent (false, qt_translate ("Not logged in"), errorMessage,
                       "liii", qt_translate ("Non-member"), "", "", "");
}

void
qt_tm_widget_rep::logout () {
  // 没有token，直接清除UI状态
  showNotLoggedInDialog (
      qt_translate ("Please login to view your account information."));
  // 关闭登录对话框
  if (m_loginDialog && m_loginDialog->isVisible ()) {
    m_loginDialog->hide ();
  }

  // 用户注销后显示访客提示条（除非用户已手动关闭）
  if (guestNotificationBar) {
    // 社区版不显示提示条
    if (is_community_stem ()) {
      guestNotificationBar->hide ();
    }
    else {
      // 商业版：用户注销后显示提示条
      guestNotificationBar->show ();
    }
  }

  // 通过tm_server获取QTMOAuth实例并调用clearInvalidTokens
  if (is_server_started ()) {
    tm_server_rep* server=
        dynamic_cast<tm_server_rep*> (get_server ().operator->());
    if (server && server->getAccount ()) {
      server->getAccount ()->clearInvalidTokens ();
    }
  }
}

void
qt_tm_widget_rep::openRenewalPage () {
  // 获取当前token
  eval ("(use-modules (liii account))");
  string  token  = as_string (call ("account-load-token"));
  QString q_token= to_qstring (token);

  // 获取定价页面URL
  string  pricingUrl= as_string (call ("account-oauth2-config", "pricing-url"));
  QString q_pricingUrl= to_qstring (pricingUrl);

  // 计算token的SHA256哈希值作为key参数
  QByteArray tokenBytes= q_token.toUtf8 ();
  QByteArray hash=
      QCryptographicHash::hash (tokenBytes, QCryptographicHash::Sha256);
  QString keyParam= hash.toHex ();

  // 构建完整URL
  QString fullUrl= q_pricingUrl + "?key=" + keyParam + "&user=" + m_userId;

  // 打开浏览器跳转到续费页面
  QDesktopServices::openUrl (QUrl (fullUrl));
}

void
qt_tm_widget_rep::checkNetworkAvailable () {
  QNetworkAccessManager* manager= new QNetworkAccessManager (mainwindow ());
  QUrl                   testUrl ("https://www.liiistem.cn");
  QNetworkRequest        request (testUrl);
  QNetworkReply*         reply= manager->head (request);

  QObject::connect (reply, &QNetworkReply::finished, [this, reply] () {
    bool success= (reply->error () == QNetworkReply::NoError);
    reply->deleteLater ();
    if (guestNotificationBar) {
      bool isLoggedIn= as_bool (call ("logged-in?"));
      if (isLoggedIn || !success) {
        guestNotificationBar->hide ();
      }
      else {
        guestNotificationBar->show ();
      }
    }
  });
}
