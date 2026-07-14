
/******************************************************************************
 * MODULE     : im_chooser_widget.hpp
 * DESCRIPTION: ImGui 文件选择器 widget（Open / Save / Save as）。
 *              参考 Qt 的 qt_chooser_widget_rep，但使用原生平台文件对话框。
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_CHOOSER_WIDGET_HPP
#define IM_CHOOSER_WIDGET_HPP

#include "command.hpp"
#include "im_widget.hpp"
#include "message.hpp"

/*! ImGui 文件选择器。
    实现与 Qt 相同的 widget 协议：
    - SLOT_FILE / SLOT_DIRECTORY 设置默认文件与目录。
    - SLOT_KEYBOARD_FOCUS 触发弹窗。
    - SLOT_STRING_INPUT 查询选中的文件路径。
    - plain_window_widget 返回自身。
    当前仅在 WASM 平台实现异步文件对话框（HTML <input type="file"> /
   window.prompt
    + 浏览器下载）；桌面端（macOS/Windows/Linux）为 stub，不弹出真实对话框。 */
class im_chooser_widget_rep : public im_widget_rep {
protected:
  command cmd;       // Scheme 回调
  command quit;      // 关闭命令
  string  type;      // 对话框类型：action_open / action_save_as / image 等
  string  prompt;    // 标题/提示
  string  win_title; // 由 plain_window_widget 设置

  string directory; // 由 SLOT_DIRECTORY 设置
  string file;      // 由 SLOT_FILE 设置，关闭后作为结果返回

  array<string> extensions; // 当前 type 允许的后缀列表
  string        def_ext;    // 保存时默认追加的后缀

public:
  im_chooser_widget_rep (command _cmd, string _type, string _prompt);

  virtual void     send (slot s, blackbox val);
  virtual blackbox query (slot s, int type_id);
  virtual widget   read (slot s, blackbox index);
  virtual widget   plain_window_widget (string s, command q, int b= 3);

  bool set_type (const string& _type);
  void perform_dialog ();

  /// WASM 异步回调入口：用户选择/取消后由 JS 调用。
  void on_cancel ();
  void on_select (string path, bool save_mode);
};

/// 平台无关的文件对话框入口。由 im_chooser_widget_rep 调用。
/// save_mode 为 true 表示保存对话框，否则为打开对话框。
bool im_show_file_dialog (string title, bool save_mode, string directory,
                          string filename, array<string> exts,
                          string& out_path);

#endif // defined IM_CHOOSER_WIDGET_HPP
