
/******************************************************************************
 * MODULE     : QTMStartupTabWidget.cpp
 * DESCRIPTION: Startup tab widget with left sidebar for Mogan STEM
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMStartupTabWidget.hpp"
#include "QTMHomePage.hpp"
#include "QTMTemplatePage.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include "template_manager.hpp"

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
constexpr int kSidebarMinWidth = 150; // 左侧导航栏最小宽度
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
 * |  (150px固定宽度)  |              (自适应剩余宽度)            |
 * +------------------+----------------------------------------+
 */
QTMStartupTabWidget::QTMStartupTabWidget (QWidget* parent)
    : QWidget (parent), currentEntry_ (Entry::Home), navHomeBtn_ (nullptr),
      navQuitBtn_ (nullptr), categoryLayout_ (nullptr),
      navButtonGroup_ (nullptr), homePage_ (nullptr), templatePage_ (nullptr),
      templateManager_ (nullptr) {

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

QTMStartupTabWidget::Entry
QTMStartupTabWidget::current_entry () const {
  return currentEntry_;
}

/**
 * @brief 切换当前入口（页面）
 * @param entry 目标入口（Home/Template）
 */
void
QTMStartupTabWidget::set_current_entry (Entry entry) {
  if (currentEntry_ != entry) {
    currentEntry_= entry;
    emit entry_changed (entry); // 通知右侧内容区切换页面
  }
  set_active_nav_button (entry); // 更新导航按钮选中状态（无论是否变化都更新）
  refresh_recent_docs_on_file_entry (entry);
}

void
QTMStartupTabWidget::refresh_recent_docs_on_file_entry (Entry entry) {
  if (entry == Entry::Home && homePage_) {
    homePage_->refreshRecentDocs ();
  }
}

/**
 * @brief 设置左侧导航栏
 * @param sidebarLayout 导航栏的垂直布局
 *
 * 导航栏结构:
 * - Navigation 标题
 * - Home/Template/ 导航按钮（互斥选中）
 * - 弹性空间（弹簧）
 * - Quit 退出按钮
 */
void
QTMStartupTabWidget::setup_left_sidebar (QVBoxLayout* sidebarLayout) {
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

  navHomeBtn_= create_nav_button (qt_translate ("Home"));
  navButtonGroup_->addButton (navHomeBtn_, static_cast<int> (Entry::Home));
  sidebarLayout->addWidget (navHomeBtn_);

  // 导航按钮点击事件：切换到对应页面
  connect (navHomeBtn_, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Home); });

  // Category buttons container (populated when categories load)
  QWidget* categoryContainer= new QWidget (this);
  categoryContainer->setObjectName ("startup-tab-category-container");
  categoryLayout_= new QVBoxLayout (categoryContainer);
  categoryLayout_->setContentsMargins (0, 0, 0, 0);
  categoryLayout_->setSpacing (DpiUtils::scaled (kSidebarSpacing));
  sidebarLayout->addWidget (categoryContainer);

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
           &QTMStartupTabWidget::on_app_quit);
  sidebarLayout->addWidget (navQuitBtn_);

  // 默认选中 Home
  navHomeBtn_->setChecked (true);

  // Connect to TemplateManager for dynamic categories
  templateManager_= TemplateManager::instance ();
  connect (templateManager_, &TemplateManager::categoriesLoaded, this,
           &QTMStartupTabWidget::onCategoriesLoaded, Qt::UniqueConnection);

  // If categories already loaded, set up immediately
  if (templateManager_->isInitialized () &&
      !templateManager_->categories ().isEmpty ()) {
    setupCategoryNavButtons ();
  }
}

/**
 * @brief 创建导航按钮（辅助函数）
 * @param text 按钮文字
 * @return 配置好的 QPushButton
 */
void
QTMStartupTabWidget::setupCategoryNavButtons () {
  if (!templateManager_ || !categoryLayout_) return;

  clearCategoryNavButtons ();

  auto categories= templateManager_->categories ();
  for (const auto& cat : categories) {
    QPushButton* btn= create_nav_button (cat.name);
    btn->setProperty ("categoryId", cat.id);
    btn->setProperty ("name", cat.name);
    navButtonGroup_->addButton (btn);
    categoryLayout_->addWidget (btn);
    navCategoryBtns_.append (btn);

    connect (btn, &QPushButton::clicked, this,
             &QTMStartupTabWidget::onCategoryClicked);
  }
}

void
QTMStartupTabWidget::clearCategoryNavButtons () {
  for (QPushButton* btn : navCategoryBtns_) {
    if (btn) {
      navButtonGroup_->removeButton (btn);
      btn->deleteLater ();
    }
  }
  navCategoryBtns_.clear ();
}

void
QTMStartupTabWidget::onCategoryClicked () {
  QPushButton* btn= qobject_cast<QPushButton*> (sender ());
  if (!btn) return;

  QString categoryId= btn->property ("categoryId").toString ();
  if (categoryId.isEmpty ()) return;

  currentCategory_= categoryId;
  if (templatePage_) {
    QString name= btn->property ("name").toString ();
    templatePage_->setCategory (categoryId, name);
  }

  if (templateManager_) {
    templateManager_->refreshTemplatesByCategory (categoryId);
  }

  set_current_entry (Entry::Template);
}

void
QTMStartupTabWidget::onCategoriesLoaded () {
  setupCategoryNavButtons ();
}

QPushButton*
QTMStartupTabWidget::create_nav_button (const QString& text) {
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
QTMStartupTabWidget::setup_right_content (QStackedWidget* stackedWidget) {
  // 添加2个页面到堆叠控件
  stackedWidget->addWidget (create_home_page ());     // index 0 - Home
  stackedWidget->addWidget (create_template_page ()); // index 1 - Template

  // 入口切换时，同步切换堆叠控件的当前页面
  connect (this, &QTMStartupTabWidget::entry_changed, stackedWidget,
           [stackedWidget] (QTMStartupTabWidget::Entry entry) {
             int index;
             switch (entry) {
             case QTMStartupTabWidget::Entry::Home:
               index= 0;
               break;
             case QTMStartupTabWidget::Entry::Template:
               index= 1;
               break;
             default:
               index= 0;
               break;
             }
             stackedWidget->setCurrentIndex (index);
           });
}

/**
 * @brief 创建 Home 页面
 * @return Home 页面控件
 *
 * 使用 QTMHomePage 实现，包含:
 * - 文档样式选择卡片（新建、打开、模板）
 * - 最近文档列表
 */
QWidget*
QTMStartupTabWidget::create_home_page () {
  homePage_= new QTMHomePage (this);
  return homePage_;
}

/**
 * @brief 创建 Template 页面
 */
QWidget*
QTMStartupTabWidget::create_template_page () {
  templatePage_= new QTMTemplatePage (this);
  templatePage_->initialize ();
  return templatePage_;
}

/**
 * @brief 更新导航按钮的选中状态
 * @param entry 当前选中的入口
 *
 * 使用 QButtonGroup 的互斥特性，自动取消其他按钮的选中状态
 */
void
QTMStartupTabWidget::set_active_nav_button (Entry entry) {
  QAbstractButton* btn= navButtonGroup_->button (static_cast<int> (entry));
  if (btn) {
    btn->setChecked (true);
    return;
  }

  // For Template entry, activate the matching category button
  if (entry == Entry::Template) {
    for (QPushButton* catBtn : navCategoryBtns_) {
      if (catBtn &&
          catBtn->property ("categoryId").toString () == currentCategory_) {
        catBtn->setChecked (true);
        return;
      }
    }
  }
}

/**
 * @brief 退出程序
 * 调用 Scheme 函数 (quit-TeXmacs)
 */
void
QTMStartupTabWidget::on_app_quit () {
  eval_scheme ("(quit-TeXmacs)");
}

void
QTMStartupTabWidget::keyPressEvent (QKeyEvent* event) {
  string key= from_key_press_event (event);
  if (is_empty (key)) return QWidget::keyPressEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}

void
QTMStartupTabWidget::keyReleaseEvent (QKeyEvent* event) {
  string key= from_key_release_event (event);
  if (is_empty (key)) return QWidget::keyReleaseEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}
