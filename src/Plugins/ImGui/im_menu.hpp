
/******************************************************************************
 * MODULE     : im_menu.hpp
 * DESCRIPTION: ImGui 即时模式菜单节点与右键弹出菜单容器。
 *              参考 Qt 的 qt_ui_element_rep / qt_menu_rep，但把"编译出 QAction"
 *              换成"每帧按 kind 递归渲染"。
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_MENU_HPP
#define IM_MENU_HPP

#include "command.hpp"
#include "im_widget.hpp"
#include "promise.hpp"
#include "widget.hpp"

/*! 即时模式菜单节点。
    对应 Qt 的 qt_ui_element_rep（单类 + types 枚举 + 递归 as_qaction），
    区别在于 ImGui 是 Immediate Mode：没有持久句柄，每帧由 render_node 按
    kind 分支调用 BeginMenu/MenuItem/Separator 等绘制。一个节点可表示容器、
    命令按钮、惰性子菜单、分隔符、分组标题或纯文本标签。 */
class im_menu_rep : public im_widget_rep {
public:
  enum kind_t {
    k_container, // horizontal_menu / vertical_menu / vertical_list / tile_menu
                 // / minibar_menu
    k_submenu,   // pulldown_button / pullright_button（惰性 promise 子菜单）
    k_button,    // menu_button（命令按钮）
    k_separator, // menu_separator
    k_group,     // menu_group（灰色居中标题）
    k_text       // text_widget（仅作标签来源）
  };

  kind_t kind;
  string label; // cork 原文：k_text/k_group 自身，k_button/k_submenu 取自内嵌
                // text_widget
  command         cmd;      // k_button 的命令
  string          pre;      // k_button 前缀："o"(单选) / "*"(勾选) / "v"
  string          ks;       // k_button 快捷键（仅显示，不注册全局快捷键）
  int             style;    // WIDGET_STYLE_* 标志（INERT 禁用、PRESSED 选中…）
  promise<widget> sub;      // k_submenu 的惰性子菜单
  bool            vertical; // k_separator 方向

  im_menu_rep (kind_t k)
      : im_widget_rep (none), kind (k), style (0), vertical (false) {}

  /// 经 i18n 翻译 + cork→utf8 后的可显示标签
  string display_label () const;

  /// 子节点访问：基类 children 为
  /// protected，渲染函数非成员无法直接访问，经此暴露。
  array<widget>& menu_children () { return children; }
};

/*! 右键上下文弹出菜单容器。
    持有一个 vertical_menu 根，处理弹出协议 (SLOT_MOUSE_GRAB / SLOT_VISIBILITY /
    SLOT_POSITION)，向全局"活动 popup"注册自身，再由帧循环用 ImGui::BeginPopup
    渲染。Qt 用 QMenu::exec() 阻塞显示；ImGui 不能阻塞，故改为帧间持续渲染 +
    在打开期间抑制编辑器的鼠标分发（见 im_tm_widget 的 GLFW 回调）。 */
class im_popup_rep : public im_widget_rep {
public:
  widget menu;         // 待渲染的 vertical_menu 根
  float  pos_x, pos_y; // 屏幕坐标（px），打开时取鼠标位置
  bool   just_opened;  // 下一帧需调用 OpenPopup

  im_popup_rep (widget m)
      : im_widget_rep (none), menu (m), pos_x (0), pos_y (0),
        just_opened (false) {
    add_child (m);
  }

  virtual void   send (slot s, blackbox val);
  virtual widget popup_window_widget (string s);
};

// —— 渲染入口（由 im_tm_widget 帧循环调用）——

/// 渲染主菜单条，内部包裹 BeginMainMenuBar/EndMainMenuBar
void im_render_main_menu (widget root);

/// 渲染全局活动弹出菜单；若用户已关闭则清空活动 popup
void im_render_active_popup ();

/// 当前是否存在活动弹出菜单（供鼠标回调判断是否抑制编辑器分发）
bool im_has_active_popup ();

/// 活动弹出菜单激消（由 im_popup_rep::send 调用）
void im_activate_popup (im_popup_rep* p);
void im_deactivate_popup (im_popup_rep* p);

/// 菜单命令入队 / 执行（遍历菜单树期间只入队，结束后统一 flush，避免重入）
void im_queue_menu_command (command cmd);
void im_flush_menu_commands ();

#endif // defined IM_MENU_HPP
