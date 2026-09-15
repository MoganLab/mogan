
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

#include <QToolButton>
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

bool
QTMBasePopup::updatePosition (qt_renderer_rep* ren) {
  if (!selectionInView ()) {
    hide ();
    return false;
  }
  int pos_x, pos_y;
  getCachedPosition (ren, pos_x, pos_y);
  move (pos_x, pos_y);
  return true;
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
QTMBasePopup::selectionRectPixels (double& x1_px, double& x2_px, double& top_px,
                                   double& bottom_px) const {
  rectangle selr            = cached_rect;
  double    inv_unit        = 1.0 / 256.0;
  double    sel_top_logic   = (selr->y1 > selr->y2) ? selr->y1 : selr->y2;
  double    sel_bottom_logic= (selr->y1 > selr->y2) ? selr->y2 : selr->y1;

  x1_px=
      ((selr->x1 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  x2_px=
      ((selr->x2 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  top_px   = -(sel_top_logic - cached_scroll_y) * cached_magf * inv_unit;
  bottom_px= -(sel_bottom_logic - cached_scroll_y) * cached_magf * inv_unit;

  // 修正：视口 > 表面：存在空白顶部
  double blank_top= blank_top_offset ();
  top_px+= blank_top;
  bottom_px+= blank_top;
}

void
QTMBasePopup::selectionEdgePixels (double& cx_px, double& top_px,
                                   double& bottom_px) const {
  double x1_px, x2_px;
  selectionRectPixels (x1_px, x2_px, top_px, bottom_px);
  cx_px= (x1_px + x2_px) * 0.5;
}

void
QTMBasePopup::clampToViewport (int& x, int& y) const {
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();
    if (x < 0) x= 0;
    if (x + cached_width > vp_w) x= vp_w - cached_width;
    if (y < 0) y= 0;
    if (y + cached_height > vp_h) y= vp_h - cached_height;
  }
}

void
QTMBasePopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  double cx_px, top_px, bottom_px;
  selectionEdgePixels (cx_px, top_px, bottom_px);

  const int above_y=
      int (std::round (top_px - cached_height - 10)); // 在选区顶部上方显示
  const int below_y=
      int (std::round (bottom_px + 10)); // 如果上面空间不够，显示在选区下方

  x= int (std::round (cx_px - cached_width * 0.5));
  y= above_y;

  // 确保悬浮框在视口内
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();

    const bool above_fits= (above_y >= 0) && (above_y + cached_height <= vp_h);
    const bool below_fits= (below_y >= 0) && (below_y + cached_height <= vp_h);

    if (above_fits) y= above_y;
    else if (below_fits) y= below_y;
    else {
      x= std::max (0, (vp_w - cached_width) / 2);
      y= std::max (0, (vp_h - cached_height) / 2);
    }

    clampToViewport (x, y);
  }
  else {
    if (y < 0) y= below_y;
  }
}

double
QTMBasePopup::blank_top_offset () const {
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport () &&
      owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) return (vp_h - surf_h) * 0.5;
  }
  return 0.0;
}

bool
QTMBasePopup::selectionInView () const {
  if (!owner || !owner->scrollarea () || !owner->scrollarea ()->viewport ())
    return true;

  double x1_px, x2_px, top_px, bottom_px;
  selectionRectPixels (x1_px, x2_px, top_px, bottom_px);

  double left = std::min (x1_px, x2_px);
  double right= std::max (x1_px, x2_px);

  int vp_w= owner->scrollarea ()->viewport ()->width ();
  int vp_h= owner->scrollarea ()->viewport ()->height ();

  if (right < 0.0 || left > vp_w) return false;
  if (bottom_px < 0.0 || top_px > vp_h) return false;
  return true;
}

void
QTMBasePopup::autoSize () {
  const double Scale     = DpiUtils::scaleFactor ();
  const double totalScale= Scale * cached_magf * 2.0;
  int          btn_size  = int (50 * totalScale);

  if (cached_magf <= 0.16) {
    btn_size= 25;
  }

  // 设置按钮大小（使用 DpiUtils 处理内边距）
  QSize                     icon_size (btn_size, btn_size);
  int                       padding= DpiUtils::scaled (16);
  QSize                     fixed_size (btn_size + padding, btn_size + padding);
  const QList<QToolButton*> buttons=
      findChildren<QToolButton*> (QString (), Qt::FindChildrenRecursively);
  for (QToolButton* button : buttons) {
    if (!button) continue;
    if (button->objectName ().isEmpty ())
      button->setObjectName ("base_popup_button");
    button->setIconSize (icon_size);
    button->setFixedSize (fixed_size);
  }

  // 同步窗口外框尺寸到当前按钮布局
  QSize popup_size= layout ? layout->sizeHint () : sizeHint ();
  setFixedSize (popup_size);
  cached_width = popup_size.width ();
  cached_height= popup_size.height ();
}
