/******************************************************************************
 * MODULE     : qtm_chat_widget.hpp
 * DESCRIPTION: Main Chat Message List Widget for Mogan
 * COPYRIGHT  : (C) 2026 Mogan Team
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTM_CHAT_WIDGET_HPP
#define QTM_CHAT_WIDGET_HPP

#include "qt_simple_widget.hpp"
#include "qtm_chat_model.hpp"
#include <QScrollArea>
#include <QWidget>
#include <QVBoxLayout>
#include <QTimer>

class QTMMessageWidget;

/******************************************************************************
 * QTMChatWidget - Main Chat Message List Component
 * Inherits from qt_simple_widget_rep for Mogan integration
 ******************************************************************************/

class QTMChatWidget : public qt_simple_widget_rep, public ChatObserver {
public:
  QTMChatWidget();
  ~QTMChatWidget();

  // ========== qt_simple_widget_rep interface ==========
  virtual QWidget* as_qwidget();
  virtual void     send(slot s, blackbox val);
  virtual blackbox query(slot s, int type_id);

  // ========== ChatObserver interface (model callbacks) ==========
  virtual void onMessageAdded(const ChatMessage& msg);
  virtual void onMessageRemoved(int msg_id);
  virtual void onCleared();
  virtual void onHistoryLoaded(const array<ChatMessage>& messages);

private:
  ChatModel*                          model;
  QWidget*                            main_widget;
  QScrollArea*                        scroll_area;
  QWidget*                            messages_container;
  QVBoxLayout*                        messages_layout;
  QTimer*                             scroll_timer;
  array<QTMMessageWidget*>            message_widgets;
  string                              history_file_path;

  // ========== UI Helper Methods ==========
  void initializeUI();
  QTMMessageWidget* createMessageWidget(const ChatMessage& msg);
  void addMessageWidgetToLayout(QTMMessageWidget* widget);
  void removeMessageWidgetFromLayout(int msg_id);
  void scrollToBottom();
  void clearLayout();

  // ========== Slot Handlers ==========
  void handleAddMessage(const string& role, const string& content);
  void handleRemoveMessage(int msg_id);
  void handleClear();
  void handleLoadHistory(const string& filepath);
  void handleSaveHistory(const string& filepath);

  // ========== Timer callback (not using Qt slots) ==========
  void doScrollToBottom();
};

#endif // QTM_CHAT_WIDGET_HPP
