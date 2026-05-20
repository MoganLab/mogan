
/******************************************************************************
 * MODULE     : qt_chat_tab_widget.cpp
 * DESCRIPTION: Mogan STEM 的 LLM 聊天标签页控件
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_chat_tab_widget.hpp"
#include "QTMGuiHelper.hpp"
#include "QTMStyle.hpp"
#include "QTMWidget.hpp"
#include "edit_interface.hpp"
#include "new_buffer.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_gui.hpp"
#include "qt_utilities.hpp"
#include "qt_widget.hpp"
#include "s7_tm.hpp"
#include "tm_window.hpp"

#include <lolly/hash/uuid.hpp>
#include <moebius/tree_label.hpp>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QCheckBox>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpacerItem>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariantAnimation>

using namespace moebius;

namespace {

/**
 * @brief 从文档树中提取纯文本标题。
 * @param body TeXmacs 文档树。
 * @param maxLen 标题最大字符数。
 * @return 提取的标题字符串。
 */
string
extract_title (tree body, int maxLen) {
  string result;
  if (is_func (body, DOCUMENT)) {
    for (int i= 0; i < N (body) && N (result) < maxLen; ++i) {
      if (is_atomic (body[i])) result << body[i]->label;
    }
  }
  if (N (result) > maxLen) {
    result= result (0, maxLen) * "...";
  }
  return result;
}

/**
 * @brief 返回聊天控件使用的嵌入样式树。
 * @return 包含 "generic" 和 "llm" 标签的复合样式树。
 */
tree
make_chat_embedded_style () {
  return compound ("style", tuple ("generic", "llm"));
}

/**
 * @brief 递归禁用所有 QAbstractScrollArea 后代控件的滚动条。
 * @param root 搜索起始的根控件。
 */
void
disable_scrollbars_recursively (QWidget* root) {
  if (!root) return;

  QList<QAbstractScrollArea*> scrollAreas=
      root->findChildren<QAbstractScrollArea*> ();
  for (QAbstractScrollArea* area : scrollAreas) {
    if (!area) continue;
    area->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    if (area->horizontalScrollBar ()) area->horizontalScrollBar ()->hide ();
    if (area->verticalScrollBar ()) area->verticalScrollBar ()->hide ();
  }
}

/// 左侧边栏最小宽度（像素）。
constexpr int kSidebarMinWidth= 200;
/// 边栏布局水平边距。
constexpr int kSidebarMarginX= 12;
/// 边栏布局垂直边距。
constexpr int kSidebarMarginY= 16;
/// 边栏元素间距。
constexpr int kSidebarSpacing= 8;
/// 导航标题字体大小（像素）。
constexpr int kNavTitleFontPx= 18;
/// 导航标题内边距。
constexpr int kNavTitlePadding= 4;
/// 导航按钮垂直内边距。
constexpr int kNavButtonPadY= 8;
/// 导航按钮水平内边距。
constexpr int kNavButtonPadX= 8;
/// 导航按钮字体大小（像素）。
constexpr int kNavButtonFontPx= 13;
/// 收缩按钮字体大小（像素）。
constexpr int kCollapseFontPx= 11;
/// 收缩按钮圆角半径。
constexpr int kCollapseBorderRadius= 4;
/// 收缩按钮垂直内边距。
constexpr int kCollapsePadY= 4;
/// 收缩按钮水平内边距。
constexpr int kCollapsePadX= 8;
/// 欢迎标题字体大小（像素）。
constexpr int kWelcomeFontPx= 34;
/// 输入编辑器单行高度（像素）。
constexpr int kInputLineHeight= 22;
/// 输入编辑器默认行数。
constexpr int kInputDefaultLines= 3;
/// 输入编辑器最大行数。
constexpr int kInputMaxLines= 10;
/// 发送按钮垂直内边距。
constexpr int kSendButtonPadY= 6;
/// 发送按钮水平内边距。
constexpr int kSendButtonPadX= 16;
/// 发送按钮字体大小（像素）。
constexpr int kSendButtonFontPx= 13;
/// 内容区水平边距。
constexpr int kContentMarginX= 24;
/// 内容区垂直边距。
constexpr int kContentMarginY= 24;
/// 内容区元素间距。
constexpr int kContentSpacing= 16;
/// 欢迎模式下顶部占位高度（像素）。
constexpr int kWelcomeTopOffsetY= 240;
/// 会话模式下顶部占位高度（像素）。
constexpr int kConversationTopOffsetY= 24;
/// 顶部面板最大宽度（像素）。
constexpr int kTopPanelMaxWidth= 680;
/// 输入/消息框圆角半径。
constexpr int kInputFrameRadius= 8;
/// 输入/消息框边框宽度（像素）。
constexpr int kInputFrameBorder= 1;
/// 输入/消息框内边距。
constexpr int kInputFramePad= 8;
/// 消息展示区最小高度（像素）。
constexpr int kMessageMinHeight= 240;
/// 欢迎态到会话态过渡动画时长（毫秒）。
constexpr int kTransitionDurationMs= 220;
/// 侧边栏切换按钮尺寸（像素）。
constexpr int kToggleBtnSize= 40;
/// 侧边栏切换按钮图标尺寸（像素）。
constexpr int kToggleIconSize= 20;
/// 浮球展开按钮水平边距（像素）。
constexpr int kFloatingBtnMarginX= 12;
/// 浮球展开按钮垂直边距（像素）。
constexpr int kFloatingBtnMarginY= 120;

} // namespace

/**
 * @brief 判断文档主体是否实际为空。
 * @param body TeXmacs 文档树。
 * @return 若主体不含可见内容则返回 true。
 */
bool
QTChatTabWidget::is_empty_document_body (tree body) {
  if (!is_func (body, DOCUMENT)) return false;
  if (N (body) == 0) return true;
  return N (body) == 1 && is_atomic (body[0]) && body[0]->label == "";
}

/**
 * @brief 单个会话面板的内部数据。
 */
struct QTChatTabWidget::ChatConversationPanel {
  QWidget*     pageWidget       = nullptr; ///< 本会话的堆叠页面。
  QLabel*      welcomeTitle     = nullptr; ///< 欢迎标题标签。
  QLabel*      modelLabel       = nullptr; ///< 模型名称标签。
  QWidget*     messageFrame     = nullptr; ///< 承载消息控件的边框。
  QWidget*     inputEditorWidget= nullptr; ///< 输入编辑器的 Qt 控件。
  QPushButton* sendButton       = nullptr; ///< 发送/停止按钮。
  QPushButton* sidebarButton    = nullptr; ///< 侧边栏入口按钮。
  QCheckBox*   selectCheckBox   = nullptr; ///< 多选复选框（仅多选模式可见）。
  QWidget*     itemWidget= nullptr; ///< 包含 checkbox + sidebarButton 的容器。
  QSpacerItem* topSpacer = nullptr; ///< 顶部占位，用于垂直偏移。
  widget       messageWidget;       ///< 消息显示的 TeXmacs 控件。
  widget       inputWidget;         ///< 用户输入的 TeXmacs 控件。
  string       sessionId;           ///< 会话 UUID（跨层交互标识）。
  bool         conversationMode= false; ///< 面板是否已离开欢迎态。
};

/**
 * @brief 构造聊天标签页控件。
 *
 * 创建左侧边栏和右侧内容区，然后创建第一个会话。
 */
QTChatTabWidget::QTChatTabWidget (QWidget* parent)
    : QWidget (parent), sidebarWidget_ (nullptr), contentWidget_ (nullptr),
      conversationCountLabel_ (nullptr), conversationListWidget_ (nullptr),
      conversationListLayout_ (nullptr), archiveHeaderButton_ (nullptr),
      archiveListWidget_ (nullptr), archiveListLayout_ (nullptr),
      archiveCollapsed_ (true), newChatButton_ (nullptr),
      collapseButton_ (nullptr), floatingExpandBtn_ (nullptr),
      sidebarNormalContent_ (nullptr), conversationStack_ (nullptr),
      activeConversation_ (nullptr), sidebarCollapsed_ (false),
      sidebarExpandedWidth_ (0), chatMenuToolBar_ (nullptr),
      chatModeToolBar_ (nullptr), chatFocusToolBar_ (nullptr),
      multiSelectMode_ (false), archiveSelectMode_ (false),
      multiSelectBar_ (nullptr), batchArchiveBtn_ (nullptr),
      searchEdit_ (nullptr) {
  setFocusPolicy (Qt::StrongFocus);

  QHBoxLayout* mainLayout= new QHBoxLayout (this);
  mainLayout->setContentsMargins (0, 0, 0, 0);
  mainLayout->setSpacing (0);

  // 左侧侧边栏
  QWidget* sidebar= new QWidget (this);
  sidebar->setObjectName ("chat-tab-sidebar");
  sidebar->setMinimumWidth (DpiUtils::scaled (kSidebarMinWidth));
  sidebar->setSizePolicy (QSizePolicy::Minimum, QSizePolicy::Preferred);
  sidebar->setStyleSheet ("background-color: #f5f5f5;");
  sidebarWidget_= sidebar;

  QVBoxLayout* sidebarLayout= new QVBoxLayout (sidebar);
  sidebarLayout->setContentsMargins (
      DpiUtils::scaled (kSidebarMarginX), DpiUtils::scaled (kSidebarMarginY),
      DpiUtils::scaled (kSidebarMarginX), DpiUtils::scaled (kSidebarMarginY));
  sidebarLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  setup_left_sidebar (sidebarLayout);
  sidebar->adjustSize ();
  const int contentWidth= sidebar->sizeHint ().width ();
  sidebarExpandedWidth_=
      qMax (DpiUtils::scaled (kSidebarMinWidth), contentWidth);
  sidebar->setFixedWidth (sidebarExpandedWidth_);
  mainLayout->addWidget (sidebar);

  // 右侧内容区
  setup_right_content (mainLayout);
  loadSessions ();
  if (conversations_.isEmpty ()) {
    string model=
        as_string (call ("chat-tab-session-select-model", string ("")));
    create_new_conversation_with_model (model);
  }
}

/**
 * @brief 销毁控件及其所有会话面板。
 */
QTChatTabWidget::~QTChatTabWidget () {
  for (ChatConversationPanel* panel : conversations_)
    delete panel;
  conversations_.clear ();
  for (ChatConversationPanel* panel : zombiePanels_)
    delete panel;
  zombiePanels_.clear ();
}

/**
 * @brief 构建左侧边栏，包含标题、新建聊天按钮和会话列表。
 * @param sidebarLayout 待填充的布局。
 */
void
QTChatTabWidget::setup_left_sidebar (QVBoxLayout* sidebarLayout) {
  // ---- 展开态：正常内容容器 ----
  QWidget* normalContent= new QWidget (sidebarWidget_);
  normalContent->setObjectName ("chat-tab-sidebar-normal");
  QVBoxLayout* normalLayout= new QVBoxLayout (normalContent);
  normalLayout->setContentsMargins (0, 0, 0, 0);
  normalLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  // 顶部标题栏（Chat + 收缩按钮）
  QWidget*     headerWidget= new QWidget (normalContent);
  QHBoxLayout* headerLayout= new QHBoxLayout (headerWidget);
  headerLayout->setContentsMargins (0, 0, 0, 0);
  headerLayout->setSpacing (0);

  QLabel* navTitle= new QLabel ("Chat", headerWidget);
  navTitle->setObjectName ("chat-tab-nav-title");
  DpiUtils::applyScaledFont (navTitle, kNavTitleFontPx);
  headerLayout->addWidget (navTitle);

  headerLayout->addStretch ();

  QPushButton* collapseBtn= new QPushButton (headerWidget);
  collapseBtn->setObjectName ("chat-tab-collapse-btn");
  collapseBtn->setFocusPolicy (Qt::NoFocus);
  collapseBtn->setCursor (Qt::PointingHandCursor);
  collapseBtn->setIcon (QIcon (":llm-chat/sidebar.svg"));
  collapseBtn->setIconSize (QSize (DpiUtils::scaled (kToggleIconSize),
                                   DpiUtils::scaled (kToggleIconSize)));
  collapseBtn->setFixedSize (DpiUtils::scaled (kToggleBtnSize),
                             DpiUtils::scaled (kToggleBtnSize));
  collapseBtn->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "background-color: #e8e8e8; }"
               "QPushButton:hover { background-color: #d0d0d0; }")
          .arg (DpiUtils::scaled (kToggleBtnSize / 2)));
  connect (collapseBtn, &QPushButton::clicked, this,
           [this] () { toggle_sidebar (); });
  collapseButton_= collapseBtn;
  headerLayout->addWidget (collapseBtn);

  normalLayout->addWidget (headerWidget);

  // New chat 按钮
  newChatButton_= new QPushButton ("New chat", normalContent);
  newChatButton_->setObjectName ("chat-tab-new-btn");
  newChatButton_->setFocusPolicy (Qt::NoFocus);
  newChatButton_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (newChatButton_, kNavButtonFontPx);
  newChatButton_->setStyleSheet (QString ("padding: %1px %2px;")
                                     .arg (DpiUtils::scaled (kNavButtonPadY))
                                     .arg (DpiUtils::scaled (kNavButtonPadX)));
  connect (newChatButton_, &QPushButton::clicked, this, [this] () {
    string model=
        as_string (call ("chat-tab-session-select-model", string ("")));
    create_new_conversation_with_model (model);
  });
  normalLayout->addWidget (newChatButton_);

  conversationCountLabel_= new QLabel ("Conversations (0)", normalContent);
  conversationCountLabel_->setObjectName ("chat-tab-conversation-count");
  DpiUtils::applyScaledFont (conversationCountLabel_, kNavTitleFontPx);
  conversationCountLabel_->setStyleSheet ("color: #666666;");
  normalLayout->addWidget (conversationCountLabel_);

  // 搜索框
  QLineEdit* searchEdit= new QLineEdit (normalContent);
  searchEdit->setObjectName ("chat-tab-search-edit");
  searchEdit->setPlaceholderText (QString::fromUtf8 ("搜索会话..."));
  searchEdit->setClearButtonEnabled (true);
  searchEdit->setFocusPolicy (Qt::ClickFocus);
  DpiUtils::applyScaledFont (searchEdit, kCollapseFontPx);
  searchEdit->setStyleSheet (
      QString ("QLineEdit { border: 1px solid #cccccc; border-radius: %1px; "
               "padding: %2px %3px; background-color: #ffffff; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (searchEdit, &QLineEdit::textChanged, this,
           [this] () { refresh_sidebar (); });
  normalLayout->addWidget (searchEdit);
  searchEdit_= searchEdit;

  // 多选模式批量操作栏（默认隐藏）
  multiSelectBar_= new QWidget (normalContent);
  multiSelectBar_->setObjectName ("chat-tab-multi-select-bar");
  QHBoxLayout* multiSelectLayout= new QHBoxLayout (multiSelectBar_);
  multiSelectLayout->setContentsMargins (0, 0, 0, 0);
  multiSelectLayout->setSpacing (DpiUtils::scaled (4));

  QPushButton* cancelSelectBtn= new QPushButton ("取消", multiSelectBar_);
  cancelSelectBtn->setObjectName ("chat-tab-cancel-select-btn");
  cancelSelectBtn->setFocusPolicy (Qt::NoFocus);
  cancelSelectBtn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (cancelSelectBtn, kCollapseFontPx);
  cancelSelectBtn->setStyleSheet (
      QString ("QPushButton { border: 1px solid #cccccc; border-radius: %1px; "
               "padding: %2px %3px; background-color: #ffffff; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (cancelSelectBtn, &QPushButton::clicked, this,
           [this] () { exit_multi_select_mode (); });
  multiSelectLayout->addWidget (cancelSelectBtn);

  QPushButton* selectAllBtn= new QPushButton ("全选", multiSelectBar_);
  selectAllBtn->setObjectName ("chat-tab-select-all-btn");
  selectAllBtn->setFocusPolicy (Qt::NoFocus);
  selectAllBtn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (selectAllBtn, kCollapseFontPx);
  selectAllBtn->setStyleSheet (
      QString ("QPushButton { border: 1px solid #cccccc; border-radius: %1px; "
               "padding: %2px %3px; background-color: #ffffff; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (selectAllBtn, &QPushButton::clicked, this, [this] () {
    for (ChatConversationPanel* panel : conversations_) {
      if (!panel || !panel->selectCheckBox) continue;
      ChatSession* s       = sessionManager_.getSession (panel->sessionId);
      bool         archived= s && s->archived;
      if (archived == archiveSelectMode_)
        panel->selectCheckBox->setChecked (true);
    }
  });
  multiSelectLayout->addWidget (selectAllBtn);

  multiSelectLayout->addStretch ();

  batchArchiveBtn_= new QPushButton ("归档", multiSelectBar_);
  batchArchiveBtn_->setObjectName ("chat-tab-batch-archive-btn");
  batchArchiveBtn_->setFocusPolicy (Qt::NoFocus);
  batchArchiveBtn_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (batchArchiveBtn_, kCollapseFontPx);
  batchArchiveBtn_->setStyleSheet (
      QString ("QPushButton { border: 1px solid #cccccc; border-radius: %1px; "
               "padding: %2px %3px; background-color: #ffffff; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (batchArchiveBtn_, &QPushButton::clicked, this, [this] () {
    QList<ChatConversationPanel*> checked= get_checked_panels ();
    for (ChatConversationPanel* panel : checked) {
      sessionManager_.archiveSession (panel->sessionId);
      saveOneSession (panel->sessionId);
    }
    exit_multi_select_mode ();
  });
  multiSelectLayout->addWidget (batchArchiveBtn_);

  QPushButton* batchDeleteBtn= new QPushButton ("删除", multiSelectBar_);
  batchDeleteBtn->setObjectName ("chat-tab-batch-delete-btn");
  batchDeleteBtn->setFocusPolicy (Qt::NoFocus);
  batchDeleteBtn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (batchDeleteBtn, kCollapseFontPx);
  batchDeleteBtn->setStyleSheet (
      QString ("QPushButton { border: 1px solid #cccccc; border-radius: %1px; "
               "padding: %2px %3px; background-color: #ffffff; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (batchDeleteBtn, &QPushButton::clicked, this, [this] () {
    QList<ChatConversationPanel*> checked= get_checked_panels ();
    if (!checked.isEmpty ()) delete_sessions (checked);
  });
  multiSelectLayout->addWidget (batchDeleteBtn);

  multiSelectBar_->hide ();
  normalLayout->addWidget (multiSelectBar_);

  // 列表滚动区
  QWidget*     scrollContent= new QWidget (normalContent);
  QVBoxLayout* scrollLayout = new QVBoxLayout (scrollContent);
  scrollLayout->setContentsMargins (0, 0, 0, 0);
  scrollLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  conversationListWidget_= new QWidget (scrollContent);
  conversationListWidget_->setObjectName ("chat-tab-conversation-list");
  conversationListLayout_= new QVBoxLayout (conversationListWidget_);
  conversationListLayout_->setContentsMargins (0, 0, 0, 0);
  conversationListLayout_->setSpacing (DpiUtils::scaled (kSidebarSpacing));
  scrollLayout->addWidget (conversationListWidget_);

  // 归档区标题按钮：点击展开/折叠归档列表
  archiveHeaderButton_= new QPushButton ("Archived (0)", scrollContent);
  archiveHeaderButton_->setObjectName ("chat-tab-archive-header");
  archiveHeaderButton_->setFocusPolicy (Qt::NoFocus);
  archiveHeaderButton_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (archiveHeaderButton_, kNavTitleFontPx);
  archiveHeaderButton_->setStyleSheet (
      QString ("QPushButton { text-align: left; border: none; "
               "padding: %1px %2px; color: #666666; background: transparent; "
               "font-weight: bold; } "
               "QPushButton:hover { color: #333333; }")
          .arg (DpiUtils::scaled (kNavTitlePadding))
          .arg (DpiUtils::scaled (kNavButtonPadX)));
  connect (archiveHeaderButton_, &QPushButton::clicked, this, [this] () {
    archiveCollapsed_= !archiveCollapsed_;
    if (archiveListWidget_) archiveListWidget_->setVisible (!archiveCollapsed_);
  });
  scrollLayout->addWidget (archiveHeaderButton_);

  // 归档会话列表
  archiveListWidget_= new QWidget (scrollContent);
  archiveListWidget_->setObjectName ("chat-tab-archive-list");
  archiveListLayout_= new QVBoxLayout (archiveListWidget_);
  archiveListLayout_->setContentsMargins (0, 0, 0, 0);
  archiveListLayout_->setSpacing (DpiUtils::scaled (kSidebarSpacing));
  archiveListWidget_->hide ();
  scrollLayout->addWidget (archiveListWidget_);

  scrollLayout->addStretch ();

  QScrollArea* scrollArea= new QScrollArea (normalContent);
  scrollArea->setWidgetResizable (true);
  scrollArea->setFrameShape (QFrame::NoFrame);
  scrollArea->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  scrollArea->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  scrollArea->setWidget (scrollContent);
  normalLayout->addWidget (scrollArea, 1);

  sidebarNormalContent_= normalContent;
  sidebarLayout->addWidget (normalContent);

  sidebarNormalContent_= normalContent;
  sidebarLayout->addWidget (normalContent);
}

/**
 * @brief 构建右侧内容区，使用 QStackedWidget 管理会话页面。
 * @param mainLayout 主水平布局，用于插入内容区。
 */
void
QTChatTabWidget::setup_right_content (QHBoxLayout* mainLayout) {
  QWidget* content= new QWidget (this);
  content->setObjectName ("chat-tab-content");
  contentWidget_= content;

  QVBoxLayout* contentLayout= new QVBoxLayout (content);
  contentLayout->setContentsMargins (0, 0, 0, 0);
  contentLayout->setSpacing (0);

  chatMenuToolBar_= new QToolBar ("chat menu toolbar", content);
  chatMenuToolBar_->setObjectName ("chat-menu-tool-bar");
  chatMenuToolBar_->setMovable (false);
  chatMenuToolBar_->setVisible (false);
  contentLayout->addWidget (chatMenuToolBar_);

  chatModeToolBar_= new QToolBar ("chat mode toolbar", content);
  chatModeToolBar_->setObjectName ("chat-mode-tool-bar");
  chatModeToolBar_->setMovable (false);
  chatModeToolBar_->setVisible (false);
  contentLayout->addWidget (chatModeToolBar_);

  chatFocusToolBar_= new QToolBar ("chat focus toolbar", content);
  chatFocusToolBar_->setObjectName ("chat-focus-tool-bar");
  chatFocusToolBar_->setMovable (false);
  chatFocusToolBar_->setVisible (false);
  contentLayout->addWidget (chatFocusToolBar_);

  conversationStack_= new QStackedWidget (content);
  conversationStack_->setObjectName ("chat-tab-conversation-stack");
  contentLayout->addWidget (conversationStack_, 1);

  mainLayout->addWidget (content, 1);

  // 浮球展开按钮（侧边栏收起时显示在内容区左上角）
  QPushButton* floatingBtn= new QPushButton (this);
  floatingBtn->setObjectName ("chat-tab-floating-expand-btn");
  floatingBtn->setFocusPolicy (Qt::NoFocus);
  floatingBtn->setCursor (Qt::PointingHandCursor);
  floatingBtn->setIcon (QIcon (":llm-chat/sidebar.svg"));
  floatingBtn->setIconSize (QSize (DpiUtils::scaled (kToggleIconSize),
                                   DpiUtils::scaled (kToggleIconSize)));
  floatingBtn->setFixedSize (DpiUtils::scaled (kToggleBtnSize),
                             DpiUtils::scaled (kToggleBtnSize));
  floatingBtn->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "background-color: #e8e8e8; } "
               "QPushButton:hover { background-color: #d0d0d0; }")
          .arg (DpiUtils::scaled (kToggleBtnSize / 2)));
  connect (floatingBtn, &QPushButton::clicked, this,
           [this] () { toggle_sidebar (); });
  floatingBtn->move (DpiUtils::scaled (kFloatingBtnMarginX),
                     DpiUtils::scaled (kFloatingBtnMarginY));
  floatingBtn->hide ();
  floatingExpandBtn_= floatingBtn;
}

/**
 * @brief 创建新的会话面板，包含控件和 buffer。
 * @param title 会话的显示标题。
 * @return 新建会话面板的指针，失败时返回 nullptr。
 */
QTChatTabWidget::ChatConversationPanel*
QTChatTabWidget::create_conversation (const QString& title) {
  if (!conversationStack_ || !conversationListLayout_) return nullptr;

  string                 sessionId= sessionManager_.createSession ();
  ChatConversationPanel* panel    = new ChatConversationPanel ();
  panel->sessionId                = sessionId;
  sessionManager_.setPanel (sessionId, panel);

  url msgBufUrl= ChatSessionManager::messageBufferUrl (sessionId);
  url inBufUrl = ChatSessionManager::inputBufferUrl (sessionId);

  QWidget* page= new QWidget (conversationStack_);
  page->setObjectName ("chat-tab-conversation-page");
  panel->pageWidget= page;

  QVBoxLayout* contentLayout= new QVBoxLayout (page);
  contentLayout->setContentsMargins (0, DpiUtils::scaled (kContentMarginY), 0,
                                     DpiUtils::scaled (kContentMarginY));
  contentLayout->setSpacing (DpiUtils::scaled (kContentSpacing));
  panel->topSpacer= new QSpacerItem (0, DpiUtils::scaled (kWelcomeTopOffsetY),
                                     QSizePolicy::Minimum, QSizePolicy::Fixed);
  contentLayout->addSpacerItem (panel->topSpacer);

  QWidget*     topPanel = new QWidget (page);
  QVBoxLayout* topLayout= new QVBoxLayout (topPanel);
  topLayout->setContentsMargins (0, 0, 0, 0);
  topLayout->setSpacing (DpiUtils::scaled (kContentSpacing));

  panel->welcomeTitle= new QLabel ("Welcome to Liii STEM!", topPanel);
  panel->welcomeTitle->setObjectName ("chat-tab-welcome-title");
  panel->welcomeTitle->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (panel->welcomeTitle, kWelcomeFontPx);
  topLayout->addWidget (panel->welcomeTitle);

  // 模型名称标签
  panel->modelLabel= new QLabel ("", topPanel);
  panel->modelLabel->setObjectName ("chat-tab-model-label");
  panel->modelLabel->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (panel->modelLabel, kNavTitleFontPx);
  panel->modelLabel->setStyleSheet (
      "color: #888888; padding: 2px 8px; background-color: #f0f0f0; "
      "border-radius: 4px;");
  panel->modelLabel->setMinimumHeight (DpiUtils::scaled (20));
  topLayout->addWidget (panel->modelLabel, 0, Qt::AlignHCenter);

  panel->messageWidget= texmacs_input_widget (
      tree (DOCUMENT, ""), make_chat_embedded_style (), msgBufUrl);

  QWidget* messageQWidget= concrete (panel->messageWidget)->as_qwidget ();
  panel->messageFrame    = new QWidget (topPanel);
  panel->messageFrame->setObjectName ("chat-tab-message-frame");
  panel->messageFrame->setStyleSheet (
      QString ("border: none; border-radius: %1px; background-color: #ffffff;")
          .arg (DpiUtils::scaled (kInputFrameRadius)));
  QVBoxLayout* messageFrameLayout= new QVBoxLayout (panel->messageFrame);
  messageFrameLayout->setContentsMargins (0, 0, 0, 0);
  messageFrameLayout->setSpacing (0);
  messageQWidget->setParent (panel->messageFrame);
  messageQWidget->setMinimumHeight (DpiUtils::scaled (kMessageMinHeight));
  messageFrameLayout->addWidget (messageQWidget);
  panel->messageFrame->hide ();
  topLayout->addWidget (panel->messageFrame, 1);

  QWidget* inputArea= new QWidget (topPanel);
  inputArea->setObjectName ("chat-tab-input-area");
  QVBoxLayout* inputAreaLayout= new QVBoxLayout (inputArea);
  inputAreaLayout->setContentsMargins (0, 0, 0, 0);
  inputAreaLayout->setSpacing (DpiUtils::scaled (kContentSpacing));

  panel->inputWidget= texmacs_input_widget (
      tree (WITH, "par-par-sep", "0.05fn", tree (DOCUMENT, "")),
      make_chat_embedded_style (), inBufUrl);
  QWidget* inputQWidget   = concrete (panel->inputWidget)->as_qwidget ();
  panel->inputEditorWidget= inputQWidget;
  {
    QList<QAbstractScrollArea*> areas=
        inputQWidget->findChildren<QAbstractScrollArea*> ();
    for (QAbstractScrollArea* area : areas) {
      if (!area) continue;
      area->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
      area->setVerticalScrollBarPolicy (Qt::ScrollBarAsNeeded);
      if (area->viewport ()) {
        area->viewport ()->setStyleSheet ("background-color: #ffffff;");
      }
      area->setStyleSheet (
          "QScrollBar:vertical { background: transparent; width: 6px; "
          "                      margin: 0px; border: none; }"
          "QScrollBar::handle:vertical { background: #c8c8c8; "
          "                            border-radius: 3px; "
          "                            min-height: 20px; }"
          "QScrollBar::handle:vertical:hover { background: #a0a0a0; }"
          "QScrollBar::add-line:vertical, "
          "QScrollBar::sub-line:vertical { height: 0px; }");
    }
  }
  if (QTMWidget* editor= inputQWidget->findChild<QTMWidget*> ()) {
    editor->setProperty ("chat_panel", QVariant::fromValue ((void*) panel));
    editor->installEventFilter (this);
  }
  QWidget* inputFrame= new QWidget (inputArea);
  inputFrame->setObjectName ("chat-tab-input-frame");
  inputFrame->setStyleSheet (
      QString ("QWidget#chat-tab-input-frame { "
               "  border: 1px solid #e0e0e0; border-radius: %1px; "
               "  background-color: #ffffff; }"
               "QWidget#chat-tab-input-frame:hover { "
               "  border: 1px solid #c0c0c0; }")
          .arg (DpiUtils::scaled (kInputFrameRadius)));
  QVBoxLayout* inputFrameLayout= new QVBoxLayout (inputFrame);
  inputFrameLayout->setContentsMargins (
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad),
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad));
  inputFrameLayout->setSpacing (0);
  inputQWidget->setParent (inputFrame);
  int defaultHeight= DpiUtils::scaled (kInputLineHeight * kInputDefaultLines);
  inputQWidget->setMinimumHeight (defaultHeight);
  inputQWidget->setMaximumHeight (defaultHeight);
  inputFrameLayout->addWidget (inputQWidget);

  QHBoxLayout* btnLayout= new QHBoxLayout ();
  btnLayout->addStretch ();

  panel->sendButton= new QPushButton (inputFrame);
  panel->sendButton->setObjectName ("chat-tab-send-btn");
  panel->sendButton->setFocusPolicy (Qt::NoFocus);
  panel->sendButton->setCursor (Qt::PointingHandCursor);
  panel->sendButton->setIcon (QIcon (":llm-chat/send.svg"));
  int sendIconSize= DpiUtils::scaled (24);
  panel->sendButton->setIconSize (QSize (sendIconSize, sendIconSize));
  panel->sendButton->setFixedSize (DpiUtils::scaled (36),
                                   DpiUtils::scaled (36));
  panel->sendButton->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "            background-color: transparent; }"
               "QPushButton:hover { background-color: #f0f0f0; }"
               "QPushButton:pressed { background-color: #e0e0e0; }")
          .arg (DpiUtils::scaled (18)));
  connect (panel->sendButton, &QPushButton::clicked, this,
           [this, panel] () { handle_send (panel); });
  btnLayout->addWidget (panel->sendButton);
  inputFrameLayout->addLayout (btnLayout);

  inputAreaLayout->addWidget (inputFrame, 0);

  QTimer* inputHeightTimer= new QTimer (inputFrame);
  inputHeightTimer->setInterval (100);
  connect (inputHeightTimer, &QTimer::timeout, this,
           [this, panel] () { adjust_input_height (panel); });
  inputHeightTimer->start ();

  QHBoxLayout* inputWrap= new QHBoxLayout ();
  inputWrap->addStretch (1);
  inputWrap->addWidget (inputArea, 8);
  inputWrap->addStretch (1);
  topLayout->addLayout (inputWrap, 0);

  contentLayout->addWidget (topPanel, 1, Qt::AlignTop);
  conversationStack_->addWidget (page);

  panel->itemWidget= new QWidget (conversationListWidget_);
  panel->itemWidget->setObjectName ("chat-tab-session-item");
  QHBoxLayout* itemLayout= new QHBoxLayout (panel->itemWidget);
  itemLayout->setContentsMargins (0, 0, 0, 0);
  itemLayout->setSpacing (DpiUtils::scaled (4));

  panel->selectCheckBox= new QCheckBox (panel->itemWidget);
  panel->selectCheckBox->setObjectName ("chat-tab-select-checkbox");
  panel->selectCheckBox->setFocusPolicy (Qt::NoFocus);
  panel->selectCheckBox->setStyleSheet (
      "QCheckBox::indicator:checked { "
      "  background-color: #4a90d9; border: 2px solid #4a90d9; "
      "  border-radius: 3px; }"
      "QCheckBox::indicator:unchecked { "
      "  background-color: #ffffff; border: 2px solid #cccccc; "
      "  border-radius: 3px; }");
  panel->selectCheckBox->hide ();
  itemLayout->addWidget (panel->selectCheckBox);

  panel->sidebarButton= new QPushButton ("新会话", panel->itemWidget);
  panel->sidebarButton->setObjectName ("chat-tab-conversation-btn");
  panel->sidebarButton->setCheckable (true);
  panel->sidebarButton->setFocusPolicy (Qt::NoFocus);
  panel->sidebarButton->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (panel->sidebarButton, kNavButtonFontPx);
  panel->sidebarButton->setStyleSheet (
      QString ("QPushButton { text-align: left; border: 1px solid #d9d9d9; "
               "border-radius: %1px; padding: %2px %3px; background-color: "
               "#ffffff; } "
               "QPushButton:checked { background-color: #e8eefc; "
               "border-color: #9bb3ff; font-weight: 600; }")
          .arg (DpiUtils::scaled (6))
          .arg (DpiUtils::scaled (kNavButtonPadY))
          .arg (DpiUtils::scaled (kNavButtonPadX)));
  connect (panel->sidebarButton, &QPushButton::clicked, this,
           [this, panel] () { activate_conversation (panel); });
  itemLayout->addWidget (panel->sidebarButton, 1);
  conversationListLayout_->addWidget (panel->itemWidget);

  return panel;
}

/**
 * @brief 使用指定模型创建并激活一个新会话。
 * @param model 模型名称。
 */
void
QTChatTabWidget::create_new_conversation_with_model (const string& model) {
  ChatConversationPanel* panel= create_conversation ("");
  if (!panel) return;
  conversations_.prepend (panel);

  // 绑定模型到会话并显示
  sessionManager_.setModel (panel->sessionId, model);
  if (panel->modelLabel) {
    panel->modelLabel->setText (to_qstring (model));
  }

  activate_conversation (panel);

  // 标记 buffer 为已保存，避免关闭 tab 时弹出保存提示
  call ("buffer-pretend-saved",
        ChatSessionManager::messageBufferUrl (panel->sessionId));
  call ("buffer-pretend-saved",
        ChatSessionManager::inputBufferUrl (panel->sessionId));

  saveOneSession (panel->sessionId);
}

/**
 * @brief 确保最新的会话是空白对话且模型一致，若不是则新建一个。
 *
 * 只检查最新（最上方）的会话，避免旧空白会话影响判断。
 */
void
QTChatTabWidget::ensure_new_conversation () {
  string currentModel=
      as_string (call ("chat-tab-session-select-model", string ("")));

  if (!conversations_.isEmpty ()) {
    ChatConversationPanel* first= conversations_.first ();
    if (!first->conversationMode) {
      // 空白 session：检查 model 是否一致
      if (sessionManager_.getModel (first->sessionId) == currentModel) {
        activate_conversation (first); // 复用，切焦点
        return;
      }
      // model 不一致：需要新建
    }
  }
  create_new_conversation_with_model (currentModel);
}

/**
 * @brief 将可见页面切换到指定会话，并更新侧边栏。
 * @param panel 待激活的会话面板。
 */
void
QTChatTabWidget::activate_conversation (ChatConversationPanel* panel) {
  if (!panel || !conversationStack_) return;
  cout << "[chat-persist] activate_conversation: sid=" << panel->sessionId
       << " panel=" << (void*) panel << " page=" << (void*) panel->pageWidget
       << " stack_count=" << conversationStack_->count () << LF;
  activeConversation_= panel;
  conversationStack_->setCurrentWidget (panel->pageWidget);
  refresh_sidebar ();
  focus_input_editor (panel);
}

/**
 * @brief 更新会话计数标签及侧边栏按钮的选中状态。
 */
void
QTChatTabWidget::refresh_sidebar () {
  cout << "[chat-persist] refresh_sidebar: conversations_.size()="
       << conversations_.size () << LF;

  QString filterText=
      searchEdit_ ? searchEdit_->text ().toLower () : QString ();

  // 标题去重：统计每个 title 出现次数（基于所有会话）
  QMap<QString, int> titleCounts;
  for (ChatConversationPanel* panel : conversations_) {
    if (!panel) continue;
    ChatSession* session= sessionManager_.getSession (panel->sessionId);
    if (!session || is_empty (session->title)) continue;
    QString t= to_qstring (session->title);
    titleCounts[t]++;
  }

  // 多选模式操作栏可见性
  if (multiSelectBar_)
    multiSelectBar_->setVisible (multiSelectMode_ || archiveSelectMode_);
  if (batchArchiveBtn_) batchArchiveBtn_->setVisible (multiSelectMode_);

  // 先将 item 容器从旧父控件移除，以便重新分配
  for (ChatConversationPanel* panel : conversations_) {
    if (!panel || !panel->itemWidget) continue;
    panel->itemWidget->setParent (nullptr);
  }

  // 清空两个列表布局
  while (conversationListLayout_->count () > 0) {
    QLayoutItem* item= conversationListLayout_->takeAt (0);
    delete item;
  }
  while (archiveListLayout_->count () > 0) {
    QLayoutItem* item= archiveListLayout_->takeAt (0);
    delete item;
  }

  // 更新侧边栏按钮
  int                activeCount  = 0;
  int                archivedCount= 0;
  QMap<QString, int> titleSeq; // 去重序号
  for (ChatConversationPanel* panel : conversations_) {
    if (!panel || !panel->sidebarButton) continue;
    ChatSession* session = sessionManager_.getSession (panel->sessionId);
    bool         archived= session && session->archived;

    cout << "[chat-persist] refresh_sidebar panel: sid=" << panel->sessionId
         << " btn=" << (void*) panel->sidebarButton
         << " parent=" << (void*) panel->sidebarButton->parent ()
         << " visible=" << panel->sidebarButton->isVisible ()
         << " enabled=" << panel->sidebarButton->isEnabled ()
         << " archived=" << archived << LF;

    QString displayText;
    if (session && !is_empty (session->title)) {
      displayText= to_qstring (session->title);
      // 标题去重：同标题追加序号
      if (titleCounts[displayText] > 1) {
        int seq= ++titleSeq[displayText];
        displayText+= QString (" (%1)").arg (seq);
      }
    }
    else {
      displayText= QString::fromUtf8 ("新会话");
    }

    panel->sidebarButton->setText (displayText);
    panel->sidebarButton->setChecked (panel == activeConversation_ &&
                                      !archived);

    // 多选模式：仅在对应区域显示 checkbox
    if (panel->selectCheckBox) {
      if (archived) panel->selectCheckBox->setVisible (archiveSelectMode_);
      else panel->selectCheckBox->setVisible (multiSelectMode_);
    }

    // 同步模型标签
    if (panel->modelLabel && session) {
      panel->modelLabel->setText (to_qstring (session->model));
    }

    // 搜索过滤
    bool matchesFilter=
        filterText.isEmpty () || displayText.toLower ().contains (filterText);
    if (!matchesFilter) {
      panel->itemWidget->setVisible (false);
      continue;
    }

    panel->itemWidget->setVisible (true);
    panel->sidebarButton->setVisible (true);

    if (archived) {
      ++archivedCount;
      // 归档会话：放入归档列表，点击切换到空白会话
      panel->itemWidget->setParent (archiveListWidget_);
      archiveListLayout_->addWidget (panel->itemWidget);
      disconnect (panel->sidebarButton, &QPushButton::clicked, this, nullptr);
      connect (panel->sidebarButton, &QPushButton::clicked, this,
               [this, panel] () {
                 // 如果当前激活的是空白的非归档会话，直接切换过去
                 if (activeConversation_ && activeConversation_ != panel &&
                     !activeConversation_->conversationMode) {
                   activate_conversation (activeConversation_);
                 }
                 else {
                   // 否则创建新会话
                   string model= as_string (
                       call ("chat-tab-session-select-model", string ("")));
                   create_new_conversation_with_model (model);
                 }
               });
    }
    else {
      ++activeCount;
      // 活跃会话：放入会话列表，点击切换
      panel->itemWidget->setParent (conversationListWidget_);
      conversationListLayout_->addWidget (panel->itemWidget);
      disconnect (panel->sidebarButton, &QPushButton::clicked, this, nullptr);
      connect (panel->sidebarButton, &QPushButton::clicked, this,
               [this, panel] () { activate_conversation (panel); });
    }

    // 右键菜单
    panel->sidebarButton->setContextMenuPolicy (Qt::CustomContextMenu);
    disconnect (panel->sidebarButton, &QPushButton::customContextMenuRequested,
                this, nullptr);
    connect (
        panel->sidebarButton, &QPushButton::customContextMenuRequested, this,
        [this, panel, archived] (const QPoint& pos) {
          QMenu        menu (panel->sidebarButton);
          ChatSession* s= sessionManager_.getSession (panel->sessionId);

          // 获取所有 checkbox 选中的 panel
          QList<ChatConversationPanel*> checked= get_checked_panels ();

          if (!checked.isEmpty ()) {
            if (!s || !s->archived) {
              menu.addAction (QString ("归档所选 (%1)").arg (checked.size ()));
            }
            menu.addAction (QString ("删除所选 (%1)").arg (checked.size ()));
            QAction* chosen=
                menu.exec (panel->sidebarButton->mapToGlobal (pos));
            if (!chosen) return;
            QString txt= chosen->text ();
            if (txt.startsWith ("归档所选")) {
              for (ChatConversationPanel* p : checked) {
                sessionManager_.archiveSession (p->sessionId);
                saveOneSession (p->sessionId);
              }
              exit_multi_select_mode ();
            }
            else if (txt.startsWith ("删除所选")) {
              delete_sessions (checked);
            }
          }
          else {
            // 单选模式：原有菜单 + 删除 + 多选
            QAction* renameAction= menu.addAction ("重命名");
            QAction* archiveAction=
                menu.addAction (s && s->archived ? "恢复" : "归档");
            QAction* deleteAction= menu.addAction ("删除");
            menu.addSeparator ();
            QAction* multiSelectAction= menu.addAction ("多选");
            QAction* chosen=
                menu.exec (panel->sidebarButton->mapToGlobal (pos));
            if (chosen == renameAction) {
              bool    ok;
              QString newTitle=
                  QInputDialog::getText (panel->sidebarButton, "重命名会话",
                                         "新标题:", QLineEdit::Normal,
                                         s ? to_qstring (s->title) : "", &ok);
              if (ok && s) {
                sessionManager_.setTitle (panel->sessionId,
                                          from_qstring (newTitle));
                refresh_sidebar ();
              }
            }
            else if (chosen == archiveAction) {
              if (s && s->archived)
                sessionManager_.restoreSession (panel->sessionId);
              else sessionManager_.archiveSession (panel->sessionId);
              saveOneSession (panel->sessionId);
              refresh_sidebar ();
            }
            else if (chosen == deleteAction) {
              QList<ChatConversationPanel*> single= {panel};
              delete_sessions (single);
            }
            else if (chosen == multiSelectAction) {
              enter_multi_select_mode (archived);
            }
          }
        });
  }

  if (conversationCountLabel_) {
    conversationCountLabel_->setText (
        QString ("Conversations (%1)").arg (activeCount));
  }

  // 更新归档区标题
  if (archiveHeaderButton_) {
    archiveHeaderButton_->setText (
        QString ("Archived (%1)").arg (archivedCount));
    archiveHeaderButton_->setVisible (true);
  }

  // 归档列表可见性：有归档项且非折叠时显示
  if (archiveListWidget_) {
    archiveListWidget_->setVisible (archivedCount > 0 && !archiveCollapsed_);
  }
}

/**
 * @brief 删除指定的会话面板列表。
 *
 * 从数据结构和 UI 中移除会话，但只隐藏控件，不释放内存。
 * @param panels 待删除的会话面板列表。
 */
void
QTChatTabWidget::delete_sessions (const QList<ChatConversationPanel*>& panels) {

  for (ChatConversationPanel* panel : panels) {
    // 1. 从 QStackedWidget 移除页面
    if (conversationStack_ && panel->pageWidget)
      conversationStack_->removeWidget (panel->pageWidget);

    // 2. 从 conversations_ 列表移除
    conversations_.removeOne (panel);

    // 3. 清理当前激活指针
    if (activeConversation_ == panel) activeConversation_= nullptr;

    // 4. 删除持久化数据（磁盘文件 + manifest 条目）
    call ("chat-persist-delete-one", panel->sessionId);

    // 5. 调用 Scheme 清理会话状态
    call ("chat-tab-session-destroy", panel->sessionId);

    // 6. 从 SessionManager 移除（含 buffer 清理）
    sessionManager_.removeSession (panel->sessionId);

    // 7. 断开信号连接
    if (panel->sidebarButton)
      disconnect (panel->sidebarButton, nullptr, this, nullptr);
    if (panel->sendButton)
      disconnect (panel->sendButton, nullptr, this, nullptr);
    if (panel->selectCheckBox)
      disconnect (panel->selectCheckBox, nullptr, this, nullptr);

    // 8. 隐藏并脱离父控件
    if (panel->itemWidget) {
      panel->itemWidget->hide ();
      panel->itemWidget->setParent (nullptr);
    }
    if (panel->pageWidget) {
      panel->pageWidget->hide ();
      panel->pageWidget->setParent (nullptr);
    }

    // 9. 移入僵尸列表，等待析构时统一释放
    zombiePanels_.append (panel);
  }

  // 退出多选模式
  multiSelectMode_  = false;
  archiveSelectMode_= false;

  // 切换到剩余会话，若无剩余则新建
  if (!conversations_.isEmpty ()) {
    ChatConversationPanel* next= nullptr;
    for (ChatConversationPanel* p : conversations_) {
      ChatSession* s= sessionManager_.getSession (p->sessionId);
      if (s && !s->archived) {
        next= p;
        break;
      }
    }
    if (!next) next= conversations_.last ();
    activeConversation_= next;
    if (conversationStack_)
      conversationStack_->setCurrentWidget (next->pageWidget);
    refresh_sidebar ();
  }
  else {
    string model=
        as_string (call ("chat-tab-session-select-model", string ("")));
    create_new_conversation_with_model (model);
  }

  // 确保所有聊天相关 buffer 标记为已保存，避免关闭 Tab 时弹出确认对话框
  for (ChatConversationPanel* panel : conversations_) {
    call ("buffer-pretend-saved",
          ChatSessionManager::messageBufferUrl (panel->sessionId));
    call ("buffer-pretend-saved",
          ChatSessionManager::inputBufferUrl (panel->sessionId));
  }
  // 主 chat-tab buffer
  call ("buffer-pretend-saved", url ("tmfs://chat-tab"));
}

/**
 * @brief 获取所有 checkbox 被勾选的会话面板。
 * @return 被勾选的面板列表。
 */
QList<QTChatTabWidget::ChatConversationPanel*>
QTChatTabWidget::get_checked_panels () const {
  QList<ChatConversationPanel*> result;
  for (ChatConversationPanel* panel : conversations_) {
    if (panel && panel->selectCheckBox && panel->selectCheckBox->isChecked ())
      result.append (panel);
  }
  return result;
}

/**
 * @brief 进入多选模式，显示 checkbox 和批量操作栏。
 * @param archived 是否从归档区进入。
 */
void
QTChatTabWidget::enter_multi_select_mode (bool archived) {
  if (archived) archiveSelectMode_= true;
  else multiSelectMode_= true;
  refresh_sidebar ();
}

/**
 * @brief 退出多选模式，隐藏 checkbox 和批量操作栏。
 */
void
QTChatTabWidget::exit_multi_select_mode () {
  multiSelectMode_  = false;
  archiveSelectMode_= false;
  // 取消所有 checkbox 选中
  for (ChatConversationPanel* panel : conversations_) {
    if (panel && panel->selectCheckBox)
      panel->selectCheckBox->setChecked (false);
  }
  refresh_sidebar ();
}

/**
 * @brief 切换侧边栏的收起/展开状态。
 *
 * 收起时 sidebar 缩为窄条（仅含展开按钮），展开时恢复完整内容。
 */
void
QTChatTabWidget::toggle_sidebar () {
  if (!sidebarWidget_) return;

  if (sidebarCollapsed_) {
    if (floatingExpandBtn_) floatingExpandBtn_->hide ();
    sidebarWidget_->show ();
    sidebarCollapsed_= false;
  }
  else {
    sidebarWidget_->hide ();
    if (floatingExpandBtn_) {
      floatingExpandBtn_->move (DpiUtils::scaled (kFloatingBtnMarginX),
                                DpiUtils::scaled (kFloatingBtnMarginY));
      floatingExpandBtn_->show ();
    }
    sidebarCollapsed_= true;
  }
}

/**
 * @brief 将指定面板从欢迎态切换到会话态。
 *
 * 播放时长为 \ref kTransitionDurationMs 毫秒的淡入淡出及间距动画。
 * @param panel 目标会话面板。
 */
void
QTChatTabWidget::enter_conversation_mode (ChatConversationPanel* panel) {
  if (!panel || panel->conversationMode) return;

  panel->conversationMode= true;
  const int startOffset  = DpiUtils::scaled (kWelcomeTopOffsetY);
  const int endOffset    = DpiUtils::scaled (kConversationTopOffsetY);

  if (panel->messageFrame) {
    QGraphicsOpacityEffect* messageEffect=
        new QGraphicsOpacityEffect (panel->messageFrame);
    messageEffect->setOpacity (0.0);
    panel->messageFrame->setGraphicsEffect (messageEffect);
    panel->messageFrame->show ();

    QPropertyAnimation* fadeIn=
        new QPropertyAnimation (messageEffect, "opacity", panel->messageFrame);
    fadeIn->setDuration (kTransitionDurationMs);
    fadeIn->setStartValue (0.0);
    fadeIn->setEndValue (1.0);
    fadeIn->start (QAbstractAnimation::DeleteWhenStopped);
  }

  if (panel->welcomeTitle) {
    QGraphicsOpacityEffect* titleEffect=
        new QGraphicsOpacityEffect (panel->welcomeTitle);
    titleEffect->setOpacity (1.0);
    panel->welcomeTitle->setGraphicsEffect (titleEffect);

    QPropertyAnimation* fadeOut=
        new QPropertyAnimation (titleEffect, "opacity", panel->welcomeTitle);
    fadeOut->setDuration (kTransitionDurationMs);
    fadeOut->setStartValue (1.0);
    fadeOut->setEndValue (0.0);
    connect (fadeOut, &QPropertyAnimation::finished, this, [panel] () {
      if (panel->welcomeTitle) panel->welcomeTitle->hide ();
    });
    fadeOut->start (QAbstractAnimation::DeleteWhenStopped);
  }

  if (panel->topSpacer && panel->pageWidget && panel->pageWidget->layout ()) {
    QVariantAnimation* offsetAnim= new QVariantAnimation (panel->pageWidget);
    offsetAnim->setDuration (kTransitionDurationMs);
    offsetAnim->setStartValue (startOffset);
    offsetAnim->setEndValue (endOffset);
    connect (offsetAnim, &QVariantAnimation::valueChanged, this,
             [panel] (const QVariant& value) {
               if (!panel->topSpacer || !panel->pageWidget ||
                   !panel->pageWidget->layout ())
                 return;
               panel->topSpacer->changeSize (
                   0, value.toInt (), QSizePolicy::Minimum, QSizePolicy::Fixed);
               panel->pageWidget->layout ()->invalidate ();
               panel->pageWidget->layout ()->activate ();
             });
    offsetAnim->start (QAbstractAnimation::DeleteWhenStopped);
  }
}

/**
 * @brief 读取面板输入内容，委托发送给 Scheme 层，成功后进入会话态。
 * @param panel 发送消息的会话面板。
 */
void
QTChatTabWidget::handle_send (ChatConversationPanel* panel) {
  if (!panel) return;

  tree inputBody= read_input_message (panel);
  if (is_empty_document_body (inputBody)) return;

  // 首次发送：自动提取标题
  ChatSession* session= sessionManager_.getSession (panel->sessionId);
  if (session && is_empty (session->title)) {
    string extracted= extract_title (inputBody, 20);
    sessionManager_.setTitle (panel->sessionId, extracted);
  }

  if (!as_bool (call ("chat-tab-send", panel->sessionId))) return;

  sessionManager_.setState (panel->sessionId, ChatState::Generating);
  enter_conversation_mode (panel);
  refresh_sidebar ();
  focus_input_editor (panel);
  saveOneSession (panel->sessionId);
}

/**
 * @brief 从面板输入 buffer 中获取文档树。
 * @param panel 待读取输入的会话面板。
 * @return 输入内容对应的 TeXmacs 树。
 */
tree
QTChatTabWidget::read_input_message (const ChatConversationPanel* panel) const {
  if (!panel) return tree (DOCUMENT, "");
  return get_buffer_body (
      ChatSessionManager::inputBufferUrl (panel->sessionId));
}

/**
 * @brief 取消当前会话的 LLM 生成。
 * @param panel 待取消的会话面板。
 */
void
QTChatTabWidget::handle_cancel (ChatConversationPanel* panel) {
  if (!panel) return;
  call ("chat-tab-cancel", panel->sessionId);
  sessionManager_.setState (panel->sessionId, ChatState::Idle);
}

/**
 * @brief 被 Scheme 侧通知生成状态变更。
 * @param sessionId 会话 ID。
 * @param stateStr 状态字符串 ("idle" 或 "generating")。
 */
void
QTChatTabWidget::notifyStateChanged (const string& sessionId,
                                     const string& stateStr) {
  ChatSession* session= sessionManager_.getSession (sessionId);
  if (!session) return;

  ChatState newState=
      (stateStr == "generating") ? ChatState::Generating : ChatState::Idle;
  sessionManager_.setState (sessionId, newState);

  // 更新按钮状态
  ChatConversationPanel* panel=
      static_cast<ChatConversationPanel*> (session->panel);
  if (!panel || !panel->sendButton) return;

  if (newState == ChatState::Generating) {
    panel->sendButton->setToolTip ("Stop");
    disconnect (panel->sendButton, &QPushButton::clicked, this, nullptr);
    connect (panel->sendButton, &QPushButton::clicked, this,
             [this, panel] () { handle_cancel (panel); });
  }
  else {
    panel->sendButton->setToolTip ("Send");
    disconnect (panel->sendButton, &QPushButton::clicked, this, nullptr);
    connect (panel->sendButton, &QPushButton::clicked, this,
             [this, panel] () { handle_send (panel); });
    // 模型输出结束，保存会话内容
    saveOneSession (sessionId);
  }
}

/**
 * @brief 将键盘焦点设置到指定面板的输入编辑器。
 * @param panel 目标会话面板。
 */
void
QTChatTabWidget::focus_input_editor (ChatConversationPanel* panel) {
  if (panel && panel->inputEditorWidget) {
    panel->inputEditorWidget->setFocus (Qt::OtherFocusReason);
  }
}

/**
 * @brief 计算输入文档的段落（行）数。
 * @param body TeXmacs 文档树。
 * @return 段落数量。
 */
int
QTChatTabWidget::count_input_lines (tree body) {
  if (!is_func (body, DOCUMENT)) return 1;
  if (N (body) == 0) return 1;
  if (N (body) == 1 && is_atomic (body[0]) && body[0]->label == "") return 1;
  return N (body);
}

/**
 * @brief 根据排版后的实际高度估算等效行数。
 * @param contentHeight 排版后的内容高度（SI 单位）。
 * @return 等效行数，若高度无效则返回 0。
 */
int
QTChatTabWidget::estimate_lines_from_height (SI contentHeight) {
  if (contentHeight <= 0) return 0;
  int px= to_qsize (0, contentHeight).height ();
  if (px <= 0) return 0;
  return (px + kInputLineHeight - 1) / kInputLineHeight;
}

/**
 * @brief 根据输入内容自适应调整输入框高度。
 * @param panel 目标会话面板。
 */
void
QTChatTabWidget::adjust_input_height (ChatConversationPanel* panel) {
  if (!panel || !panel->inputEditorWidget) return;

  tree body       = read_input_message (panel);
  int  docLines   = count_input_lines (body);
  int  visualLines= 0;

  if (QTMWidget* editor= panel->inputEditorWidget->findChild<QTMWidget*> ()) {
    if (edit_interface_rep* ed=
            dynamic_cast<edit_interface_rep*> (editor->tm_widget ())) {
      visualLines= estimate_lines_from_height (ed->get_total_height (true));
    }
  }

  int lines       = qMax (docLines, visualLines);
  int targetLines = qMax (kInputDefaultLines, lines);
  targetLines     = qMin (targetLines, kInputMaxLines);
  int targetHeight= DpiUtils::scaled (kInputLineHeight * targetLines);

  if (panel->inputEditorWidget->minimumHeight () != targetHeight ||
      panel->inputEditorWidget->maximumHeight () != targetHeight) {
    panel->inputEditorWidget->setMinimumHeight (targetHeight);
    panel->inputEditorWidget->setMaximumHeight (targetHeight);
  }
}

/**
 * @brief 通过 \c eval_scheme 将按键按下事件转发到 Scheme 层。
 * @param event Qt 按键事件。
 */
void
QTChatTabWidget::keyPressEvent (QKeyEvent* event) {
  string key= from_key_press_event (event);
  if (is_empty (key)) return QWidget::keyPressEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}

/**
 * @brief 通过 \c eval_scheme 将按键释放事件转发到 Scheme 层。
 * @param event Qt 按键事件。
 */
void
QTChatTabWidget::keyReleaseEvent (QKeyEvent* event) {
  string key= from_key_release_event (event);
  if (is_empty (key)) return QWidget::keyReleaseEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}

bool
QTChatTabWidget::eventFilter (QObject* watched, QEvent* event) {
  if (event->type () == QEvent::KeyPress) {
    QKeyEvent* keyEvent= static_cast<QKeyEvent*> (event);
    if ((keyEvent->key () == Qt::Key_Return ||
         keyEvent->key () == Qt::Key_Enter) &&
        (keyEvent->modifiers () & Qt::ShiftModifier)) {
      void* ptr= watched->property ("chat_panel").value<void*> ();
      if (ptr) {
        ChatConversationPanel* panel= static_cast<ChatConversationPanel*> (ptr);
        handle_send (panel);
        return true;
      }
    }
  }
  return QWidget::eventFilter (watched, event);
}

void
QTChatTabWidget::install_chat_menu_bar (widget menuWidget) {
  if (!chatMenuToolBar_) return;
  QList<QAction*>* src= concrete (menuWidget)->get_qactionlist ();
  if (!src) return;

  QMenuBar* dest = new QMenuBar ();
  double    scale= DpiUtils::scaleFactor ();
  int       h    = DpiUtils::scaled (108);
  dest->setFixedHeight (h);
  if (tm_style_sheet == "") dest->setStyle (qtmstyle ());
  dest->setNativeMenuBar (false);

  dest->clear ();
  for (int i= 0; i < src->count (); i++) {
    QAction* a= (*src)[i];
    if (a->menu ()) {
      a->menu ()->addAction ("native menubar trick");
      dest->addAction (a->menu ()->menuAction ());
      QObject::connect (a->menu (), SIGNAL (aboutToShow ()),
                        the_gui->gui_helper, SLOT (aboutToShowMainMenu ()));
      QObject::connect (a->menu (), SIGNAL (aboutToHide ()),
                        the_gui->gui_helper, SLOT (aboutToHideMainMenu ()));
    }
  }

  QList<QWidget*> widgets= chatMenuToolBar_->findChildren<QWidget*> ();
  for (QWidget* w : widgets) {
    w->setParent (nullptr);
  }
  chatMenuToolBar_->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Fixed);
  if (chatMenuToolBar_->layout ()) {
    chatMenuToolBar_->layout ()->setContentsMargins (2, 0, 2, 0);
    chatMenuToolBar_->layout ()->setSpacing (4);
  }
  chatMenuToolBar_->addWidget (dest);
  chatMenuToolBar_->setVisible (true);
}

void
QTChatTabWidget::set_chat_mode_icons (widget modeWidget) {
  if (!chatModeToolBar_) return;
  QList<QAction*>* src= concrete (modeWidget)->get_qactionlist ();
  if (!src) return;

  chatModeToolBar_->setUpdatesEnabled (false);
  bool visible= chatModeToolBar_->isVisible ();
  if (visible) chatModeToolBar_->hide ();

  QList<QAction*> actions= chatModeToolBar_->actions ();
  for (int i= 0; i < actions.count (); i++) {
    chatModeToolBar_->removeAction (actions[i]);
  }
  for (int i= 0; i < src->count (); i++) {
    chatModeToolBar_->addAction ((*src)[i]);
  }

  QList<QObject*> list= chatModeToolBar_->children ();
  for (int i= 0; i < list.count (); ++i) {
    QToolButton* button= qobject_cast<QToolButton*> (list[i]);
    if (button) {
      button->setPopupMode (QToolButton::InstantPopup);
      if (tm_style_sheet == "") button->setStyle (qtmstyle ());
    }
  }

  if (visible) chatModeToolBar_->show ();
  chatModeToolBar_->setUpdatesEnabled (true);
  chatModeToolBar_->setVisible (true);
}

void
QTChatTabWidget::set_chat_focus_icons (widget focusWidget) {
  if (!chatFocusToolBar_) return;
  QList<QAction*>* src= concrete (focusWidget)->get_qactionlist ();
  if (!src) return;

  chatFocusToolBar_->setUpdatesEnabled (false);
  bool visible= chatFocusToolBar_->isVisible ();
  if (visible) chatFocusToolBar_->hide ();

  QList<QAction*> actions= chatFocusToolBar_->actions ();
  for (int i= 0; i < actions.count (); i++) {
    chatFocusToolBar_->removeAction (actions[i]);
  }
  for (int i= 0; i < src->count (); i++) {
    chatFocusToolBar_->addAction ((*src)[i]);
  }

  QList<QObject*> list= chatFocusToolBar_->children ();
  for (int i= 0; i < list.count (); ++i) {
    QToolButton* button= qobject_cast<QToolButton*> (list[i]);
    if (button) {
      button->setPopupMode (QToolButton::InstantPopup);
      if (tm_style_sheet == "") button->setStyle (qtmstyle ());
    }
  }

  if (visible) chatFocusToolBar_->show ();
  chatFocusToolBar_->setUpdatesEnabled (true);
  chatFocusToolBar_->setVisible (true);
}

/******************************************************************************
 * ChatSessionManager 实现
 ******************************************************************************/

string
ChatSessionManager::createSession () {
  string      sessionId= lolly::hash::uuid_make ();
  ChatSession session;
  session.sessionId= sessionId;
  session.state    = ChatState::Idle;
  session.archived = false;
  session.panel    = nullptr;
  sessions_.insert (std::make_pair (sessionId, session));
  return sessionId;
}

void
ChatSessionManager::removeSession (const string& sessionId) {
  sessions_.erase (sessionId);
}

void
ChatSessionManager::archiveSession (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (s) s->archived= true;
}

void
ChatSessionManager::restoreSession (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (s) s->archived= false;
}

void
ChatSessionManager::setTitle (const string& sessionId, const string& title) {
  ChatSession* s= getSession (sessionId);
  if (s) s->title= title;
}

void
ChatSessionManager::setState (const string& sessionId, ChatState state) {
  ChatSession* s= getSession (sessionId);
  if (s) s->state= state;
}

void
ChatSessionManager::setModel (const string& sessionId, const string& model) {
  ChatSession* s= getSession (sessionId);
  if (s) s->model= model;
}

string
ChatSessionManager::getModel (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (s) return s->model;
  return "";
}

ChatSession*
ChatSessionManager::getSession (const string& sessionId) {
  auto it= sessions_.find (sessionId);
  if (it != sessions_.end ()) return &(it->second);
  return nullptr;
}

std::vector<string>
ChatSessionManager::getAllSessionIds () const {
  std::vector<string> ids;
  for (const auto& kv : sessions_)
    ids.push_back (kv.first);
  return ids;
}

ChatSession*
ChatSessionManager::findSessionByPanel (void* panel) {
  for (auto& kv : sessions_) {
    if (kv.second.panel == panel) return &kv.second;
  }
  return nullptr;
}

void
ChatSessionManager::setPanel (const string& sessionId, void* panel) {
  ChatSession* s= getSession (sessionId);
  if (s) s->panel= panel;
}

url
ChatSessionManager::messageBufferUrl (const string& sessionId) {
  return url ("tmfs://chat-message-" * sessionId);
}

url
ChatSessionManager::inputBufferUrl (const string& sessionId) {
  return url ("tmfs://chat-input-" * sessionId);
}

void
ChatSessionManager::insertSession (const ChatSession& session) {
  sessions_.insert (std::make_pair (session.sessionId, session));
}

/******************************************************************************
 * QTChatTabWidget 持久化实现
 ******************************************************************************/

void
QTChatTabWidget::saveOneSession (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s) return;
  call ("chat-persist-save-one", sessionId, s->title, s->model,
        s->archived ? string ("true") : string ("false"));
}

void
QTChatTabWidget::loadSessions () {
  cout << "[chat-persist] loadSessions started" << LF;
  // Scheme 端 chat-persist-load-all 会逐个调用 qt-chat-tab-restore-session
  call ("chat-persist-load-all");
  cout << "[chat-persist] loadSessions: restored " << conversations_.size ()
       << " sessions" << LF;

  // 激活第一个非归档会话，否则激活第一个
  for (ChatConversationPanel* panel : conversations_) {
    if (!panel) continue;
    ChatSession* s= sessionManager_.getSession (panel->sessionId);
    cout << "[chat-persist] loadSessions scanning: sid=" << panel->sessionId
         << " archived=" << (s ? s->archived : -1) << " panel=" << (void*) panel
         << " page=" << (void*) panel->pageWidget << LF;
    if (s && !s->archived) {
      cout << "[chat-persist] loadSessions: activating first non-archived"
           << LF;
      activate_conversation (panel);
      return;
    }
  }
  if (!conversations_.isEmpty ()) {
    cout << "[chat-persist] loadSessions: activating first (all archived)"
         << LF;
    activate_conversation (conversations_.first ());
  }

  // 恢复 Scheme 层的全局当前模型
  if (!conversations_.isEmpty ()) {
    ChatConversationPanel* last= conversations_.last ();
    ChatSession*           s   = sessionManager_.getSession (last->sessionId);
    if (s && !is_empty (s->model)) {
      call ("chat-tab-session-select-model", s->model);
    }
  }
}

void
QTChatTabWidget::addConversation (ChatConversationPanel* panel) {
  if (panel) conversations_.append (panel);
}

QTChatTabWidget::ChatConversationPanel*
QTChatTabWidget::restore_conversation (const string& sessionId,
                                       const string& title, const string& model,
                                       bool archived) {
  if (!conversationStack_ || !conversationListLayout_) return nullptr;

  // 注册 Scheme 侧会话状态
  call ("chat-persist-register-session", sessionId, model);

  // 手动插入会话元数据
  ChatSession session;
  session.sessionId= sessionId;
  session.title    = title;
  session.model    = model;
  session.state    = ChatState::Idle;
  session.archived = archived;
  session.panel    = nullptr;
  sessionManager_.insertSession (session);

  ChatConversationPanel* panel= new ChatConversationPanel ();
  panel->sessionId            = sessionId;
  sessionManager_.setPanel (sessionId, panel);

  url msgBufUrl= ChatSessionManager::messageBufferUrl (sessionId);
  url inBufUrl = ChatSessionManager::inputBufferUrl (sessionId);

  QWidget* page= new QWidget (conversationStack_);
  page->setObjectName ("chat-tab-conversation-page");
  panel->pageWidget= page;

  QVBoxLayout* contentLayout= new QVBoxLayout (page);
  contentLayout->setContentsMargins (0, DpiUtils::scaled (kContentMarginY), 0,
                                     DpiUtils::scaled (kContentMarginY));
  contentLayout->setSpacing (DpiUtils::scaled (kContentSpacing));
  panel->topSpacer= new QSpacerItem (0, DpiUtils::scaled (kWelcomeTopOffsetY),
                                     QSizePolicy::Minimum, QSizePolicy::Fixed);
  contentLayout->addSpacerItem (panel->topSpacer);

  QWidget*     topPanel = new QWidget (page);
  QVBoxLayout* topLayout= new QVBoxLayout (topPanel);
  topLayout->setContentsMargins (0, 0, 0, 0);
  topLayout->setSpacing (DpiUtils::scaled (kContentSpacing));

  panel->welcomeTitle= new QLabel ("Welcome to Liii STEM!", topPanel);
  panel->welcomeTitle->setObjectName ("chat-tab-welcome-title");
  panel->welcomeTitle->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (panel->welcomeTitle, kWelcomeFontPx);
  topLayout->addWidget (panel->welcomeTitle);

  panel->modelLabel= new QLabel ("", topPanel);
  panel->modelLabel->setObjectName ("chat-tab-model-label");
  panel->modelLabel->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (panel->modelLabel, kNavTitleFontPx);
  panel->modelLabel->setStyleSheet (
      "color: #888888; padding: 2px 8px; background-color: #f0f0f0; "
      "border-radius: 4px;");
  panel->modelLabel->setMinimumHeight (DpiUtils::scaled (20));
  topLayout->addWidget (panel->modelLabel, 0, Qt::AlignHCenter);

  // 恢复会话时，buffer 中已有 Scheme 加载的消息内容，需使用 buffer 内容而非空
  // tree
  tree msgBody= get_buffer_body (msgBufUrl);
  if (is_empty_document_body (msgBody)) msgBody= tree (DOCUMENT, "");
  panel->messageWidget=
      texmacs_input_widget (msgBody, make_chat_embedded_style (), msgBufUrl);

  QWidget* messageQWidget= concrete (panel->messageWidget)->as_qwidget ();
  panel->messageFrame    = new QWidget (topPanel);
  panel->messageFrame->setObjectName ("chat-tab-message-frame");
  panel->messageFrame->setStyleSheet (
      QString ("border: none; border-radius: %1px; background-color: #ffffff;")
          .arg (DpiUtils::scaled (kInputFrameRadius)));
  QVBoxLayout* messageFrameLayout= new QVBoxLayout (panel->messageFrame);
  messageFrameLayout->setContentsMargins (0, 0, 0, 0);
  messageFrameLayout->setSpacing (0);
  messageQWidget->setParent (panel->messageFrame);
  messageQWidget->setMinimumHeight (DpiUtils::scaled (kMessageMinHeight));
  messageFrameLayout->addWidget (messageQWidget);
  panel->messageFrame->hide ();
  topLayout->addWidget (panel->messageFrame, 1);

  QWidget* inputArea= new QWidget (topPanel);
  inputArea->setObjectName ("chat-tab-input-area");
  QVBoxLayout* inputAreaLayout= new QVBoxLayout (inputArea);
  inputAreaLayout->setContentsMargins (0, 0, 0, 0);
  inputAreaLayout->setSpacing (DpiUtils::scaled (kContentSpacing));

  panel->inputWidget= texmacs_input_widget (
      tree (WITH, "par-par-sep", "0.05fn", tree (DOCUMENT, "")),
      make_chat_embedded_style (), inBufUrl);
  QWidget* inputQWidget   = concrete (panel->inputWidget)->as_qwidget ();
  panel->inputEditorWidget= inputQWidget;
  {
    QList<QAbstractScrollArea*> areas=
        inputQWidget->findChildren<QAbstractScrollArea*> ();
    for (QAbstractScrollArea* area : areas) {
      if (!area) continue;
      area->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
      area->setVerticalScrollBarPolicy (Qt::ScrollBarAsNeeded);
      if (area->viewport ()) {
        area->viewport ()->setStyleSheet ("background-color: #ffffff;");
      }
      area->setStyleSheet (
          "QScrollBar:vertical { background: transparent; width: 6px; "
          "                      margin: 0px; border: none; }"
          "QScrollBar::handle:vertical { background: #c8c8c8; "
          "                            border-radius: 3px; "
          "                            min-height: 20px; }"
          "QScrollBar::handle:vertical:hover { background: #a0a0a0; }"
          "QScrollBar::add-line:vertical, "
          "QScrollBar::sub-line:vertical { height: 0px; }");
    }
  }
  if (QTMWidget* editor= inputQWidget->findChild<QTMWidget*> ()) {
    editor->setProperty ("chat_panel", QVariant::fromValue ((void*) panel));
    editor->installEventFilter (this);
  }
  QWidget* inputFrame= new QWidget (inputArea);
  inputFrame->setObjectName ("chat-tab-input-frame");
  inputFrame->setStyleSheet (
      QString ("QWidget#chat-tab-input-frame { "
               "  border: 1px solid #e0e0e0; border-radius: %1px; "
               "  background-color: #ffffff; }"
               "QWidget#chat-tab-input-frame:hover { "
               "  border: 1px solid #c0c0c0; }")
          .arg (DpiUtils::scaled (kInputFrameRadius)));
  QVBoxLayout* inputFrameLayout= new QVBoxLayout (inputFrame);
  inputFrameLayout->setContentsMargins (
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad),
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad));
  inputFrameLayout->setSpacing (0);
  inputQWidget->setParent (inputFrame);
  int defaultHeight= DpiUtils::scaled (kInputLineHeight * kInputDefaultLines);
  inputQWidget->setMinimumHeight (defaultHeight);
  inputQWidget->setMaximumHeight (defaultHeight);
  inputFrameLayout->addWidget (inputQWidget);

  QHBoxLayout* btnLayout= new QHBoxLayout ();
  btnLayout->addStretch ();

  panel->sendButton= new QPushButton (inputFrame);
  panel->sendButton->setObjectName ("chat-tab-send-btn");
  panel->sendButton->setFocusPolicy (Qt::NoFocus);
  panel->sendButton->setCursor (Qt::PointingHandCursor);
  panel->sendButton->setIcon (QIcon (":llm-chat/send.svg"));
  int sendIconSize= DpiUtils::scaled (24);
  panel->sendButton->setIconSize (QSize (sendIconSize, sendIconSize));
  panel->sendButton->setFixedSize (DpiUtils::scaled (36),
                                   DpiUtils::scaled (36));
  panel->sendButton->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "            background-color: transparent; }"
               "QPushButton:hover { background-color: #f0f0f0; }"
               "QPushButton:pressed { background-color: #e0e0e0; }")
          .arg (DpiUtils::scaled (18)));
  connect (panel->sendButton, &QPushButton::clicked, this,
           [this, panel] () { handle_send (panel); });
  btnLayout->addWidget (panel->sendButton);
  inputFrameLayout->addLayout (btnLayout);

  inputAreaLayout->addWidget (inputFrame, 0);

  QTimer* inputHeightTimer= new QTimer (inputFrame);
  inputHeightTimer->setInterval (100);
  connect (inputHeightTimer, &QTimer::timeout, this,
           [this, panel] () { adjust_input_height (panel); });
  inputHeightTimer->start ();

  QHBoxLayout* inputWrap= new QHBoxLayout ();
  inputWrap->addStretch (1);
  inputWrap->addWidget (inputArea, 8);
  inputWrap->addStretch (1);
  topLayout->addLayout (inputWrap, 0);

  contentLayout->addWidget (topPanel, 1, Qt::AlignTop);
  conversationStack_->addWidget (page);

  panel->itemWidget= new QWidget (conversationListWidget_);
  panel->itemWidget->setObjectName ("chat-tab-session-item");
  QHBoxLayout* itemLayout= new QHBoxLayout (panel->itemWidget);
  itemLayout->setContentsMargins (0, 0, 0, 0);
  itemLayout->setSpacing (DpiUtils::scaled (4));

  panel->selectCheckBox= new QCheckBox (panel->itemWidget);
  panel->selectCheckBox->setObjectName ("chat-tab-select-checkbox");
  panel->selectCheckBox->setFocusPolicy (Qt::NoFocus);
  panel->selectCheckBox->setStyleSheet (
      "QCheckBox::indicator:checked { "
      "  background-color: #4a90d9; border: 2px solid #4a90d9; "
      "  border-radius: 3px; }"
      "QCheckBox::indicator:unchecked { "
      "  background-color: #ffffff; border: 2px solid #cccccc; "
      "  border-radius: 3px; }");
  panel->selectCheckBox->hide ();
  itemLayout->addWidget (panel->selectCheckBox);

  panel->sidebarButton= new QPushButton ("新会话", panel->itemWidget);
  panel->sidebarButton->setObjectName ("chat-tab-conversation-btn");
  panel->sidebarButton->setCheckable (true);
  panel->sidebarButton->setFocusPolicy (Qt::NoFocus);
  panel->sidebarButton->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (panel->sidebarButton, kNavButtonFontPx);
  panel->sidebarButton->setStyleSheet (
      QString ("QPushButton { text-align: left; border: 1px solid #d9d9d9; "
               "border-radius: %1px; padding: %2px %3px; background-color: "
               "#ffffff; } "
               "QPushButton:checked { background-color: #e8eefc; "
               "border-color: #9bb3ff; font-weight: 600; }")
          .arg (DpiUtils::scaled (6))
          .arg (DpiUtils::scaled (kNavButtonPadY))
          .arg (DpiUtils::scaled (kNavButtonPadX)));
  connect (panel->sidebarButton, &QPushButton::clicked, this,
           [this, panel] () { activate_conversation (panel); });
  itemLayout->addWidget (panel->sidebarButton, 1);
  conversationListLayout_->addWidget (panel->itemWidget);

  // 如果消息 buffer 非空，进入会话模式
  if (!is_empty_document_body (msgBody)) {
    enter_conversation_mode (panel);
  }

  if (panel->modelLabel) {
    panel->modelLabel->setText (to_qstring (model));
  }

  cout << "[chat-persist] restore_conversation: sid=" << sessionId
       << " panel=" << (void*) panel << " btn=" << (void*) panel->sidebarButton
       << " page=" << (void*) panel->pageWidget
       << " stack_count=" << conversationStack_->count () << LF;

  return panel;
}

/******************************************************************************
 * qt_chat_tab_set_state 自由函数（Scheme→C++ 回调）
 ******************************************************************************/

void
qt_chat_tab_set_state (string sessionId, string stateStr) {
  // 从顶层窗口查找 QTChatTabWidget
  QWidgetList topWidgets= QApplication::topLevelWidgets ();
  for (QWidget* top : topWidgets) {
    QTChatTabWidget* chat= top->findChild<QTChatTabWidget*> ();
    if (chat) {
      chat->notifyStateChanged (sessionId, stateStr);
      return;
    }
  }
}

/**
 * @brief Scheme→C++ 回调：恢复单个聊天会话。
 */
void
qt_chat_tab_restore_session (string sessionId, string title, string model,
                             string archived) {
  QWidgetList topWidgets= QApplication::topLevelWidgets ();
  for (QWidget* top : topWidgets) {
    QTChatTabWidget* chat= top->findChild<QTChatTabWidget*> ();
    if (chat) {
      bool  isArchived= (archived == "true");
      auto* panel=
          chat->restore_conversation (sessionId, title, model, isArchived);
      if (panel) chat->addConversation (panel);
      return;
    }
  }
}

/**
 * @brief Scheme→C++ 回调：加载所有聊天会话。
 */
void
qt_chat_tab_load_sessions () {
  cout << "[chat-persist] qt_chat_tab_load_sessions called" << LF;
  QWidgetList topWidgets= QApplication::topLevelWidgets ();
  cout << "[chat-persist] top-level widgets count: " << topWidgets.size ()
       << LF;
  for (QWidget* top : topWidgets) {
    QTChatTabWidget* chat= top->findChild<QTChatTabWidget*> ();
    if (chat) {
      cout << "[chat-persist] found QTChatTabWidget, calling loadSessions"
           << LF;
      chat->loadSessions ();
      return;
    }
  }
  cout << "[chat-persist] WARNING: QTChatTabWidget not found in any top-level "
          "widget"
       << LF;
}
