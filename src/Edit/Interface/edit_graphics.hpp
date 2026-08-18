
/******************************************************************************
 * MODULE     : edit_graphics.hpp
 * DESCRIPTION: the interface for TeXmacs
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef EDIT_GRAPHICS_H
#define EDIT_GRAPHICS_H
#include "editor.hpp"
#include "tm_timer.hpp"

class edit_graphics_rep : virtual public editor_rep {
private:
  box           go_box;     // The graphical object typesetted as a box
  double        p_x, p_y;   // Last unadjusted (x, y) position
  double        gr_x, gr_y; // Last (x, y) position of the mouse
  gr_selections gs;         // Last graphical_select (x, y)
  grid          gr0;        // Last grid

protected:
  point cur_pos;
  tree  graphical_object;

public:
  edit_graphics_rep ();
  ~edit_graphics_rep ();

  path   graphics_path ();
  bool   inside_graphics (bool b);
  bool   inside_active_graphics (bool b);
  bool   over_graphics (SI x, SI y);
  tree   get_graphics ();
  double get_x ();
  double get_y ();
  double get_pixel ();
  frame  find_frame (bool last= false);
  grid   find_grid ();
  void   find_limits (point& lim1, point& lim2);
  bool   find_graphical_region (SI& x1, SI& y1, SI& x2, SI& y2);
  point  adjust (point p);
  tree   find_point (point p);
  tree   graphical_select (double x, double y);
  tree   graphical_select (double x1, double y1, double x2, double y2);
  tree   get_graphical_object ();
  void   set_graphical_object (tree t);
  void   invalidate_graphical_object ();
  void   draw_graphical_object (renderer ren);
  bool mouse_graphics (string s, SI x, SI y, int m, time_t t, array<double> d);
  void back_in_text_at (tree t, path p, bool forward);
};

/**
 * @brief 注册一个线段中点：加入显示集合（去重），鼠标足够近时追加吸附候选
 * @param fp            鼠标位置（屏幕/布局坐标系）
 * @param snap_distance 吸附距离：鼠标与中点距离小于它才追加吸附候选，
 *                      避免长边远端被强行拉到中点
 * @param ms            中点（与 fp 同坐标系），作为吸附候选的位置
 * @param sx, sy        中点的文档坐标字符串，用作去重键与装饰绘制坐标
 * @param points        [inout] 中点显示集合（TUPLE 树，元素为 (sx sy)），
 *                      已含相同 (sx, sy) 时直接返回、不重复注册
 * @param sels          [inout] 吸附候选集合，命中时追加 curve-mid-point 候选
 * @param cp            候选的控制点路径（绘制中的折线无文档路径，传空）
 * @param pts           候选的控制点坐标（同上，可传空）
 */
void register_midpoint (point fp, double snap_distance, point ms, string sx,
                        string sy, tree& points, gr_selections& sels,
                        array<path> cp, array<point> pts);

#endif // defined EDIT_GRAPHICS_H
