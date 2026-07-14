
/******************************************************************************
 * MODULE     : im_chooser_widget.cpp
 * DESCRIPTION: ImGui 文件选择器 widget 的跨平台实现（非 macOS 部分）。
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "im_chooser_widget.hpp"

#include <cstdio> // popen / fgets / pclose

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "analyze.hpp"    // starts, ends
#include "convert.hpp"    // strip
#include "converter.hpp"  // utf8_to_cork, cork_to_utf8
#include "dictionary.hpp" // translate
#include "file.hpp"       // url_system, as_system_string
#include "moebius/data/scheme.hpp"
#include "scheme.hpp" // call, exec_delayed
#include "string.hpp"
#include "sys_utils.hpp" // get_env

using moebius::data::scm_quote;

/******************************************************************************
 * 构造函数
 ******************************************************************************/

im_chooser_widget_rep::im_chooser_widget_rep (command _cmd, string _type,
                                              string _prompt)
    : im_widget_rep (im_widget_rep::none), cmd (_cmd), prompt (_prompt),
      directory (""), file ("#f") {
  if (!set_type (_type)) set_type ("generic");
}

/******************************************************************************
 * WASM 文件对话框实现（异步）
 ******************************************************************************/

#ifdef __EMSCRIPTEN__

static im_chooser_widget_rep* g_active_chooser= nullptr;

extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_wasm_file_cancelled () {
  if (g_active_chooser == nullptr) return;
  g_active_chooser->on_cancel ();
}

static void
im_wasm_start_open_dialog (array<string>& exts) {
  string accept;
  for (int i= 0; i < N (exts); ++i) {
    if (i > 0) accept= accept * ",";
    accept= accept * "." * exts[i];
  }
  c_string accept_exts (accept);
  EM_ASM (
      {
        var acceptExts     = UTF8ToString ($0);
        var input          = document.createElement ('input');
        input.type         = 'file';
        input.accept       = acceptExts;
        input.style.display= 'none';
        document.body.appendChild (input);
        input.onchange= function (e) {
          document.body.removeChild (input);
          var file= e.target.files[0];
          if (!file) {
            ccall ('mogan_wasm_file_cancelled', null, [], []);
            return;
          }
          var reader   = new FileReader ();
          reader.onload= function (ev) {
            var data= new Uint8Array (ev.target.result);
            var path= '/tmp/mogan_open_' + file.name;
            try {
              FS.mkdir ('/tmp');
            } catch (err) {
            }
            FS.writeFile (path, data);
            ccall ('mogan_wasm_file_selected', null, [ 'string', 'number' ],
                   [ path, 0 ]);
          };
          reader.readAsArrayBuffer (file);
        };
        input.click ();
      },
      (char*) accept_exts);
}

static void
im_wasm_start_save_dialog (const string& filename) {
  string   default_name= (N (filename) > 0) ? filename : "document.tmu";
  c_string def_name (default_name);
  EM_ASM (
      {
        var defaultName= UTF8ToString ($0);
        var name       = window.prompt ("Save as:", defaultName);
        if (!name) {
          ccall ('mogan_wasm_file_cancelled', null, [], []);
          return;
        }
        var path= '/tmp/mogan_save_' + name;
        try {
          FS.mkdir ('/tmp');
        } catch (err) {
        }
        ccall ('mogan_wasm_file_selected', null, [ 'string', 'number' ],
               [ path, 1 ]);
      },
      (char*) def_name);
}

static void
im_wasm_download_file (const string& path, const string& filename) {
  c_string p (path);
  c_string n (filename);
  EM_ASM (
      {
        var path    = UTF8ToString ($0);
        var filename= UTF8ToString ($1);
        setTimeout (
            function () {
              try {
                var data= FS.readFile (path);
                var blob= new Blob ([data], {
                  type:
                    'application/octet-stream'
                });
                var url   = URL.createObjectURL (blob);
                var a     = document.createElement ('a');
                a.href    = url;
                a.download= filename;
                a.click ();
                URL.revokeObjectURL (url);
              } catch (e) {
                console.warn ('WASM download failed:', e);
              }
            },
            500);
      },
      (char*) p, (char*) n);
}

extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_wasm_file_selected (const char* path, int save_mode) {
  if (g_active_chooser == nullptr || path == nullptr) return;
  g_active_chooser->on_select (string (path), save_mode != 0);
}
#endif // __EMSCRIPTEN__

/******************************************************************************
 * 类型与过滤器设置
 ******************************************************************************/

bool
im_chooser_widget_rep::set_type (const string& _type) {
  extensions= array<string> ();
  def_ext   = "";

  if (_type == "directory") {
    type= _type;
    return true;
  }

  if (_type == "image") {
    type= _type;
    extensions << string ("jpg") << string ("jpeg") << string ("jpe")
               << string ("png") << string ("tif") << string ("tiff")
               << string ("svg") << string ("pdf") << string ("webp");
    return true;
  }

  if (_type == "text") {
    type= _type;
    extensions << string ("txt");
    return true;
  }

  if (_type == "action_open") {
    type= _type;
    extensions << string ("tmu") << string ("tm") << string ("ts")
               << string ("tp") << string ("stem") << string ("pdf")
               << string ("json") << string ("csv") << string ("md")
               << string ("txt");
    return true;
  }

  if (_type == "action_save_as") {
    type= _type;
    extensions << string ("tmu");
    def_ext= "tmu";
    return true;
  }

  if (_type == "action_include") {
    type= _type;
    extensions << string ("tmu") << string ("tm");
    return true;
  }

  if (_type == "generic") {
    type= _type;
    return true;
  }

  // 尝试从 format 注册表读取后缀（参考 Qt 的 format-get-suffixes*）
  object        ret     = call ("format-get-suffixes*", _type);
  array<object> suffixes= as_array_object (ret);
  if (N (suffixes) > 1) {
    type= _type;
    for (int i= 1; i < N (suffixes); ++i)
      extensions << as_string (suffixes[i]);
    if (N (suffixes) > 1) def_ext= as_string (suffixes[1]);
    return true;
  }

  return false;
}

/******************************************************************************
 * Widget 协议
 ******************************************************************************/

void
im_chooser_widget_rep::send (slot s, blackbox val) {
  switch (s) {
  case SLOT_VISIBILITY:
    // 文件选择器为一次性的模态对话框，忽略可见性变化。
    break;
  case SLOT_SIZE:
  case SLOT_POSITION:
    // 原生对话框忽略尺寸/位置。
    break;
  case SLOT_KEYBOARD_FOCUS:
    // 获得焦点时触发弹窗。
    perform_dialog ();
    break;
  case SLOT_STRING_INPUT:
    // 不处理。
    break;
  case SLOT_INPUT_TYPE:
    set_type (open_box<string> (val));
    break;
  case SLOT_FILE:
    file= open_box<string> (val);
    break;
  case SLOT_DIRECTORY: {
    string d = open_box<string> (val);
    directory= as_string (url_pwd () * url_system (d));
  } break;
  default:
    im_widget_rep::send (s, val);
  }
}

blackbox
im_chooser_widget_rep::query (slot s, int type_id) {
  (void) type_id;
  switch (s) {
  case SLOT_POSITION:
    return close_box<coord2> (coord2 (0, 0));
  case SLOT_SIZE:
    return close_box<coord2> (coord2 (800 * PIXEL, 600 * PIXEL));
  case SLOT_STRING_INPUT:
    return close_box<string> (file);
  default:
    return im_widget_rep::query (s, type_id);
  }
}

widget
im_chooser_widget_rep::read (slot s, blackbox index) {
  (void) index;
  switch (s) {
  case SLOT_WINDOW:
  case SLOT_FORM_FIELD:
  case SLOT_FILE:
  case SLOT_DIRECTORY:
    return this;
  default:
    return im_widget_rep::read (s, index);
  }
}

widget
im_chooser_widget_rep::plain_window_widget (string s, command q, int b) {
  (void) b;
  win_title= s;
  quit     = q;
  return this;
}

/******************************************************************************
 * 对话框执行
 ******************************************************************************/

static bool
im_is_save_mode (const string& type, const string& prompt) {
  if (N (prompt) > 0) return true;
  return type == "action_save_as" || type == "action_include";
}

static string
im_format_result (const string& type, const string& path) {
  if (type == "directory") {
    return "(system->url " * scm_quote (path) * ")";
  }
  // 普通文件直接返回 (system->url "path")
  return "(system->url " * scm_quote (path) * ")";
}

void
im_chooser_widget_rep::perform_dialog () {
  file= "#f";

  bool   save_mode= im_is_save_mode (type, prompt);
  string title    = (N (win_title) > 0) ? win_title : prompt;
  if (N (title) == 0)
    title= save_mode
               ? translate ((string) "Save", "english", get_output_language ())
               : translate ((string) "Open", "english", get_output_language ());

  string filename= "";
  if (type != "directory") {
    url u= url_system (file);
    if (is_scratch (u)) filename= "";
    else filename= as_system_string (u);
  }

#ifdef __EMSCRIPTEN__
  if (g_active_chooser != nullptr) {
    // 已有未完成的 WASM 对话框，先取消旧的。
    im_chooser_widget_rep* old= g_active_chooser;
    g_active_chooser          = nullptr;
    old->on_cancel ();
  }
  g_active_chooser= this;
  if (save_mode) im_wasm_start_save_dialog (filename);
  else im_wasm_start_open_dialog (extensions);
  return;
#endif

  string out_path;
  bool   ok= im_show_file_dialog (title, save_mode, directory, filename,
                                  extensions, out_path);

  if (ok && !is_empty (out_path)) {
    // 保存模式下若无后缀且指定了默认后缀，则追加后缀。
    if (save_mode && !is_empty (def_ext)) {
      int dot_pos= -1;
      for (int i= N (out_path) - 1; i >= 0; --i) {
        if (out_path[i] == '/') break;
        if (out_path[i] == '.') {
          dot_pos= i;
          break;
        }
      }
      if (dot_pos < 0) out_path= out_path * "." * def_ext;
    }
    file= im_format_result (type, out_path);
  }

  cmd ();
  if (!is_nil (quit)) quit ();
}

void
im_chooser_widget_rep::on_cancel () {
  g_active_chooser= nullptr;
  file            = "#f";
  if (!is_nil (cmd)) cmd ();
  if (!is_nil (quit)) quit ();
}

void
im_chooser_widget_rep::on_select (string path, bool save_mode) {
  g_active_chooser= nullptr;
  file            = "(system->url " * scm_quote (path) * ")";
  cout << cmd << LF;
  if (!is_nil (cmd)) cmd ();
  if (!is_nil (quit)) quit ();
  if (save_mode) {
    // 保存/另存为：在回调保存完成后触发浏览器下载。
    string name= "";
    int    len = N (path);
    for (int i= len - 1; i >= 0; --i) {
      if (path[i] == '/') {
        name= path (i + 1, len);
        break;
      }
    }
    if (is_empty (name)) name= "document.tmu";
    im_wasm_download_file (path, name);
  }
}

/******************************************************************************
 * 平台文件对话框入口（桌面端 stub，真实实现仅在 WASM 的异步回调中）
 ******************************************************************************/

bool
im_show_file_dialog (string title, bool save_mode, string directory,
                     string filename, array<string> exts, string& out_path) {
  out_path= "";
  (void) title;
  (void) save_mode;
  (void) directory;
  (void) filename;
  (void) exts;
  // 桌面端当前为 stub，不实现模态文件对话框。
  return false;
}
