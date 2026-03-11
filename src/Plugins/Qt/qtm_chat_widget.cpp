/******************************************************************************
 * MODULE     : qtm_chat_widget.cpp
 * DESCRIPTION: Main Chat Widget Implementation
 * COPYRIGHT  : (C) 2026 Mogan Team
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qtm_chat_widget.hpp"
#include "qtm_message_widget.hpp"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QDebug>

QTMChatWidget::QTMChatWidget()
    : qt_simple_widget_rep(),
      model(nullptr),
      main_widget(nullptr),
      scroll_area(nullptr),
      messages_container(nullptr),
      scroll_timer(nullptr) {
  type= tm_chat_messages_widget;
  model= new ChatModel();
  model->attachObserver(this);

  initializeUI();

  qDebug() << "[QTMChatWidget] Created";
}

QTMChatWidget::~QTMChatWidget() {
  if (model) {
    model->detachObserver(this);
    delete model;
  }
  // scroll_timer not used (using QTimer::singleShot instead)

  message_widgets= array<QTMMessageWidget*>(0);
}

void
QTMChatWidget::initializeUI() {
  qDebug() << "[QTMChatWidget::initializeUI] Starting...";
  // Main container
  main_widget= new QWidget();
  qDebug() << "[QTMChatWidget::initializeUI] main_widget created:" << main_widget;
  QVBoxLayout* main_layout= new QVBoxLayout(main_widget);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(0);

  // Scroll area
  scroll_area= new QScrollArea();
  scroll_area->setWidgetResizable(true);

  // Messages container with layout
  messages_container= new QWidget();
  messages_layout= new QVBoxLayout(messages_container);
  messages_layout->setSpacing(5);
  messages_layout->setContentsMargins(5, 5, 5, 5);
  messages_layout->addStretch();  // Add stretch at bottom initially

  scroll_area->setWidget(messages_container);

  // Add scroll area to main layout
  main_layout->addWidget(scroll_area);
  main_widget->setLayout(main_layout);

  // Timer for deferred scroll-to-bottom (using singleShot, no slots needed)
  scroll_timer= nullptr;  // Not needed with singleShot approach

  qwid= main_widget;
  qDebug() << "[QTMChatWidget::initializeUI] qwid set to:" << qwid;
}

QWidget*
QTMChatWidget::as_qwidget() {
  qDebug() << "[QTMChatWidget::as_qwidget] called, main_widget=" << main_widget;
  if (!main_widget) {
    qDebug() << "[QTMChatWidget::as_qwidget] ERROR: main_widget is null!";
    // 如果 main_widget 为 null，调用父类的实现
    return qt_simple_widget_rep::as_qwidget();
  }
  return main_widget;
}

void
QTMChatWidget::send(slot s, blackbox val) {
  switch (s) {
  case SLOT_CHAT_ADD_MESSAGE: {
    typedef pair<string, string> T;
    T data= open_box<T>(val);
    handleAddMessage(data.x1, data.x2);
  } break;

  case SLOT_CHAT_REMOVE_MESSAGE: {
    int msg_id= open_box<int>(val);
    handleRemoveMessage(msg_id);
  } break;

  case SLOT_CHAT_CLEAR: {
    handleClear();
  } break;

  case SLOT_CHAT_LOAD_HISTORY: {
    string filepath= open_box<string>(val);
    handleLoadHistory(filepath);
  } break;

  case SLOT_CHAT_SAVE_HISTORY: {
    string filepath= open_box<string>(val);
    handleSaveHistory(filepath);
  } break;

  default:
    qt_simple_widget_rep::send(s, val);
  }
}

blackbox
QTMChatWidget::query(slot s, int type_id) {
  switch (s) {
  case SLOT_CHAT_GET_MESSAGES: {
    // Return messages as scheme tree
    tree msg_tree= model->messagesToSchemeTree();
    return close_box<tree>(msg_tree);
  }

  default:
    return qt_simple_widget_rep::query(s, type_id);
  }
}

void
QTMChatWidget::handleAddMessage(const string& role, const string& content) {
  model->addMessage(role, content);
}

void
QTMChatWidget::handleRemoveMessage(int msg_id) {
  model->removeMessage(msg_id);
}

void
QTMChatWidget::handleClear() {
  model->clearMessages();
}

void
QTMChatWidget::handleLoadHistory(const string& filepath) {
  history_file_path= filepath;
  model->loadFromFile(filepath);
}

void
QTMChatWidget::handleSaveHistory(const string& filepath) {
  history_file_path= filepath;
  model->saveToFile(filepath);
}

QTMMessageWidget*
QTMChatWidget::createMessageWidget(const ChatMessage& msg) {
  QTMMessageWidget* widget= new QTMMessageWidget(msg, messages_container);
  return widget;
}

void
QTMChatWidget::addMessageWidgetToLayout(QTMMessageWidget* widget) {
  if (!widget) return;

  // Insert before the stretch item (which is at the end)
  int stretch_index= messages_layout->count() - 1;
  if (stretch_index < 0) stretch_index= 0;
  messages_layout->insertWidget(stretch_index, widget, 0);

  message_widgets << widget;

  // Schedule scroll-to-bottom (deferred to let layout recalculate)
  // Using QTimer::singleShot with lambda (no slots needed)
  QTimer::singleShot(50, [this]() { this->doScrollToBottom(); });
}

void
QTMChatWidget::removeMessageWidgetFromLayout(int msg_id) {
  for (int i= 0; i < N(message_widgets); ++i) {
    if (message_widgets[i] && message_widgets[i]->getMessageId() == msg_id) {
      messages_layout->removeWidget(message_widgets[i]);
      delete message_widgets[i];
      // Remove by creating new array without this element
      array<QTMMessageWidget*> new_widgets;
      for (int j= 0; j < N(message_widgets); ++j) {
        if (j != i) new_widgets << message_widgets[j];
      }
      message_widgets= new_widgets;
      return;
    }
  }
}

void
QTMChatWidget::clearLayout() {
  // Remove all message widgets
  for (int i= 0; i < N(message_widgets); ++i) {
    if (message_widgets[i]) {
      messages_layout->removeWidget(message_widgets[i]);
      delete message_widgets[i];
    }
  }
  message_widgets= array<QTMMessageWidget*>(0);

  // Re-add stretch at the end
  messages_layout->addStretch();
}

void
QTMChatWidget::onMessageAdded(const ChatMessage& msg) {
  qDebug() << "[View] Message added:" << as_charp(msg.role);

  QTMMessageWidget* widget= createMessageWidget(msg);
  addMessageWidgetToLayout(widget);
}

void
QTMChatWidget::onMessageRemoved(int msg_id) {
  qDebug() << "[View] Message removed:" << msg_id;
  removeMessageWidgetFromLayout(msg_id);
}

void
QTMChatWidget::onCleared() {
  qDebug() << "[View] Messages cleared";
  clearLayout();
}

void
QTMChatWidget::onHistoryLoaded(const array<ChatMessage>& messages) {
  qDebug() << "[View] History loaded:" << N(messages) << "messages";

  clearLayout();
  for (int i= 0; i < N(messages); ++i) {
    // Cast away const to access array elements
    ChatMessage& msg= ((array<ChatMessage>&)messages)[i];
    QTMMessageWidget* widget= createMessageWidget(msg);
    messages_layout->insertWidget(messages_layout->count() - 1, widget, 0);
    message_widgets << widget;
  }

  scrollToBottom();
}

void
QTMChatWidget::scrollToBottom() {
  QScrollBar* scrollbar= scroll_area->verticalScrollBar();
  if (scrollbar) {
    scrollbar->setValue(scrollbar->maximum());
  }
}

void
QTMChatWidget::doScrollToBottom() {
  // scroll_timer no longer used, just scroll directly
  scrollToBottom();
}
