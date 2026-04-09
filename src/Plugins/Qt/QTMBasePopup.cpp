
/******************************************************************************
 * MODULE     : QTMBasePopup.cpp
 * DESCRIPTION: Base class implementation for popup widgets
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMBasePopup.hpp"
#include "qt_utilities.hpp"

#include <cmath>

QTMBasePopup::QTMBasePopup (QWidget* parent, qt_simple_widget_rep* owner)
    : QWidget (parent), owner (owner), layout (nullptr), effect (nullptr),
      cached_scroll_x (0), cached_scroll_y (0), cached_canvas_x (0),
      cached_canvas_y (0), cached_width (0), cached_height (0),
      cached_magf (0.0) {
  setObjectName ("base_popup");
  setWindowFlags (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute (Qt::WA_ShowWithoutActivating);
  setMouseTracking (true);
  setFocusPolicy (Qt::NoFocus);

  layout= new QHBoxLayout (this);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  layout->setSpacing (1);
  setLayout (layout);

  initCommonUI ();
}

QTMBasePopup::~QTMBasePopup () {}

void
QTMBasePopup::initCommonUI () {
  // 添加阴影效果
  effect= new QGraphicsDropShadowEffect (this);
  effect->setBlurRadius (40);
  effect->setOffset (0, 4);
  effect->setColor (QColor (0, 0, 0, 120));
  this->setGraphicsEffect (effect);
}

void
QTMBasePopup::updatePosition (qt_renderer_rep* ren) {
  if (!selectionInView ()) {
    hide ();
    return;
  }
  int pos_x, pos_y;
  getCachedPosition (ren, pos_x, pos_y);
  move (pos_x, pos_y);
}

void
QTMBasePopup::scrollBy (int x, int y) {
  cached_scroll_x-= (int) (x / cached_magf);
  cached_scroll_y-= (int) (y / cached_magf);
}

void
QTMBasePopup::cachePosition (rectangle selr, double magf, int scroll_x,
                             int scroll_y, int canvas_x, int canvas_y) {
  cached_rect    = selr;
  cached_magf    = magf;
  cached_scroll_x= scroll_x;
  cached_scroll_y= scroll_y;
  cached_canvas_x= canvas_x;
  cached_canvas_y= canvas_y;
}

void
QTMBasePopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  rectangle selr     = cached_rect;
  double    inv_unit = 1.0 / 256.0;
  double    cx_logic = (selr->x1 + selr->x2) * 0.5;
  double    top_logic= selr->y2; // 使用选区底部作为参考点

  // 使用公式计算QT坐标
  double cx_px=
      ((cx_logic - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double top_px= -(top_logic - cached_scroll_y) * cached_magf * inv_unit;

  // 修正：视口 > 表面：存在空白顶部
  double blank_top= 0.0;
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport () &&
      owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  top_px+= blank_top;

  // 基础位置：在选区上方居中显示
  x= int (std::round (cx_px - cached_width * 0.5));
  y= int (std::round (top_px - cached_height));

  // 确保悬浮框在视口内
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();

    // 如果上方空间不足，尝试显示在选区下方
    if (y < 0) {
      double bottom_logic= selr->y1;
      double bottom_px=
          -(bottom_logic - cached_scroll_y) * cached_magf * inv_unit;
      bottom_px+= blank_top;
      y= int (std::round (bottom_px + 10)); // 选区下方10像素
    }

    // 水平边界检查
    if (x < 0) x= 0;
    if (x + cached_width > vp_w) x= vp_w - cached_width;

    // 垂直边界检查
    if (y < 0) y= 0;
    if (y + cached_height > vp_h) y= vp_h - cached_height;
  }
}

bool
QTMBasePopup::selectionInView () const {
  if (!owner || !owner->scrollarea () || !owner->scrollarea ()->viewport ())
    return true;

  rectangle selr    = cached_rect;
  double    inv_unit= 1.0 / 256.0;

  double x1_px=
      ((selr->x1 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double x2_px=
      ((selr->x2 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double y1_px= -(selr->y1 - cached_scroll_y) * cached_magf * inv_unit;
  double y2_px= -(selr->y2 - cached_scroll_y) * cached_magf * inv_unit;

  double blank_top= 0.0;
  if (owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  y1_px+= blank_top;
  y2_px+= blank_top;

  double left  = std::min (x1_px, x2_px);
  double right = std::max (x1_px, x2_px);
  double top   = std::min (y1_px, y2_px);
  double bottom= std::max (y1_px, y2_px);

  int vp_w= owner->scrollarea ()->viewport ()->width ();
  int vp_h= owner->scrollarea ()->viewport ()->height ();

  if (right < 0.0 || left > vp_w) return false;
  if (bottom < 0.0 || top > vp_h) return false;
  return true;
}