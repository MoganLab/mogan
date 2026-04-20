/*****************************************************************************/
/* MODULE     : updatenotificationbar.cpp                                    */
/* DESCRIPTION : Version update notification bar (horizontal layout)         */
/* COPYRIGHT   : (C) 2026 Mogan STEM authors                                 */
/*****************************************************************************/

#include "updatenotificationbar.hpp"
#include "qt_utilities.hpp"

#include <QApplication>

namespace QWK {

UpdateNotificationBar::UpdateNotificationBar (QWidget* parent)
    : QFrame (parent) {
  setupUI ();
}

UpdateNotificationBar::~UpdateNotificationBar ()= default;

void
UpdateNotificationBar::setupUI () {
  // 主布局（横条）
  m_layout= new QHBoxLayout (this);
  m_layout->setContentsMargins (12, 8, 12, 8);
  m_layout->setSpacing (0);

  // 左侧 stretch（将内容推向中间）
  m_layout->addStretch (1);

  // 图标 🎉
  m_iconLabel= new QLabel (this);
  m_iconLabel->setText ("🎉");
  m_iconLabel->setStyleSheet ("font-size: 16px; margin-right: 8px;");
  m_layout->addWidget (m_iconLabel);

  // 提示文字（居中对齐）
  m_messageLabel= new QLabel (this);
  m_messageLabel->setObjectName ("updateNotificationMessage");
  m_messageLabel->setAlignment (Qt::AlignCenter | Qt::AlignVCenter);
  m_messageLabel->setWordWrap (false);
  m_layout->addWidget (m_messageLabel);

  // 消息和按钮之间的间距
  m_layout->addSpacing (16);

  // 按钮容器
  QWidget*     btnWidget= new QWidget (this);
  QHBoxLayout* btnLayout= new QHBoxLayout (btnWidget);
  btnLayout->setContentsMargins (0, 0, 0, 0);
  btnLayout->setSpacing (8);

  // 立即更新按钮
  m_updateNowBtn= new QPushButton (qt_translate ("Update now"), btnWidget);
  m_updateNowBtn->setObjectName ("updateNotificationUpdateButton");
  m_updateNowBtn->setCursor (Qt::PointingHandCursor);
  connect (m_updateNowBtn, &QPushButton::clicked, this,
           &UpdateNotificationBar::updateNowRequested);
  btnLayout->addWidget (m_updateNowBtn);

  // 稍后提醒按钮
  m_snoozeBtn= new QPushButton (qt_translate ("Remind later"), btnWidget);
  m_snoozeBtn->setObjectName ("updateNotificationSnoozeButton");
  m_snoozeBtn->setCursor (Qt::PointingHandCursor);
  connect (m_snoozeBtn, &QPushButton::clicked, this,
           &UpdateNotificationBar::snoozeRequested);
  btnLayout->addWidget (m_snoozeBtn);

  m_layout->addWidget (btnWidget);

  // 右侧 stretch（与左侧对称，将内容推向中间）
  m_layout->addStretch (1);

  // 关闭按钮
  m_closeBtn= new QPushButton ("×", this);
  m_closeBtn->setObjectName ("updateNotificationCloseButton");
  m_closeBtn->setCursor (Qt::PointingHandCursor);
  m_closeBtn->setFixedSize (24, 24);
  m_closeBtn->setStyleSheet ("text-align: center; font-size: 18px;");
  connect (m_closeBtn, &QPushButton::clicked, this,
           &UpdateNotificationBar::closeRequested);
  m_layout->addWidget (m_closeBtn);

  // 对象名（用于样式表）
  setObjectName ("updateNotificationBar");
}

void
UpdateNotificationBar::setVersionInfo (const QString& currentVersion,
                                       const QString& remoteVersion) {
  QString msg= QString ("%1 v%2 (%3: v%4)")
                   .arg (qt_translate ("New version available"))
                   .arg (remoteVersion)
                   .arg (qt_translate ("current"))
                   .arg (currentVersion);
  m_messageLabel->setText (msg);
}

} // namespace QWK
