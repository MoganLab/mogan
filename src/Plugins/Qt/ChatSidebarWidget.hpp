/******************************************************************************
 * MODULE     : ChatSidebarWidget.hpp
 * DESCRIPTION: Static chat sidebar widget skeleton
 * COPYRIGHT  : (C) 2025  Mogan Contributors
 ******************************************************************************/

#ifndef CHAT_SIDEBAR_WIDGET_H
#define CHAT_SIDEBAR_WIDGET_H

#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include "qt_widget.hpp"

class ChatSidebarWidget : public QDockWidget {
  Q_OBJECT

public:
  explicit ChatSidebarWidget (QWidget* parent= nullptr);
  ~ChatSidebarWidget ();
  bool hasEmbeddedBuffers () const;
  void setMessageWidget (qt_widget w);
  void setInputWidget (qt_widget w);

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;
  void hideEvent (QHideEvent* event) override;
  void closeEvent (QCloseEvent* event) override;
  void keyPressEvent (QKeyEvent* event) override;

private:
  void installInputEventFilter (QWidget* widget);
  void removeInputEventFilter (QWidget* widget);
  void setupUI ();
  void setupConnections ();
  bool shouldBlockCtrlWheel (QObject* watched) const;
  bool matchesEmbeddedWidget (QObject* watched, QWidget* root) const;

  QWidget*     m_container;
  QVBoxLayout* m_mainLayout;
  QSplitter*   m_splitter;

  // Title bar
  QWidget*     m_titleBar;
  QHBoxLayout* m_titleLayout;
  QLabel*      m_titleLabel;
  QPushButton* m_closeButton;

  QWidget*     m_messageContainer;
  QVBoxLayout* m_messageLayout;
  qt_widget    m_messageWidget;
  QWidget*     m_messageQWidget;

  QWidget*     m_permissionPanel;
  QVBoxLayout* m_permissionLayout;
  QLabel*      m_permissionTitleLabel;
  QLabel*      m_permissionQuestionLabel;
  QLabel*      m_permissionStatusLabel;
  QHBoxLayout* m_permissionButtonLayout;
  QPushButton* m_permissionYesButton;
  QPushButton* m_permissionNoButton;

  QWidget*     m_inputContainer;
  QVBoxLayout* m_inputLayout;
  qt_widget    m_inputWidget;
  QWidget*     m_inputQWidget;

  // Send button area
  QWidget*      m_sendPanel;
  QHBoxLayout*  m_sendLayout;
  QComboBox*    m_modelComboBox;
  QComboBox*    m_permissionComboBox;
  QRadioButton* m_thinkingRadioButton;
  QPushButton*  m_sendButton;
};

#endif // CHAT_SIDEBAR_WIDGET_H
