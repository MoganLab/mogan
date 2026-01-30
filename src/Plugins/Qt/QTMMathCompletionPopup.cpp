
/******************************************************************************
 * MODULE     : QTMMathCompletionPopup.cpp
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2025 JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMMathCompletionPopup.hpp"
#include "server.hpp"

#include <QPainter>
#include <QPen>
#include <QTimer>
#include <cmath>

QTMMathCompletionPopup::QTMMathCompletionPopup (QWidget*              parent,
                                                qt_simple_widget_rep* owner)
    : QWidget (parent), owner (owner), layout (nullptr) {
  setObjectName ("math_completion_popup");
  setWindowFlags (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute (Qt::WA_ShowWithoutActivating);
  setMouseTracking (true);
  setFocusPolicy (Qt::NoFocus);
  layout= new QVBoxLayout (this);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  setLayout (layout);
  // 阴影效果
  QGraphicsDropShadowEffect* effect= new QGraphicsDropShadowEffect (this);
  effect->setBlurRadius (40);
  effect->setOffset (0, 4);
  effect->setColor (QColor (0, 0, 0, 120));
  this->setGraphicsEffect (effect);
}

QTMMathCompletionPopup::~QTMMathCompletionPopup () {
  // layout 会被 Qt 自动销毁，无需手动 delete
}

void
QTMMathCompletionPopup::installEventFilterRecursively (QWidget* widget,
                                                       QObject* filterObj) {
  // 给组件和子组件递归装上事件过滤器
  if (!widget) return;

  // 安装事件过滤器到当前组件
  widget->installEventFilter (filterObj);

  // 递归安装事件过滤器到所有子组件
  const QObjectList& children= widget->children ();
  for (QObject* child : children) {
    QWidget* childWidget= qobject_cast<QWidget*> (child);
    if (childWidget) {
      installEventFilterRecursively (childWidget, filterObj);
    }
  }
}

void
QTMMathCompletionPopup::cleanLayout () {
  // 清空 Layout 中已有的内容
  QLayoutItem* item;
  while ((item= layout->takeAt (0)) != nullptr) {
    if (item->widget ()) {
      item->widget ()->setParent (nullptr);
    }
    delete item;
  }
  // 如果布局为空，隐藏窗口
  if (layout->count () == 0) {
    this->hide ();
  }
}

void
QTMMathCompletionPopup::setWidget (QWidget* w) {
  if (w) {
    cleanLayout ();
    // 暂停绘制，防止闪烁
    this->setUpdatesEnabled (false);

    w->setParent (this);
    layout->addWidget (w);
    installEventFilterRecursively (w, this);

    // 提前显示组件，防止闪烁
    w->show ();
    this->adjustSize ();

    // 恢复绘制
    this->setUpdatesEnabled (true);
    this->update ();
  }
}

void
QTMMathCompletionPopup::showMathCompletions (struct cursor cu, double magf,
                                             int scroll_x, int scroll_y,
                                             int canvas_x) {
  cachePosition (cu, magf, scroll_x, scroll_y, canvas_x);
  int x, y;
  getCachedPosition (x, y);
  QPoint topLeft (x, y);
  move (topLeft);
  raise ();
  show ();
  this->adjustSize ();
  QTimer::singleShot (0, this, [this] () { updatePosition (); });
}

void
QTMMathCompletionPopup::cachePosition (struct cursor cu, double magf,
                                       int scroll_x, int scroll_y,
                                       int canvas_x) {
  cached_cursor_x= cu->ox;
  cached_cursor_y= cu->oy;
  cached_scroll_x= scroll_x;
  cached_scroll_y= scroll_y;
  cached_canvas_x= canvas_x;
  cached_magf    = magf;
}

void
QTMMathCompletionPopup::getCachedPosition (int& x, int& y) {
  QTMWidget* canvas= owner ? owner->canvas () : nullptr;
  QPoint     cursor_pos;
  QPoint     origin;
  QPoint     surface_top_left;
  if (canvas && canvas->surface ()) {
    cursor_pos      = canvas->cursorPos ();
    origin          = canvas->origin ();
    surface_top_left= canvas->surface ()->geometry ().topLeft ();
  }
  x= cursor_pos.x () - origin.x () + surface_top_left.x ();
  y= cursor_pos.y () - origin.y () + surface_top_left.y ();
  y+= 10;
}

void
QTMMathCompletionPopup::updatePosition () {
  int pos_x, pos_y;
  getCachedPosition (pos_x, pos_y);
  move (pos_x, pos_y);
}

void
QTMMathCompletionPopup::scrollBy (int x, int y) {
  cached_scroll_x-= (int) (x / cached_magf);
  cached_scroll_y-= (int) (y / cached_magf);
}

void
QTMMathCompletionPopup::paintEvent (QPaintEvent* event) {
  // 保持原有绘制
  QPainter painter (this);
  painter.setRenderHint (QPainter::Antialiasing);
  // 绘制白色背景
  painter.setPen (Qt::NoPen);
  painter.setBrush (QColor (255, 255, 255, 255));
  QRectF bgRect= this->rect ();
  painter.drawRoundedRect (bgRect, 6, 6);
  // 绘制黑色边框
  QPen pen (Qt::black, 1.5);
  painter.setPen (pen);
  painter.setBrush (Qt::NoBrush);
  QRectF rect= this->rect ();
  rect.adjust (0.75, 0.75, -0.75, -0.75); // 居中描边
  painter.drawRoundedRect (rect, 6, 6);   // 圆角
}

bool
QTMMathCompletionPopup::eventFilter (QObject* obj, QEvent* event) {
  if (event->type () == QEvent::MouseButtonPress) {
    const char* className= obj->metaObject ()->className ();
    if (!strcmp (className, "QToolButton")) {
      // 如果点击的是
      // QToolButton，即是一个图标按钮，就提前进行一次删除，以达到替换的效果
      call ("kbd-backspace");
    }
    hide ();      // 当 Popup 窗口中任意组件被点击时，隐藏 Popup 窗口
    return false; // false 表示继续传播；true 表示拦截
  }
  return QWidget::eventFilter (obj, event);
}
