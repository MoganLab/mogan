/*****************************************************************************/
/* MODULE     : notificationbar.cpp                                          */
/* DESCRIPTION: SCM-driven notification bar shell                            */
/*****************************************************************************/

#include "notificationbar.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QStyle>
#include <QWidget>

namespace QWK {

NotificationBar::NotificationBar (QWidget* parent)
    : QFrame (parent), m_layout (nullptr), m_contentHost (nullptr),
      m_contentLayout (nullptr), m_contentWidget (nullptr),
      m_snoozeButton (nullptr), m_closeButton (nullptr) {
  setupUI ();
}

NotificationBar::~NotificationBar ()= default;

void
NotificationBar::setupUI () {
  m_layout= new QHBoxLayout (this);
  m_layout->setContentsMargins (12, 8, 12, 8);
  m_layout->setSpacing (0);

  // Mirror the close button width on the left so the content group sits on
  // the true visual center of the bar.
  m_layout->addSpacing (24);
  m_layout->addStretch (1);

  m_contentHost= new QWidget (this);
  m_contentHost->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
  m_contentLayout= new QHBoxLayout (m_contentHost);
  m_contentLayout->setContentsMargins (0, 0, 0, 0);
  m_contentLayout->setSpacing (0);
  m_layout->addWidget (m_contentHost, 0, Qt::AlignCenter);
  m_layout->addStretch (1);

  m_snoozeButton= new QPushButton (this);
  m_snoozeButton->setObjectName ("notificationSnoozeButton");
  m_snoozeButton->setCursor (Qt::PointingHandCursor);
  m_snoozeButton->setFlat (true);
  m_snoozeButton->setFocusPolicy (Qt::NoFocus);
  m_snoozeButton->hide ();
  connect (m_snoozeButton, &QPushButton::clicked, this,
           &NotificationBar::snoozeRequested);
  m_layout->addWidget (m_snoozeButton, 0, Qt::AlignVCenter);
  m_layout->addSpacing (8);

  m_closeButton= new QPushButton ("×", this);
  m_closeButton->setObjectName ("notificationCloseButton");
  m_closeButton->setCursor (Qt::PointingHandCursor);
  m_closeButton->setFixedSize (24, 24);
  m_closeButton->setStyleSheet ("text-align: center; font-size: 18px;");
  connect (m_closeButton, &QPushButton::clicked, this,
           &NotificationBar::closeRequested);
  m_layout->addWidget (m_closeButton);

  setObjectName ("notificationBar");
}

void
NotificationBar::setContentWidget (QWidget* widget) {
  if (m_contentWidget) {
    m_contentLayout->removeWidget (m_contentWidget);
    m_contentWidget->deleteLater ();
    m_contentWidget= nullptr;
  }

  if (!widget) {
    hide ();
    return;
  }

  widget->setParent (m_contentHost);
  widget->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
  m_contentLayout->addWidget (widget, 0, Qt::AlignCenter);
  m_contentWidget= widget;
  updateActionStyles ();
  show ();
}

void
NotificationBar::setSnoozeText (const QString& text) {
  if (!m_snoozeButton) return;

  m_snoozeButton->setText (text);
  m_snoozeButton->setVisible (!text.isEmpty ());
}

QWidget*
NotificationBar::contentWidget () const {
  return m_contentWidget;
}

void
NotificationBar::clearContent () {
  setSnoozeText (QString ());
  setContentWidget (nullptr);
}

void
NotificationBar::updateActionStyles () {
  if (!m_contentWidget) return;

  const QList<QLabel*> labels= m_contentWidget->findChildren<QLabel*> ();
  for (QLabel* label : labels) {
    label->setObjectName ("notificationMessage");
    label->setAlignment (Qt::AlignCenter | Qt::AlignVCenter);
    // Menu-generated text widgets come with an inline black text color.
    // Clear it so the theme rule on #notificationBar QLabel can take effect.
    label->setStyleSheet ("");
    label->style ()->unpolish (label);
    label->style ()->polish (label);
  }

  const QList<QPushButton*> buttons=
      m_contentWidget->findChildren<QPushButton*> ();
  for (int i= 0; i < buttons.size (); ++i) {
    QPushButton* button= buttons.at (i);
    button->setProperty ("notificationRole",
                         (i == 0) ? "primary" : "secondary");
    button->style ()->unpolish (button);
    button->style ()->polish (button);
  }
}

} // namespace QWK
