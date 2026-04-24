/******************************************************************************
 * MODULE     : logindialog.cpp
 * COPYRIGHT  : (C) 2025 Liii
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "logindialog.hpp"
#include "logindialog_p.hpp"

#include <QtGui/QtEvents>

namespace QWK {

LoginDialogPrivate::LoginDialogPrivate () {
  layout       = nullptr;
  contentWidget= nullptr;
}

LoginDialogPrivate::~LoginDialogPrivate ()= default;

void
LoginDialogPrivate::init () {
  Q_Q (LoginDialog);
  layout= new QVBoxLayout ();
  layout->setContentsMargins (8, 8, 8, 8);
  layout->setSpacing (4);
  q->setLayout (layout);

  // 设置对话框属性 - 改为下拉菜单样式
  q->setWindowFlags (Qt::Popup | Qt::FramelessWindowHint |
                     Qt::NoDropShadowWindowHint);
  q->setAttribute (Qt::WA_TranslucentBackground);

  // 安装事件过滤器以处理外部点击关闭
  q->installEventFilter (q);

  // 创建默认内容区域
  contentWidget= new QWidget (q);
  contentWidget->setObjectName ("login-dialog-content");
  layout->addWidget (contentWidget);
}

LoginDialog::LoginDialog (QWidget* parent)
    : LoginDialog (*new LoginDialogPrivate (), parent) {}

LoginDialog::~LoginDialog ()= default;

void
LoginDialog::setContentWidget (QWidget* widget) {
  Q_D (LoginDialog);
  if (d->contentWidget) {
    d->layout->removeWidget (d->contentWidget);
    d->contentWidget->deleteLater ();
  }
  d->contentWidget= widget;
  if (widget) {
    d->layout->addWidget (widget);
  }
}

QWidget*
LoginDialog::contentWidget () const {
  Q_D (const LoginDialog);
  return d->contentWidget;
}

void
LoginDialog::showAtPosition (const QPoint& globalPos) {
  // 计算显示位置，使弹窗顶部中间对准给定的全局位置
  QScreen* screen= QGuiApplication::screenAt (globalPos);
  if (!screen) {
    screen= QGuiApplication::primaryScreen ();
  }

  QRect  screenGeometry= screen->availableGeometry ();
  QPoint pos (globalPos.x () - width () / 2, globalPos.y ());

  // 检查右侧边界
  if (pos.x () + width () > screenGeometry.right ()) {
    pos.setX (screenGeometry.right () - width ());
  }

  // 底部空间不足时，尝试显示在锚点上方
  if (pos.y () + height () > screenGeometry.bottom ()) {
    pos.setY (globalPos.y () - height ());
  }

  // 检查左侧边界
  if (pos.x () < screenGeometry.left ()) {
    pos.setX (screenGeometry.left ());
  }

  // 检查顶部边界
  if (pos.y () < screenGeometry.top ()) {
    pos.setY (screenGeometry.top ());
  }

  move (pos);
  show ();
  raise ();
  activateWindow ();
}

void
LoginDialog::showAtRect (const QRect& globalRect, int gap) {
  QScreen* screen= QGuiApplication::screenAt (globalRect.center ());
  if (!screen) {
    screen= QGuiApplication::primaryScreen ();
  }

  QRect     screenGeometry= screen->availableGeometry ();
  const int belowGap      = 0;
  QPoint    pos (globalRect.center ().x () - width () / 2,
                 globalRect.bottom () + 1 + belowGap);

  if (pos.x () + width () > screenGeometry.right ()) {
    pos.setX (screenGeometry.right () - width ());
  }

  if (pos.y () + height () > screenGeometry.bottom ()) {
    pos.setY (globalRect.top () - height () - gap);
  }

  if (pos.x () < screenGeometry.left ()) {
    pos.setX (screenGeometry.left ());
  }

  if (pos.y () < screenGeometry.top ()) {
    pos.setY (screenGeometry.top ());
  }

  move (pos);
  show ();
  raise ();
  activateWindow ();
}

void
LoginDialog::showEvent (QShowEvent* event) {
  // 不再自动居中，使用自定义位置
  QDialog::showEvent (event);
}

void
LoginDialog::closeEvent (QCloseEvent* event) {
  // 可以在这里添加关闭前的逻辑
  QDialog::closeEvent (event);
}

bool
LoginDialog::eventFilter (QObject* obj, QEvent* event) {
  Q_D (LoginDialog);
  if (event->type () == QEvent::MouseButtonPress) {
    QMouseEvent* mouseEvent= static_cast<QMouseEvent*> (event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint globalPos= mouseEvent->globalPosition ().toPoint ();
#else
    QPoint globalPos= mouseEvent->globalPos ();
#endif
    if (!geometry ().contains (globalPos)) {
      // 点击了弹窗外部，关闭弹窗
      hide ();
      return true;
    }
  }
  return QDialog::eventFilter (obj, event);
}

LoginDialog::LoginDialog (LoginDialogPrivate& d, QWidget* parent)
    : QDialog (parent), d_ptr (&d) {
  d.q_ptr= this;

  d.init ();
}

} // namespace QWK
