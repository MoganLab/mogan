
/******************************************************************************
 * MODULE     : qt_chat_tab_widget.cpp
 * DESCRIPTION: LLM Chat tab widget for Mogan STEM
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_chat_tab_widget.hpp"
#include "new_buffer.hpp"
#include "qt_dpi_utils.hpp"
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
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVariantAnimation>

using namespace moebius;

namespace {

/**
 * @brief Generates the next unique id for chat buffers.
 * @return Incremented id.
 */
int
next_chat_input_buffer_id () {
  static int s_nextId= 0;
  return ++s_nextId;
}

/**
 * @brief Creates a unique input buffer URL.
 * @return URL in the form tmfs://chat-input-<id>.
 */
url
make_chat_input_buffer_name () {
  return url ("tmfs://chat-input-" * as_string (next_chat_input_buffer_id ()));
}

/**
 * @brief Creates a unique message buffer URL.
 * @return URL in the form tmfs://chat-message-<id>.
 */
url
make_chat_message_buffer_name () {
  return url ("tmfs://chat-message-" *
              as_string (next_chat_input_buffer_id ()));
}

/**
 * @brief Returns the embedded style tree for chat widgets.
 * @return A compound style tree with "generic" and "llm" tags.
 */
tree
make_chat_embedded_style () {
  return compound ("style", tuple ("generic", "llm"));
}

/**
 * @brief Disables scrollbars for all QAbstractScrollArea descendants.
 * @param root Root widget to search from.
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
 * @brief Checks whether a document body is effectively empty.
 * @param body TeXmacs document tree.
 * @return True if the body contains no visible content.
 */
bool
is_empty_document_body (tree body) {
  if (!is_func (body, DOCUMENT)) return false;
  if (N (body) == 0) return true;
  return N (body) == 1 && is_atomic (body[0]) && body[0]->label == "";
}

/// Minimum width of the left sidebar in pixels.
constexpr int kSidebarMinWidth       = 200;
/// Horizontal margin of the sidebar layout.
constexpr int kSidebarMarginX        = 12;
/// Vertical margin of the sidebar layout.
constexpr int kSidebarMarginY        = 16;
/// Spacing between sidebar elements.
constexpr int kSidebarSpacing        = 8;
/// Font size of the navigation title in pixels.
constexpr int kNavTitleFontPx        = 11;
/// Padding around the navigation title.
constexpr int kNavTitlePadding       = 4;
/// Vertical padding of navigation buttons.
constexpr int kNavButtonPadY         = 8;
/// Horizontal padding of navigation buttons.
constexpr int kNavButtonPadX         = 12;
/// Font size of navigation button text in pixels.
constexpr int kNavButtonFontPx       = 13;
/// Font size of the collapse button in pixels.
constexpr int kCollapseFontPx        = 11;
/// Border radius of the collapse button.
constexpr int kCollapseBorderRadius  = 4;
/// Vertical padding of the collapse button.
constexpr int kCollapsePadY          = 4;
/// Horizontal padding of the collapse button.
constexpr int kCollapsePadX          = 8;
/// Font size of the welcome title in pixels.
constexpr int kWelcomeFontPx         = 34;
/// Fixed height of the input editor in pixels.
constexpr int kInputHeight           = 44;
/// Vertical padding of the send button.
constexpr int kSendButtonPadY        = 6;
/// Horizontal padding of the send button.
constexpr int kSendButtonPadX        = 16;
/// Font size of the send button text in pixels.
constexpr int kSendButtonFontPx      = 13;
/// Horizontal margin of the content area.
constexpr int kContentMarginX        = 24;
/// Vertical margin of the content area.
constexpr int kContentMarginY        = 24;
/// Spacing between content area elements.
constexpr int kContentSpacing        = 16;
/// Top spacer height in welcome mode in pixels.
constexpr int kWelcomeTopOffsetY     = 240;
/// Top spacer height in conversation mode in pixels.
constexpr int kConversationTopOffsetY= 24;
/// Maximum width of the top panel in pixels.
constexpr int kTopPanelMaxWidth      = 680;
/// Border radius of input/message frames.
constexpr int kInputFrameRadius      = 8;
/// Border width of input/message frames in pixels.
constexpr int kInputFrameBorder      = 1;
/// Padding inside input/message frames.
constexpr int kInputFramePad         = 8;
/// Minimum height of the message display area in pixels.
constexpr int kMessageMinHeight      = 240;
/// Duration of welcome-to-conversation transition in milliseconds.
constexpr int kTransitionDurationMs  = 220;

} // namespace

/**
 * @brief Internal data for a single conversation panel.
 */
struct QTChatTabWidget::ChatConversationPanel {
  QWidget*     pageWidget       = nullptr; ///\< Stacked page for this conversation.
  QLabel*      welcomeTitle     = nullptr; ///\< Welcome title label.
  QWidget*     messageFrame     = nullptr; ///\< Frame holding the message widget.
  QWidget*     inputEditorWidget= nullptr; ///\< Qt widget of the input editor.
  QPushButton* sendButton       = nullptr; ///\< Send button.
  QPushButton* sidebarButton    = nullptr; ///\< Sidebar entry button.
  QSpacerItem* topSpacer        = nullptr; ///\< Top spacer for vertical offset.
  widget       messageWidget;              ///\< TeXmacs widget for message display.
  widget       inputWidget;                ///\< TeXmacs widget for user input.
  url          messageBufferName;          ///\< Buffer URL for message history.
  url          inputBufferName;            ///\< Buffer URL for the input editor.
  bool         conversationMode= false;    ///\< Whether the panel has left welcome mode.
  QString      title;                      ///\< Display title of the conversation.
};

/**
 * @brief Constructs the chat tab widget.
 *
 * Creates a left sidebar and a right content area, then creates the first
 * conversation.
 */
QTChatTabWidget::QTChatTabWidget (QWidget* parent)
    : QWidget (parent), sidebarWidget_ (nullptr), contentWidget_ (nullptr),
      conversationCountLabel_ (nullptr), conversationListWidget_ (nullptr),
      conversationListLayout_ (nullptr), newChatButton_ (nullptr),
      conversationStack_ (nullptr), activeConversation_ (nullptr),
      nextConversationTitleId_ (1) {
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
 * @brief Destroys the widget and all conversation panels.
 */
QTChatTabWidget::~QTChatTabWidget () {
  for (ChatConversationPanel* panel : conversations_)
    delete panel;
  conversations_.clear ();
}

/**
 * @brief Builds the left sidebar with title, new-chat button, and conversation list.
 * @param sidebarLayout Layout to populate.
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
 * @brief Builds the right content area using a QStackedWidget for conversation pages.
 * @param mainLayout Main horizontal layout to insert into.
 */
void
QTChatTabWidget::setup_right_content (QHBoxLayout* mainLayout) {
  QWidget* content= new QWidget (this);
  content->setObjectName ("chat-tab-content");
  contentWidget_= content;

  QVBoxLayout* contentLayout= new QVBoxLayout (content);
  contentLayout->setContentsMargins (0, 0, 0, 0);
  contentLayout->setSpacing (0);
  conversationStack_= new QStackedWidget (content);
  conversationStack_->setObjectName ("chat-tab-conversation-stack");
  contentLayout->addWidget (conversationStack_, 1);

  mainLayout->addWidget (content, 1);
}

/**
 * @brief Creates a new conversation panel with widgets and buffers.
 * @param title Display title for the conversation.
 * @return Pointer to the newly created panel, or nullptr on failure.
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
 * @brief Creates and activates a new conversation with an auto-generated title.
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
 * @brief Switches the visible page to the given conversation and updates the sidebar.
 * @param panel Conversation panel to activate.
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
 * @brief Updates the conversation count label and sidebar button checked states.
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
 * @brief Transitions the given panel from welcome state to conversation state.
 *
 * Plays fade and spacer animations over \ref kTransitionDurationMs milliseconds.
 * @param panel Target conversation panel.
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
 * @brief Reads input from the panel, delegates sending to Scheme, and enters
 *        conversation mode on success.
 * @param panel Conversation panel to send from.
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
 * @brief Retrieves the document tree from the panel's input buffer.
 * @param panel Conversation panel whose input is read.
 * @return The input body as a TeXmacs tree.
 */
tree
QTChatTabWidget::read_input_message (const ChatConversationPanel* panel) const {
  if (!panel) return tree (DOCUMENT, "");
  return get_buffer_body (panel->inputBufferName);
}

/**
 * @brief Sets keyboard focus to the input editor of the given panel.
 * @param panel Target conversation panel.
 */
void
QTChatTabWidget::focus_input_editor (ChatConversationPanel* panel) {
  if (panel && panel->inputEditorWidget) {
    panel->inputEditorWidget->setFocus (Qt::OtherFocusReason);
  }
}

/**
 * @brief Forwards key press events to the Scheme layer via \c eval_scheme.
 * @param event The Qt key event.
 */
void
QTChatTabWidget::keyPressEvent (QKeyEvent* event) {
  string key= from_key_press_event (event);
  if (is_empty (key)) return QWidget::keyPressEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}

/**
 * @brief Forwards key release events to the Scheme layer via \c eval_scheme.
 * @param event The Qt key event.
 */
void
QTChatTabWidget::keyReleaseEvent (QKeyEvent* event) {
  string key= from_key_release_event (event);
  if (is_empty (key)) return QWidget::keyReleaseEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}
