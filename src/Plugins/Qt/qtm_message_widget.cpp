/******************************************************************************
 * MODULE     : qtm_message_widget.cpp
 * DESCRIPTION: Single Chat Message Widget Implementation
 * COPYRIGHT  : (C) 2026 Mogan Team
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qtm_message_widget.hpp"
#include <QHBoxLayout>
#include <QFont>
#include <QString>
#include <QDateTime>
#include <QTime>
#include <ctime>

QTMMessageWidget::QTMMessageWidget(const ChatMessage& msg, QWidget* parent)
    : QWidget(parent), message(msg), layout(nullptr), role_label(nullptr),
      content_text(nullptr) {
  setupUI(msg);
}

QTMMessageWidget::~QTMMessageWidget() {}

void
QTMMessageWidget::setupUI(const ChatMessage& msg) {
  // Create layouts
  layout= new QVBoxLayout(this);
  layout->setContentsMargins(10, 5, 10, 5);
  layout->setSpacing(5);

  // Role label with timestamp
  role_label= new QLabel(this);
  QFont font= role_label->font();
  font.setBold(true);
  role_label->setFont(font);

  // Content widget (read-only text edit for word wrap support)
  content_text= new QTextEdit(this);
  content_text->setReadOnly(true);
  content_text->setWordWrapMode(QTextOption::WordWrap);
  content_text->setMaximumHeight(100);

  // Set message data
  setMessage(msg);

  // Add to layout
  layout->addWidget(role_label, 0);
  layout->addWidget(content_text, 1);
  setLayout(layout);

  setMinimumHeight(60);
}

void
QTMMessageWidget::setMessage(const ChatMessage& msg) {
  message= msg;

  // Format role label
  QString role_text;
  if (msg.role == "user") {
    role_text= "You:";
  } else if (msg.role == "assistant") {
    role_text= "AI:";
  } else {
    role_text= QString::fromUtf8(as_charp(msg.role)) + ":";
  }

  // Format timestamp
  time_t    ts       = msg.timestamp;
  struct tm timeinfo = *localtime(&ts);
  char      buffer[80];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  role_text+= QString(" [%1]").arg(buffer);

  role_label->setText(role_text);

  // Set content
  content_text->setText(contentToHtml(msg.content));

  // Apply role-based styling
  applyRoleStyle(msg.role);
}

QString
QTMMessageWidget::contentToHtml(const string& content) {
  // Simple HTML escaping for now
  QString html= QString::fromUtf8(as_charp(content));
  html.replace("<", "&lt;");
  html.replace(">", "&gt;");
  html.replace("\n", "<br>");
  return html;
}

void
QTMMessageWidget::applyRoleStyle(const string& role) {
  if (role == "user") {
    // User message: light blue background
    setStyleSheet(
        "QWidget { background-color: #E3F2FD; border-radius: 5px; }"
        "QLabel { color: #1565C0; font-weight: bold; }"
        "QTextEdit { background-color: #E3F2FD; border: none; color: #000000; }");
  } else if (role == "assistant") {
    // Assistant message: light gray background
    setStyleSheet(
        "QWidget { background-color: #F5F5F5; border-radius: 5px; }"
        "QLabel { color: #616161; font-weight: bold; }"
        "QTextEdit { background-color: #F5F5F5; border: none; color: #000000; }");
  } else {
    // Other roles: neutral
    setStyleSheet(
        "QWidget { background-color: #FAFAFA; border-radius: 5px; }"
        "QLabel { color: #424242; font-weight: bold; }"
        "QTextEdit { background-color: #FAFAFA; border: none; color: #000000; }");
  }
}
