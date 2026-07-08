
/******************************************************************************
 * MODULE     : im_widget.hpp
 * DESCRIPTION: ImGui widget base class
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_WIDGET_HPP
#define IM_WIDGET_HPP

#include "fast_alloc.hpp"
#include "message.hpp"
#include "widget.hpp"

class im_widget;

// Coordinate helpers in TeXmacs SI units
typedef pair<SI, SI>            coord2;
typedef quartet<SI, SI, SI, SI> coord4;

/*
 ImGui 界面中所有 TeXmacs 控件的基类。

 这是 qt_widget_rep 在 ImGui 后端中的对应实现。但是与 Qt 不同，ImGui 采用
 Immediate Mode，因此这一层没有与每个控件对应的持久的句柄。

 只有唯一的顶层编辑器窗口（im_tm_widget_rep）拥有真正的 GLFWwindow 和 ImGui
 上下文

 目前也只实现了顶层编辑器窗口
*/
class im_widget_rep : public widget_rep {
protected:
  array<widget> children;

public:
  int64_t id;

  enum types {
    none= 0,
    texmacs_widget,
    embedded_tm_widget,
  };

  types type;

  im_widget_rep (types _type= none);
  virtual ~im_widget_rep ();
  virtual string get_nickname () { return "popup"; }

  virtual widget plain_window_widget (string name, command quit, int b= 3);
  virtual widget make_popup_widget ();
  virtual widget popup_window_widget (string s);
  virtual widget tooltip_window_widget (string s);

  void add_child (widget a);
  void add_children (array<widget> a);

  ////////////////////// Handling of TeXmacs' messages

  /// See widkit_wrapper.cpp / qt_widget_rep for the reference list of slots.
  virtual void     send (slot s, blackbox val);
  virtual blackbox query (slot s, int type_id);

  virtual widget read (slot s, blackbox index) {
    (void) index;
#ifdef LIII_DEBUG
    cout << "im_widget_rep::read(), unhandled " << slot_name (s) << LF;
#endif
    return widget ();
  }

  virtual void write (slot s, blackbox index, widget w) {
    (void) index;
    (void) w;
#ifdef LIII_DEBUG
    cout << "im_widget_rep::write(), unhandled " << slot_name (s) << LF;
#endif
  }

  virtual void notify (slot s, blackbox new_val) {
    (void) new_val;
#ifdef LIII_DEBUG
    cout << "im_widget_rep::notify(), unhandled " << slot_name (s) << LF;
#endif
  }
};

template <> void tm_delete<im_widget_rep> (im_widget_rep*);

/*! Reference counting wrapper around im_widget_rep (cf. qt_widget). */
class im_widget {
public:
  ABSTRACT_NULL (im_widget);

  inline bool operator== (im_widget w) { return rep == w.rep; }
  inline bool operator!= (im_widget w) { return rep != w.rep; }
};
ABSTRACT_NULL_CODE (im_widget);

tm_ostream& operator<< (tm_ostream& out, im_widget w);

/*! Casting from im_widget to widget */
inline widget
abstract (im_widget w) {
  return widget (w.rep);
}

/*! Casting from widget to im_widget */
inline im_widget
concrete (widget w) {
  return im_widget (static_cast<im_widget_rep*> (w.rep));
}

#endif // defined IM_WIDGET_HPP
