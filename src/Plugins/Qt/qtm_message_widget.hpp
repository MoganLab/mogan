/******************************************************************************
 * MODULE     : qtm_message_widget.hpp
 * DESCRIPTION: Single Chat Message Widget with Rich Text Support
 * COPYRIGHT  : (C) 2026 Mogan Team
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTM_MESSAGE_WIDGET_HPP
#define QTM_MESSAGE_WIDGET_HPP

#include "qtm_chat_model.hpp"
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

/******************************************************************************
 * QTMMessageWidget - Renders a Single Chat Message
 ******************************************************************************/

class QTMMessageWidget : public QWidget {
  Q_OBJECT

private:
  ChatMessage       message;
  QVBoxLayout*      layout;
  QLabel*           role_label;
  QTextEdit*        content_text;

public:
  QTMMessageWidget(const ChatMessage& msg, QWidget* parent= nullptr);
  ~QTMMessageWidget();

  // Update message content
  void setMessage(const ChatMessage& msg);

  // Get message ID
  int getMessageId() const { return message.id; }

private:
  // Initialize UI based on message role
  void setupUI(const ChatMessage& msg);

  // Convert content to HTML (support basic Markdown)
  QString contentToHtml(const string& content);

  // Style the message based on role (user vs assistant)
  void applyRoleStyle(const string& role);
};

#endif // QTM_MESSAGE_WIDGET_HPP
