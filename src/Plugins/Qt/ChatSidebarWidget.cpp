/******************************************************************************
 * MODULE     : ChatSidebarWidget.cpp
 * DESCRIPTION: Static chat sidebar widget skeleton
 * COPYRIGHT  : (C) 2025  Mogan Contributors
 ******************************************************************************/

#include "ChatSidebarWidget.hpp"
#include "command.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_gui.hpp"
#include "qt_widget.hpp"
#include "scheme.hpp"

#include <QCloseEvent>
#include <QEvent>
#include <QFont>
#include <QKeyEvent>
#include <QWheelEvent>

ChatSidebarWidget::ChatSidebarWidget (QWidget* parent)
    : QDockWidget ("Chat", parent), m_messageWidget (qt_widget ()),
      m_messageQWidget (nullptr), m_inputWidget (qt_widget ()),
      m_inputQWidget (nullptr) {
  setObjectName ("chat_sidebar");
  setAllowedAreas (Qt::RightDockWidgetArea);
  setFeatures (QDockWidget::DockWidgetMovable |
               QDockWidget::DockWidgetFloatable);
  setFloating (false);

  // Disable default title bar, use custom one
  setTitleBarWidget (new QWidget ());

  setupUI ();
  setupConnections ();
}

ChatSidebarWidget::~ChatSidebarWidget () {}

bool
ChatSidebarWidget::hasEmbeddedBuffers () const {
  return m_messageWidget.rep != nullptr && m_inputWidget.rep != nullptr;
}

void
ChatSidebarWidget::setupUI () {
  m_container= new QWidget (this);
  m_container->setObjectName ("chat-sidebar-container");
  m_mainLayout= new QVBoxLayout (m_container);
  m_mainLayout->setSpacing (0);
  m_mainLayout->setContentsMargins (0, 0, 0, 0);

  // Custom title bar
  m_titleBar= new QWidget (m_container);
  m_titleBar->setObjectName ("chat-sidebar-titlebar");
  m_titleLayout= new QHBoxLayout (m_titleBar);
  m_titleLayout->setSpacing (DpiUtils::scaled (4));
  m_titleLayout->setContentsMargins (DpiUtils::scaled (8), DpiUtils::scaled (4),
                                     DpiUtils::scaled (8),
                                     DpiUtils::scaled (4));

  m_titleLabel   = new QLabel ("Chat", m_titleBar);
  QFont titleFont= m_titleLabel->font ();
  titleFont.setBold (true);
  titleFont.setPixelSize (DpiUtils::scaled (14));
  m_titleLabel->setFont (titleFont);

  m_refreshButton= new QPushButton ("Refresh", m_titleBar);
  m_refreshButton->setFixedHeight (DpiUtils::scaled (24));

  m_titleLayout->addWidget (m_titleLabel);
  m_titleLayout->addStretch ();
  m_titleLayout->addWidget (m_refreshButton);
  m_mainLayout->addWidget (m_titleBar);

  // Splitter for message/input areas
  m_splitter= new QSplitter (Qt::Vertical, m_container);
  m_splitter->setHandleWidth (DpiUtils::scaled (2));

  // Message area placeholder
  m_messageContainer= new QWidget (m_splitter);
  m_messageContainer->setObjectName ("chat-sidebar-message-container");
  m_messageLayout= new QVBoxLayout (m_messageContainer);
  m_messageLayout->setSpacing (0);
  m_messageLayout->setContentsMargins (
      DpiUtils::scaled (12), DpiUtils::scaled (12), DpiUtils::scaled (12),
      DpiUtils::scaled (12));
  m_messageLayout->addStretch ();

  // Input area placeholder
  m_inputContainer= new QWidget (m_splitter);
  m_inputContainer->setObjectName ("chat-sidebar-input-container");
  m_inputLayout= new QVBoxLayout (m_inputContainer);
  m_inputLayout->setSpacing (0);
  m_inputLayout->setContentsMargins (
      DpiUtils::scaled (12), DpiUtils::scaled (12), DpiUtils::scaled (12),
      DpiUtils::scaled (12));
  m_inputLayout->addStretch ();

  m_splitter->addWidget (m_messageContainer);
  m_splitter->addWidget (m_inputContainer);

  // Set stretch factors: message area = 4, input area = 1
  m_splitter->setStretchFactor (0, 4);
  m_splitter->setStretchFactor (1, 1);

  m_mainLayout->addWidget (m_splitter, 1);

  // Send button panel
  m_sendPanel= new QWidget (m_container);
  m_sendPanel->setObjectName ("chat-sidebar-send-panel");
  m_sendLayout= new QHBoxLayout (m_sendPanel);
  m_sendLayout->setSpacing (DpiUtils::scaled (4));
  m_sendLayout->setContentsMargins (DpiUtils::scaled (4), DpiUtils::scaled (4),
                                    DpiUtils::scaled (4), DpiUtils::scaled (4));

  m_sendButton= new QPushButton ("Send", m_sendPanel);
  m_sendButton->setFixedHeight (DpiUtils::scaled (28));
  m_sendButton->setEnabled (false);

  m_sendLayout->addStretch ();
  m_sendLayout->addWidget (m_sendButton);
  m_mainLayout->addWidget (m_sendPanel);

  setWidget (m_container);
  m_container->setStyleSheet (
      "QWidget#chat-sidebar-container { background-color: #f1f1f1; }"
      "QWidget#chat-sidebar-titlebar { background-color: #e8e8e8; }"
      "QWidget#chat-sidebar-message-container { background-color: #f1f1f1; }"
      "QWidget#chat-sidebar-input-container { background-color: #f1f1f1; }"
      "QWidget#chat-sidebar-send-panel { background-color: #e8e8e8; }");

  // Default width
  setMinimumWidth (DpiUtils::scaled (560));
}

void
ChatSidebarWidget::setupConnections () {
  connect (m_refreshButton, &QPushButton::clicked, this, [this] {
    this->raise ();
    exec_delayed (scheme_cmd (
        "(when (defined? 'chat-sidebar-refresh!) (chat-sidebar-refresh!))"));
  });
  m_sendButton->setEnabled (true);
  connect (m_sendButton, &QPushButton::clicked, this, [] {
    exec_delayed (scheme_cmd (
        "(when (defined? 'chat-sidebar-send) (chat-sidebar-send))"));
  });
}

void
ChatSidebarWidget::setMessageWidget (qt_widget w) {
  if (m_messageQWidget != nullptr) removeInputEventFilter (m_messageQWidget);

  while (QLayoutItem* child= m_messageLayout->takeAt (0)) {
    if (child->widget ()) child->widget ()->deleteLater ();
    delete child;
  }

  m_messageWidget = w;
  m_messageQWidget= nullptr;
  if (m_messageWidget.rep != nullptr) {
    QWidget* qwidget= m_messageWidget->as_qwidget ();
    if (qwidget) {
      qwidget->setParent (m_messageContainer);
      m_messageLayout->addWidget (qwidget);
      m_messageQWidget= qwidget;
      installInputEventFilter (qwidget);
      qwidget->show ();
      return;
    }
  }

  m_messageLayout->addStretch ();
}

void
ChatSidebarWidget::setInputWidget (qt_widget w) {
  if (m_inputQWidget != nullptr) removeInputEventFilter (m_inputQWidget);

  while (QLayoutItem* child= m_inputLayout->takeAt (0)) {
    if (child->widget ()) child->widget ()->deleteLater ();
    delete child;
  }

  m_inputWidget = w;
  m_inputQWidget= nullptr;
  if (m_inputWidget.rep != nullptr) {
    QWidget* qwidget= m_inputWidget->as_qwidget ();
    if (qwidget) {
      qwidget->setParent (m_inputContainer);
      m_inputLayout->addWidget (qwidget);
      m_inputQWidget= qwidget;
      installInputEventFilter (qwidget);
      qwidget->show ();
      return;
    }
  }

  m_inputLayout->addStretch ();
}

bool
ChatSidebarWidget::eventFilter (QObject* watched, QEvent* event) {
  if (event->type () == QEvent::Wheel && shouldBlockCtrlWheel (watched)) {
    QWheelEvent* wheelEvent= static_cast<QWheelEvent*> (event);
    if ((wheelEvent->modifiers () & Qt::ControlModifier) != 0) {
      wheelEvent->accept ();
      return true;
    }
  }
  return QDockWidget::eventFilter (watched, event);
}

bool
ChatSidebarWidget::shouldBlockCtrlWheel (QObject* watched) const {
  return matchesEmbeddedWidget (watched, m_messageQWidget) ||
         matchesEmbeddedWidget (watched, m_inputQWidget);
}

bool
ChatSidebarWidget::matchesEmbeddedWidget (QObject* watched,
                                          QWidget* root) const {
  QWidget* watchedWidget= qobject_cast<QWidget*> (watched);
  return watchedWidget != nullptr && root != nullptr &&
         (watchedWidget == root || root->isAncestorOf (watchedWidget));
}

void
ChatSidebarWidget::installInputEventFilter (QWidget* widget) {
  widget->installEventFilter (this);
  const auto children= widget->findChildren<QWidget*> ();
  for (QWidget* child : children)
    child->installEventFilter (this);
}

void
ChatSidebarWidget::removeInputEventFilter (QWidget* widget) {
  widget->removeEventFilter (this);
  const auto children= widget->findChildren<QWidget*> ();
  for (QWidget* child : children)
    child->removeEventFilter (this);
}

void
ChatSidebarWidget::closeEvent (QCloseEvent* event) {
  exec_delayed (scheme_cmd (
      "(when (defined? 'close-chat-sidebar) (close-chat-sidebar))"));
  event->accept ();
}

void
ChatSidebarWidget::keyPressEvent (QKeyEvent* event) {
  if (event->key () == Qt::Key_Escape) {
    close ();
  }
  else {
    QDockWidget::keyPressEvent (event);
  }
}
