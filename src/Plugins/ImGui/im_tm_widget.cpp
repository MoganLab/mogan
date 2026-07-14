
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

#include "analyze.hpp"   // starts, as_string
#include "converter.hpp" // utf8_to_cork
#include "cork.hpp"      // tm_string_length
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
#include <GLFW/emscripten_glfw3.h>
#include <emscripten.h>
#endif

#include <cmath>
#include <cstdint>
#include <cstdio>

static int s_next_win_id= 0; // unique non-zero identifiers for SLOT_IDENTIFIER

static im_tm_widget_rep* im_primary_window= nullptr;

/******************************************************************************
 * IME (Input Method Editor) support — Emscripten only.
 *
 * The community GLFW port (pongasoft/emscripten-glfw) has no IME handling: its
 * char callback is driven by keydown, which during IME composition reports the
 * raw letter (not "Process"), so GLFW inserts letters alongside the IME and
 * committed text never reaches the editor via the IME. We bypass GLFW: a hidden
 * real <textarea> is the IME editing host; its composition events feed the
 * editor, mirroring Qt's QTMWidget::inputMethodEvent. A synthetic focus event
 * on the canvas keeps GLFW's fFocused and the editor's got_focus true so the
 * normal key/mouse paths keep working.
 ******************************************************************************/
#ifdef __EMSCRIPTEN__
// Pending IME events enqueued by the JS composition listeners. Drained at the
// top of im_main_loop (same safe point as GLFW key events) because the DOM
// fires composition events asynchronously. Each entry is the final key string
// (a plain UTF-8 commit, or "pre-edit:<utf8>" rebuilt by the drain into the
// "pre-edit:<pos>:<cork>" form that key_press expects).
static array<string> g_ime_pending;

// Pull ccall() into scope for the EM_JS block below (no
// EXPORTED_RUNTIME_METHODS change needed).
EM_JS_DEPS (mogan_ime, "$ccall");

// Install (once) a hidden <textarea> as the IME editing host plus the browser
// composition listeners. Called from run() (after the GLFW window exists).
EM_JS (void, im_install_ime_listeners, (), {
  if (window.__moganImeInstalled) return;
  window.__moganImeInstalled= true;
  function preedit (s) {
    ccall ('mogan_ime_preedit', null, ['string'], [s == null ? '' : s]);
  }
  function commit (s) {
    ccall ('mogan_ime_commit', null, ['string'], [s == null ? '' : s]);
  }
  // canvas 不是 IME 编辑宿主（contentEditable 对 WebGL canvas 无效），浏览器
  // 不会在它上面激活中文输入法。用一个隐藏的真实 textarea 接住 composition。
  var imeInput= document.createElement ('textarea');
  imeInput.id = 'mogan-ime-input';
  imeInput.setAttribute ('autocomplete', 'off');
  imeInput.setAttribute ('autocorrect', 'off');
  imeInput.setAttribute ('autocapitalize', 'off');
  imeInput.setAttribute ('spellcheck', 'false');
  var st     = imeInput.style;
  st.position= 'fixed';
  st.top     = '0px';
  st.left    = '0px';
  st.width   = '1px';
  st.height  = '1px';
  st.opacity = '0';
  st.border  = 'none';
  st.padding = '0px';
  st.resize  = 'none';
  st.zIndex  = '-1';
  document.body.appendChild (imeInput);
  var canvas= document.getElementById ('main-canvas');
  // 聚焦 textarea 接住 IME。但这会让 canvas 失焦：contrib GLFW 的 fFocused
  // 跟踪 canvas 焦点，失焦后停止转发按键；编辑器 got_focus 也变 false
  // （apply_changes 与鼠标处理依赖它）。在 canvas 上派发合成 focus 事件恢复
  // 二者——GLFW 的 focus 监听挂在 canvas（bubble 阶段）且忽略事件数据，仅置
  // fFocused=true 并触发
  // glfw_window_focus_callback→handle_keyboard_focus(true)。
  function engageIme () {
    imeInput.focus ({preventScroll : true});
    canvas.dispatchEvent (new FocusEvent ('focus'));
  }
  engageIme ();
  // 点击 canvas 定位光标后，把焦点重新交给 textarea（setTimeout 等浏览器
  // 处理完 mousedown 的默认聚焦动作）。
  canvas.addEventListener (
      'mousedown', function () { setTimeout (engageIme, 0); });
  // textarea 的文本内容不使用（只用 composition），非 composition 时清空。
  imeInput.addEventListener (
      'input', function (e) {
        if (e.isComposing) return;
        imeInput.value= '';
      });
  var composing= false;
  // composition 事件冒泡到 window（textarea 聚焦时事件源自 textarea）。
  window.addEventListener (
      'compositionstart', function (e) {
        composing= true;
        preedit (e.data);
      });
  window.addEventListener (
      'compositionupdate', function (e) { preedit (e.data); });
  window.addEventListener (
      'compositionend', function (e) {
        composing= false;
        commit (e.data);
        preedit ('');
      });
  // 阻止 GLFW 在 IME composition 期间收到 keydown：浏览器对 IME 处理的按键报
  // keyCode=229（即便 key 仍是原始字母 'n'/'i'/' '）、且 isComposing=true。若
  // 放行，GLFW 的 char 回调会把字母/空格直插进文档，与最终提交的中文重复。
  // 在 textarea（事件 target）上 stopPropagation 即可拦住向 window 的冒泡；
  // 不 preventDefault，以便 IME 仍能接收按键。
  imeInput.addEventListener (
      'keydown', function (e) {
        if (composing || e.isComposing || e.keyCode === 229)
          e.stopPropagation ();
      });
});

// JS composition listener → here. Queue the UTF-8 commit text for dispatch.
extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_ime_commit (const char* utf8) {
  if (utf8 == nullptr) return;
  g_ime_pending << string (utf8);
}

// JS composition listener → here. Queue a pre-edit marker + raw UTF-8 text.
// The drain (im_main_loop) cork-converts and rebuilds the
// "pre-edit:<pos>:<text>" form with pos = char count so the caret lands at the
// end of the pre-edit (matching Qt). Disabled in math mode (avoids a
// QQPinyin-class crash). The browser CompositionEvent exposes no caret, so we
// always place it at the end (end-typing).
extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_ime_preedit (const char* utf8) {
  if (utf8 == nullptr) return;
  if (as_bool (call ("in-math?"))) return;
  g_ime_pending << ("pre-edit:" * string (utf8));
}
#endif // __EMSCRIPTEN__

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
  SI win_w, win_h;
  gui_root_extents (win_w, win_h);
  win_w               = win_w / PIXEL;
  win_h               = win_h / PIXEL;
  GLFWmonitor* monitor= glfwGetPrimaryMonitor ();
  int          wa_x= 0, wa_y= 0, wa_w= win_w, wa_h= win_h;
  if (monitor != nullptr)
    glfwGetMonitorWorkarea (monitor, &wa_x, &wa_y, &wa_w, &wa_h);
#ifdef __EMSCRIPTEN__
  glfwWindowHint (GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE); // Enable retina
  emscripten_glfw_set_next_window_canvas_selector ("#main-canvas");
#endif
  window= glfwCreateWindow ((int) win_w * main_scale, (int) win_h * main_scale,
                            "Mogan (ImGui)", nullptr, nullptr);
#ifdef __EMSCRIPTEN__
  emscripten_glfw_make_canvas_resizable (window, "window",
                                         nullptr); // emscripten 3.1.56 specific
#endif
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
#ifdef __EMSCRIPTEN__
  // Drain IME composition events queued by the JS listeners (see
  // mogan_ime_commit/preedit). Swap out the snapshot first so any events the
  // browser fires while we dispatch are processed next frame (single-threaded,
  // so no race).
  if (N (g_ime_pending) > 0) {
    array<string> pending= g_ime_pending;
    g_ime_pending        = array<string> (0);
    for (int i= 0; i < N (pending); i++) {
      // pre-edit 走立即路径：handle_keypress 会把它交给 delayed-keyboard-press
      // 去抖，而 ImGui 帧结构里 idle_time 难以达到阈值、去抖永不完成 → preedit
      // 不显示。这里直接调 keyboard-press（key-press→key_press）立即插入
      // preedit（与 Qt 延迟 apply 走同一路径、同一 context）。提交文本仍走
      // dispatch_keypress→handle_keypress（立即处理普通文本，含 cork 与 editing
      // 事务）。
      if (starts (pending[i], "pre-edit:")) {
        // 取 "pre-edit:" 之后的原始 UTF-8 文本，转 cork，按字符数重建为
        // "pre-edit:<n>:<cork>"，使 key_press 把光标放到 preedit 末尾（Qt
        // 行为）。
        string text= utf8_to_cork (pending[i](9, N (pending[i])));
        string key=
            "pre-edit:" * as_string (tm_string_length (text)) * ":" * text;
        call ("keyboard-press", object (key),
              object ((double) texmacs_time ()));
      }
      else dispatch_keypress (pending[i]);
    }
  }
#endif
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
  // Set up IME: hidden textarea + composition listeners + focus bridging.
  im_install_ime_listeners ();
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
