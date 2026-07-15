
/******************************************************************************
 * MODULE     : im_menu.cpp
 * DESCRIPTION: ImGui 即时模式菜单渲染 + 右键弹出菜单。
 *              参考 Qt 的 qt_ui_element_rep / qt_menu_rep / QTMCommand。
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "im_menu.hpp"

#include "converter.hpp"  // cork_to_utf8
#include "cork.hpp"       // tm_var_encode
#include "dictionary.hpp" // translate, get_input/output_language
#include "message.hpp"    // slot, blackbox, SLOT_*, open_box

#include "imgui.h"

/******************************************************************************
 * 标签
 ******************************************************************************/

string
im_menu_rep::display_label () const {
  // 与 Qt qt_translate 一致：先 i18n 翻译，再 tm_var_encode 处理特殊字形，
  // 最后 cork→utf8 供 ImGui 显示。
  string t= translate (label, get_input_language (), get_output_language ());
  return cork_to_utf8 (tm_var_encode (t));
}

/******************************************************************************
 * 命令延迟队列
 *   渲染菜单树期间点击的命令只入队，待遍历结束后由 im_flush_menu_commands
 *   统一执行——避免命令（可能触发 menu refresh / 切 buffer）在遍历过程中重入，
 *   对应 Qt 的 QTMCommand::apply → process_command 的延迟语义。
 ******************************************************************************/

static array<command> g_menu_cmd_q;

void
im_queue_menu_command (command cmd) {
  if (!is_nil (cmd)) g_menu_cmd_q << cmd;
}

void
im_flush_menu_commands () {
  if (N (g_menu_cmd_q) == 0) return;
  array<command> q= g_menu_cmd_q;
  // 先清空：命令回调里可能再次触发菜单并入队，避免无限增长
  g_menu_cmd_q= array<command> (0);
  for (int i= 0; i < N (q); ++i)
    if (!is_nil (q[i])) q[i]();
}

/******************************************************************************
 * 递归渲染
 ******************************************************************************/

static void
render_node (widget w) {
  im_menu_rep* m= dynamic_cast<im_menu_rep*> (w.rep);
  if (m == nullptr) return; // 非菜单节点（桩等）直接跳过
  switch (m->kind) {
  case im_menu_rep::k_container: {
    // horizontal_menu / vertical_menu / …：逐项渲染子节点
    array<widget>& kids= m->menu_children ();
    for (int i= 0; i < N (kids); ++i)
      render_node (kids[i]);
  } break;
  case im_menu_rep::k_submenu: {
    // pulldown / pullright：BeginMenu 展开惰性 promise 子菜单。
    // 每帧打开时 force promise（动态菜单不缓存，与 Qt QTMLazyMenu 一致）。
    c_string lbl (m->display_label ());
    bool     enabled= (m->style & WIDGET_STYLE_INERT) == 0;
    if (ImGui::BeginMenu ((const char*) lbl, enabled)) {
      if (!is_nil (m->sub)) render_node (m->sub ());
      ImGui::EndMenu ();
    }
  } break;
  case im_menu_rep::k_button: {
    c_string    lbl (m->display_label ());
    c_string    ks (m->ks);
    bool        selected= m->pre != "" || (m->style & WIDGET_STYLE_PRESSED);
    bool        enabled = (m->style & WIDGET_STYLE_INERT) == 0;
    const char* shortcut= (N (m->ks) > 0) ? (const char*) ks : nullptr;
    if (ImGui::MenuItem ((const char*) lbl, shortcut, selected, enabled)) {
      if (!is_nil (m->cmd)) im_queue_menu_command (m->cmd);
    }
  } break;
  case im_menu_rep::k_separator: {
    ImGui::Separator ();
  } break;
  case im_menu_rep::k_group: {
    c_string lbl (m->display_label ());
    ImGui::SeparatorText ((const char*) lbl);
  } break;
  case im_menu_rep::k_text: {
    // 菜单里独立出现的纯文本：按禁用样式显示
    c_string lbl (m->display_label ());
    ImGui::TextDisabled ("%s", (const char*) lbl);
  } break;
  }
}

void
im_render_main_menu (widget root) {
  im_menu_rep* m= dynamic_cast<im_menu_rep*> (root.rep);
  if (m == nullptr) return;
  if (ImGui::BeginMainMenuBar ()) {
    array<widget>& kids= m->menu_children ();
    for (int i= 0; i < N (kids); ++i)
      render_node (kids[i]);
    ImGui::EndMainMenuBar ();
  }
}

/******************************************************************************
 * 右键弹出菜单
 ******************************************************************************/

static im_popup_rep* g_active_popup= nullptr;

void
im_activate_popup (im_popup_rep* p) {
  // 打开位置取当前鼠标坐标（右键发生的地点）。ImGui 构建下编辑器不会发送
  // SLOT_POSITION，故在此直接读 ImGui 的鼠标位置。
  ImGuiIO& io   = ImGui::GetIO ();
  p->pos_x      = io.MousePos.x;
  p->pos_y      = io.MousePos.y;
  p->just_opened= true;
  g_active_popup= p;
}

void
im_deactivate_popup (im_popup_rep* p) {
  if (g_active_popup == p) g_active_popup= nullptr;
}

bool
im_has_active_popup () {
  return g_active_popup != nullptr;
}

void
im_render_active_popup () {
  if (g_active_popup == nullptr) return;
  im_popup_rep* p = g_active_popup;
  const char*   id= "##mogan_popup";
  if (p->just_opened) {
    // OpenPopup 与 BeginPopup 必须在同一 ID 栈位置；此处位于帧顶层（菜单条
    // 的 Begin/End 已平衡），每帧一致。
    ImGui::SetNextWindowPos (ImVec2 (p->pos_x, p->pos_y));
    ImGui::OpenPopup (id);
    p->just_opened= false;
  }
  if (ImGui::BeginPopup (id)) {
    im_menu_rep* m= dynamic_cast<im_menu_rep*> (p->menu.rep);
    if (m != nullptr) {
      array<widget>& kids= m->menu_children ();
      for (int i= 0; i < N (kids); ++i)
        render_node (kids[i]);
    }
    ImGui::EndPopup ();
  }
  else {
    // 用户点选项或点外部关闭 → 释放活动 popup。下一帧起恢复编辑器鼠标分发；
    // 之后首个编辑器鼠标事件会经 edit_mouse.cpp 的清场逻辑销毁 popup_win。
    g_active_popup= nullptr;
  }
}

/******************************************************************************
 * im_popup_rep：弹出协议
 ******************************************************************************/

void
im_popup_rep::send (slot s, blackbox val) {
  switch (s) {
  case SLOT_MOUSE_GRAB: {
    bool grab= open_box<bool> (val);
    if (grab) im_activate_popup (this);
    else im_deactivate_popup (this);
  } break;
  case SLOT_VISIBILITY: {
    // false（隐藏/关闭）→ 注销活动 popup
    if (!open_box<bool> (val)) im_deactivate_popup (this);
  } break;
  case SLOT_POSITION: {
    // ImGui 构建下编辑器不发送 SLOT_POSITION（受 QTTEXMACS/AQUATEXMACS 包裹），
    // 这里仅作兼容：把 SI 坐标转 px 记录。
    coord2 p= open_box<coord2> (val);
    pos_x   = (float) (p.x1 / PIXEL);
    pos_y   = (float) (p.x2 / PIXEL);
  } break;
  default:
    im_widget_rep::send (s, val);
  }
}

widget
im_popup_rep::popup_window_widget (string s) {
  (void) s;
  // 与 Qt qt_menu_rep 一致：popup_window_widget 返回自身，使 set_visibility /
  // send_mouse_grab 命中同一个 rep。
  return widget (this);
}
