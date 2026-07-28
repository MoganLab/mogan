
/******************************************************************************
 * MODULE     : cli_widget.hpp
 * DESCRIPTION: CLI 前端的最小 widget 基类（无真实窗口/渲染）
 *
 * 无 UI 库（不依赖 Qt/ImGui/glfw）的渲染前端：widget 树只用于满足构造与槽
 * 协议；渲染经 make_raster_image（mupdf）离屏完成，故此处一切为占位实现。
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef CLI_WIDGET_HPP
#define CLI_WIDGET_HPP

#include "message.hpp"
#include "widget.hpp"

typedef pair<SI, SI>            coord2;
typedef quartet<SI, SI, SI, SI> coord4;

class cli_widget_rep : public widget_rep {
public:
  inline cli_widget_rep () {}
  inline virtual ~cli_widget_rep () {}
  virtual string get_nickname () { return "cli"; }

  // 槽分发：widget_rep 的默认 send/query/read/write 会 TM_FAILED("no default
  // implementation")。CLI 不交互，全 no-op（query 返回空 box），避免 init /
  // ensure_window 向桩 widget
  // 投递槽时抛异常。渲染路径(render_to_images)不触槽。
  virtual void send (slot s, blackbox val) {
    (void) s;
    (void) val;
  }
  virtual blackbox query (slot s, int type_id) {
    (void) type_id;
    // 处理 init/ensure_window 期间会查的槽（仿 im_widget_rep::query）；
    // 其余槽返回空 box（与 ImGui 默认一致）。
    switch (s) {
    case SLOT_IDENTIFIER: {
      static int id= 1;
      return close_box<int> (id++);
    }
    case SLOT_MAIN_ICONS_VISIBILITY:
      return close_box<bool> (false);
    case SLOT_POSITION:
      return close_box<coord2> (coord2 (0, 0));
    case SLOT_SIZE:
      return close_box<coord2> (coord2 (800, 600));
    case SLOT_INVALID:
      return close_box<bool> (false);
    case SLOT_SCROLL_POSITION:
      return close_box<coord2> (coord2 (0, 0));
    case SLOT_EXTENTS:
      return close_box<coord4> (coord4 (0, 0, 800, 600));
    case SLOT_VISIBLE_PART:
      return close_box<coord4> (coord4 (0, 0, 800, 600));
    default:
      cout << "CLI query unhandled slot: " << slot_name (s) << "\n";
      return blackbox ();
    }
  }
  virtual widget read (slot s, blackbox index) {
    (void) s;
    (void) index;
    return widget ();
  }
  virtual void write (slot s, blackbox index, widget w) {
    (void) s;
    (void) index;
    (void) w;
  }

  // 窗口工厂：无真实窗口，返回自身占位，避免上层对 null widget 解引用。
  virtual widget plain_window_widget (string name, command quit, int b= 3) {
    (void) name;
    (void) quit;
    (void) b;
    return widget ((widget_rep*) this);
  }
  virtual widget make_popup_widget () { return widget ((widget_rep*) this); }
  virtual widget popup_window_widget (string s) {
    (void) s;
    return widget ((widget_rep*) this);
  }
  virtual widget tooltip_window_widget (string s) {
    (void) s;
    return widget ((widget_rep*) this);
  }
};

#endif // defined CLI_WIDGET_HPP
