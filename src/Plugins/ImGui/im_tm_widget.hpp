
/******************************************************************************
 * MODULE     : im_tm_widget.hpp
 * DESCRIPTION: The main TeXmacs editor window for the ImGui
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_TM_WIDGET_HPP
#define IM_TM_WIDGET_HPP

#include "boxes.hpp"
#include "command.hpp"
#include "im_simple_widget.hpp"
#include "im_widget.hpp"
#include "imgui.h"
#include "picture.hpp"

struct GLFWwindow; // defined by <GLFW/glfw3.h>

class im_tm_widget_rep : public im_widget_rep {
protected:
  int     win_id;
  string  orig_name;
  command quit;
  bool    visibility[12]; // decoded from the mask, cf. qt_tm_widget_rep

  box    the_box;
  double zoomf;

  GLFWwindow*  window;
  unsigned int document_texture; // cached GL texture (GLuint)
  int          tex_w, tex_h;     // texture width / height
  bool         texture_dirty;    // re-rasterize on next frame
  bool         initialized;
  bool needs_refocus; // when a stub window was created -> re-grab focus next
                      // frame

  picture the_picture;
  int     pic_w, pic_h;
  int     pic_rf; // retina factor

  widget main_widget;
  // 主菜单条（由 SLOT_MAIN_MENU 写入）与菜单条占用的顶部高度（SI），后者供
  // screen_to_si 把鼠标坐标对齐到菜单条下方的画布原点。
  widget menu_widget;
  SI     menu_offset_y;

  // 底部状态栏（由 SLOT_*_FOOTER 写入）及其高度（SI），供画布尺寸计算。
  string footer_left;
  string footer_middle;
  string footer_right;
  SI     footer_height;
  bool   footer_interactive;

  ImGuiIO* io;

  void render_editor ();
  void shutdown_context ();
  bool visibility_index (slot s, int& i);

  // GLFW input callbacks. GLFW event -> Mogan event
  static void glfw_key_callback (GLFWwindow* w, int key, int scancode,
                                 int action, int mods);
  static void glfw_char_callback (GLFWwindow* w, unsigned int codepoint);
  static void glfw_mouse_button_callback (GLFWwindow* w, int button, int action,
                                          int mods);
  static void glfw_cursor_pos_callback (GLFWwindow* w, double xpos,
                                        double ypos);
  static void glfw_scroll_callback (GLFWwindow* w, double xoffset,
                                    double yoffset);
  static void glfw_window_focus_callback (GLFWwindow* w, int focused);

  // Forward to editor
  void dispatch_keypress (const string& key);
  void dispatch_mouse (const string& kind, SI x, SI y, int mstate,
                       array<double> data= array<double> ());
  // Convert a position in GLFW screen coordinates to the renderer's SI space
  void screen_to_si (double xpos, double ypos, SI& sx, SI& sy);
  // ImGui's main loop
  void im_main_loop ();
  // The attached editor canvas. nullptr when no real editor is wired
  im_simple_widget_rep* canvas ();

public:
  im_tm_widget_rep (int mask, command quit);
  ~im_tm_widget_rep ();

  static void em_main_loop_wrapper (void* arg);

  // Provide the typeset document to display
  void set_box (box b, double zoom_factor);

  virtual widget plain_window_widget (string name, command quit, int b= 3);

  // The GLFW/ImGui main loop. Returns when the window is closed
  void run ();

  // The native GLFW window handle (null if is stub window). Used by
  // the clipboard backend for glfwSet/GetClipboardString.
  GLFWwindow* glfw_window () const { return window; }

  ////////////////////// Handling of TeXmacs' messages
  virtual void     send (slot s, blackbox val);
  virtual blackbox query (slot s, int type_id);
  virtual widget   read (slot s, blackbox index);
  virtual void     write (slot s, blackbox index, widget w);
};

void im_run_main_loop ();

// Primary window's GLFW handle. Used by the
// clipboard backend (set_selection/get_selection) for
// glfwSet/GetClipboardString.
GLFWwindow* im_primary_glfw_window ();

#endif // defined IM_TM_WIDGET_HPP
