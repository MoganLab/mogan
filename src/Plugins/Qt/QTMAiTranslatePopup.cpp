/******************************************************************************
 * MODULE     : QTMAiTranslatePopup.cpp
 * DESCRIPTION: AI translate button popup shown next to the text selection
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMAiTranslatePopup.hpp"
#include "QTMStyle.hpp"
#include "edit_interface.hpp"
#include "qt_utilities.hpp"

#include <QFont>
#include <QPushButton>
#include <algorithm>
#include <cmath>

QTMAiTranslatePopup::QTMAiTranslatePopup (QWidget*              parent,
                                          qt_simple_widget_rep* owner)
    : QTMBasePopup (parent, owner) {
  // translate 只折叠首字符，"Ai translate" 折叠后为词典键 "ai translate"
  translateButton= new QPushButton (qt_translate ("Ai translate"), this);
  translateButton->setFocusPolicy (Qt::NoFocus);
  layout->setContentsMargins (2, 2, 2, 2);
  layout->addWidget (translateButton);
  // 第一步仅挂接显隐，点击后的翻译流程在后续任务接入
  connect (translateButton, &QPushButton::clicked, this, [this] () {
    if (edit_interface_rep* ed=
            dynamic_cast<edit_interface_rep*> (this->owner)) {
      ed->dismiss_translate_popup ();
    }
    else {
      hide ();
    }
  });
}

void
QTMAiTranslatePopup::autoSize () {
  // 按钮字号与内边距跟随选区内最小文字的渲染高度（含文档缩放因子）；
  // 取不到时退回 mini 控件字号
  double inv_unit= 1.0 / 256.0;
  double text_px=
      sel_text_height > 0 ? sel_text_height * cached_magf * inv_unit : 0;
  QFont f= translateButton->font ();
  if (text_px > 0) {
    f.setPixelSize (std::max (8, int (std::round (text_px * 0.75))));
  }
  else {
    f.setPointSize (qt_zoom (QTM_MINI_FONTSIZE));
  }
  translateButton->setFont (f);
  int m= text_px > 0 ? std::max (2, int (std::round (text_px * 0.2))) : 2;
  layout->setContentsMargins (m, m, m, m);
  QSize popup_size= layout ? layout->sizeHint () : sizeHint ();
  setFixedSize (popup_size);
  cached_width = popup_size.width ();
  cached_height= popup_size.height ();
}

void
QTMAiTranslatePopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  (void) ren;
  rectangle selr            = cached_rect;
  double    inv_unit        = 1.0 / 256.0;
  double    sel_top_logic   = std::max (selr->y1, selr->y2);
  double    sel_bottom_logic= std::min (selr->y1, selr->y2);

  double left_px=
      ((selr->x1 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double right_px=
      ((selr->x2 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double top_px= -(sel_top_logic - cached_scroll_y) * cached_magf * inv_unit;
  double bottom_px=
      -(sel_bottom_logic - cached_scroll_y) * cached_magf * inv_unit;

  // 视口大于画布表面时顶部存在居中留白，需补偿（同基类算法）
  double blank_top= 0.0;
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport () &&
      owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  top_px+= blank_top;
  bottom_px+= blank_top;

  const int gap= 4;
  if (tail_free) {
    // 末尾右侧空闲：显示在最后一个选中文字的右方（垂直居中）
    x= int (std::round (right_px + gap));
    y= int (std::round ((top_px + bottom_px - cached_height) * 0.5));
  }
  else {
    // 末尾右侧被后续文字占用：显示在选区右下方，右缘与选区右缘对齐
    x= int (std::round (right_px - cached_width));
    y= int (std::round (bottom_px + gap));
  }

  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();

    // 右侧放不下时退到选区左侧；下方放不下时退到选区上方
    if (tail_free && x + cached_width > vp_w) {
      x= int (std::round (left_px - cached_width - gap));
    }
    if (!tail_free && y + cached_height > vp_h) {
      y= int (std::round (top_px - cached_height - gap));
    }
    if (x < 0) x= 0;
    if (x + cached_width > vp_w) x= vp_w - cached_width;
    if (y < 0) y= 0;
    if (y + cached_height > vp_h) y= vp_h - cached_height;
  }
}

void
QTMAiTranslatePopup::showPopup (qt_renderer_rep* ren, rectangle selr,
                                double magf, int scroll_x, int scroll_y,
                                int canvas_x, int canvas_y) {
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  autoSize ();
  if (!selectionInView ()) {
    hide ();
    return;
  }
  updatePosition (ren);
  show ();
  raise ();
}
