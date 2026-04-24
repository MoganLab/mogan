
/******************************************************************************
 * MODULE     : qt_startup_tab_widget.cpp
 * DESCRIPTION: Startup tab widget with left sidebar for Mogan STEM
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_startup_tab_widget.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_file_page.hpp"
#include "qt_settings_page.hpp"
#include "qt_template_page.hpp"
#include "qt_utilities.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "s7_tm.hpp"

namespace {
constexpr int kMinWidth        = 600; // 启动页最小宽度
constexpr int kMinHeight       = 400; // 启动页最小高度
constexpr int kSidebarMinWidth = 120; // 左侧导航栏最小宽度
constexpr int kSidebarMarginX  = 8;   // 左侧导航栏水平内边距
constexpr int kSidebarMarginY  = 16;  // 左侧导航栏垂直内边距
constexpr int kSidebarSpacing  = 4;   // 左侧导航栏控件间距
constexpr int kQuitTopSpacing  = 24;  // Quit 按钮上方额外间距
constexpr int kNavTitlePadding = 4;   // Navigation 标题内边距
constexpr int kNavTitleFontPx  = 11;  // Navigation 标题字号
constexpr int kNavButtonPadY   = 8;   // 导航按钮纵向内边距
constexpr int kNavButtonPadX   = 12;  // 导航按钮横向内边距
constexpr int kNavButtonFontPx = 13;  // 导航按钮字号
constexpr int kQuitBorderWidth = 1;   // Quit 按钮边框宽度
constexpr int kQuitBorderRadius= 4;   // Quit 按钮圆角
constexpr int kQuitPadY        = 8;   // Quit 按钮纵向内边距
constexpr int kQuitPadX        = 12;  // Quit 按钮横向内边距
constexpr int kQuitButtonFontPx= 13;  // Quit 按钮字号
} // namespace

/**
 * @brief 构造函数 - 初始化启动标签页界面
 *
 * 布局结构:
 * +------------------+----------------------------------------+
 * |  左侧导航栏       |              右侧内容区                  |
 * |  (120px固定宽度)  |              (自适应剩余宽度)            |
 * +------------------+----------------------------------------+
 */
QTStartupTabWidget::QTStartupTabWidget (QWidget* parent)
    : QWidget (parent), currentEntry_ (Entry::File), navFileBtn_ (nullptr),
      navTemplateBtn_ (nullptr), navOpenDocBtn_ (nullptr),
      navSettingsBtn_ (nullptr), navQuitBtn_ (nullptr),
      navButtonGroup_ (nullptr), filePage_ (nullptr), settingsPage_ (nullptr),
      templatePage_ (nullptr) {

  setMinimumSize (DpiUtils::scaled (kMinWidth), DpiUtils::scaled (kMinHeight));
  setFocusPolicy (Qt::StrongFocus);

  // 主布局：水平排列，左侧导航栏 + 右侧内容区
  QHBoxLayout* mainLayout= new QHBoxLayout (this);
  mainLayout->setContentsMargins (0, 0, 0, 0);
  mainLayout->setSpacing (0);

  // 左侧导航栏
  QWidget* sidebar= new QWidget (this);
  sidebar->setObjectName ("startup-tab-sidebar"); // 样式在主题CSS中定义
  sidebar->setMinimumWidth (DpiUtils::scaled (kSidebarMinWidth));
  sidebar->setSizePolicy (QSizePolicy::Minimum, QSizePolicy::Preferred);

  QVBoxLayout* sidebarLayout= new QVBoxLayout (sidebar);
  sidebarLayout->setContentsMargins (
      DpiUtils::scaled (kSidebarMarginX), DpiUtils::scaled (kSidebarMarginY),
      DpiUtils::scaled (kSidebarMarginX), DpiUtils::scaled (kSidebarMarginY));
  sidebarLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  setup_left_sidebar (sidebarLayout);
  sidebar->adjustSize ();
  const int contentWidth= sidebar->sizeHint ().width ();
  sidebar->setFixedWidth (
      qMax (DpiUtils::scaled (kSidebarMinWidth), contentWidth));
  mainLayout->addWidget (sidebar);

  // 右侧内容区（使用堆叠控件切换不同页面）
  QStackedWidget* stackedWidget= new QStackedWidget (this);
  stackedWidget->setObjectName ("startup-tab-content"); // 样式在主题CSS中定义
  setup_right_content (stackedWidget);
  mainLayout->addWidget (stackedWidget, 1);
}

QTStartupTabWidget::Entry
QTStartupTabWidget::current_entry () const {
  return currentEntry_;
}

/**
 * @brief 切换当前入口（页面）
 * @param entry 目标入口（File/Template/Recent/Settings）
 */
void
QTStartupTabWidget::set_current_entry (Entry entry) {
  if (currentEntry_ != entry) {
    currentEntry_= entry;
    emit entry_changed (entry); // 通知右侧内容区切换页面
  }
  set_active_nav_button (entry); // 更新导航按钮选中状态（无论是否变化都更新）
  refresh_recent_docs_on_file_entry (entry);
}

void
QTStartupTabWidget::refresh_recent_docs_on_file_entry (Entry entry) {
  if (entry == Entry::File && filePage_) {
    filePage_->refreshRecentDocs ();
  }
}

/**
 * @brief 设置左侧导航栏
 * @param sidebarLayout 导航栏的垂直布局
 *
 * 导航栏结构:
 * - Navigation 标题
 * - File/Template/Recent/Settings 导航按钮（互斥选中）
 * - 弹性空间（弹簧）
 * - Quit 退出按钮
 */
void
QTStartupTabWidget::setup_left_sidebar (QVBoxLayout* sidebarLayout) {
  // Navigation 分组标题
  QLabel* navTitle= new QLabel (qt_translate ("Navigation"), this);
  navTitle->setObjectName ("startup-tab-nav-title");
  DpiUtils::applyScaledFont (navTitle, kNavTitleFontPx);
  navTitle->setContentsMargins (
      DpiUtils::scaled (kNavTitlePadding), DpiUtils::scaled (kNavTitlePadding),
      DpiUtils::scaled (kNavTitlePadding), DpiUtils::scaled (kNavTitlePadding));
  sidebarLayout->addWidget (navTitle);

  // 创建互斥按钮组
  navButtonGroup_= new QButtonGroup (this);
  navButtonGroup_->setExclusive (true);

  // 导航按钮（4个入口）
  navFileBtn_    = create_nav_button (qt_translate ("File"));
  navTemplateBtn_= create_nav_button (qt_translate ("Template"));
  navOpenDocBtn_ = create_nav_button (qt_translate ("Open a Document"));
  navSettingsBtn_= create_nav_button (qt_translate ("Settings"));

  // 添加到按钮组和布局（Open a Document 不在互斥组中，因为它没有对应页面）
  navButtonGroup_->addButton (navFileBtn_, static_cast<int> (Entry::File));
  navButtonGroup_->addButton (navTemplateBtn_,
                              static_cast<int> (Entry::Template));
  navButtonGroup_->addButton (navSettingsBtn_,
                              static_cast<int> (Entry::Settings));

  sidebarLayout->addWidget (navFileBtn_);
  sidebarLayout->addWidget (navTemplateBtn_);
  sidebarLayout->addWidget (navOpenDocBtn_);
  sidebarLayout->addWidget (navSettingsBtn_);

  // 导航按钮点击事件：切换到对应页面
  connect (navFileBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::File); });
  connect (navTemplateBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Template); });
  connect (navOpenDocBtn_, &QPushButton::clicked, this,
           &QTStartupTabWidget::on_file_open);
  connect (navSettingsBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Settings); });

  // Open a Document 不是 checkable 按钮（没有对应页面）
  navOpenDocBtn_->setCheckable (false);

  // 弹性空间，将 Quit 按钮推到底部
  sidebarLayout->addStretch ();
  sidebarLayout->addSpacing (DpiUtils::scaled (kQuitTopSpacing));

  // Quit 退出按钮
  navQuitBtn_= new QPushButton (qt_translate ("Quit"), this);
  navQuitBtn_->setObjectName ("startup-tab-quit-btn");
  navQuitBtn_->setFocusPolicy (Qt::NoFocus);
  navQuitBtn_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (navQuitBtn_, kQuitButtonFontPx);
  navQuitBtn_->setStyleSheet (
      QString ("border-width: %1px; border-radius: %2px; padding: %3px %4px;")
          .arg (DpiUtils::scaled (kQuitBorderWidth))
          .arg (DpiUtils::scaled (kQuitBorderRadius))
          .arg (DpiUtils::scaled (kQuitPadY))
          .arg (DpiUtils::scaled (kQuitPadX)));
  connect (navQuitBtn_, &QPushButton::clicked, this,
           &QTStartupTabWidget::on_app_quit);
  sidebarLayout->addWidget (navQuitBtn_);

  // 默认选中 File
  navFileBtn_->setChecked (true);
}

/**
 * @brief 创建导航按钮（辅助函数）
 * @param text 按钮文字
 * @return 配置好的 QPushButton
 */
QPushButton*
QTStartupTabWidget::create_nav_button (const QString& text) {
  QPushButton* btn= new QPushButton (text, this);
  btn->setObjectName ("startup-tab-nav-btn"); // 样式在主题CSS中定义
  btn->setFocusPolicy (Qt::NoFocus);
  btn->setCheckable (true); // 支持选中状态
  btn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (btn, kNavButtonFontPx);
  btn->setStyleSheet (QString ("padding: %1px %2px;")
                          .arg (DpiUtils::scaled (kNavButtonPadY))
                          .arg (DpiUtils::scaled (kNavButtonPadX)));
  return btn;
}

/**
 * @brief 设置右侧内容区
 * @param stackedWidget 堆叠控件，用于页面切换
 */
void
QTStartupTabWidget::setup_right_content (QStackedWidget* stackedWidget) {
  // 添加3个页面到堆叠控件（OpenDocument没有页面，直接触发操作）
  stackedWidget->addWidget (create_file_page ());     // index 0 - File
  stackedWidget->addWidget (create_template_page ()); // index 1 - Template
  stackedWidget->addWidget (create_settings_page ()); // index 2 - Settings

  // 入口切换时，同步切换堆叠控件的当前页面
  // 注意：OpenDocument 没有对应页面，需要调整索引映射
  connect (this, &QTStartupTabWidget::entry_changed, stackedWidget,
           [stackedWidget] (QTStartupTabWidget::Entry entry) {
             int index;
             switch (entry) {
             case QTStartupTabWidget::Entry::File:
               index= 0;
               break;
             case QTStartupTabWidget::Entry::Template:
               index= 1;
               break;
             case QTStartupTabWidget::Entry::Settings:
               index= 2;
               break;
             default:
               index= 0;
               break;
             }
             stackedWidget->setCurrentIndex (index);
           });
}

/**
 * @brief 创建 File 页面
 * @return File 页面控件
 *
 * 使用 QtFilePage 实现，包含:
 * - 文档样式选择卡片
 * - 最近文档列表
 * - 打开文件按钮
 */
QWidget*
QTStartupTabWidget::create_file_page () {
  filePage_= new QtFilePage (this);
  return filePage_;
}

/**
 * @brief 创建 Template 页面
 */
QWidget*
QTStartupTabWidget::create_template_page () {
  templatePage_= new QTTemplatePage (this);
  templatePage_->initialize ();

  // Connect template opened signal to load document
  connect (templatePage_, &QTTemplatePage::templateOpened, this,
           [this] (const QString& filePath) {
             if (filePage_) {
               filePage_->addRecentDoc (filePath);
             }
             eval_scheme ("(load-document " * qt_scheme_quote_utf8 (filePath) *
                          ")");
           });

  return templatePage_;
}

/**
 * @brief 创建 Settings 页面
 */
QWidget*
QTStartupTabWidget::create_settings_page () {
  settingsPage_= new QTSettingsPage (this);
  return settingsPage_;
}

/**
 * @brief 更新导航按钮的选中状态
 * @param entry 当前选中的入口
 *
 * 使用 QButtonGroup 的互斥特性，自动取消其他按钮的选中状态
 */
void
QTStartupTabWidget::set_active_nav_button (Entry entry) {
  QAbstractButton* btn= navButtonGroup_->button (static_cast<int> (entry));
  if (btn) {
    btn->setChecked (true);
  }
}

/**
 * @brief 退出程序
 * 调用 Scheme 函数 (quit-TeXmacs)
 */
void
QTStartupTabWidget::on_app_quit () {
  eval_scheme ("(quit-TeXmacs)");
}

/**
 * @brief 打开文档
 * 调用 Scheme 函数 (open-document)
 */
void
QTStartupTabWidget::on_file_open () {
  eval_scheme ("(open-document)");
}

void
QTStartupTabWidget::keyPressEvent (QKeyEvent* event) {
  string key= from_key_press_event (event);
  if (is_empty (key)) return QWidget::keyPressEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}

void
QTStartupTabWidget::keyReleaseEvent (QKeyEvent* event) {
  string key= from_key_release_event (event);
  if (is_empty (key)) return QWidget::keyReleaseEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}
