/******************************************************************************
 * MODULE     : qt_flow_layout.cpp
 * DESCRIPTION: 自动换行流式布局（仿 Qt widgets/flowlayout 示例，BSD）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_flow_layout.hpp"

#include <QWidget>

QtFlowLayout::~QtFlowLayout () {
  // QLayout 不拥有子项，须逐个 delete（item 持有的 widget 随之析构）
  while (QLayoutItem* item= takeAt (0))
    delete item;
}

QLayoutItem*
QtFlowLayout::takeAt (int index) {
  if (index >= 0 && index < items_.size ()) return items_.takeAt (index);
  return nullptr;
}

int
QtFlowLayout::smartSpacing (QStyle::PixelMetric pm) const {
  QObject* parent= this->parent ();
  if (!parent) return -1;
  if (parent->isWidgetType ()) {
    QWidget* pw= static_cast<QWidget*> (parent);
    return pw->style ()->pixelMetric (pm, nullptr, pw);
  }
  return static_cast<QLayout*> (parent)->spacing ();
}

QSize
QtFlowLayout::minimumSize () const {
  QSize size;
  for (const QLayoutItem* item : items_)
    size= size.expandedTo (item->minimumSize ());
  size+= QSize (2 * contentsMargins ().left (), 2 * contentsMargins ().top ());
  return size;
}

int
QtFlowLayout::heightForWidth (int width) const {
  return doLayout (QRect (0, 0, width, 0), true);
}

void
QtFlowLayout::setGeometry (const QRect& rect) {
  QLayout::setGeometry (rect);
  doLayout (rect, false);
}

int
QtFlowLayout::doLayout (const QRect& rect, bool testOnly) const {
  int left, top, right, bottom;
  getContentsMargins (&left, &top, &right, &bottom);
  QRect effective = rect.adjusted (left, top, -right, -bottom);
  int   x         = effective.x ();
  int   y         = effective.y ();
  int   lineHeight= 0;
  int   hSpace    = hSpace_ >= 0 ? hSpace_
                                 : smartSpacing (QStyle::PM_LayoutHorizontalSpacing);
  int   vSpace=
      vSpace_ >= 0 ? vSpace_ : smartSpacing (QStyle::PM_LayoutVerticalSpacing);
  if (hSpace < 0) hSpace= 6;
  if (vSpace < 0) vSpace= 6;

  for (QLayoutItem* item : items_) {
    const QSize hint = item->sizeHint ();
    int         nextX= x + hint.width () + hSpace;
    if (nextX - hSpace > effective.right () + 1 && lineHeight > 0) {
      x         = effective.x ();
      y         = y + lineHeight + vSpace;
      nextX     = x + hint.width () + hSpace;
      lineHeight= 0;
    }
    if (!testOnly) item->setGeometry (QRect (QPoint (x, y), hint));
    x         = nextX;
    lineHeight= qMax (lineHeight, hint.height ());
  }
  return y + lineHeight - rect.y () + bottom;
}
