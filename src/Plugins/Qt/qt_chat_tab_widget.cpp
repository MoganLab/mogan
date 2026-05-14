
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
int
next_chat_input_buffer_id () {
  static int s_nextId= 0;
  return ++s_nextId;
}

url
make_chat_input_buffer_name () {
  return url ("tmfs://chat-input-" * as_string (next_chat_input_buffer_id ()));
}

url
make_chat_message_buffer_name () {
  return url ("tmfs://chat-message-" *
              as_string (next_chat_input_buffer_id ()));
}

tree
make_chat_embedded_style () {
  return compound ("style", tuple ("generic", "llm"));
}

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

bool
is_empty_document_body (tree body) {
  if (!is_func (body, DOCUMENT)) return false;
  if (N (body) == 0) return true;
  return N (body) == 1 && is_atomic (body[0]) && body[0]->label == "";
}

constexpr int kSidebarMinWidth       = 200;
constexpr int kSidebarMarginX        = 12;
constexpr int kSidebarMarginY        = 16;
constexpr int kSidebarSpacing        = 8;
constexpr int kNavTitleFontPx        = 11;
constexpr int kNavTitlePadding       = 4;
constexpr int kNavButtonPadY         = 8;
constexpr int kNavButtonPadX         = 12;
constexpr int kNavButtonFontPx       = 13;
constexpr int kCollapseFontPx        = 11;
constexpr int kCollapseBorderRadius  = 4;
constexpr int kCollapsePadY          = 4;
constexpr int kCollapsePadX          = 8;
constexpr int kWelcomeFontPx         = 34;
constexpr int kInputHeight           = 44;
constexpr int kSendButtonPadY        = 6;
constexpr int kSendButtonPadX        = 16;
constexpr int kSendButtonFontPx      = 13;
constexpr int kContentMarginX        = 24;
constexpr int kContentMarginY        = 24;
constexpr int kContentSpacing        = 16;
constexpr int kWelcomeTopOffsetY     = 240;
constexpr int kConversationTopOffsetY= 24;
constexpr int kTopPanelMaxWidth      = 680;
constexpr int kInputFrameRadius      = 8;
constexpr int kInputFrameBorder      = 1;
constexpr int kInputFramePad         = 8;
constexpr int kMessageMinHeight      = 240;
constexpr int kTransitionDurationMs  = 220;
} // namespace

struct QTChatTabWidget::ChatConversationPanel {
  QWidget*     pageWidget       = nullptr;
  QLabel*      welcomeTitle     = nullptr;
  QWidget*     messageFrame     = nullptr;
  QWidget*     inputEditorWidget= nullptr;
  QPushButton* sendButton       = nullptr;
  QPushButton* sidebarButton    = nullptr;
  QSpacerItem* topSpacer        = nullptr;
  widget       messageWidget;
  widget       inputWidget;
  url          messageBufferName;
  url          inputBufferName;
  bool         conversationMode= false;
  QString      title;
};

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

QTChatTabWidget::~QTChatTabWidget () {
  for (ChatConversationPanel* panel : conversations_)
    delete panel;
  conversations_.clear ();
}

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

void
QTChatTabWidget::create_new_conversation () {
  QString title= QString ("Chat %1").arg (nextConversationTitleId_++);
  ChatConversationPanel* panel= create_conversation (title);
  if (!panel) return;
  conversations_.append (panel);
  activate_conversation (panel);
}

void
QTChatTabWidget::activate_conversation (ChatConversationPanel* panel) {
  if (!panel || !conversationStack_) return;
  activeConversation_= panel;
  conversationStack_->setCurrentWidget (panel->pageWidget);
  refresh_sidebar ();
  focus_input_editor (panel);
}

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

tree
QTChatTabWidget::read_input_message (const ChatConversationPanel* panel) const {
  if (!panel) return tree (DOCUMENT, "");
  return get_buffer_body (panel->inputBufferName);
}

void
QTChatTabWidget::focus_input_editor (ChatConversationPanel* panel) {
  if (panel && panel->inputEditorWidget) {
    panel->inputEditorWidget->setFocus (Qt::OtherFocusReason);
  }
}

void
QTChatTabWidget::keyPressEvent (QKeyEvent* event) {
  string key= from_key_press_event (event);
  if (is_empty (key)) return QWidget::keyPressEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}

void
QTChatTabWidget::keyReleaseEvent (QKeyEvent* event) {
  string key= from_key_release_event (event);
  if (is_empty (key)) return QWidget::keyReleaseEvent (event);

  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}
