
/******************************************************************************
 * MODULE     : qt_chat_tab_widget.cpp
 * DESCRIPTION: Mogan STEM 的 LLM 聊天标签页控件（纯 View）
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
#include "new_view.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_gui.hpp"
#include "qt_utilities.hpp"
#include "qt_widget.hpp"
#include "s7_tm.hpp"
#include "tm_window.hpp"

#include <moebius/tree_label.hpp>

#include <QAbstractScrollArea>
#include <QCheckBox>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariantAnimation>

using namespace moebius;

namespace {

// ---- Widget 框架相关常量 ----
constexpr int kSidebarMinWidth          = 200;
constexpr int kSidebarMarginX           = 12;
constexpr int kSidebarMarginY           = 16;
constexpr int kSidebarSpacing           = 8;
constexpr int kNavChatFontPx            = 22;
constexpr int kNavButtonFontPx          = 13;
constexpr int kToggleBtnSize            = 40;
constexpr int kToggleIconSize           = 20;
constexpr int kFloatingBtnMarginX       = 12;
constexpr int kFloatingBtnMarginY       = 12;
constexpr int kFloatingContainerPad     = 4;
constexpr int kFloatingBtnSpacing       = 4;
constexpr int kNewChatIconSize          = 18;
constexpr int kNewChatButtonHeight      = 36;
constexpr int kNewChatButtonWidth       = 140;
constexpr int kNewChatShadowBlur        = 3;
constexpr int kNewChatShadowAlpha       = 25;
constexpr int kNewChatShadowOffsetY     = 1;
constexpr int kNewChatHoverShadowBlur   = 6;
constexpr int kNewChatHoverShadowAlpha  = 50;
constexpr int kNewChatHoverShadowOffsetY= 2;
constexpr int kNavButtonPadY            = 8;
constexpr int kNavButtonPadX            = 8;
constexpr int kSessionItemSpacing       = 4;

// ---- 侧边栏常量 ----
constexpr int kNavTitleFontPx      = 11;
constexpr int kNavTitlePadding     = 4;
constexpr int kCollapseFontPx      = 11;
constexpr int kCollapseBorderRadius= 4;
constexpr int kCollapsePadY        = 4;
constexpr int kCollapsePadX        = 8;
constexpr int kMultiSelectSpacing  = 4;

// ---- Panel 内容区常量 ----
constexpr int kWelcomeFontPx             = 34;
constexpr int kInputLineHeight           = 22;
constexpr int kInputDefaultLines         = 3;
constexpr int kInputMaxLines             = 10;
constexpr int kContentMarginY            = 24;
constexpr int kContentSpacing            = 16;
constexpr int kWelcomeTopOffsetY         = 240;
constexpr int kConversationTopOffsetY    = 24;
constexpr int kInputFrameRadius          = 8;
constexpr int kInputFramePad             = 8;
constexpr int kMessageMinHeight          = 240;
constexpr int kTransitionDurationMs      = 220;
constexpr int kModelLabelMinHeight       = 20;
constexpr int kModelLabelRadius          = 4;
constexpr int kSendIconSize              = 24;
constexpr int kSendButtonSize            = 36;
constexpr int kSendButtonRadius          = 18;
constexpr int kConversationBtnRadius     = 6;
constexpr int kInputHeightCheckIntervalMs= 100;

constexpr char kChatEmbeddedStyle[]= "style";

} // namespace

/******************************************************************************
 * ChatConversationPanel 实现
 ******************************************************************************/

ChatConversationPanel::ChatConversationPanel (const string& sessionId,
                                              QWidget*      parent)
    : QWidget (parent), sessionId_ (sessionId) {
  setObjectName ("chat-tab-conversation-page");
  setup_ui ();
}

void
ChatConversationPanel::setup_ui () {
  QVBoxLayout* contentLayout= new QVBoxLayout (this);
  contentLayout->setContentsMargins (0, DpiUtils::scaled (kContentMarginY), 0,
                                     DpiUtils::scaled (kContentMarginY));
  contentLayout->setSpacing (DpiUtils::scaled (kContentSpacing));
  topSpacer_= new QSpacerItem (0, DpiUtils::scaled (kWelcomeTopOffsetY),
                               QSizePolicy::Minimum, QSizePolicy::Fixed);
  contentLayout->addSpacerItem (topSpacer_);

  QWidget*     topPanel = new QWidget (this);
  QVBoxLayout* topLayout= new QVBoxLayout (topPanel);
  topLayout->setContentsMargins (0, 0, 0, 0);
  topLayout->setSpacing (DpiUtils::scaled (kContentSpacing));

  // Welcome title
  welcomeTitle_= new QLabel ("Welcome to Liii STEM!", topPanel);
  welcomeTitle_->setObjectName ("chat-tab-welcome-title");
  welcomeTitle_->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (welcomeTitle_, kWelcomeFontPx);
  topLayout->addWidget (welcomeTitle_);

  // Model label
  modelLabel_= new QLabel ("", topPanel);
  modelLabel_->setObjectName ("chat-tab-model-label");
  modelLabel_->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (modelLabel_, kNavButtonFontPx);
  modelLabel_->setStyleSheet (
      QString ("padding: 2px %1px; border-radius: %2px;")
          .arg (DpiUtils::scaled (kNavButtonPadX))
          .arg (DpiUtils::scaled (kModelLabelRadius)));
  modelLabel_->setMinimumHeight (DpiUtils::scaled (kModelLabelMinHeight));
  topLayout->addWidget (modelLabel_, 0, Qt::AlignHCenter);

  // Message area
  url msgBufUrl = ChatSessionManager::messageBufferUrl (sessionId_);
  messageWidget_= texmacs_input_widget (
      tree (DOCUMENT, ""), compound (kChatEmbeddedStyle, tuple ("generic")),
      msgBufUrl);

  QWidget* messageQWidget= concrete (messageWidget_)->as_qwidget ();
  messageFrame_          = new QWidget (topPanel);
  messageFrame_->setObjectName ("chat-tab-message-frame");
  messageFrame_->setStyleSheet (
      QString ("border: none; border-radius: %1px;")
          .arg (DpiUtils::scaled (kInputFrameRadius)));
  QVBoxLayout* messageFrameLayout= new QVBoxLayout (messageFrame_);
  messageFrameLayout->setContentsMargins (0, 0, 0, 0);
  messageFrameLayout->setSpacing (0);
  messageQWidget->setParent (messageFrame_);
  messageQWidget->setMinimumHeight (DpiUtils::scaled (kMessageMinHeight));
  messageFrameLayout->addWidget (messageQWidget);
  messageFrame_->hide ();
  topLayout->addWidget (messageFrame_, 1);

  // Input area
  QWidget*     inputArea      = new QWidget (topPanel);
  QVBoxLayout* inputAreaLayout= new QVBoxLayout (inputArea);
  inputAreaLayout->setContentsMargins (0, 0, 0, 0);
  inputAreaLayout->setSpacing (DpiUtils::scaled (kContentSpacing));

  url inBufUrl= ChatSessionManager::inputBufferUrl (sessionId_);
  inputWidget = texmacs_input_widget (
      tree (WITH, "par-par-sep", "0.05fn", tree (DOCUMENT, "")),
      compound (kChatEmbeddedStyle, tuple ("generic")), inBufUrl);
  QWidget* inputQWidget= concrete (inputWidget)->as_qwidget ();
  inputEditorWidget_   = inputQWidget;
  {
    QList<QAbstractScrollArea*> areas=
        inputQWidget->findChildren<QAbstractScrollArea*> ();
    for (QAbstractScrollArea* area : areas) {
      if (!area) continue;
      area->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
      area->setVerticalScrollBarPolicy (Qt::ScrollBarAsNeeded);
      area->setStyleSheet (
          "QScrollBar:vertical { background: transparent; width: 6px; "
          "                      margin: 0px; border: none; }"
          "QScrollBar::handle:vertical { border-radius: 3px; "
          "                            min-height: 20px; }"
          "QScrollBar::add-line:vertical, "
          "QScrollBar::sub-line:vertical { height: 0px; }");
    }
  }
  if (QTMWidget* editor= inputQWidget->findChild<QTMWidget*> ()) {
    editor->setProperty ("chat_panel", QVariant::fromValue ((void*) this));
    editor->installEventFilter (this);
  }

  QWidget* inputFrame= new QWidget (inputArea);
  inputFrame->setObjectName ("chat-tab-input-frame");
  inputFrame->setStyleSheet (
      QString ("QWidget#chat-tab-input-frame { "
               "  border: 1px solid; border-radius: %1px; }"
               "QWidget#chat-tab-input-frame:hover { "
               "  border: 1px solid; }")
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

  // Send button
  sendButton_= new QPushButton (inputFrame);
  sendButton_->setObjectName ("chat-tab-send-btn");
  sendButton_->setFocusPolicy (Qt::NoFocus);
  sendButton_->setCursor (Qt::PointingHandCursor);
  sendButton_->setIcon (QIcon (":llm-chat/send.svg"));
  int sendIconSize= DpiUtils::scaled (kSendIconSize);
  sendButton_->setIconSize (QSize (sendIconSize, sendIconSize));
  sendButton_->setFixedSize (DpiUtils::scaled (kSendButtonSize),
                             DpiUtils::scaled (kSendButtonSize));
  sendButton_->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "            background-color: transparent; }")
          .arg (DpiUtils::scaled (kSendButtonRadius)));
  connect (sendButton_, &QPushButton::clicked, this,
           [this] () { emit sendRequested (sessionId_); });
  btnLayout->addWidget (sendButton_);
  inputFrameLayout->addLayout (btnLayout);

  inputAreaLayout->addWidget (inputFrame, 0);

  // Input height timer
  QTimer* inputHeightTimer= new QTimer (inputFrame);
  inputHeightTimer->setInterval (kInputHeightCheckIntervalMs);
  connect (inputHeightTimer, &QTimer::timeout, this,
           [this] () { adjust_input_height (); });
  inputHeightTimer->start ();

  QHBoxLayout* inputWrap= new QHBoxLayout ();
  inputWrap->addStretch (1);
  inputWrap->addWidget (inputArea, 8);
  inputWrap->addStretch (1);
  topLayout->addLayout (inputWrap, 0);

  contentLayout->addWidget (topPanel, 1, Qt::AlignTop);
}

void
ChatConversationPanel::enterConversationMode () {
  if (conversationMode_) return;

  conversationMode_    = true;
  const int startOffset= DpiUtils::scaled (kWelcomeTopOffsetY);
  const int endOffset  = DpiUtils::scaled (kConversationTopOffsetY);

  if (messageFrame_) {
    QGraphicsOpacityEffect* messageEffect=
        new QGraphicsOpacityEffect (messageFrame_);
    messageEffect->setOpacity (0.0);
    messageFrame_->setGraphicsEffect (messageEffect);
    messageFrame_->show ();

    QPropertyAnimation* fadeIn=
        new QPropertyAnimation (messageEffect, "opacity", messageFrame_);
    fadeIn->setDuration (kTransitionDurationMs);
    fadeIn->setStartValue (0.0);
    fadeIn->setEndValue (1.0);
    fadeIn->start (QAbstractAnimation::DeleteWhenStopped);
  }

  if (welcomeTitle_) {
    QGraphicsOpacityEffect* titleEffect=
        new QGraphicsOpacityEffect (welcomeTitle_);
    titleEffect->setOpacity (1.0);
    welcomeTitle_->setGraphicsEffect (titleEffect);

    QPropertyAnimation* fadeOut=
        new QPropertyAnimation (titleEffect, "opacity", welcomeTitle_);
    fadeOut->setDuration (kTransitionDurationMs);
    fadeOut->setStartValue (1.0);
    fadeOut->setEndValue (0.0);
    connect (fadeOut, &QPropertyAnimation::finished, this, [this] () {
      if (welcomeTitle_) welcomeTitle_->hide ();
    });
    fadeOut->start (QAbstractAnimation::DeleteWhenStopped);
  }

  if (topSpacer_ && layout ()) {
    QVariantAnimation* offsetAnim= new QVariantAnimation (this);
    offsetAnim->setDuration (kTransitionDurationMs);
    offsetAnim->setStartValue (startOffset);
    offsetAnim->setEndValue (endOffset);
    connect (offsetAnim, &QVariantAnimation::valueChanged, this,
             [this] (const QVariant& value) {
               if (!topSpacer_ || !layout ()) return;
               topSpacer_->changeSize (0, value.toInt (), QSizePolicy::Minimum,
                                       QSizePolicy::Fixed);
               layout ()->invalidate ();
               layout ()->activate ();
             });
    offsetAnim->start (QAbstractAnimation::DeleteWhenStopped);
  }
}

void
ChatConversationPanel::focusInput () {
  if (inputEditorWidget_) {
    inputEditorWidget_->setFocus (Qt::OtherFocusReason);
    url inBufUrl= ChatSessionManager::inputBufferUrl (sessionId_);
    url vw      = get_passive_view (inBufUrl);
    if (!is_none (vw)) {
      set_current_view (vw);
      call ("update-menus");
    }
  }
}

tree
ChatConversationPanel::readInputMessage () const {
  return get_buffer_body (ChatSessionManager::inputBufferUrl (sessionId_));
}

bool
ChatConversationPanel::is_empty_document_body (tree body) {
  if (!is_func (body, DOCUMENT)) return false;
  if (N (body) == 0) return true;
  return N (body) == 1 && is_atomic (body[0]) && body[0]->label == "";
}

int
ChatConversationPanel::count_input_lines (tree body) {
  if (!is_func (body, DOCUMENT)) return 1;
  if (N (body) == 0) return 1;
  if (N (body) == 1 && is_atomic (body[0]) && body[0]->label == "") return 1;
  return N (body);
}

int
ChatConversationPanel::estimate_lines_from_height (SI contentHeight) {
  if (contentHeight <= 0) return 0;
  int px= to_qsize (0, contentHeight).height ();
  if (px <= 0) return 0;
  return (px + kInputLineHeight - 1) / kInputLineHeight;
}

void
ChatConversationPanel::adjust_input_height () {
  if (!inputEditorWidget_) return;

  tree body       = readInputMessage ();
  int  docLines   = count_input_lines (body);
  int  visualLines= 0;

  if (QTMWidget* editor= inputEditorWidget_->findChild<QTMWidget*> ()) {
    if (edit_interface_rep* ed=
            dynamic_cast<edit_interface_rep*> (editor->tm_widget ())) {
      visualLines= estimate_lines_from_height (ed->get_total_height (true));
    }
  }

  int lines       = qMax (docLines, visualLines);
  int targetLines = qMax (kInputDefaultLines, lines);
  targetLines     = qMin (targetLines, kInputMaxLines);
  int targetHeight= DpiUtils::scaled (kInputLineHeight * targetLines);

  if (inputEditorWidget_->minimumHeight () != targetHeight ||
      inputEditorWidget_->maximumHeight () != targetHeight) {
    inputEditorWidget_->setMinimumHeight (targetHeight);
    inputEditorWidget_->setMaximumHeight (targetHeight);
  }
}

bool
ChatConversationPanel::eventFilter (QObject* watched, QEvent* event) {
  if (event->type () == QEvent::KeyPress) {
    QKeyEvent* keyEvent= static_cast<QKeyEvent*> (event);
    if ((keyEvent->key () == Qt::Key_Return ||
         keyEvent->key () == Qt::Key_Enter) &&
        !(keyEvent->modifiers () & Qt::ShiftModifier)) {
      void* ptr= watched->property ("chat_panel").value<void*> ();
      if (ptr == this) {
        emit sendRequested (sessionId_);
        return true;
      }
    }
  }
  return QWidget::eventFilter (watched, event);
}

/******************************************************************************
 * ChatSidebar 实现
 ******************************************************************************/

ChatSidebar::ChatSidebar (const QList<SessionDisplayInfo>& sessions,
                          const string& activeSessionId, QWidget* parent)
    : QWidget (parent) {
  setObjectName ("chat-tab-sidebar-list");

  QVBoxLayout* mainLayout= new QVBoxLayout (this);
  mainLayout->setContentsMargins (0, 0, 0, 0);
  mainLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  // 会话数量标签
  conversationCountLabel_= new QLabel ("Conversations (0)", this);
  conversationCountLabel_->setObjectName ("chat-tab-conversation-count");
  DpiUtils::applyScaledFont (conversationCountLabel_, kNavTitleFontPx);
  mainLayout->addWidget (conversationCountLabel_);

  // 搜索框
  searchEdit_= new QLineEdit (this);
  searchEdit_->setObjectName ("chat-tab-search-edit");
  searchEdit_->setPlaceholderText (QString::fromUtf8 ("搜索会话..."));
  searchEdit_->setClearButtonEnabled (true);
  searchEdit_->setFocusPolicy (Qt::ClickFocus);
  DpiUtils::applyScaledFont (searchEdit_, kCollapseFontPx);
  searchEdit_->setStyleSheet (
      QString ("QLineEdit { border: none; border-radius: %1px; "
               "padding: %2px %3px; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (searchEdit_, &QLineEdit::textChanged, this,
           [this] () { applySearchFilter (); });
  mainLayout->addWidget (searchEdit_);

  // 多选模式批量操作栏（默认隐藏）
  multiSelectBar_= new QWidget (this);
  multiSelectBar_->setObjectName ("chat-tab-multi-select-bar");
  QHBoxLayout* multiSelectLayout= new QHBoxLayout (multiSelectBar_);
  multiSelectLayout->setContentsMargins (0, 0, 0, 0);
  multiSelectLayout->setSpacing (DpiUtils::scaled (kMultiSelectSpacing));

  QPushButton* cancelSelectBtn= new QPushButton ("取消", multiSelectBar_);
  cancelSelectBtn->setFocusPolicy (Qt::NoFocus);
  cancelSelectBtn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (cancelSelectBtn, kCollapseFontPx);
  cancelSelectBtn->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "padding: %2px %3px; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (cancelSelectBtn, &QPushButton::clicked, this,
           [this] () { exitMultiSelectMode (); });
  multiSelectLayout->addWidget (cancelSelectBtn);

  QPushButton* selectAllBtn= new QPushButton ("全选", multiSelectBar_);
  selectAllBtn->setFocusPolicy (Qt::NoFocus);
  selectAllBtn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (selectAllBtn, kCollapseFontPx);
  selectAllBtn->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "padding: %2px %3px; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (selectAllBtn, &QPushButton::clicked, this, [this] () {
    for (auto it= items_.begin (); it != items_.end (); ++it) {
      if (!it->selectCheckBox) continue;
      for (const auto& info : sessionCache_) {
        if (info.sessionId == it.key () &&
            info.archived == archiveSelectMode_) {
          it->selectCheckBox->setChecked (true);
        }
      }
    }
  });
  multiSelectLayout->addWidget (selectAllBtn);

  multiSelectLayout->addStretch ();

  batchArchiveBtn_= new QPushButton ("归档", multiSelectBar_);
  batchArchiveBtn_->setFocusPolicy (Qt::NoFocus);
  batchArchiveBtn_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (batchArchiveBtn_, kCollapseFontPx);
  batchArchiveBtn_->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "padding: %2px %3px; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (batchArchiveBtn_, &QPushButton::clicked, this, [this] () {
    QList<string> ids= getCheckedSessionIds ();
    if (!ids.isEmpty ()) emit multiArchiveRequested (ids);
  });
  multiSelectLayout->addWidget (batchArchiveBtn_);

  QPushButton* batchDeleteBtn= new QPushButton ("删除", multiSelectBar_);
  batchDeleteBtn->setFocusPolicy (Qt::NoFocus);
  batchDeleteBtn->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (batchDeleteBtn, kCollapseFontPx);
  batchDeleteBtn->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; "
               "padding: %2px %3px; }")
          .arg (DpiUtils::scaled (kCollapseBorderRadius))
          .arg (DpiUtils::scaled (kCollapsePadY))
          .arg (DpiUtils::scaled (kCollapsePadX)));
  connect (batchDeleteBtn, &QPushButton::clicked, this, [this] () {
    QList<string> ids= getCheckedSessionIds ();
    if (!ids.isEmpty ()) emit multiDeleteRequested (ids);
  });
  multiSelectLayout->addWidget (batchDeleteBtn);

  multiSelectBar_->hide ();
  mainLayout->addWidget (multiSelectBar_);

  // 列表滚动区
  QWidget*     scrollContent= new QWidget (this);
  QVBoxLayout* scrollLayout = new QVBoxLayout (scrollContent);
  scrollLayout->setContentsMargins (0, 0, 0, 0);
  scrollLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  conversationListWidget_= new QWidget (scrollContent);
  conversationListWidget_->setObjectName ("chat-tab-conversation-list");
  conversationListLayout_= new QVBoxLayout (conversationListWidget_);
  conversationListLayout_->setContentsMargins (0, 0, 0, 0);
  conversationListLayout_->setSpacing (DpiUtils::scaled (kSidebarSpacing));
  scrollLayout->addWidget (conversationListWidget_);

  // 归档区标题
  archiveHeaderButton_= new QPushButton ("Archived (0)", scrollContent);
  archiveHeaderButton_->setObjectName ("chat-tab-archive-header");
  archiveHeaderButton_->setFocusPolicy (Qt::NoFocus);
  archiveHeaderButton_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (archiveHeaderButton_, kNavTitleFontPx);
  archiveHeaderButton_->setStyleSheet (
      QString ("QPushButton { border: none; "
               "padding: %1px %2px; } ")
          .arg (DpiUtils::scaled (kNavTitlePadding))
          .arg (DpiUtils::scaled (kNavButtonPadX)));
  connect (archiveHeaderButton_, &QPushButton::clicked, this, [this] () {
    archiveCollapsed_= !archiveCollapsed_;
    if (archiveListWidget_) archiveListWidget_->setVisible (!archiveCollapsed_);
  });
  scrollLayout->addWidget (archiveHeaderButton_);

  archiveListWidget_= new QWidget (scrollContent);
  archiveListWidget_->setObjectName ("chat-tab-archive-list");
  archiveListLayout_= new QVBoxLayout (archiveListWidget_);
  archiveListLayout_->setContentsMargins (0, 0, 0, 0);
  archiveListLayout_->setSpacing (DpiUtils::scaled (kSidebarSpacing));
  archiveListWidget_->hide ();
  scrollLayout->addWidget (archiveListWidget_);

  scrollLayout->addStretch ();

  QScrollArea* scrollArea= new QScrollArea (this);
  scrollArea->setWidgetResizable (true);
  scrollArea->setFrameShape (QFrame::NoFrame);
  scrollArea->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  scrollArea->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  scrollArea->setWidget (scrollContent);
  mainLayout->addWidget (scrollArea, 1);

  // 构造时直接创建 items，按 archived 分组
  sessionCache_   = sessions;
  activeSessionId_= activeSessionId;

  for (const SessionDisplayInfo& info : sessions) {
    SidebarItem item= createItem (info.sessionId);
    item.sidebarButton->setText (to_qstring (info.displayTitle));
    item.sidebarButton->setChecked (info.sessionId == activeSessionId &&
                                    !info.archived);
    item.isArchived= info.archived;

    if (info.archived) {
      archiveListLayout_->addWidget (item.itemWidget);
    }
    else {
      conversationListLayout_->addWidget (item.itemWidget);
    }
    items_.insert (info.sessionId, item);
  }

  updateCountLabels ();
}

void
ChatSidebar::addItem (const string& sessionId, const string& displayTitle) {
  if (items_.contains (sessionId)) return;

  SidebarItem item= createItem (sessionId);
  item.sidebarButton->setText (to_qstring (displayTitle));
  item.sidebarButton->setChecked (true);
  item.isArchived= false;
  conversationListLayout_->insertWidget (0, item.itemWidget);
  items_.insert (sessionId, item);

  // 更新 sessionCache_ 以保持右键菜单数据同步
  SessionDisplayInfo info;
  info.sessionId   = sessionId;
  info.displayTitle= displayTitle;
  info.archived    = false;
  sessionCache_.prepend (info);
  activeSessionId_= sessionId;

  updateCountLabels ();
}

void
ChatSidebar::updateItemTitle (const string& sessionId,
                              const string& displayTitle) {
  auto it= items_.find (sessionId);
  if (it == items_.end ()) return;
  if (it->sidebarButton) {
    it->sidebarButton->setText (to_qstring (displayTitle));
  }
  // 更新 sessionCache_ 中的显示标题
  for (auto& info : sessionCache_) {
    if (info.sessionId == sessionId) {
      info.displayTitle= displayTitle;
      break;
    }
  }
}

void
ChatSidebar::setActiveItem (const string& sessionId) {
  activeSessionId_= sessionId;
  for (auto it= items_.begin (); it != items_.end (); ++it) {
    if (!it->sidebarButton) continue;
    bool isActive= (it.key () == sessionId && !it->isArchived);
    it->sidebarButton->setChecked (isActive);
  }
}

void
ChatSidebar::moveToArchive (const string& sessionId) {
  auto it= items_.find (sessionId);
  if (it == items_.end ()) return;
  SidebarItem& item= it.value ();

  if (!item.isArchived) {
    item.isArchived= true;
    archiveListLayout_->addWidget (item.itemWidget);
    item.sidebarButton->setChecked (false);
  }

  // 更新 sessionCache_ 中的 archived 状态
  for (auto& info : sessionCache_) {
    if (info.sessionId == sessionId) {
      info.archived= true;
      break;
    }
  }
  if (activeSessionId_ == sessionId) activeSessionId_= "";

  updateCountLabels ();
}

void
ChatSidebar::moveFromArchive (const string& sessionId) {
  auto it= items_.find (sessionId);
  if (it == items_.end ()) return;
  SidebarItem& item= it.value ();

  if (item.isArchived) {
    item.isArchived= false;
    conversationListLayout_->insertWidget (0, item.itemWidget);
  }

  // 更新 sessionCache_ 中的 archived 状态
  for (auto& info : sessionCache_) {
    if (info.sessionId == sessionId) {
      info.archived= false;
      break;
    }
  }

  updateCountLabels ();
}

void
ChatSidebar::applySearchFilter () {
  QString filterText=
      searchEdit_ ? searchEdit_->text ().toLower () : QString ();

  for (auto it= items_.begin (); it != items_.end (); ++it) {
    SidebarItem& item= it.value ();
    if (!item.sidebarButton || !item.itemWidget) continue;

    // 多选模式：checkbox 可见性
    if (item.selectCheckBox) {
      if (item.isArchived) item.selectCheckBox->setVisible (archiveSelectMode_);
      else item.selectCheckBox->setVisible (multiSelectMode_);
    }

    QString displayText= item.sidebarButton->text ();
    bool    matchesFilter=
        filterText.isEmpty () || displayText.toLower ().contains (filterText);
    item.itemWidget->setVisible (matchesFilter);
  }

  updateCountLabels ();
}

void
ChatSidebar::updateCountLabels () {
  int activeCount  = 0;
  int archivedCount= 0;

  for (auto it= items_.constBegin (); it != items_.constEnd (); ++it) {
    if (it->isArchived) ++archivedCount;
    else ++activeCount;
  }

  if (conversationCountLabel_) {
    conversationCountLabel_->setText (
        QString ("Conversations (%1)").arg (activeCount));
    conversationCountLabel_->setVisible (true);
  }
  if (archiveHeaderButton_) {
    archiveHeaderButton_->setText (
        QString ("Archived (%1)").arg (archivedCount));
    archiveHeaderButton_->setVisible (true);
  }
}

ChatSidebar::SidebarItem
ChatSidebar::createItem (const string& sessionId) {
  SidebarItem item;

  item.itemWidget= new QWidget ();
  item.itemWidget->setObjectName ("chat-tab-session-item");
  QHBoxLayout* itemLayout= new QHBoxLayout (item.itemWidget);
  itemLayout->setContentsMargins (0, 0, 0, 0);
  itemLayout->setSpacing (DpiUtils::scaled (kSessionItemSpacing));

  item.selectCheckBox= new QCheckBox (item.itemWidget);
  item.selectCheckBox->setObjectName ("chat-tab-select-checkbox");
  item.selectCheckBox->setFocusPolicy (Qt::NoFocus);
  item.selectCheckBox->setStyleSheet ("QCheckBox::indicator:checked { "
                                      "  border: none; "
                                      "  border-radius: 3px; }"
                                      "QCheckBox::indicator:unchecked { "
                                      "  border: none; "
                                      "  border-radius: 3px; }");
  item.selectCheckBox->hide ();
  itemLayout->addWidget (item.selectCheckBox);

  item.sidebarButton= new QPushButton ("新会话", item.itemWidget);
  item.sidebarButton->setObjectName ("chat-tab-conversation-btn");
  item.sidebarButton->setCheckable (true);
  item.sidebarButton->setFocusPolicy (Qt::NoFocus);
  item.sidebarButton->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (item.sidebarButton, kNavButtonFontPx);
  item.sidebarButton->setStyleSheet (
      QString ("QPushButton { border: none; border-radius: %1px; padding: %2px "
               "%3px; }")
          .arg (DpiUtils::scaled (kConversationBtnRadius))
          .arg (DpiUtils::scaled (kNavButtonPadY))
          .arg (DpiUtils::scaled (kNavButtonPadX)));
  itemLayout->addWidget (item.sidebarButton, 1);

  // clicked 信号：连接一次，不再在 refreshInternal 中反复 disconnect/connect
  connect (item.sidebarButton, &QPushButton::clicked, this,
           [this, sid= sessionId] () { emit sessionClicked (sid); });

  // 右键菜单：连接一次，执行时从 sessionCache_ 查找当前状态
  item.sidebarButton->setContextMenuPolicy (Qt::CustomContextMenu);
  connect (
      item.sidebarButton, &QPushButton::customContextMenuRequested, this,
      [this, sid= sessionId] (const QPoint& pos) {
        QPushButton* senderBtn= qobject_cast<QPushButton*> (sender ());
        bool         archived = false;
        for (const auto& info : sessionCache_) {
          if (info.sessionId == sid) {
            archived= info.archived;
            break;
          }
        }

        QMenu         menu;
        QList<string> checked= getCheckedSessionIds ();

        if (!checked.isEmpty ()) {
          if (!archived) {
            menu.addAction (QString ("归档所选 (%1)").arg (checked.size ()));
          }
          menu.addAction (QString ("删除所选 (%1)").arg (checked.size ()));
          QAction* chosen= menu.exec (senderBtn->mapToGlobal (pos));
          if (!chosen) return;
          QString txt= chosen->text ();
          if (txt.startsWith ("归档所选")) {
            emit multiArchiveRequested (checked);
          }
          else if (txt.startsWith ("删除所选")) {
            emit multiDeleteRequested (checked);
          }
        }
        else {
          QAction* renameAction = menu.addAction ("重命名");
          QAction* archiveAction= menu.addAction (archived ? "恢复" : "归档");
          QAction* deleteAction = menu.addAction ("删除");
          menu.addSeparator ();
          QAction* multiSelectAction= menu.addAction ("多选");
          QAction* chosen           = menu.exec (senderBtn->mapToGlobal (pos));
          if (chosen == renameAction) {
            emit renameRequested (sid, "");
          }
          else if (chosen == archiveAction) {
            if (archived) emit restoreRequested (sid);
            else emit archiveRequested (sid);
          }
          else if (chosen == deleteAction) {
            emit deleteRequested (sid);
          }
          else if (chosen == multiSelectAction) {
            enterMultiSelectMode (archived);
          }
        }
      });

  return item;
}

void
ChatSidebar::destroyItem (const string& sessionId) {
  auto it= items_.find (sessionId);
  if (it == items_.end ()) return;

  SidebarItem& item= it.value ();
  if (item.sidebarButton)
    disconnect (item.sidebarButton, nullptr, this, nullptr);
  if (item.selectCheckBox)
    disconnect (item.selectCheckBox, nullptr, this, nullptr);
  if (item.itemWidget) {
    item.itemWidget->hide ();
    item.itemWidget->setParent (nullptr);
    delete item.itemWidget;
  }
  items_.erase (it);
}

void
ChatSidebar::removeItem (const string& sessionId) {
  destroyItem (sessionId);
  if (activeSessionId_ == sessionId) activeSessionId_= "";
  // 从 sessionCache_ 中移除
  for (int i= 0; i < sessionCache_.size (); ++i) {
    if (sessionCache_[i].sessionId == sessionId) {
      sessionCache_.removeAt (i);
      break;
    }
  }
  updateCountLabels ();
}

void
ChatSidebar::enterMultiSelectMode (bool archived) {
  if (archived) archiveSelectMode_= true;
  else multiSelectMode_= true;
  if (multiSelectBar_)
    multiSelectBar_->setVisible (multiSelectMode_ || archiveSelectMode_);
  if (batchArchiveBtn_) batchArchiveBtn_->setVisible (multiSelectMode_);
  applySearchFilter ();
}

void
ChatSidebar::exitMultiSelectMode () {
  multiSelectMode_  = false;
  archiveSelectMode_= false;
  for (auto it= items_.begin (); it != items_.end (); ++it) {
    if (it->selectCheckBox) it->selectCheckBox->setChecked (false);
  }
  if (multiSelectBar_) multiSelectBar_->hide ();
  applySearchFilter ();
}

const string&
ChatSidebar::activeSessionId () const {
  return activeSessionId_;
}

QList<string>
ChatSidebar::getCheckedSessionIds () const {
  QList<string> result;
  for (auto it= items_.constBegin (); it != items_.constEnd (); ++it) {
    if (it->selectCheckBox && it->selectCheckBox->isChecked ())
      result.append (it.key ());
  }
  return result;
}

/******************************************************************************
 * QTChatTabWidget 构造/析构
 ******************************************************************************/

QTChatTabWidget::QTChatTabWidget (const QList<SessionDisplayInfo>& sessions,
                                  const string& activeSessionId,
                                  QWidget*      parent)
    : QWidget (parent), sidebarWidget_ (nullptr), contentWidget_ (nullptr),
      collapseButton_ (nullptr), floatingExpandBtn_ (nullptr),
      floatingNewChatBtn_ (nullptr), floatingBtnContainer_ (nullptr),
      newChatButton_ (nullptr), sidebarNormalContent_ (nullptr),
      conversationStack_ (nullptr) {
  setFocusPolicy (Qt::StrongFocus);

  QHBoxLayout* mainLayout= new QHBoxLayout (this);
  mainLayout->setContentsMargins (0, 0, 0, 0);
  mainLayout->setSpacing (0);

  // 左侧侧边栏
  QWidget* sidebar= new QWidget (this);
  sidebar->setObjectName ("chat-tab-sidebar");
  sidebar->setMinimumWidth (DpiUtils::scaled (kSidebarMinWidth));
  sidebar->setSizePolicy (QSizePolicy::Minimum, QSizePolicy::Preferred);
  sidebarWidget_= sidebar;

  QVBoxLayout* sidebarLayout= new QVBoxLayout (sidebar);
  sidebarLayout->setContentsMargins (
      DpiUtils::scaled (kSidebarMarginX), DpiUtils::scaled (kSidebarMarginY),
      DpiUtils::scaled (kSidebarMarginX), DpiUtils::scaled (kSidebarMarginY));
  sidebarLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  setup_left_sidebar (sidebarLayout, sessions, activeSessionId);
  sidebar->adjustSize ();
  const int contentWidth= sidebar->sizeHint ().width ();
  sidebarExpandedWidth_=
      qMax (DpiUtils::scaled (kSidebarMinWidth), contentWidth);
  sidebar->setFixedWidth (sidebarExpandedWidth_);
  mainLayout->addWidget (sidebar);

  // 右侧内容区
  setup_right_content (mainLayout);
}

QTChatTabWidget::~QTChatTabWidget () {
  for (ChatConversationPanel* panel : conversations_)
    delete panel;
  conversations_.clear ();
}

/******************************************************************************
 * QTChatTabWidget 公共方法（View 接口，被 Controller 调用）
 ******************************************************************************/

ChatConversationPanel*
QTChatTabWidget::createPanel (const string& sessionId) {
  if (!conversationStack_) return nullptr;

  ChatConversationPanel* panel=
      new ChatConversationPanel (sessionId, conversationStack_);
  conversationStack_->addWidget (panel);
  conversations_.append (panel);
  return panel;
}

void
QTChatTabWidget::activatePanel (ChatConversationPanel* panel) {
  if (!panel || !conversationStack_) return;
  activeConversation_= panel;
  conversationStack_->setCurrentWidget (panel);
  panel->focusInput ();
}

void
QTChatTabWidget::removePanel (ChatConversationPanel* panel) {
  if (!panel) return;

  if (conversationStack_) conversationStack_->removeWidget (panel);

  conversations_.removeOne (panel);

  if (activeConversation_ == panel) activeConversation_= nullptr;

  if (sidebar_) sidebar_->removeItem (panel->sessionId ());

  delete panel;
}

/******************************************************************************
 * QTChatTabWidget UI 设置
 ******************************************************************************/

void
QTChatTabWidget::setup_left_sidebar (QVBoxLayout* sidebarLayout,
                                     const QList<SessionDisplayInfo>& sessions,
                                     const string& activeSessionId) {
  QWidget* normalContent= new QWidget (sidebarWidget_);
  normalContent->setObjectName ("chat-tab-sidebar-normal");
  QVBoxLayout* normalLayout= new QVBoxLayout (normalContent);
  normalLayout->setContentsMargins (0, 0, 0, 0);
  normalLayout->setSpacing (DpiUtils::scaled (kSidebarSpacing));

  // 顶部标题栏（Chat + 收缩按钮）
  QWidget* headerWidget= new QWidget (normalContent);
  headerWidget->setObjectName ("chat-tab-header");
  QHBoxLayout* headerLayout= new QHBoxLayout (headerWidget);
  headerLayout->setContentsMargins (0, 0, 0, 0);
  headerLayout->setSpacing (0);

  QLabel* navTitle= new QLabel ("Chat", headerWidget);
  navTitle->setObjectName ("chat-tab-nav-title");
  DpiUtils::applyScaledFont (navTitle, kNavChatFontPx);
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
      QString ("QPushButton { border: none; border-radius: %1px; }")
          .arg (DpiUtils::scaled (kToggleBtnSize / 2)));
  connect (collapseBtn, &QPushButton::clicked, this,
           [this] () { toggle_sidebar (); });
  collapseButton_= collapseBtn;
  headerLayout->addWidget (collapseBtn);

  normalLayout->addWidget (headerWidget);

  // New chat 按钮
  newChatButton_= new QPushButton (qt_translate ("New chat"), normalContent);
  newChatButton_->setObjectName ("chat-tab-new-btn");
  newChatButton_->setFocusPolicy (Qt::NoFocus);
  newChatButton_->setCursor (Qt::PointingHandCursor);
  DpiUtils::applyScaledFont (newChatButton_, kNavButtonFontPx);
  newChatButton_->setIcon (QIcon (":llm-chat/addchat.svg"));
  newChatButton_->setIconSize (QSize (DpiUtils::scaled (kNewChatIconSize),
                                      DpiUtils::scaled (kNewChatIconSize)));
  newChatButton_->setFixedSize (
      QSize (DpiUtils::scaled (kNewChatButtonWidth),
             DpiUtils::scaled (kNewChatButtonHeight)));
  newChatButton_->setStyleSheet (
      QString ("QPushButton { border-radius: %1px; padding: %2px %3px; }")
          .arg (DpiUtils::scaled (kNewChatButtonHeight / 2))
          .arg (DpiUtils::scaled (kNavButtonPadY))
          .arg (DpiUtils::scaled (kNavButtonPadX)));

  QGraphicsDropShadowEffect* newChatShadow=
      new QGraphicsDropShadowEffect (newChatButton_);
  newChatShadow->setBlurRadius (DpiUtils::scaled (kNewChatShadowBlur));
  newChatShadow->setColor (QColor (0, 0, 0, kNewChatShadowAlpha));
  newChatShadow->setOffset (0, DpiUtils::scaled (kNewChatShadowOffsetY));
  newChatButton_->setGraphicsEffect (newChatShadow);

  newChatButton_->setAttribute (Qt::WA_Hover);
  newChatButton_->installEventFilter (this);
  normalLayout->addWidget (newChatButton_, 0, Qt::AlignHCenter);

  // ChatSidebar
  sidebar_= new ChatSidebar (sessions, activeSessionId, normalContent);
  normalLayout->addWidget (sidebar_, 1);

  sidebarNormalContent_= normalContent;
  sidebarLayout->addWidget (normalContent);
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

  // 浮球按钮容器
  QWidget* floatingContainer= new QWidget (this);
  floatingContainer->setObjectName ("chat-tab-floating-container");
  QHBoxLayout* floatingLayout= new QHBoxLayout (floatingContainer);
  floatingLayout->setContentsMargins (DpiUtils::scaled (kFloatingContainerPad),
                                      DpiUtils::scaled (kFloatingContainerPad),
                                      DpiUtils::scaled (kFloatingContainerPad),
                                      DpiUtils::scaled (kFloatingContainerPad));
  floatingLayout->setSpacing (DpiUtils::scaled (kFloatingBtnSpacing));
  floatingContainer->setStyleSheet (
      QString ("QWidget#chat-tab-floating-container { "
               "border-radius: %1px; }")
          .arg (DpiUtils::scaled (kToggleBtnSize / 2 + kFloatingContainerPad)));

  auto make_floating_btn= [] (QWidget* parent, const QString& name,
                              const QString& icon) {
    QPushButton* btn= new QPushButton (parent);
    btn->setObjectName (name);
    btn->setFocusPolicy (Qt::NoFocus);
    btn->setCursor (Qt::PointingHandCursor);
    btn->setIcon (QIcon (icon));
    btn->setIconSize (QSize (DpiUtils::scaled (kToggleIconSize),
                             DpiUtils::scaled (kToggleIconSize)));
    btn->setFixedSize (DpiUtils::scaled (kToggleBtnSize),
                       DpiUtils::scaled (kToggleBtnSize));
    btn->setStyleSheet (
        QString ("QPushButton { border: none; border-radius: %1px; "
                 "background-color: transparent; } ")
            .arg (DpiUtils::scaled (kToggleBtnSize / 2)));
    return btn;
  };

  QPushButton* floatingBtn=
      make_floating_btn (floatingContainer, "chat-tab-floating-expand-btn",
                         ":llm-chat/sidebar.svg");
  connect (floatingBtn, &QPushButton::clicked, this,
           [this] () { toggle_sidebar (); });
  floatingLayout->addWidget (floatingBtn);
  floatingExpandBtn_= floatingBtn;

  QPushButton* floatingNewBtn= make_floating_btn (
      floatingContainer, "chat-tab-floating-new-btn", ":llm-chat/addchat.svg");
  floatingNewChatBtn_= floatingNewBtn;
  floatingLayout->addWidget (floatingNewBtn);

  floatingContainer->adjustSize ();
  floatingContainer->move (DpiUtils::scaled (kFloatingBtnMarginX),
                           DpiUtils::scaled (kFloatingBtnMarginY));
  floatingContainer->hide ();
  floatingBtnContainer_= floatingContainer;
}

/******************************************************************************
 * QTChatTabWidget 内部 UI 方法
 ******************************************************************************/

void
QTChatTabWidget::toggle_sidebar () {
  if (!sidebarWidget_) return;
  if (sidebarCollapsed_) {
    if (floatingBtnContainer_) floatingBtnContainer_->hide ();
    sidebarWidget_->show ();
    sidebarCollapsed_= false;
  }
  else {
    sidebarWidget_->hide ();
    if (floatingBtnContainer_) {
      floatingBtnContainer_->move (DpiUtils::scaled (kFloatingBtnMarginX),
                                   DpiUtils::scaled (kFloatingBtnMarginY));
      floatingBtnContainer_->show ();
    }
    sidebarCollapsed_= true;
  }
}

/******************************************************************************
 * QTChatTabWidget 事件处理
 ******************************************************************************/

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

bool
QTChatTabWidget::eventFilter (QObject* watched, QEvent* event) {
  if (watched == newChatButton_) {
    if (event->type () == QEvent::HoverEnter) {
      if (QGraphicsDropShadowEffect* effect=
              qobject_cast<QGraphicsDropShadowEffect*> (
                  newChatButton_->graphicsEffect ())) {
        effect->setBlurRadius (DpiUtils::scaled (kNewChatHoverShadowBlur));
        effect->setColor (QColor (0, 0, 0, kNewChatHoverShadowAlpha));
        effect->setOffset (0, DpiUtils::scaled (kNewChatHoverShadowOffsetY));
      }
    }
    else if (event->type () == QEvent::HoverLeave) {
      if (QGraphicsDropShadowEffect* effect=
              qobject_cast<QGraphicsDropShadowEffect*> (
                  newChatButton_->graphicsEffect ())) {
        effect->setBlurRadius (DpiUtils::scaled (kNewChatShadowBlur));
        effect->setColor (QColor (0, 0, 0, kNewChatShadowAlpha));
        effect->setOffset (0, DpiUtils::scaled (kNewChatShadowOffsetY));
      }
    }
  }
  return QWidget::eventFilter (watched, event);
}
