
/******************************************************************************
 * MODULE     : edit_repaint.cpp
 * DESCRIPTION: repaint invalid rectangles
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Interface/edit_interface.hpp"
#include "gui.hpp" // for gui_interrupted
#include "message.hpp"
#include "preferences.hpp"
#include "sys_utils.hpp"

#include <lolly/data/unicode.hpp>

using namespace moebius;

extern int  nr_painted;
extern void clear_pattern_rectangles (renderer ren, rectangle m, rectangles l);
extern bool animated_flag;

/******************************************************************************
 * repainting the window
 ******************************************************************************/

void
edit_interface_rep::draw_background (renderer ren, SI x1, SI y1, SI x2, SI y2) {
  tree bg= get_init_value (BG_COLOR);
  ren->set_background (bg);
  if (get_init_value (PAGE_MEDIUM) == "paper") eb->clear (ren, x1, y1, x2, y2);
  else {
    rectangle m (eb->x1, eb->y1, eb->x2, eb->y2);
    rectangle r (x1, y1, x2, y2);
    rectangle tm= translate (m, ren->ox, ren->oy);
    rectangle tr= translate (r, ren->ox, ren->oy);
    clear_pattern_rectangles (ren, tm, tr);
  }
}

void
edit_interface_rep::draw_text (renderer ren, rectangles& l) {
  nr_painted   = 0;
  bool tp_found= false;
  tree bg      = get_init_value (BG_COLOR);
  ren->set_background (bg);
  animated_flag= (texmacs_time () >= anim_next);
  if (animated_flag) anim_next= 1.0e12;
  eb->redraw (ren, eb->find_box_path (tp, tp_found), l);
  if (animated_flag) {
    double t = max (((double) texmacs_time ()) + 25.0, eb->anim_next ());
    anim_next= min (anim_next, t);
  }
}

void
edit_interface_rep::draw_env (renderer ren) {
  if (!full_screen) {
    if (!is_nil (env_rects)) {
      color col= get_env_color (CONTEXT_COLOR);
      ren->set_pencil (pencil (col, ren->pixel));
      ren->draw_rectangles (env_rects);
    }
    if (!is_nil (foc_rects)) {
      color col= get_env_color (FOCUS_COLOR);
      ren->set_pencil (pencil (col, ren->pixel));
      ren->draw_rectangles (foc_rects);
    }
    if (!is_nil (sem_rects)) {
      if (sem_correct) {
        color col= get_env_color (CORRECT_COLOR);
        ren->set_pencil (pencil (col, ren->pixel));
      }
      else {
        color col= get_env_color (INCORRECT_COLOR);
        ren->set_pencil (pencil (col, ren->pixel));
      }
      ren->draw_rectangles (sem_rects);
    }
  }
}

void
edit_interface_rep::draw_cursor (renderer ren) {
  if (get_preference ("draw cursor") == "on" && !temp_invalid_cursor &&
      (got_focus || full_screen)) {
    if (cursor_blink_active && !cursor_blink_visible) return;
    cursor cu= get_cursor ();
    if (!inside_active_graphics ()) {
      SI dw= 0;
      if (tremble_count > 3) dw= min (tremble_count - 3, 25) * pixel;
      cu->y1-= 2 * zpixel + dw;
      cu->y2+= 2 * zpixel + dw;
      SI     x1= cu->ox + ((SI) (cu->y1 * cu->slope)), y1= cu->oy + cu->y1;
      SI     x2= cu->ox + ((SI) (cu->y2 * cu->slope)), y2= cu->oy + cu->y2;
      string mode= get_env_string (MODE);
      string family, series;
      color  cuc= get_env_color (CURSOR_COLOR);
      if (!cu->valid) cuc= green;
      else if (mode == "math") cuc= get_env_color (MATH_CURSOR_COLOR);
      ren->set_pencil (pencil (cuc, zpixel + dw));
      if ((mode == "text") || (mode == "src")) {
        family= get_env_string (FONT_FAMILY);
        series= get_env_string (FONT_SERIES);
      }
      else if (mode == "math") {
        family= get_env_string (MATH_FONT_FAMILY);
        series= get_env_string (MATH_FONT_SERIES);
      }
      else if (mode == "prog") {
        family= get_env_string (PROG_FONT_FAMILY);
        series= get_env_string (PROG_FONT_SERIES);
      }
      SI lserif= (series == "bold" ? 2 * zpixel : zpixel) + dw;
      SI rserif= zpixel + dw;
      if (family == "ss") lserif= rserif= 0;
      ren->line (x1 - lserif, y1, x1 + rserif, y1);
      if (y1 <= y2 - zpixel) {
        ren->line (x1, y1, x2, y2 - zpixel);
        if (series == "bold")
          ren->line (x1 - zpixel, y1, x2 - zpixel, y2 - zpixel);
        ren->line (x2 - lserif, y2 - zpixel, x2 + rserif, y2 - zpixel);
      }
    }
  }
}

void
edit_interface_rep::draw_surround (renderer ren, rectangle r) {
  ren->set_background (tm_background);
  string medium= get_init_string (PAGE_MEDIUM);
  if (medium == "automatic") return;
  if (medium == "beamer" && full_screen) return;
  ren->clear (r->x1, r->y1, max (r->x1, eb->x1), r->y2);
  ren->clear (min (r->x2, eb->x2), r->y1, r->x2, r->y2);
  if (medium == "papyrus") return;
  ren->clear (r->x1, r->y1, r->x2, max (r->y1, eb->y1));
  ren->clear (r->x1, min (r->y2, eb->y2), r->x2, r->y2);
}

void
edit_interface_rep::draw_context (renderer ren, rectangle r) {
  draw_surround (ren, r);
}

void
edit_interface_rep::draw_selection (renderer ren, rectangle r) {
  rectangles visible (thicken (r, 2 * ren->pixel, 2 * ren->pixel));
  if (!is_nil (locus_rects)) {
    color col= mouse_clickable_color ();
    ren->set_pencil (pencil (col, ren->pixel));
    ren->draw_rectangles (locus_rects);
  }
  int alt_count= N (alt_selection_rects);
  for (int i= 0; i < alt_count; i++) {
    // 跳过当前选择项，避免颜色混合
    if (!is_nil (selection_rects) &&
        alt_selection_rects[i] == selection_rects) {
      continue;
    }
    color col= get_env_color (MATCH_COLOR);
    ren->set_pencil (pencil (col, ren->pixel));
#ifdef QTTEXMACS
    ren->draw_selection (alt_selection_rects[i] & visible);
#else
    ren->draw_rectangles (alt_selection_rects[i] & visible);
#endif
  }
  if (!is_nil (selection_rects)) {
    color col= get_env_color (SELECTION_COLOR);
    if (table_selection) col= get_env_color (TABLE_SELECTION_COLOR);
    ren->set_pencil (pencil (col, ren->pixel));
#ifdef QTTEXMACS
    ren->draw_selection (selection_rects & visible);
#else
    ren->draw_rectangles (selection_rects & visible);
#endif
  }

  draw_image_resize_handles (ren);
  draw_table_resize_handles (ren);
}

void
edit_interface_rep::draw_image_resize_handles (renderer ren) {
  // 鼠标位于图片中时，绘制 handles
  SI        hs            = 10 * ren->pixel; // handles 半径
  rectangle new_image_brec= rectangle (0, 0, 0, 0);
  SI        x1= 0, y1= 0, x2= 0, y2= 0, mx= 0, my= 0;
  bool      have_bbox= false;

  // 检测当前路径往上的祖先有没有是 IMAGE 的
  for (path p= path_up (tp); !is_nil (p) && p != rp; p= path_up (p)) {
    tree st= subtree (et, p);
    if (!is_func (st, IMAGE)) continue;

    selection sel= eb->find_check_selection (p * 0, p * 1);
    if (!sel->valid || is_nil (sel->rs)) break;

    rectangle bbox= least_upper_bound (sel->rs); // 获取盒子边界
    x1            = bbox->x1;
    y1            = bbox->y1;
    x2            = bbox->x2;
    y2            = bbox->y2;
    mx            = (x1 + x2) / 2;
    my            = (y1 + y2) / 2;
    new_image_brec= rectangle (x1 - hs, y1 - hs, x2 + hs, y2 + hs);
    have_bbox     = true;
    break; // 只处理最近的 IMAGE 祖先
  }

  if (new_image_brec != last_image_brec) {
    if (!is_zero (last_image_brec)) invalidate (last_image_brec);
    if (!is_zero (new_image_brec)) invalidate (new_image_brec);
    last_image_brec= new_image_brec;
  }

  if (!have_bbox) { // 不用画，但要设置缓存
    last_image_hr= 0;
    return;
  }
  last_image_hr= hs;

  // 8 个 handles，4 个位于边中点，4 个位于角上
  color border_col= get_env_color (FOCUS_COLOR);
  SI    border_w  = max (2 * ren->pixel, hs / 3);
  ren->set_pencil (pencil (border_col, border_w));

  // 8 个点分别是：sw, se, nw, ne, s, n, w, e
  SI hx[8]= {x1, x2, x1, x2, mx, mx, x1, x2};
  SI hy[8]= {y1, y1, y2, y2, y1, y2, my, my};

  for (int i= 0; i < 8; i++) {
    SI cx= hx[i], cy= hy[i];

    // 外圆作为边框
    ren->set_brush (brush (border_col));
    ren->fill_arc (cx - hs, cy - hs, cx + hs, cy + hs, 0, 64 * 360);

    // 内圆作为填充
    SI inner_r= max (hs - border_w, ren->pixel);
    ren->set_brush (brush (white));
    ren->fill_arc (cx - inner_r, cy - inner_r, cx + inner_r, cy + inner_r, 0,
                   64 * 360);
  }
}

void
edit_interface_rep::draw_table_resize_handles (renderer ren) {
  // 鼠标位于表格中时，绘制 handles
  SI        hs            = 8 * ren->pixel; // handles 半径（正方形半边长）
  rectangle new_table_brec= rectangle (0, 0, 0, 0);
  SI        x1= 0, y1= 0, x2= 0, y2= 0, mx= 0, my= 0;
  bool      have_bbox= false;

  // 检测当前路径往上的祖先有没有是 TABLE 的
  for (path p= path_up (tp); !is_nil (p) && p != rp; p= path_up (p)) {
    tree st= subtree (et, p);
    if (!is_func (st, TABLE)) continue;

    selection sel= eb->find_check_selection (p * 0, p * 1);
    if (!sel->valid || is_nil (sel->rs)) break;

    table_scale_path= ::table_search_format (et, p); // 这里就可以缓存 path 了

    rectangle bbox= least_upper_bound (sel->rs); // 获取盒子边界
    x1            = bbox->x1;
    y1            = bbox->y1;
    x2            = bbox->x2;
    y2            = bbox->y2;
    mx            = (x1 + x2) / 2;
    my            = (y1 + y2) / 2;
    new_table_brec= rectangle (x1, y1 - 2 * hs, x2 + 2 * hs, y2);
    have_bbox     = true;
    break; // 只处理最近的 TABLE 祖先
  }

  if (new_table_brec != last_table_brec) {
    if (!is_zero (last_table_brec)) invalidate (last_table_brec);
    if (!is_zero (new_table_brec)) invalidate (new_table_brec);
    last_table_brec= new_table_brec;
  }

  if (!have_bbox) { // 不用画，但要设置缓存
    last_table_hr= 0;
    return;
  }
  last_table_hr= hs;

  // 3 个 handles：右侧、下侧、右下角
  color border_col= get_env_color (FOCUS_COLOR);
  SI    border_w  = max (2 * ren->pixel, hs / 3);

  // 3 个点分别是：下侧中点、右侧中点、右下角
  SI hx[3]= {(x1 + x2) / 2, x2 + hs, x2 + hs};
  SI hy[3]= {y1 - hs, (y1 + y2) / 2, y1 - hs};

  // 通过光标位置定位表格信息
  table_hit current_hit;
  cursor    current_cursor= get_cursor ();
  if (current_cursor->valid)
    table_line_hit (current_cursor->ox, current_cursor->oy, current_hit);
  table_scale_wide_flag= current_hit.wide_flag;

  int ed= table_scale_wide_flag ? 1 : 3;
  for (int i= 0; i < ed; i++) {
    SI cx= hx[i], cy= hy[i];

    // 外正方形作为边框
    ren->set_brush (brush (border_col));
    ren->fill (cx - hs, cy - hs, cx + hs, cy + hs);

    // 内正方形作为填充（留出边框）
    SI inner_hs= max (hs - border_w, ren->pixel);
    ren->set_brush (brush (white));
    ren->fill (cx - inner_hs, cy - inner_hs, cx + inner_hs, cy + inner_hs);
  }
}

void
edit_interface_rep::draw_graphics (renderer ren) {
  if (got_focus || full_screen) {
    cursor cu= get_cursor ();
    if (over_graphics (cu->ox, cu->oy) && inside_active_graphics ()) {
      eval ("(graphics-reset-context 'graphics-cursor)");
      draw_graphical_object (ren);
      string tm_curs= as_string (eval ("graphics-texmacs-pointer"));
      if (tm_curs != "none") {
        if (tm_curs == "graphics-cross") {
          ren->set_pencil (pencil (red, pixel));
          ren->line (cu->ox, cu->oy - 5 * pixel, cu->ox, cu->oy + 5 * pixel);
          ren->line (cu->ox - 5 * pixel, cu->oy, cu->ox + 5 * pixel, cu->oy);
        }
        else if (tm_curs == "graphics-cross-arrows") {
          static int s= 6 * pixel, a= 2 * pixel;
          ren->set_pencil (pencil (red, pixel));
          ren->line (cu->ox, cu->oy - s, cu->ox, cu->oy + s);
          ren->line (cu->ox - s, cu->oy, cu->ox + s, cu->oy);
          ren->line (cu->ox, cu->oy - s, cu->ox - a, cu->oy - s + a);
          ren->line (cu->ox, cu->oy - s, cu->ox + a, cu->oy - s + a);
          ren->line (cu->ox, cu->oy + s, cu->ox - a, cu->oy + s - a);
          ren->line (cu->ox, cu->oy + s, cu->ox + a, cu->oy + s - a);
          ren->line (cu->ox - s, cu->oy, cu->ox - s + a, cu->oy + a);
          ren->line (cu->ox - s, cu->oy, cu->ox - s + a, cu->oy - a);
          ren->line (cu->ox + s, cu->oy, cu->ox + s - a, cu->oy + a);
          ren->line (cu->ox + s, cu->oy, cu->ox + s - a, cu->oy - a);
        }
      }
    }
    else eval ("(graphics-reset-context 'text-cursor)");
  }
}

void
edit_interface_rep::draw_pre (renderer win, renderer ren, rectangle r) {
  // draw surroundings
  draw_background (ren, r->x1, r->y1, r->x2, r->y2);
  draw_surround (ren, r);

  // predraw cursor
  draw_cursor (ren);
  rectangles l= copy_always;
  while (!is_nil (l)) {
    rectangle lr (l->item);
    win->put_shadow (ren, lr->x1, lr->y1, lr->x2, lr->y2);
    l= l->next;
  }
}

void
edit_interface_rep::draw_post (renderer win, renderer ren, rectangle r) {
  win->set_zoom_factor (zoomf);
  ren->set_zoom_factor (zoomf);
  draw_context (ren, r);
  draw_env (ren);
  draw_selection (ren, r);
  draw_graphics (ren);
  draw_cursor (ren); // the text cursor must be drawn over the graphical object
  ren->reset_zoom_factor ();
  win->reset_zoom_factor ();
}

void
edit_interface_rep::draw_with_shadow (renderer win, rectangle r) {
  rectangle sr= r * magf;
  win->new_shadow (shadow);
  win->get_shadow (shadow, sr->x1, sr->y1, sr->x2, sr->y2);
  renderer ren= shadow;

  rectangles l;
  win->set_zoom_factor (zoomf);
  ren->set_zoom_factor (zoomf);
  draw_pre (win, ren, r);
  draw_text (ren, l);
  ren->reset_zoom_factor ();
  win->reset_zoom_factor ();

  if (gui_interrupted ()) {
    ren->set_zoom_factor (zoomf);
    l= l & rectangles (translate (r, ren->ox, ren->oy));
    simplify (l);
    copy_always= translate (copy_always, ren->ox, ren->oy);
    while (!is_nil (copy_always)) {
      l          = rectangles (copy_always->item, l);
      copy_always= copy_always->next;
    }
    ren->reset_zoom_factor ();

    draw_post (win, ren, r);
    while (!is_nil (l)) {
      SI x1= ((SI) (l->item->x1 * magf)) - ren->ox - PIXEL;
      SI y1= ((SI) (l->item->y1 * magf)) - ren->oy - PIXEL;
      SI x2= ((SI) (l->item->x2 * magf)) - ren->ox + PIXEL;
      SI y2= ((SI) (l->item->y2 * magf)) - ren->oy + PIXEL;
      ren->outer_round (x1, y1, x2, y2);
      win->put_shadow (ren, x1, y1, x2, y2);
      l= l->next;
    }
  }
}

void
edit_interface_rep::draw_with_stored (renderer win, rectangle r) {
  // cout << "Redraw " << (r*magf/PIXEL) << "\n";

  /* Verify whether the backing store is still valid */
  if (!is_nil (stored_rects)) {
    SI w1, h1, w2, h2;
    win->get_extents (w1, h1);
    stored->get_extents (w2, h2);
    if (stored->ox != win->ox || stored->oy != win->oy || w1 != w2 ||
        h1 != h2) {
      // cout << "x"; cout.flush ();
      stored_rects= rectangles ();
    }
  }

  /* Either draw with backing store or regenerate */
  rectangle sr= r * magf;
  if (is_nil (rectangles (r) - stored_rects) && !is_nil (stored_rects)) {
    // cout << "*"; cout.flush ();
    win->new_shadow (shadow);
    win->get_shadow (shadow, sr->x1, sr->y1, sr->x2, sr->y2);
    shadow->put_shadow (stored, sr->x1, sr->y1, sr->x2, sr->y2);
    draw_post (win, shadow, r);
    win->put_shadow (shadow, sr->x1, sr->y1, sr->x2, sr->y2);
  }
  else {
    // cout << "."; cout.flush ();
    draw_with_shadow (win, r);
    if (!gui_interrupted ()) {
      if (inside_active_graphics ()) {
        shadow->new_shadow (stored);
        shadow->get_shadow (stored, sr->x1, sr->y1, sr->x2, sr->y2);
        // stored_rects= /*stored_rects |*/ rectangles (r);
        stored_rects= simplify (rectangles (r, stored_rects));
        // cout << "Stored: " << stored_rects << "\n";
        // cout << "M"; cout.flush ();
      }
      draw_post (win, shadow, r);
      win->put_shadow (shadow, sr->x1, sr->y1, sr->x2, sr->y2);
    }
    else draw_post (win, win, r);
  }
}

/******************************************************************************
 * event handlers
 ******************************************************************************/

void
edit_interface_rep::handle_clear (renderer win, SI x1, SI y1, SI x2, SI y2) {
  x1= (SI) (x1 / magf);
  y1= (SI) (y1 / magf);
  x2= (SI) (x2 / magf);
  y2= (SI) (y2 / magf);
  win->set_zoom_factor (zoomf);
  draw_background (win, max (eb->x1, x1), max (eb->y1, y1), min (eb->x2, x2),
                   min (eb->y2, y2));
  draw_surround (win, rectangle (x1, y1, x2, y2));
  win->reset_zoom_factor ();
}

void
edit_interface_rep::handle_repaint (renderer win, SI x1, SI y1, SI x2, SI y2) {
  if (is_nil (eb)) apply_changes ();
  if (env_change != 0) {
    std_warning << "Invalid situation (" << env_change << ")"
                << " in edit_interface_rep::handle_repaint\n";
    return;
  }

  /*
  // In the past, we used the code below in order to hide the trace of
  // a moving cursor. This code is now incorrect, because the rectangle
  // (x1, y1)--(x2, y2) does not correspond to the repaint region clipping.
  // Nevertheless, the code seems no longer necessary. In case it would be,
  // it should be moved somewhere inside the internal repaint routines.
  SI extra= 3 * get_init_int (FONT_BASE_SIZE) * PIXEL * magf / 2;
  SI X1= (x1-extra) / magf, Y1= (y1-extra) / magf;
  SI X2= (x2+extra) / magf, Y2= (y2+extra) / magf;
  draw_with_stored (rectangle (X1, Y1, X2, Y2));
  */

  // cout << "Repainting\n";
  draw_with_stored (win, rectangle (x1, y1, x2, y2) / magf);
  if (last_change - last_update > 0) last_change= texmacs_time ();
  // cout << "Repainted\n";
}
