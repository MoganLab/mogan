
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
#include "new_buffer.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_gui.hpp"
#include "qt_utilities.hpp"
#include "qt_widget.hpp"
#include "s7_tm.hpp"
#include "tm_window.hpp"

#include <moebius/tree_label.hpp>

#include <QAbstractScrollArea>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariantAnimation>

using namespace moebius;

namespace {

/**
 * @brief 生成下一个聊天 buffer 的唯一 ID。
 * @return 递增后的 ID。
 */
int
next_chat_input_buffer_id () {
  static int s_nextId= 0;
  return ++s_nextId;
}

/**
 * @brief 创建唯一的输入 buffer URL。
 * @return 格式为 tmfs://chat-input-<id> 的 URL。
 */
url
make_chat_input_buffer_name () {
  return url ("tmfs://chat-input-" * as_string (next_chat_input_buffer_id ()));
}

/**
 * @brief 创建唯一的消息 buffer URL。
 * @return 格式为 tmfs://chat-message-<id> 的 URL。
 */
url
make_chat_message_buffer_name () {
  return url ("tmfs://chat-message-" *
              as_string (next_chat_input_buffer_id ()));
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

/**
 * @brief 判断文档主体是否实际为空。
 * @param body TeXmacs 文档树。
 * @return 若主体不含可见内容则返回 true。
 */
bool
is_empty_document_body (tree body) {
  if (!is_func (body, DOCUMENT)) return false;
  if (N (body) == 0) return true;
  return N (body) == 1 && is_atomic (body[0]) && body[0]->label == "";
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
constexpr int kNavTitleFontPx= 11;
/// 导航标题内边距。
constexpr int kNavTitlePadding= 4;
/// 导航按钮垂直内边距。
constexpr int kNavButtonPadY= 8;
/// 导航按钮水平内边距。
constexpr int kNavButtonPadX= 12;
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
/// 输入编辑器固定高度（像素）。
constexpr int kInputHeight= 44;
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

} // namespace

/**
 * @brief 单个会话面板的内部数据。
 */
struct QTChatTabWidget::ChatConversationPanel {
  QWidget*     pageWidget       = nullptr; ///< 本会话的堆叠页面。
  QLabel*      welcomeTitle     = nullptr; ///< 欢迎标题标签。
  QWidget*     messageFrame     = nullptr; ///< 承载消息控件的边框。
  QWidget*     inputEditorWidget= nullptr; ///< 输入编辑器的 Qt 控件。
  QPushButton* sendButton       = nullptr; ///< 发送按钮。
  QPushButton* sidebarButton    = nullptr; ///< 侧边栏入口按钮。
  QSpacerItem* topSpacer        = nullptr; ///< 顶部占位，用于垂直偏移。
  widget       messageWidget;              ///< 消息显示的 TeXmacs 控件。
  widget       inputWidget;                ///< 用户输入的 TeXmacs 控件。
  url          messageBufferName;          ///< 消息历史 buffer 的 URL。
  url          inputBufferName;            ///< 输入编辑器 buffer 的 URL。
  bool         conversationMode= false;    ///< 面板是否已离开欢迎态。
  QString      title;                      ///< 会话的显示标题。
};

/**
 * @brief 构造聊天标签页控件。
 *
 * 创建左侧边栏和右侧内容区，然后创建第一个会话。
 */
QTChatTabWidget::QTChatTabWidget (QWidget* parent)
    : QWidget (parent), sidebarWidget_ (nullptr), contentWidget_ (nullptr),
      conversationCountLabel_ (nullptr), conversationListWidget_ (nullptr),
      conversationListLayout_ (nullptr), newChatButton_ (nullptr),
      conversationStack_ (nullptr), activeConversation_ (nullptr),
      nextConversationTitleId_ (1), chatMenuToolBar_ (nullptr),
      chatModeToolBar_ (nullptr), chatFocusToolBar_ (nullptr) {
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
  sidebar->setFixedWidth (
      qMax (DpiUtils::scaled (kSidebarMinWidth), contentWidth));
  mainLayout->addWidget (sidebar);

  // 右侧内容区
  setup_right_content (mainLayout);
  create_new_conversation ();
}

/**
 * @brief 销毁控件及其所有会话面板。
 */
QTChatTabWidget::~QTChatTabWidget () {
  for (ChatConversationPanel* panel : conversations_)
    delete panel;
  conversations_.clear ();
}

/**
 * @brief 构建左侧边栏，包含标题、新建聊天按钮和会话列表。
 * @param sidebarLayout 待填充的布局。
 */
void
QTChatTabWidget::setup_left_sidebar (QVBoxLayout* sidebarLayout) {
  // 顶部行：标题 + 收缩按钮
  QHBoxLayout* headerLayout= new QHBoxLayout ();
  headerLayout->setSpacing (0);

  QLabel* navTitle= new QLabel ("Chat", this);
  navTitle->setObjectName ("chat-tab-nav-title");
  DpiUtils::applyScaledFont (navTitle, kNavTitleFontPx);
  headerLayout->addWidget (navTitle);

  headerLayout->addStretch ();

  QPushButton* collapseBtn= new QPushButton ("收缩", this);
  collapseBtn->setObjectName ("chat-tab-collapse-btn");
  collapseBtn->setFocusPolicy (Qt::NoFocus);
  collapseBtn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (collapseBtn, kCollapseFontPx);
  collapseBtn->setStyleSheet (
      QString ("QPushButton { border: 1px solid #cccccc; border-radius: %1px; "
               "padding: %2px %3px; background-color: #ffffff; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  headerLayout->addWidget (collapseBtn);

  sidebarLayout->addLayout (headerLayout);

  // New chat 按钮
  newChatButton_= new QPushButton ("New chat", this);
  newChatButton_->setObjectName ("chat-tab-new-btn");
  newChatButton_->setFocusPolicy (Qt::NoFocus);
  newChatButton_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (newChatButton_, kNavButtonFontPx);
  newChatButton_->setStyleSheet (QString ("padding: %1px %2px;")
                                     .arg (DpiUtils::scaled (kNavButtonPadY))
                                     .arg (DpiUtils::scaled (kNavButtonPadX)));
  connect (newChatButton_, &QPushButton::clicked, this,
           [this] () { create_new_conversation (); });
  sidebarLayout->addWidget (newChatButton_);

  conversationCountLabel_= new QLabel ("Conversations (0)", this);
  conversationCountLabel_->setObjectName ("chat-tab-conversation-count");
  DpiUtils::applyScaledFont (conversationCountLabel_, kNavTitleFontPx);
  conversationCountLabel_->setStyleSheet ("color: #666666;");
  sidebarLayout->addWidget (conversationCountLabel_);

  conversationListWidget_= new QWidget (this);
  conversationListWidget_->setObjectName ("chat-tab-conversation-list");
  conversationListLayout_= new QVBoxLayout (conversationListWidget_);
  conversationListLayout_->setContentsMargins (0, 0, 0, 0);
  conversationListLayout_->setSpacing (DpiUtils::scaled (kSidebarSpacing));
  sidebarLayout->addWidget (conversationListWidget_);

  sidebarLayout->addStretch ();
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
}

/**
 * @brief 创建新的会话面板，包含控件和 buffer。
 * @param title 会话的显示标题。
 * @return 新建会话面板的指针，失败时返回 nullptr。
 */
QTChatTabWidget::ChatConversationPanel*
QTChatTabWidget::create_conversation (const QString& title) {
  if (!conversationStack_ || !conversationListLayout_) return nullptr;

  ChatConversationPanel* panel= new ChatConversationPanel ();
  panel->title                = title;
  panel->messageBufferName    = make_chat_message_buffer_name ();
  panel->inputBufferName      = make_chat_input_buffer_name ();

  QWidget* page= new QWidget (conversationStack_);
  page->setObjectName ("chat-tab-conversation-page");
  panel->pageWidget= page;

  QVBoxLayout* contentLayout= new QVBoxLayout (page);
  contentLayout->setContentsMargins (
      DpiUtils::scaled (kContentMarginX), DpiUtils::scaled (kContentMarginY),
      DpiUtils::scaled (kContentMarginX), DpiUtils::scaled (kContentMarginY));
  contentLayout->setSpacing (DpiUtils::scaled (kContentSpacing));
  panel->topSpacer= new QSpacerItem (0, DpiUtils::scaled (kWelcomeTopOffsetY),
                                     QSizePolicy::Minimum, QSizePolicy::Fixed);
  contentLayout->addSpacerItem (panel->topSpacer);

  QWidget* topPanel= new QWidget (page);
  topPanel->setMaximumWidth (DpiUtils::scaled (kTopPanelMaxWidth));
  QVBoxLayout* topLayout= new QVBoxLayout (topPanel);
  topLayout->setContentsMargins (0, 0, 0, 0);
  topLayout->setSpacing (DpiUtils::scaled (kContentSpacing));

  panel->welcomeTitle= new QLabel ("Welcome to Liii STEM!", topPanel);
  panel->welcomeTitle->setObjectName ("chat-tab-welcome-title");
  panel->welcomeTitle->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (panel->welcomeTitle, kWelcomeFontPx);
  topLayout->addWidget (panel->welcomeTitle);

  panel->messageWidget=
      texmacs_input_widget (tree (DOCUMENT, ""), make_chat_embedded_style (),
                            panel->messageBufferName);
  QWidget* messageQWidget= concrete (panel->messageWidget)->as_qwidget ();
  panel->messageFrame    = new QWidget (topPanel);
  panel->messageFrame->setObjectName ("chat-tab-message-frame");
  panel->messageFrame->setStyleSheet (
      QString ("border: %1px solid #d9d9d9; border-radius: %2px; "
               "background-color: #ffffff;")
          .arg (DpiUtils::scaled (kInputFrameBorder))
          .arg (DpiUtils::scaled (kInputFrameRadius)));
  QVBoxLayout* messageFrameLayout= new QVBoxLayout (panel->messageFrame);
  messageFrameLayout->setContentsMargins (
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad),
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad));
  messageFrameLayout->setSpacing (0);
  messageQWidget->setParent (panel->messageFrame);
  messageQWidget->setMinimumHeight (DpiUtils::scaled (kMessageMinHeight));
  messageFrameLayout->addWidget (messageQWidget);
  panel->messageFrame->hide ();
  topLayout->addWidget (panel->messageFrame, 1);

  panel->inputWidget= texmacs_input_widget (
      tree (DOCUMENT, ""), make_chat_embedded_style (), panel->inputBufferName);
  QWidget* inputQWidget   = concrete (panel->inputWidget)->as_qwidget ();
  panel->inputEditorWidget= inputQWidget;
  disable_scrollbars_recursively (inputQWidget);
  if (QTMWidget* editor= inputQWidget->findChild<QTMWidget*> ()) {
    editor->setProperty ("chat_panel", QVariant::fromValue ((void*) panel));
    editor->installEventFilter (this);
  }
  QWidget* inputFrame= new QWidget (topPanel);
  inputFrame->setObjectName ("chat-tab-input-frame");
  inputFrame->setStyleSheet (
      QString ("border: %1px solid #d9d9d9; border-radius: %2px; "
               "background-color: #ffffff;")
          .arg (DpiUtils::scaled (kInputFrameBorder))
          .arg (DpiUtils::scaled (kInputFrameRadius)));
  QVBoxLayout* inputFrameLayout= new QVBoxLayout (inputFrame);
  inputFrameLayout->setContentsMargins (
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad),
      DpiUtils::scaled (kInputFramePad), DpiUtils::scaled (kInputFramePad));
  inputFrameLayout->setSpacing (0);
  inputQWidget->setParent (inputFrame);
  inputQWidget->setFixedHeight (DpiUtils::scaled (kInputHeight));
  inputFrameLayout->addWidget (inputQWidget);
  topLayout->addWidget (inputFrame, 0);

  QHBoxLayout* btnLayout= new QHBoxLayout ();
  btnLayout->addStretch ();

  panel->sendButton= new QPushButton ("Send", topPanel);
  panel->sendButton->setObjectName ("chat-tab-send-btn");
  panel->sendButton->setFocusPolicy (Qt::NoFocus);
  panel->sendButton->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (panel->sendButton, kSendButtonFontPx);
  panel->sendButton->setStyleSheet (
      QString ("padding: %1px %2px;")
          .arg (DpiUtils::scaled (kSendButtonPadY))
          .arg (DpiUtils::scaled (kSendButtonPadX)));
  connect (panel->sendButton, &QPushButton::clicked, this,
           [this, panel] () { handle_send (panel); });
  btnLayout->addWidget (panel->sendButton);
  topLayout->addLayout (btnLayout);

  contentLayout->addWidget (topPanel, 1, Qt::AlignHCenter | Qt::AlignTop);
  conversationStack_->addWidget (page);

  panel->sidebarButton= new QPushButton (title, conversationListWidget_);
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
  conversationListLayout_->addWidget (panel->sidebarButton);

  return panel;
}

/**
 * @brief 创建并激活一个以自动生成标题命名的新会话。
 */
void
QTChatTabWidget::create_new_conversation () {
  QString title= QString ("Chat %1").arg (nextConversationTitleId_++);
  ChatConversationPanel* panel= create_conversation (title);
  if (!panel) return;
  conversations_.append (panel);
  activate_conversation (panel);
}

/**
 * @brief 将可见页面切换到指定会话，并更新侧边栏。
 * @param panel 待激活的会话面板。
 */
void
QTChatTabWidget::activate_conversation (ChatConversationPanel* panel) {
  if (!panel || !conversationStack_) return;
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
  if (conversationCountLabel_) {
    conversationCountLabel_->setText (
        QString ("Conversations (%1)").arg (conversations_.size ()));
  }
  for (ChatConversationPanel* panel : conversations_) {
    if (!panel || !panel->sidebarButton) continue;
    panel->sidebarButton->setText (panel->title);
    panel->sidebarButton->setChecked (panel == activeConversation_);
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

  if (!as_bool (call ("chat-tab-send", as_string (panel->messageBufferName),
                      as_string (panel->inputBufferName), object (inputBody))))
    return;

  enter_conversation_mode (panel);
  focus_input_editor (panel);
}

/**
 * @brief 从面板输入 buffer 中获取文档树。
 * @param panel 待读取输入的会话面板。
 * @return 输入内容对应的 TeXmacs 树。
 */
tree
QTChatTabWidget::read_input_message (const ChatConversationPanel* panel) const {
  if (!panel) return tree (DOCUMENT, "");
  return get_buffer_body (panel->inputBufferName);
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

  QMenuBar* dest= new QMenuBar ();
  double    scale= DpiUtils::scaleFactor ();
  int       h= DpiUtils::scaled (108);
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
