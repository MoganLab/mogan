/******************************************************************************
 * MODULE     : fast_search.hpp
 * DESCRIPTION: Fast multiple searches in same string
 * COPYRIGHT  : (C) 2026 Mogan Team
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "guestnotificationbar.hpp"
#include "qt_utilities.hpp"

#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QStyle>

namespace QWK {

GuestNotificationBar::GuestNotificationBar (QWidget* parent)
    : QFrame (parent), m_layout (nullptr), m_messageLabel (nullptr),
      m_loginButton (nullptr), m_closeButton (nullptr) {
  setupUI ();
  applyStyle ();
}

GuestNotificationBar::~GuestNotificationBar ()= default;

void
GuestNotificationBar::setupUI () {
  // 设置主布局
  m_layout= new QHBoxLayout (this);
  m_layout->setContentsMargins (12, 8, 12, 8);
  m_layout->setSpacing (0);

  // 添加左侧拉伸空间
  m_layout->addStretch ();

  // 创建内容容器，用于包裹图标、文字和登录按钮
  QWidget*     contentWidget= new QWidget (this);
  QHBoxLayout* contentLayout= new QHBoxLayout (contentWidget);
  contentLayout->setContentsMargins (0, 0, 0, 0);
  contentLayout->setSpacing (8);

  // 提示文字
  m_messageLabel= new QLabel (contentWidget);
  m_messageLabel->setText (
      qt_translate ("You are currently in guest mode, login to enable AI, "
                    "MathOCR,and other features"));
  m_messageLabel->setAlignment (Qt::AlignLeft | Qt::AlignVCenter);
  m_messageLabel->setWordWrap (false);
  contentLayout->addWidget (m_messageLabel);

  // 立即登录按钮
  m_loginButton= new QPushButton (qt_translate ("Login Now"), contentWidget);
  m_loginButton->setCursor (Qt::PointingHandCursor);
  m_loginButton->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  contentLayout->addWidget (m_loginButton);

  // 将内容容器添加到主布局
  m_layout->addWidget (contentWidget);

  // 添加右侧拉伸空间
  m_layout->addStretch ();

  // 关闭按钮 - 单独放在最右侧
  m_closeButton= new QPushButton (this);
  m_closeButton->setText ("×"); // 使用乘号字符
  m_closeButton->setCursor (Qt::PointingHandCursor);
  m_closeButton->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_closeButton->setFixedSize (24, 24);
  // 设置文本对齐方式为居中
  m_closeButton->setStyleSheet ("text-align: center;");
  m_layout->addWidget (m_closeButton);

  // 连接信号
  connect (m_loginButton, &QPushButton::clicked, this,
           &GuestNotificationBar::loginRequested);
  connect (m_closeButton, &QPushButton::clicked, this,
           &GuestNotificationBar::closeRequested);

  // 设置对象名用于样式表
  setObjectName ("guestNotificationBar");
  m_messageLabel->setObjectName ("guestNotificationMessage");
  m_loginButton->setObjectName ("guestNotificationLoginButton");
  m_closeButton->setObjectName ("guestNotificationCloseButton");
}

void
GuestNotificationBar::applyStyle () {
  // 样式已迁移到主题文件中 (liii.css 和 liii-night.css)
  // 不再需要内联样式表
}

void
GuestNotificationBar::setMessage (const QString& message) {
  m_messageLabel->setText (qt_translate (from_qstring (message)));
}

QString
GuestNotificationBar::message () const {
  return m_messageLabel->text ();
}

void
GuestNotificationBar::setLoginButtonVisible (bool visible) {
  m_loginButton->setVisible (visible);
}

bool
GuestNotificationBar::isLoginButtonVisible () const {
  return m_loginButton->isVisible ();
}

void
GuestNotificationBar::setCloseButtonVisible (bool visible) {
  m_closeButton->setVisible (visible);
}

bool
GuestNotificationBar::isCloseButtonVisible () const {
  return m_closeButton->isVisible ();
}

QPushButton*
GuestNotificationBar::loginButton () const {
  return m_loginButton;
}

QPushButton*
GuestNotificationBar::closeButton () const {
  return m_closeButton;
}

} // namespace QWK