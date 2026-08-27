/******************************************************************************
 * MODULE     : loginbutton.cpp
 * COPYRIGHT  : (C) 2025 Liii
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "loginbutton.hpp"
#include "loginbutton_p.hpp"

#include <QtGui/QPainter>
#include <QtGui/QtEvents>

#include "qt_dpi_utils.hpp"

namespace QWK {

// macOS 特定按钮样式
static const char* macOSButtonStyle= R"(
QPushButton {
  background: transparent;
  border: none;
  min-width: 46px;
  min-height: 32px;
}
QPushButton:hover {
  background: rgba(0,0,0,0.1);
  border-radius: 4px;
}
QPushButton:pressed {
  background: rgba(0,0,0,0.2);
  border-radius: 4px;
}
)";

LoginButtonPrivate::LoginButtonPrivate () {
  hovered     = false;
  pressed     = false;
  badgeVisible= false;
}

LoginButtonPrivate::~LoginButtonPrivate ()= default;

void
LoginButtonPrivate::init () {
  Q_Q (LoginButton);
  q->setFocusPolicy (Qt::NoFocus);

#ifdef Q_OS_MAC
  // macOS特定调整：确保按钮在macOS上正确显示
  q->setStyleSheet (macOSButtonStyle);
#endif
}

void
LoginButtonPrivate::reloadIcon () {
  Q_Q (LoginButton);

  if (!q->isEnabled () && !iconDisabled.isNull ()) {
    q->setIcon (iconDisabled);
    return;
  }

  if (pressed && !iconPressed.isNull ()) {
    q->setIcon (iconPressed);
    return;
  }

  if (hovered && !iconHover.isNull ()) {
    q->setIcon (iconHover);
    return;
  }

  if (!iconNormal.isNull ()) {
    q->setIcon (iconNormal);
  }
}

LoginButton::LoginButton (QWidget* parent)
    : LoginButton (*new LoginButtonPrivate (), parent) {}

LoginButton::~LoginButton ()= default;

QIcon
LoginButton::iconNormal () const {
  Q_D (const LoginButton);
  return d->iconNormal;
}

void
LoginButton::setIconNormal (const QIcon& icon) {
  Q_D (LoginButton);
  d->iconNormal= icon;
  d->reloadIcon ();
}

QIcon
LoginButton::iconHover () const {
  Q_D (const LoginButton);
  return d->iconHover;
}

void
LoginButton::setIconHover (const QIcon& icon) {
  Q_D (LoginButton);
  d->iconHover= icon;
  d->reloadIcon ();
}

QIcon
LoginButton::iconPressed () const {
  Q_D (const LoginButton);
  return d->iconPressed;
}

void
LoginButton::setIconPressed (const QIcon& icon) {
  Q_D (LoginButton);
  d->iconPressed= icon;
  d->reloadIcon ();
}

QIcon
LoginButton::iconDisabled () const {
  Q_D (const LoginButton);
  return d->iconDisabled;
}

void
LoginButton::setIconDisabled (const QIcon& icon) {
  Q_D (LoginButton);
  d->iconDisabled= icon;
  d->reloadIcon ();
}

void
LoginButton::enterEvent (QEnterEvent* event) {
  Q_D (LoginButton);
  d->hovered= true;
  d->reloadIcon ();
  QPushButton::enterEvent (event);
}

void
LoginButton::leaveEvent (QEvent* event) {
  Q_D (LoginButton);
  d->hovered= false;
  d->reloadIcon ();
  QPushButton::leaveEvent (event);
}

void
LoginButton::mousePressEvent (QMouseEvent* event) {
  Q_D (LoginButton);
  if (event->button () == Qt::LeftButton) {
    d->pressed= true;
    d->reloadIcon ();
  }
  QPushButton::mousePressEvent (event);
}

void
LoginButton::mouseReleaseEvent (QMouseEvent* event) {
  Q_D (LoginButton);
  if (event->button () == Qt::LeftButton) {
    d->pressed= false;
    d->reloadIcon ();
  }
  QPushButton::mouseReleaseEvent (event);
}

bool
LoginButton::badgeVisible () const {
  Q_D (const LoginButton);
  return d->badgeVisible;
}

void
LoginButton::setBadgeVisible (bool visible) {
  Q_D (LoginButton);
  if (d->badgeVisible != visible) {
    d->badgeVisible= visible;
    update (); // 触发重绘
  }
}

void
LoginButton::paintEvent (QPaintEvent* event) {
  QPushButton::paintEvent (event); // 先绘制按钮本身

  Q_D (LoginButton);
  if (!d->badgeVisible) return;

  QPainter painter (this);
  painter.setRenderHint (QPainter::Antialiasing);

  // 使用 DpiUtils 进行 DPI 缩放
  int badgeSize  = DpiUtils::scaled (6);
  int borderWidth= DpiUtils::scaled (1);
  int marginRight= DpiUtils::scaled (10);
  int marginTop  = DpiUtils::scaled (8);

  // 红点位置：右上角，更靠近图标中心
  int x= width () - badgeSize - marginRight;
  int y= marginTop;

  // 绘制白色边框
  painter.setBrush (Qt::white);
  painter.setPen (Qt::NoPen);
  painter.drawEllipse (x - borderWidth, y - borderWidth,
                       badgeSize + 2 * borderWidth,
                       badgeSize + 2 * borderWidth);

  // 绘制红色圆点
  painter.setBrush (QColor ("#FF4D4F"));
  painter.drawEllipse (x, y, badgeSize, badgeSize);
}

LoginButton::LoginButton (LoginButtonPrivate& d, QWidget* parent)
    : QPushButton (parent), d_ptr (&d) {
  d.q_ptr= this;

  d.init ();
}

} // namespace QWK
