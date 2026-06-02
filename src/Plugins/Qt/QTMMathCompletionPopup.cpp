
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
#include "qt_dpi_utils.hpp"
#include "server.hpp"

#include <QFrame>
#include <QWindow>

static constexpr int    kContainerBorderRadius= 6;
static constexpr double kContainerBorderWidth = 1;
static constexpr int    kContentMargin        = 2;
#ifdef Q_OS_MAC
static constexpr int kPositionOffsetX= 15;
static constexpr int kPositionOffsetY= 15;
#else
static constexpr int kPositionOffsetX= 8;
static constexpr int kPositionOffsetY= 8;
#endif

QTMMathCompletionPopup::QTMMathCompletionPopup (QWidget*              parent,
                                                qt_simple_widget_rep* owner)
    : QWidget (parent), owner (owner), layout (nullptr) {
  setObjectName ("math_completion_popup");
  setWindowFlags (Qt::ToolTip | Qt::FramelessWindowHint |
                  Qt::WindowStaysOnTopHint);
  setAttribute (Qt::WA_ShowWithoutActivating);
  setAttribute (Qt::WA_DeleteOnClose, false);
  setAttribute (Qt::WA_TranslucentBackground);
  setMouseTracking (true);
  setFocusPolicy (Qt::NoFocus);

  installTopLevelWindowFilter ();

  QVBoxLayout* mainLayout= new QVBoxLayout (this);
  mainLayout->setSizeConstraint (QLayout::SetMinimumSize);
  setLayout (mainLayout);

  QFrame* container= new QFrame (this);
  container->setObjectName ("math_completion_container");
  container->setStyleSheet (
      QString ("QFrame#math_completion_container { "
               "border: %1px solid; "
               "border-radius: %2px; }")
          .arg (DpiUtils::scaledF (kContainerBorderWidth))
          .arg (DpiUtils::scaled (kContainerBorderRadius)));
  mainLayout->addWidget (container);

  int margin= DpiUtils::scaled (kContentMargin);
  layout    = new QVBoxLayout (container);
  layout->setContentsMargins (margin, margin, margin, margin);
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  container->setLayout (layout);
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
  QLayoutItem* item;
  while ((item= layout->takeAt (0)) != nullptr) {
    if (item->widget ()) {
      item->widget ()->setParent (nullptr);
    }
    delete item;
  }
}

void
QTMMathCompletionPopup::setWidget (QWidget* w) {
  if (w) {
    // 暂停绘制，防止 cleanLayout 和添加新 widget 之间闪烁
    this->setUpdatesEnabled (false);

    cleanLayout ();

    w->setParent (layout->parentWidget ());
    layout->addWidget (w);
    installEventFilterRecursively (w, this);
    w->show ();

    this->adjustSize ();

    // 恢复绘制
    this->setUpdatesEnabled (true);
    updatePosition ();
  }
}

void
QTMMathCompletionPopup::showMathCompletions (struct cursor cu, double magf,
                                             int scroll_x, int scroll_y,
                                             int canvas_x) {
  cachePosition (cu, magf, scroll_x, scroll_y, canvas_x);

  int x, y;
  getCachedPosition (x, y);

  if (!isVisible ()) {
    move (x, y);
    show ();
  }
  else {
    QPoint cur= pos ();
    if (cur.x () != x || cur.y () != y) {
      move (x, y);
    }
  }
  raise ();
  this->adjustSize ();
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
  if (canvas && canvas->surface ()) {
    QPoint cursor_pos      = canvas->cursorPos ();
    QPoint origin          = canvas->origin ();
    QPoint surface_top_left= canvas->surface ()->geometry ().topLeft ();
    QPoint local_pos (cursor_pos.x () - origin.x () + surface_top_left.x (),
                      cursor_pos.y () - origin.y () + surface_top_left.y ());
    QPoint global_pos= canvas->viewport ()->mapToGlobal (local_pos);
    x                = global_pos.x () - DpiUtils::scaled (kPositionOffsetX);
    y                = global_pos.y () - DpiUtils::scaled (kPositionOffsetY);
  }
  else {
    x= 0;
    y= 0;
  }
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
QTMMathCompletionPopup::installTopLevelWindowFilter () {
  // 监听所属顶层窗口的状态变化，当主窗口最小化/隐藏时自动隐藏 popup。
  // 由于 popup 是独立顶层窗口（Qt::ToolTip），不会跟随主窗口自动隐藏。
  QWidget* w= parentWidget ();
  while (w) {
    if (w->isWindow ()) {
      w->installEventFilter (this);
      break;
    }
    w= w->parentWidget ();
  }
}

bool
QTMMathCompletionPopup::eventFilter (QObject* obj, QEvent* event) {
  if (event->type () == QEvent::WindowStateChange) {
    // 主窗口最小化时隐藏 popup
    QWidget* tlw= qobject_cast<QWidget*> (obj);
    if (tlw && tlw->windowState () & Qt::WindowMinimized) {
      hide ();
    }
  }
  else if (event->type () == QEvent::Hide) {
    // 主窗口被隐藏时隐藏 popup
    hide ();
  }
  else if (event->type () == QEvent::MouseButtonPress) {
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
