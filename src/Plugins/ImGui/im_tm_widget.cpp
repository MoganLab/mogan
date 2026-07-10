
/******************************************************************************
 * MODULE     : im_tm_widget.cpp
 * DESCRIPTION: The main TeXmacs editor window for the ImGui.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "im_tm_widget.hpp"

#include "message.hpp"
#include "mupdf_picture.hpp" // fz_pixmap_* (via <mupdf/fitz.h>) + mupdf_context
#include "object.hpp"        // object (Scheme value, for zoom-in call)
#include "picture.hpp"       // native_picture
#include "renderer.hpp"      // picture_renderer
#include "scheme.hpp"        // exec_pending_commands, call
#include "tm_timer.hpp"      // texmacs_time

#include "im_gui.hpp" // im_interpose (drives apply_changes each frame)
#include "im_input.hpp"
#include "im_simple_widget.hpp" // editor canvas (main_widget)

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#ifndef __EMSCRIPTEN__
#include "backends/imgui_impl_opengl3_loader.h"
#endif
#include "imgui.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // defines GLFWwindow, drags in system OpenGL headers

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <cmath>
#include <cstdint>
#include <cstdio>

static int s_next_win_id= 0; // unique non-zero identifiers for SLOT_IDENTIFIER

static im_tm_widget_rep* im_primary_window= nullptr;

/******************************************************************************
 * Local helpers
 ******************************************************************************/

// Upload an MuPDF pixmap to a GL texture
static unsigned int
upload_texture (fz_pixmap* pix) {
  unsigned int tex= 0;
  glGenTextures (1, &tex);
  glBindTexture (GL_TEXTURE_2D, tex);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
                fz_pixmap_width (mupdf_context (), pix),
                fz_pixmap_height (mupdf_context (), pix), 0, GL_RGBA,
                GL_UNSIGNED_BYTE, fz_pixmap_samples (mupdf_context (), pix));
  glBindTexture (GL_TEXTURE_2D, 0);
  return tex;
}

void
im_tm_widget_rep::render_editor () {
  if (is_nil (main_widget)) return;
  // Render at *device pixel* resolution (the framebuffer size) so the result is
  // retina-crisp.
  // Note:
  //   mupdf_qt_simple_widget:
  //     backing store = retina_factor × logical size,
  //     renderer zoom = std_shrinkf × retina_factor.
  //
  int win_w= 0, win_h= 0;
  glfwGetWindowSize (window, &win_w, &win_h);
  int fb_w= 0, fb_h= 0;
  glfwGetFramebufferSize (window, &fb_w, &fb_h);
  if (win_w <= 0 || win_h <= 0 || fb_w <= 0 || fb_h <= 0) return;

  int rf= (int) lround ((double) fb_w / (double) win_w);
  if (rf < 1) rf= 1;
  if (rf != get_retina_factor ()) set_retina_factor (rf);

  if (is_nil (the_picture) || fb_w != pic_w || fb_h != pic_h || rf != pic_rf) {
    the_picture= native_picture (fb_w, fb_h, 0, 0);
    pic_w      = fb_w;
    pic_h      = fb_h;
    pic_rf     = rf;
  }
  picture& pic= the_picture;
  renderer ren= picture_renderer (pic, std_shrinkf * rf);

  SI sx= 0, sy= 0;
  if (!is_nil (main_widget)) get_scroll_position (main_widget, sx, sy);
  ren->set_origin (-sx, -sy);
  SI x1= 0, y1= 0, x2= fb_w, y2= fb_h;
  ren->encode (x1, y1);
  ren->encode (x2, y2);
  ren->set_clipping (x1, y2, x2, y1);

  im_simple_widget_rep* ed=
      static_cast<im_simple_widget_rep*> (main_widget.rep);
  // Note: for now only the real editor (edit_interface_rep) renders content
  if (!ed->is_editor_widget ()) {
    tm_delete (ren);
    return;
  }
  ed->handle_repaint (ren, x1, y2, x2, y1);

  tm_delete (ren);

  mupdf_picture_rep* rep= (mupdf_picture_rep*) pic->get_handle ();
  if (rep == nullptr || rep->pix == nullptr) return;
  if (document_texture) glDeleteTextures (1, &document_texture);
  document_texture= upload_texture (rep->pix);
  // Note: tex_w/tex_h are the texture's device-pixel size; the host ImGui
  // window displays it at the (smaller) screen-coordinate size, so OpenGL
  // downsamples a retina-resolution texture -> crisp on screen.
  tex_w        = fb_w;
  tex_h        = fb_h;
  texture_dirty= false;
}

bool
im_tm_widget_rep::visibility_index (slot s, int& i) {
  switch (s) {
  case SLOT_HEADER_VISIBILITY:
    i= 0;
    return true;
  case SLOT_MAIN_ICONS_VISIBILITY:
    i= 1;
    return true;
  case SLOT_MODE_ICONS_VISIBILITY:
    i= 2;
    return true;
  case SLOT_FOCUS_ICONS_VISIBILITY:
    i= 3;
    return true;
  case SLOT_USER_ICONS_VISIBILITY:
    i= 4;
    return true;
  case SLOT_FOOTER_VISIBILITY:
    i= 5;
    return true;
  case SLOT_SIDE_TOOLS_VISIBILITY:
    i= 6;
    return true;
  case SLOT_LEFT_TOOLS_VISIBILITY:
    i= 7;
    return true;
  case SLOT_BOTTOM_TOOLS_VISIBILITY:
    i= 8;
    return true;
  case SLOT_EXTRA_TOOLS_VISIBILITY:
    i= 9;
    return true;
  case SLOT_TAB_PAGES_VISIBILITY:
    i= 10;
    return true;
  case SLOT_AUXILIARY_WIDGET_VISIBILITY:
    i= 11;
    return true;
  default:
    return false;
  }
}

/******************************************************************************
 * Constructor and destructor
 ******************************************************************************/

im_tm_widget_rep::im_tm_widget_rep (int mask, command _quit)
    : im_widget_rep (im_widget_rep::texmacs_widget), win_id (++s_next_win_id),
      orig_name ("popup"), quit (_quit), zoomf (1.0), window (nullptr),
      document_texture (0), tex_w (0), tex_h (0), texture_dirty (true),
      initialized (false), needs_refocus (false), pic_w (0), pic_h (0),
      pic_rf (0) {
  visibility[0] = (mask & 1) == 1;       // header
  visibility[1] = (mask & 2) == 2;       // main icons
  visibility[2] = (mask & 4) == 4;       // mode icons
  visibility[3] = (mask & 8) == 8;       // focus icons
  visibility[4] = (mask & 16) == 16;     // user icons
  visibility[5] = (mask & 32) == 32;     // footer
  visibility[6] = (mask & 64) == 64;     // side tools
  visibility[7] = (mask & 128) == 128;   // left tools
  visibility[8] = (mask & 256) == 256;   // bottom tools
  visibility[9] = (mask & 512) == 512;   // extra tools
  visibility[10]= (mask & 1024) == 1024; // tab pages
  visibility[11]= (mask & 2048) == 2048; // auxiliary widget

  // Note:
  // Dear ImGui (GLFW backend) is single-window per ImGui context: a second
  // ImGui_ImplGlfw_InitForOpenGL would trip IM_ASSERT(BackendPlatformUserData)
  // and abort.
  if (im_primary_window != nullptr) {
    // set need_refocus, so when calls (like cmd-f cmd-r) happens, regain the
    // focus
    im_primary_window->needs_refocus= true;
    return;
  }

// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  const char* glsl_version= "#version 100";
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  const char* glsl_version= "#version 300 es";
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  const char* glsl_version= "#version 150";
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char* glsl_version= "#version 130";
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  float main_scale=
      ImGui_ImplGlfw_GetContentScaleForMonitor (glfwGetPrimaryMonitor ());
  const int    win_w  = 800; // size for init
  const int    win_h  = 600;
  GLFWmonitor* monitor= glfwGetPrimaryMonitor ();
  int          wa_x= 0, wa_y= 0, wa_w= win_w, wa_h= win_h;
  if (monitor != nullptr)
    glfwGetMonitorWorkarea (monitor, &wa_x, &wa_y, &wa_w, &wa_h);
  window= glfwCreateWindow ((int) win_w * main_scale, (int) win_h * main_scale,
                            "Mogan (ImGui)", nullptr, nullptr);
  if (window == nullptr) {
    return;
  }
  glfwMakeContextCurrent (window);
#ifndef __EMSCRIPTEN__
  glfwSwapInterval (1);
#endif

  // Match retina factor
  float xscale= 1.0f, yscale= 1.0f;
  glfwGetWindowContentScale (window, &xscale, &yscale);
  int rf0= (int) lround (xscale);
  if (rf0 < 1) rf0= 1;
  set_retina_factor (rf0);

  // Register our input callbacks before ImGui_ImplGlfw_InitForOpenGL installs
  // its own with install_callbacks=true
  glfwSetWindowUserPointer (window, this);
  glfwSetKeyCallback (window, &im_tm_widget_rep::glfw_key_callback);
  glfwSetCharCallback (window, &im_tm_widget_rep::glfw_char_callback);
  glfwSetMouseButtonCallback (window,
                              &im_tm_widget_rep::glfw_mouse_button_callback);
  glfwSetCursorPosCallback (window,
                            &im_tm_widget_rep::glfw_cursor_pos_callback);
  glfwSetScrollCallback (window, &im_tm_widget_rep::glfw_scroll_callback);
  glfwSetWindowFocusCallback (window,
                              &im_tm_widget_rep::glfw_window_focus_callback);

  IMGUI_CHECKVERSION ();
  ImGui::CreateContext ();
  io= &ImGui::GetIO ();
  io->ConfigFlags|= ImGuiConfigFlags_NavEnableKeyboard;
  io->ConfigFlags|= ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark ();
  ImGuiStyle& style= ImGui::GetStyle ();
  style.ScaleAllSizes (main_scale);
  style.FontScaleDpi= main_scale;

  ImGui_ImplGlfw_InitForOpenGL (window, true);
#ifdef __EMSCRIPTEN__
  ImGui_ImplGlfw_InstallEmscriptenCallbacks (window, "#canvas");
#endif
  ImGui_ImplOpenGL3_Init (glsl_version);

  initialized= true;
  if (im_primary_window == nullptr) im_primary_window= this;
}

im_tm_widget_rep::~im_tm_widget_rep () {
  shutdown_context ();
  if (im_primary_window == this) im_primary_window= nullptr;
}

void
im_tm_widget_rep::shutdown_context () {
  if (!initialized) return;
  initialized= false;
  if (document_texture) {
    glDeleteTextures (1, &document_texture);
    document_texture= 0;
  }
  ImGui_ImplOpenGL3_Shutdown ();
  ImGui_ImplGlfw_Shutdown ();
  ImGui::DestroyContext ();
  glfwDestroyWindow (window);
  // Note: glfwTerminate() is process-global and owned by gui_close() (which
  // runs after the main loop returns). GLFW init 和 termination 都在 im_gui
  // 定义的周期中
  window= nullptr;
}

/******************************************************************************
 * Accessors
 ******************************************************************************/

void
im_tm_widget_rep::set_box (box b, double zoom_factor) {
  the_box      = b;
  zoomf        = zoom_factor;
  texture_dirty= true;
}

widget
im_tm_widget_rep::plain_window_widget (string name, command _quit, int b) {
  // same as qt_tm_widget_rep::plain_window_widget
  (void) b;
  (void) _quit;
  orig_name= name;
  return this;
}

/******************************************************************************
 * The main loop (cf. main.cpp's while-loop)
 ******************************************************************************/

void
im_tm_widget_rep::im_main_loop () {
  glfwPollEvents ();
  // 创建了一个 stub 窗口（例如 Cmd+F 的查找窗口等）时 server
  // 将当前窗口从我们这里切换走， 编辑器被 suspend () 挂起（got_focus=false）
  // 在这里重新恢复这两种焦点状态，以确保后续的键盘和鼠标事件能够继续正常传递。
  // 此代码会在创建该占位窗口的 Scheme 处理函数返回之后执行
  // （即 glfwPollEvents 已经分发了 Cmd+F 的按键事件）。
  if (needs_refocus) {
    needs_refocus= false;
    if (window) glfwFocusWindow (window);
    im_simple_widget_rep* ed= canvas ();
    if (ed) ed->handle_keyboard_focus (true, texmacs_time ());
  }
  // Auto-scroll while dragging (left button held) near the top/bottom edge.
  // ImGui 实现的拖动滚动逻辑
  if (!is_nil (main_widget) && window != nullptr &&
      glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    double xpos= 0, ypos= 0;
    glfwGetCursorPos (window, &xpos, &ypos);
    int ww= 0, wh= 0;
    glfwGetWindowSize (window, &ww, &wh);
    const double EDGE= 48.0;            // px proximity band
    const SI MAXD= (SI) (80.0 * PIXEL); // cap per-frame scroll to avoid jitter
    SI       sx= 0, sy= 0;
    get_scroll_position (main_widget, sx, sy);
    SI new_sy= sy;
    if (wh > 0 && ypos < EDGE) {
      SI d= (SI) ((EDGE - ypos) * PIXEL);
      if (d > MAXD) d= MAXD;
      new_sy= sy + d; // near/over top → up
    }
    else if (wh > 0 && ypos > (double) wh - EDGE) {
      SI d= (SI) ((ypos - ((double) wh - EDGE)) * PIXEL);
      if (d > MAXD) d= MAXD;
      new_sy= sy - d; // near/over bottom → down
    }
    if (new_sy != sy) {
      main_widget->send (SLOT_SCROLL_POSITION,
                         close_box<coord2> (coord2 (sx, new_sy)));
      // Re-dispatch a move so the editor's drag selection tracks the
      // viewport. Clamp ypos to [0, wh].
      double cy= ypos;
      if (cy < 0.0) cy= 0.0;
      if (wh > 0 && cy > (double) wh) cy= (double) wh;
      double cx= xpos;
      if (cx < 0.0) cx= 0.0;
      if (ww > 0 && cx > (double) ww) cx= (double) ww;
      SI msx= 0, msy= 0, nsx= 0, nsy= 0;
      screen_to_si (cx, cy, msx, msy);
      get_scroll_position (main_widget, nsx, nsy);
      dispatch_mouse ("move", msx + nsx, msy + nsy, 1);
    }
  }

  // Drive one TeXmacs tick. 驱动编辑器。
  im_interpose ();
  if (glfwGetWindowAttrib (window, GLFW_ICONIFIED) != 0) {
    ImGui_ImplGlfw_Sleep (10);
    return;
  }

  ImGui_ImplOpenGL3_NewFrame ();
  ImGui_ImplGlfw_NewFrame ();
  ImGui::NewFrame ();
  // Keep the editor canvas informed of the visible size (Qt does this via
  // resize events); otherwise update_visible()/SLOT_VISIBLE_PART report a
  // zero-sized region and the editor has nothing to render.
  static int last_cw= 0, last_ch= 0;
  if (!is_nil (main_widget)) {
    int cw= 0, ch= 0;
    glfwGetWindowSize (window, &cw, &ch);
    if (cw > 0 && ch > 0 && (cw != last_cw || ch != last_ch)) {
      last_cw= cw;
      last_ch= ch;
      main_widget->send (SLOT_SIZE, close_box<coord2> (coord2 (
                                        (SI) (cw * PIXEL), (SI) (ch * PIXEL))));
      texture_dirty= true; // force a re-rasterization at the new canvas size
    }
  }

  static SI last_sx= 0, last_sy= 0;
  if (!is_nil (main_widget)) {
    SI sx= 0, sy= 0;
    get_scroll_position (main_widget, sx, sy);
    if (sx != last_sx || sy != last_sy) {
      last_sx      = sx;
      last_sy      = sy;
      texture_dirty= true;
    }
  }

  // Re-render only when something actually changed: either the window was
  // invalidated (texture_dirty, set by windows_refresh / SLOT_INVALIDATE_*),
  // or the editor canvas reports invalid regions.
  if (!is_nil (main_widget)) {
    if (texture_dirty || query_invalid (main_widget)) render_editor ();
  }

#ifdef LIII_DEBUG
  {
    // show FPS
    ImGui::Begin ("FPS");
    ImGui::Text ("Application average %.3f ms/frame (%.1f FPS)",
                 1000.0f / io->Framerate, io->Framerate);
    ImGui::End ();
  }
#endif

  // Display the canvas
  int win_w= 0, win_h= 0;
  glfwGetWindowSize (window, &win_w, &win_h);
  int fb_w= 0, fb_h= 0;
  glfwGetFramebufferSize (window, &fb_w, &fb_h);
  ImGui::SetNextWindowPos (ImVec2 (0, 0));
  ImGui::SetNextWindowSize (ImVec2 ((float) win_w, (float) win_h));
  ImGui::PushStyleVar (ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar (ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar (ImGuiStyleVar_WindowPadding, ImVec2 (0, 0));
  {
    ImGui::Begin ("Mogan Canvas", nullptr,
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                      ImGuiWindowFlags_NoBringToFrontOnFocus |
                      ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar (3);
    if (document_texture != 0) {
      ImVec2 avail= ImGui::GetContentRegionAvail ();
      float  img_w= (float) tex_w;
      float  img_h= (float) tex_h;
      if (img_w > avail.x) img_w= avail.x;
      if (img_h > avail.y) img_h= avail.y;
      // Note: ImGui 系永远从 (0, 0) 开始，所有位移用 scroll_x, scroll_y 完成
      ImGui::Image ((ImTextureID) (intptr_t) document_texture,
                    ImVec2 (img_w, img_h));
    }
    else {
      ImGui::Text ("Editor canvas not yet wired.");
      ImGui::Text ("Close this window to quit.");
    }
    ImGui::End ();
  }

  ImGui::Render ();
  glViewport (0, 0, fb_w, fb_h);
  glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
  glClear (GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
  glfwSwapBuffers (window);
}

void
im_tm_widget_rep::em_main_loop_wrapper (void* arg) {
  auto* self= static_cast<im_tm_widget_rep*> (arg);
  self->im_main_loop ();
}

void
im_tm_widget_rep::run () {
  if (!initialized || window == nullptr) return;
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg (im_tm_widget_rep::em_main_loop_wrapper, this, 0,
                                true);
  emscripten_cancel_main_loop ();
#else
  while (!glfwWindowShouldClose (window)) {
    im_main_loop ();
  }
#endif
  shutdown_context ();
}

void
im_run_main_loop () {
  if (im_primary_window != nullptr) im_primary_window->run ();
}

// The primary window's GLFW handle
// Clipboard need this
GLFWwindow*
im_primary_glfw_window () {
  return (im_primary_window != nullptr) ? im_primary_window->glfw_window ()
                                        : nullptr;
}

im_tm_widget_rep*
im_self (GLFWwindow* w) {
  return static_cast<im_tm_widget_rep*> (glfwGetWindowUserPointer (w));
}

im_simple_widget_rep*
im_tm_widget_rep::canvas () {
  if (is_nil (main_widget)) return nullptr;
  auto* ed= static_cast<im_simple_widget_rep*> (main_widget.rep);
  return ed->is_editor_widget () ? ed : nullptr;
}

void
im_tm_widget_rep::screen_to_si (double xpos, double ypos, SI& sx, SI& sy) {
  (void) window;
  sx= (SI) (xpos * PIXEL);
  sy= (SI) (-ypos * PIXEL);
}

/******************************************************************************
 * Input forwarding (GLFW -> editor canvas)
 ******************************************************************************/

void
im_tm_widget_rep::dispatch_keypress (const string& key) {
  if (is_empty (key)) return;
  im_simple_widget_rep* ed= canvas ();
  if (ed == nullptr) return;
  ed->handle_keypress (key, texmacs_time ());
}

void
im_tm_widget_rep::dispatch_mouse (const string& kind, SI x, SI y, int mstate,
                                  array<double> data) {
  im_simple_widget_rep* ed= canvas ();
  if (ed == nullptr) return;
  ed->handle_mouse (kind, x, y, mstate, texmacs_time (), data);
}

void
im_tm_widget_rep::glfw_key_callback (GLFWwindow* w, int key, int scancode,
                                     int action, int mods) {
  im_tm_widget_rep* self= im_self (w);
  if (self == nullptr) return;
  string r= im_from_key_event (key, scancode, action, mods);
  self->dispatch_keypress (r);
}

void
im_tm_widget_rep::glfw_char_callback (GLFWwindow* w, unsigned int codepoint) {
  im_tm_widget_rep* self= im_self (w);
  if (self == nullptr) return;
  // Printable Unicode text input (the GLFW counterpart of Qt's inputMethodEvent
  // commit string). Skip control characters — they arrive via
  // glfw_key_callback.
  if (codepoint < 32) return;
  self->dispatch_keypress (im_from_char (codepoint));
}

void
im_tm_widget_rep::glfw_mouse_button_callback (GLFWwindow* w, int button,
                                              int action, int mods) {
  im_tm_widget_rep* self= im_self (w);
  if (self == nullptr || self->canvas () == nullptr) return;
  // im_mouse_state mirrors Qt's mouse_state(), including the macOS
  // Ctrl→right-click and Option→middle-click emulation.
  int    mstate= im_mouse_state (button, mods, action == GLFW_PRESS);
  string kind=
      (action == GLFW_PRESS ? "press-" : "release-") * im_mouse_decode (mstate);
  double xpos= 0, ypos= 0;
  glfwGetCursorPos (w, &xpos, &ypos);
  SI sx, sy;
  self->screen_to_si (xpos, ypos, sx, sy);
  SI scroll_x= 0;
  SI scroll_y= 0;
  get_scroll_position (self->main_widget, scroll_x, scroll_y);
  self->dispatch_mouse (kind, sx + scroll_x, sy + scroll_y, mstate);
}

void
im_tm_widget_rep::glfw_cursor_pos_callback (GLFWwindow* w, double xpos,
                                            double ypos) {
  im_tm_widget_rep* self= im_self (w);
  if (self == nullptr || self->canvas () == nullptr) return;
  // Only report drags while a button is held (matches Qt, which only acts on
  // mouseMoveEvent with buttons pressed).
  if (glfwGetMouseButton (w, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS &&
      glfwGetMouseButton (w, GLFW_MOUSE_BUTTON_MIDDLE) != GLFW_PRESS &&
      glfwGetMouseButton (w, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
    return;
  int mstate= 0;
  if (glfwGetMouseButton (w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) mstate|= 1;
  if (glfwGetMouseButton (w, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    mstate|= 2;
  if (glfwGetMouseButton (w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) mstate|= 4;
  SI sx, sy;
  self->screen_to_si (xpos, ypos, sx, sy);
  SI scroll_x= 0;
  SI scroll_y= 0;
  get_scroll_position (self->main_widget, scroll_x, scroll_y);
  self->dispatch_mouse ("move", sx + scroll_x, sy + scroll_y, mstate);
}

void
im_tm_widget_rep::glfw_scroll_callback (GLFWwindow* w, double xoffset,
                                        double yoffset) {
  im_tm_widget_rep* self= im_self (w);
  if (self == nullptr || self->canvas () == nullptr) return;
  // Mirror Qt's wheelEvent: Ctrl/Meta ⇒ zoom.
  bool zoom_mod= glfwGetKey (w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                 glfwGetKey (w, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
                 glfwGetKey (w, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
                 glfwGetKey (w, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
  if (zoom_mod) {
    double factor= (sqrt (sqrt (sqrt (2.0))));
    if (yoffset < 0) call ("zoom-out", object (factor));
    else call ("zoom-in", object (factor));
    return;
  }
  if (yoffset == 0.0) return;
  widget mw= self->main_widget;
  SI     sx= 0, sy= 0;
  get_scroll_position (mw, sx, sy);
  coord4 ce= open_box<coord4> (mw->query (SLOT_EXTENTS, 0));
  int    ww= 0, wh= 0;
  glfwGetWindowSize (w, &ww, &wh);
  SI canvas_h= (SI) wh * PIXEL;
  SI doc_h   = ce.x4 - ce.x2;
  SI delta   = (SI) (yoffset * 10.0 * PIXEL);
  SI new_sy  = sy + delta;
  if (doc_h > canvas_h) {
    if (new_sy > 0) new_sy= 0;
    if (-new_sy > doc_h - canvas_h) new_sy= -(doc_h - canvas_h);
  }
  else new_sy= 0; // whole document fits in the canvas → no vertical scroll
  mw->send (SLOT_SCROLL_POSITION, close_box<coord2> (coord2 (sx, new_sy)));
}

void
im_tm_widget_rep::glfw_window_focus_callback (GLFWwindow* w, int focused) {
  im_tm_widget_rep* self= im_self (w);
  if (self == nullptr) return;
  im_simple_widget_rep* ed= self->canvas ();
  if (ed == nullptr) return;
  ed->handle_keyboard_focus (focused != 0, texmacs_time ());
}

/******************************************************************************
 * Handling of TeXmacs' messages
 ******************************************************************************/

void
im_tm_widget_rep::send (slot s, blackbox val) {
  switch (s) {
  case SLOT_SIZE: {
    coord2 p= open_box<coord2> (val);
    if (window)
      glfwSetWindowSize (window, (int) (p.x1 / PIXEL), (int) (p.x2 / PIXEL));
  } break;
  case SLOT_POSITION: {
    coord2 p= open_box<coord2> (val);
    if (window)
      glfwSetWindowPos (window, (int) (p.x1 / PIXEL), (int) (p.x2 / PIXEL));
  } break;
  case SLOT_VISIBILITY: {
    bool flag= open_box<bool> (val);
    if (window) {
      if (flag) glfwShowWindow (window);
      else glfwHideWindow (window);
    }
  } break;
  case SLOT_NAME: {
    string name= open_box<string> (val);
    if (window) {
      c_string tmp (name);
      glfwSetWindowTitle (window, tmp);
    }
  } break;
  case SLOT_KEYBOARD_FOCUS: {
    bool focus= open_box<bool> (val);
    if (focus && window) glfwFocusWindow (window);
  } break;
  case SLOT_INVALIDATE:
  case SLOT_INVALIDATE_ALL:
    texture_dirty= true;
    break;
  case SLOT_ZOOM_FACTOR: {
    zoomf        = open_box<double> (val);
    texture_dirty= true;
    if (!is_nil (main_widget)) main_widget->send (s, val);
  } break;
  case SLOT_DESTROY: {
    if (!is_nil (quit)) quit ();
    if (window) glfwSetWindowShouldClose (window, GLFW_TRUE);
    im_widget_rep::send (s, val); // forward to tracked children
  } break;
  case SLOT_MODIFIED:
  case SLOT_FULL_SCREEN:
    // window-level only; no-op for the static viewer
    break;
  case SLOT_MOUSE_GRAB:
  case SLOT_MOUSE_POINTER:
  case SLOT_CURSOR:
  case SLOT_EXTENTS:
  case SLOT_SCROLL_POSITION:
    if (!is_nil (main_widget)) main_widget->send (s, val);
    break;
  default: {
    int vi;
    if (visibility_index (s, vi)) visibility[vi]= open_box<bool> (val);
    else im_widget_rep::send (s, val);
  }
  }
}

blackbox
im_tm_widget_rep::query (slot s, int type_id) {
  (void) type_id; // open_box<T> already asserts the caller's expected type
  switch (s) {
  case SLOT_SCROLL_POSITION:
  case SLOT_EXTENTS:
  case SLOT_VISIBLE_PART:
  case SLOT_ZOOM_FACTOR:
    // Canvas geometry is owned by the editor canvas (main_widget)
    if (!is_nil (main_widget)) return main_widget->query (s, type_id);
    if (s == SLOT_ZOOM_FACTOR) return close_box<double> (zoomf);
    if (s == SLOT_SCROLL_POSITION) return close_box<coord2> (coord2 (0, 0));
    return close_box<coord4> (coord4 (0, 0, 0, 0));
  case SLOT_IDENTIFIER:
    return close_box<int> (win_id);
  case SLOT_POSITION: {
    int x= 0, y= 0;
    if (window) glfwGetWindowPos (window, &x, &y);
    return close_box<coord2> (coord2 ((SI) (x * PIXEL), (SI) (y * PIXEL)));
  }
  case SLOT_SIZE: {
    int w= 800, h= 600;
    if (window) glfwGetWindowSize (window, &w, &h);
    return close_box<coord2> (coord2 ((SI) (w * PIXEL), (SI) (h * PIXEL)));
  }
  default: {
    int vi;
    if (visibility_index (s, vi)) return close_box<bool> (visibility[vi]);
    return im_widget_rep::query (s, type_id);
  }
  }
}

widget
im_tm_widget_rep::read (slot s, blackbox index) {
  (void) index;
  switch (s) {
  case SLOT_WINDOW:
    return this;
  case SLOT_CANVAS:
    return main_widget;
  default:
    return im_widget_rep::read (s, index);
  }
}

void
im_tm_widget_rep::write (slot s, blackbox index, widget w) {
  (void) index;
  switch (s) {
  case SLOT_SCROLLABLE:
    main_widget= w;
    // Tell the canvas which window owns it so get_window(editor) (used by
    // mouse_adjust on right-click, get_size for page sizing, etc.) resolves to
    // this im_tm_widget_rep instead of a nil widget.
    if (!is_nil (main_widget)) {
      im_simple_widget_rep* ed=
          static_cast<im_simple_widget_rep*> (main_widget.rep);
      ed->set_window (widget (this), win_id);
    }
    break;
  default:
    im_widget_rep::write (s, index, w);
  }
}
