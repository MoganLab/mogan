
/******************************************************************************
 * MODULE     : im_widget.cpp
 * DESCRIPTION: ImGui widget base class
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "im_widget.hpp"

#include "message.hpp"

/******************************************************************************
 * im_widget_rep: the base widget for the ImGui port.
 ******************************************************************************/

template <>
void
tm_delete<im_widget_rep> (im_widget_rep* ptr) {
  if (ptr == NULL) return;
  void* mem= ptr->derived_this ();
  ptr->~im_widget_rep ();
  fast_delete (mem);
}

static long widget_counter= 0;

im_widget_rep::im_widget_rep (types _type)
    : widget_rep (), id (widget_counter++), type (_type) {}

im_widget_rep::~im_widget_rep () {}

void
im_widget_rep::add_child (widget w) {
  children << w;
}

void
im_widget_rep::add_children (array<widget> a) {
  children << a;
}

void
im_widget_rep::send (slot s, blackbox val) {
  (void) val;
  switch (s) {
  case SLOT_DESTROY: {
    // Re-send to tracked children (cf. qt_widget_rep::send).
    for (int i= 0; i < N (children); ++i)
      if (!is_nil (children[i])) children[i]->send (s, val);
  } break;
  default:
#ifdef LIII_DEBUG
    cout << "im_widget_rep::send(), unhandled " << slot_name (s) << LF;
#endif
    break;
  }
}

blackbox
im_widget_rep::query (slot s, int type_id) {
  (void) type_id; // open_box<T> already asserts the caller's expected type
  static int id= 1;
  switch (s) {
  case SLOT_IDENTIFIER: {
    return close_box<int> (id++);
  }
  case SLOT_MAIN_ICONS_VISIBILITY: {
    return close_box<bool> (false);
  }
  case SLOT_POSITION: {
    return close_box<coord2> (coord2 (0, 0));
  }
  case SLOT_SIZE: {
    return close_box<coord2> (coord2 (800, 600));
  }
  default:
#ifdef LIII_DEBUG
    cout << "im_widget_rep::query(), unhandled " << slot_name (s) << LF;
#endif
    return blackbox ();
  }
}

widget
im_widget_rep::plain_window_widget (string name, command quit, int b) {
  (void) name;
  (void) quit;
  (void) b;
  // 在 WASM/ImGui 后端中只有 im_tm_widget_rep 是真实顶层窗口；
  // 其它 widget 作为 stub 窗口返回自身，避免 nil widget 进入 window_table。
  return widget (this);
}

widget
im_widget_rep::make_popup_widget () {
  // Stub
  return widget (this);
}

widget
im_widget_rep::popup_window_widget (string s) {
  (void) s;
  // Stub
  return widget (this);
}

widget
im_widget_rep::tooltip_window_widget (string s) {
  (void) s;
  return widget (this);
}

tm_ostream&
operator<< (tm_ostream& out, im_widget w) {
  if (is_nil (w)) return out << "nil";
  return out << "im_widget of type " << (int) w.rep->type << " id "
             << w.rep->id;
}
