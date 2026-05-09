/******************************************************************************
 * MODULE     : ChatSidebarWidget.hpp
 * DESCRIPTION: Static chat sidebar widget skeleton
 * COPYRIGHT  : (C) 2025  Mogan Contributors
 ******************************************************************************/

#ifndef CHAT_SIDEBAR_WIDGET_H
#define CHAT_SIDEBAR_WIDGET_H

#include <QDockWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>

class ChatSidebarWidget : public QDockWidget {
  Q_OBJECT

public:
  explicit ChatSidebarWidget(QWidget* parent = nullptr);
  ~ChatSidebarWidget();

protected:
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private:
  void setupUI();
  void setupConnections();

  QWidget*     m_container;
  QVBoxLayout* m_mainLayout;
  QSplitter*   m_splitter;

  // Title bar
  QWidget*     m_titleBar;
  QHBoxLayout* m_titleLayout;
  QLabel*      m_titleLabel;
  QPushButton* m_refreshButton;

  QWidget*     m_messageContainer;
  QVBoxLayout* m_messageLayout;
  QLabel*      m_messagePlaceholder;

  QWidget*     m_inputContainer;
  QVBoxLayout* m_inputLayout;
  QLabel*      m_inputPlaceholder;

  // Send button area
  QWidget*     m_sendPanel;
  QHBoxLayout* m_sendLayout;
  QPushButton* m_sendButton;
};

#endif // CHAT_SIDEBAR_WIDGET_H
