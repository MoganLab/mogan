
/******************************************************************************
 * MODULE     : QTMScrollView.cpp
 * DESCRIPTION: QT Texmacs abstract scroll view widget
 * COPYRIGHT  : (C) 2009 Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "message.hpp"

#include "QTMScrollView.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>

/*! Provide automatic centering of the working area inside the viewport.

 The only purpose of this widget is to provide this centering. To support this
 we "un-wired" the event redirection built-in in QAbstractScrollArea (from the
 viewport widget to the QAbstractScrollArea) and re-wired event redirection
 from the surface to the QTMScrollView (see event())

 All relevant events like resize, I/O events and the like which are sent to the
 surface are resent QTMScrollView::surfaceEvent() for handling. This allows to
 concentrate all the logic in only one object.
 */
class QTMSurface : public QWidget {

  QTMScrollView* sv;

public:
  QTMSurface (QWidget* p, QTMScrollView* _sv) : QWidget (p), sv (_sv) {}

protected:
  virtual bool event (QEvent* event) {
    return sv->surfaceEvent (event) ? true : QWidget::event (event);
  }
};

/*! Constructor.

 NOTE:
 We tell the p_surface to use all available space by default (this is needed by
 embedded widgets) by setting the SizePolicy to (Expanding, Expanding).
 In order to draw the vertical margins around the working area use a horizontal
 policy of Fixedl, as in qt_tm_widget_rep (see SLOT_SCROLLABLE there)

 NOTE:
 Don't try to disable double buffering even if we do our own: the flag
 Qt::WA_PaintOnScreen is only supported on X11 and anyway makes things slower
 */
QTMScrollView::QTMScrollView (QWidget* _parent)
    : QAbstractScrollArea (_parent), editor_flag (false),
      laying_out_overlay_ (false), p_extents (QRect (0, 0, 0, 0)) {
  QWidget* _viewport= QAbstractScrollArea::viewport ();
  _viewport->setBackgroundRole (QPalette::Mid);
  _viewport->setAutoFillBackground (true);
  setFrameShape (QFrame::NoFrame);

  p_surface= new QTMSurface (_viewport, this);
  p_surface->setAttribute (Qt::WA_NoSystemBackground);
  p_surface->setAttribute (Qt::WA_StaticContents);
#if QT_VERSION < 0x060000
  p_surface->setAttribute (Qt::WA_MacNoClickThrough);
#endif
  p_surface->setAutoFillBackground (false);
  p_surface->setBackgroundRole (QPalette::NoRole);
  p_surface->setAttribute (Qt::WA_OpaquePaintEvent);
  p_surface->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

  QHBoxLayout* layout= new QHBoxLayout ();
  layout->addWidget (p_surface, 0, Qt::AlignHCenter | Qt::AlignVCenter);
  layout->setContentsMargins (0, 0, 0, 0);
  _viewport->setLayout (layout);

  // 滚动条悬浮在 viewport 之上，确保容器背景透明，不遮挡底层画面
  auto setupOverlayBar= [this] (QScrollBar* bar) {
    if (!bar) return;
    bar->setAttribute (Qt::WA_NoSystemBackground, true);
    bar->setAutoFillBackground (false);
    bar->installEventFilter (this);
    if (QWidget* container= bar->parentWidget ()) {
      if (container != this) {
        container->setAttribute (Qt::WA_NoSystemBackground, true);
        container->setAutoFillBackground (false);
        container->setStyleSheet ("background: transparent; border: none;");
        container->installEventFilter (this);
      }
    }
  };
  setupOverlayBar (QAbstractScrollArea::horizontalScrollBar ());
  setupOverlayBar (QAbstractScrollArea::verticalScrollBar ());
}

void
QTMScrollView::setOrigin (QPoint newOrigin) {
  if (!isVisible ()) return;
  if (newOrigin.x () != p_origin.x ())
    QAbstractScrollArea::horizontalScrollBar ()->setSliderPosition (
        newOrigin.x ());
  if (newOrigin.y () != p_origin.y ())
    QAbstractScrollArea::verticalScrollBar ()->setSliderPosition (
        newOrigin.y ());
}

void
QTMScrollView::setExtents (QRect newExtents) {
  if (newExtents.width () < 0) newExtents.setWidth (0);
  if (newExtents.height () < 0) newExtents.setHeight (0);
  if (p_extents != newExtents) {
    p_extents= newExtents;
    updateScrollBars ();
  }
}

/*! Scrolls contents so that the given point is visible. */
void
QTMScrollView::ensureVisible (int cx, int cy, int mx, int my) {
  QWidget* _viewport= QAbstractScrollArea::viewport ();
  int      w        = _viewport->width ();
  int      h        = _viewport->height ();

  int dx= -p_origin.x ();
  int dy= -p_origin.y ();
  int cw= p_extents.width ();
  int ch= p_extents.height ();

  if (w < mx * 2) mx= w / 2;
  if (h < my * 2) my= h / 2;

  if (cw <= w) {
    mx= 0;
    dx= 0;
  }

  if (ch <= h) {
    my= 0;
    dy= 0;
  }

  if (cx < mx - dx) dx= mx - cx;
  else if (cx >= w - mx - dx) dx= w - mx - cx;

  if (cy < my - dy) dy= my - cy;
  else if (cy >= h - my - dy) dy= h - my - cy;

  if (dx > 0) dx= 0;
  else if (dx < w - cw && cw > w) dx= w - cw;

  if (dy > 0) dy= 0;
  else if (dy < h - ch && ch > h) dy= h - ch;

  setOrigin (QPoint (-dx, -dy));
}

/*! Scrollbar stabilization */
void
QTMScrollView::updateScrollBars (void) {
  QScrollBar* _hScrollBar= QAbstractScrollArea::horizontalScrollBar ();
  QScrollBar* _vScrollBar= QAbstractScrollArea::verticalScrollBar ();

  int xw= p_extents.width ();
  int xh= p_extents.height ();
  // 悬浮滚动条不占用 viewport 空间，直接使用内容区完整宽高计算可滚动范围
  QRect cr= contentsRect ();
  int   w = cr.width ();
  int   h = cr.height ();
  if (!editor_flag) {
    if (xw < w) xw= w;
    if (xh < h) xh= h;
  }

  int cw= (xw > w ? xw - w : 0);
  if (_hScrollBar->sliderPosition () > cw) _hScrollBar->setSliderPosition (cw);
  _hScrollBar->setRange (0, cw);
  _hScrollBar->setSingleStep ((w >> 4) + 1);
  _hScrollBar->setPageStep (w);

  int ch= (xh > h ? xh - h : 0);
  if (_vScrollBar->sliderPosition () > ch) _vScrollBar->setSliderPosition (ch);
  _vScrollBar->setRange (0, ch);
  _vScrollBar->setSingleStep ((h >> 4) + 1);
  _vScrollBar->setPageStep (h);

  surface ()->setMinimumWidth (w < xw ? w : xw);
  surface ()->setMinimumHeight (h < xh ? h : xh);

  // we may need a relayout if the surface width is changed
  updateGeometry ();
  layoutOverlayScrollBars ();
}

/*!
 * @brief 将水平和垂直滚动条以悬浮透明层方式定位在 viewport 边缘之上。
 */
void
QTMScrollView::layoutOverlayScrollBars () {
  if (laying_out_overlay_) return;
  laying_out_overlay_= true;

  QRect    cr       = contentsRect ();
  QWidget* _viewport= QAbstractScrollArea::viewport ();
  if (_viewport && _viewport->geometry () != cr) {
    _viewport->setGeometry (cr);
  }

  QScrollBar* hbar= QAbstractScrollArea::horizontalScrollBar ();
  QScrollBar* vbar= QAbstractScrollArea::verticalScrollBar ();
  bool hVis= hbar && horizontalScrollBarPolicy () != Qt::ScrollBarAlwaysOff &&
             (hbar->maximum () > hbar->minimum () ||
              horizontalScrollBarPolicy () == Qt::ScrollBarAlwaysOn);
  bool vVis= vbar && verticalScrollBarPolicy () != Qt::ScrollBarAlwaysOff &&
             (vbar->maximum () > vbar->minimum () ||
              verticalScrollBarPolicy () == Qt::ScrollBarAlwaysOn);

  int hH= hbar ? hbar->sizeHint ().height () : 0;
  if (hH <= 0) hH= qApp->style ()->pixelMetric (QStyle::PM_ScrollBarExtent);
  int vW= vbar ? vbar->sizeHint ().width () : 0;
  if (vW <= 0) vW= qApp->style ()->pixelMetric (QStyle::PM_ScrollBarExtent);

  if (hVis && hbar) {
    QWidget* target= (hbar->parentWidget () && hbar->parentWidget () != this)
                         ? hbar->parentWidget ()
                         : static_cast<QWidget*> (hbar);
    int      w     = qMax (0, cr.width () - (vVis ? vW : 0));
    QRect    r (cr.x (), cr.bottom () - hH + 1, w, hH);
    if (target->geometry () != r) target->setGeometry (r);
    target->raise ();
  }

  if (vVis && vbar) {
    QWidget* target= (vbar->parentWidget () && vbar->parentWidget () != this)
                         ? vbar->parentWidget ()
                         : static_cast<QWidget*> (vbar);
    int      h     = qMax (0, cr.height () - (hVis ? hH : 0));
    QRect    r (cr.right () - vW + 1, cr.y (), vW, h);
    if (target->geometry () != r) target->setGeometry (r);
    target->raise ();
  }

  laying_out_overlay_= false;
}

/*! Scroll area updater */
void
QTMScrollView::scrollContentsBy (int dx, int dy) {
  if (dx) p_origin.setX (p_origin.x () - dx);
  if (dy) p_origin.setY (p_origin.y () - dy);
}

bool
QTMScrollView::viewportEvent (QEvent* e) {
  switch (e->type ()) {
  case QEvent::Resize: {
    QWidget* _viewport= QAbstractScrollArea::viewport ();
    if (!laying_out_overlay_ && _viewport &&
        _viewport->geometry () != contentsRect ()) {
      layoutOverlayScrollBars ();
    }
    updateScrollBars ();
    return false;
  }
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseButtonDblClick:
#if QT_VERSION >= 0x040600
  case QEvent::TouchBegin:
  case QEvent::TouchUpdate:
  case QEvent::TouchEnd:
#endif
  case QEvent::MouseMove:
  case QEvent::ContextMenu:
  case QEvent::Drop:
  case QEvent::DragEnter:
  case QEvent::DragMove:
  case QEvent::DragLeave:
    //      return QFrame::event(e);
    return false; // let the viewport widget handle the event
  case QEvent::Wheel:
    return QFrame::event (e);
  case QEvent::LayoutRequest:
#if QT_VERSION >= 0x040600
#ifndef QT_NO_GESTURES
  case QEvent::Gesture:
  case QEvent::GestureOverride:
    return event (e);
#endif
#endif
  default:
    break;
  }
  return false; // let the viewport widget handle the event
}

bool
QTMScrollView::surfaceEvent (QEvent* e) {
  switch (e->type ()) {
  case QEvent::Resize:
  case QEvent::Paint:
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseButtonDblClick:
#if QT_VERSION >= 0x040600
  case QEvent::TouchBegin:
  case QEvent::TouchUpdate:
  case QEvent::TouchEnd:
#endif
  case QEvent::MouseMove:
  case QEvent::ContextMenu:
  case QEvent::Wheel:
  case QEvent::Drop:
  case QEvent::DragEnter:
  case QEvent::DragMove:
  case QEvent::DragLeave:
    return QFrame::event (e);
  case QEvent::LayoutRequest:
#if QT_VERSION >= 0x040600
#ifndef QT_NO_GESTURES
  case QEvent::Gesture:
  case QEvent::GestureOverride:
    return event (e);
#endif
#endif
  default:
    break;
  }
  return false; // let the surface widget handle the event
}

bool
QTMScrollView::event (QEvent* event) {
  switch (event->type ()) {
  case QEvent::Resize: {
    bool          res= QAbstractScrollArea::event (event);
    QResizeEvent* re = static_cast<QResizeEvent*> (event);
    layoutOverlayScrollBars ();
    updateScrollBars ();
    resizeEventBis (re);
    return res;
  }
  case QEvent::LayoutRequest:
  case QEvent::PolishRequest:
  case QEvent::StyleChange:
  case QEvent::Show: {
    bool res= QAbstractScrollArea::event (event);
    layoutOverlayScrollBars ();
    return res;
  }
  default:
    break;
  }
  bool res= QAbstractScrollArea::event (event);
  if (!laying_out_overlay_ && viewport () &&
      viewport ()->geometry () != contentsRect ()) {
    layoutOverlayScrollBars ();
  }
  return res;
}

bool
QTMScrollView::eventFilter (QObject* obj, QEvent* e) {
  switch (e->type ()) {
  case QEvent::Show:
  case QEvent::Hide:
  case QEvent::Move:
  case QEvent::Resize:
    if (!laying_out_overlay_) {
      layoutOverlayScrollBars ();
    }
    break;
  default:
    break;
  }
  return QAbstractScrollArea::eventFilter (obj, e);
}

void
QTMScrollView::resizeEventBis (QResizeEvent* event) {
  (void) event;
}

/*
 // this code is wrong
 void
 QTMScrollView::wheelEvent ( QWheelEvent *wheelEvent ) {
 if (wheelEvent->modifiers()
 & (Qt::ShiftModifier | Qt::ControlModifier)) {
 setOrigin(QPoint(p_origin.x() + wheelEvent->delta(), p_origin.y()));
 }
 else QAbstractScrollArea::wheelEvent(wheelEvent);
 }
 */
