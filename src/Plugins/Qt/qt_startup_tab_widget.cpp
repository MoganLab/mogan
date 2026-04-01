
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

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "s7_tm.hpp"

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
      navTemplateBtn_ (nullptr), navRecentBtn_ (nullptr),
      navSettingsBtn_ (nullptr), navQuitBtn_ (nullptr),
      navButtonGroup_ (nullptr) {

  setMinimumSize (600, 400);
  setFocusPolicy (Qt::NoFocus);

  // 主布局：水平排列，左侧导航栏 + 右侧内容区
  QHBoxLayout* mainLayout= new QHBoxLayout (this);
  mainLayout->setContentsMargins (0, 0, 0, 0);
  mainLayout->setSpacing (0);

  // 左侧导航栏
  QWidget* sidebar= new QWidget (this);
  sidebar->setObjectName ("startup-tab-sidebar"); // 样式在主题CSS中定义
  sidebar->setFixedWidth (120);

  QVBoxLayout* sidebarLayout= new QVBoxLayout (sidebar);
  sidebarLayout->setContentsMargins (8, 16, 8, 16);
  sidebarLayout->setSpacing (4);

  setup_left_sidebar (sidebarLayout);
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
  QLabel* navTitle= new QLabel ("Navigation", this);
  navTitle->setObjectName ("startup-tab-nav-title");
  sidebarLayout->addWidget (navTitle);

  // 创建互斥按钮组
  navButtonGroup_= new QButtonGroup (this);
  navButtonGroup_->setExclusive (true);

  // 导航按钮（4个入口）
  navFileBtn_    = create_nav_button ("File");
  navTemplateBtn_= create_nav_button ("Template");
  navRecentBtn_  = create_nav_button ("Recent");
  navSettingsBtn_= create_nav_button ("Settings");

  // 添加到按钮组和布局
  navButtonGroup_->addButton (navFileBtn_, static_cast<int> (Entry::File));
  navButtonGroup_->addButton (navTemplateBtn_,
                              static_cast<int> (Entry::Template));
  navButtonGroup_->addButton (navRecentBtn_, static_cast<int> (Entry::Recent));
  navButtonGroup_->addButton (navSettingsBtn_,
                              static_cast<int> (Entry::Settings));

  sidebarLayout->addWidget (navFileBtn_);
  sidebarLayout->addWidget (navTemplateBtn_);
  sidebarLayout->addWidget (navRecentBtn_);
  sidebarLayout->addWidget (navSettingsBtn_);

  // 导航按钮点击事件：切换到对应页面
  connect (navFileBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::File); });
  connect (navTemplateBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Template); });
  connect (navRecentBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Recent); });
  connect (navSettingsBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Settings); });

  // 弹性空间，将 Quit 按钮推到底部
  sidebarLayout->addStretch ();
  sidebarLayout->addSpacing (24);

  // Quit 退出按钮
  navQuitBtn_= new QPushButton ("Quit", this);
  navQuitBtn_->setObjectName ("startup-tab-quit-btn");
  navQuitBtn_->setFocusPolicy (Qt::NoFocus);
  navQuitBtn_->setCursor (Qt::PointingHandCursor);
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
  return btn;
}

/**
 * @brief 设置右侧内容区
 * @param stackedWidget 堆叠控件，用于页面切换
 */
void
QTStartupTabWidget::setup_right_content (QStackedWidget* stackedWidget) {
  // 添加4个页面到堆叠控件
  stackedWidget->addWidget (create_file_page ());     // index 0 - File
  stackedWidget->addWidget (create_template_page ()); // index 1 - Template
  stackedWidget->addWidget (create_recent_page ());   // index 2 - Recent
  stackedWidget->addWidget (create_settings_page ()); // index 3 - Settings

  // 入口切换时，同步切换堆叠控件的当前页面
  connect (this, &QTStartupTabWidget::entry_changed, stackedWidget,
           [stackedWidget] (QTStartupTabWidget::Entry entry) {
             int index= static_cast<int> (entry);
             stackedWidget->setCurrentIndex (index);
           });
}

/**
 * @brief 创建 File 页面
 * @return File 页面控件
 *
 * 页面内容:
 * - 标题 "File"
 * - 说明文字
 * - New Document 按钮（主按钮）
 * - Open Document 按钮（次按钮）
 */
QWidget*
QTStartupTabWidget::create_file_page () {
  QWidget*     page  = new QWidget (this);
  QVBoxLayout* layout= new QVBoxLayout (page);
  layout->setContentsMargins (32, 32, 32, 32);

  // 按钮行布局
  QHBoxLayout* btnLayout= new QHBoxLayout;
  btnLayout->setSpacing (16);

  // New Document 按钮（主按钮）
  QPushButton* newBtn= new QPushButton ("New Document", page);
  newBtn->setObjectName ("startup-tab-primary-btn");
  newBtn->setFocusPolicy (Qt::NoFocus);
  newBtn->setCursor (Qt::PointingHandCursor);
  connect (newBtn, &QPushButton::clicked, this,
           &QTStartupTabWidget::on_file_new);
  btnLayout->addWidget (newBtn);

  // Open Document 按钮（次按钮）
  QPushButton* openBtn= new QPushButton ("Open Document", page);
  openBtn->setObjectName ("startup-tab-secondary-btn");
  openBtn->setFocusPolicy (Qt::NoFocus);
  openBtn->setCursor (Qt::PointingHandCursor);
  connect (openBtn, &QPushButton::clicked, this,
           &QTStartupTabWidget::on_file_open);
  btnLayout->addWidget (openBtn);

  btnLayout->addStretch ();
  layout->addLayout (btnLayout);

  layout->addStretch ();
  return page;
}

/**
 * @brief 创建 Template 页面（占位）
 */
QWidget*
QTStartupTabWidget::create_template_page () {
  QWidget*     page  = new QWidget (this);
  QVBoxLayout* layout= new QVBoxLayout (page);
  layout->setContentsMargins (32, 32, 32, 32);

  QLabel* title= new QLabel ("Template Center", page);
  title->setObjectName ("startup-tab-page-title");
  layout->addWidget (title);

  QLabel* desc= new QLabel (
      "Coming soon: Browse and download templates from Gitee Releases.", page);
  desc->setObjectName ("startup-tab-page-desc");
  layout->addWidget (desc);

  layout->addStretch ();
  return page;
}

/**
 * @brief 创建 Recent 页面（占位）
 */
QWidget*
QTStartupTabWidget::create_recent_page () {
  QWidget*     page  = new QWidget (this);
  QVBoxLayout* layout= new QVBoxLayout (page);
  layout->setContentsMargins (32, 32, 32, 32);

  QLabel* title= new QLabel ("Recent Documents", page);
  title->setObjectName ("startup-tab-page-title");
  layout->addWidget (title);

  QLabel* desc= new QLabel (
      "Coming soon: Recently opened documents will appear here.", page);
  desc->setObjectName ("startup-tab-page-desc");
  layout->addWidget (desc);

  layout->addStretch ();
  return page;
}

/**
 * @brief 创建 Settings 页面（占位）
 */
QWidget*
QTStartupTabWidget::create_settings_page () {
  QWidget*     page  = new QWidget (this);
  QVBoxLayout* layout= new QVBoxLayout (page);
  layout->setContentsMargins (32, 32, 32, 32);

  QLabel* title= new QLabel ("Settings", page);
  title->setObjectName ("startup-tab-page-title");
  layout->addWidget (title);

  QLabel* desc= new QLabel (
      "Coming soon: Configure startup tab behavior and preferences.", page);
  desc->setObjectName ("startup-tab-page-desc");
  layout->addWidget (desc);

  layout->addStretch ();
  return page;
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
 * @brief 新建文档
 * 调用 Scheme 函数 (new-document)
 */
void
QTStartupTabWidget::on_file_new () {
  eval_scheme ("(new-document)");
}

/**
 * @brief 打开文档
 * 调用 Scheme 函数 (open-document)
 */
void
QTStartupTabWidget::on_file_open () {
  eval_scheme ("(open-document)");
}

/**
 * @brief 退出程序
 * 调用 Scheme 函数 (quit-TeXmacs)
 */
void
QTStartupTabWidget::on_app_quit () {
  eval_scheme ("(quit-TeXmacs)");
}
