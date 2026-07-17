
/******************************************************************************
 * MODULE     : edit_modify.cpp
 * DESCRIPTION: base routines for modifying the edit tree + notification
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "edit_modify.hpp"
#include "modification.hpp"
#include "observers.hpp"
#include "tm_window.hpp"
#include "tree_observer.hpp"

/******************************************************************************
 * Constructors and destructors
 ******************************************************************************/

#ifdef LORO_ENABLED
extern void (*g_loro_broadcast_update) (string bytes);

static void local_update_cb(void* user_data, const uint8_t* bytes, size_t len) {
  if (g_loro_broadcast_update) {
    cout << "[Loro] Local update generated, size: " << len << " bytes. Broadcasting...\n";
    string data((const char*)bytes, len);
    g_loro_broadcast_update(data);
  } else {
    cout << "[Loro] Local update generated, but no broadcast handler registered.\n";
  }
}
#endif

edit_modify_rep::edit_modify_rep ()
    : editor_rep (), // NOTE: ignored by the compiler, but suppresses warning
      author (new_author ()), arch (author, rp)
#ifdef LORO_ENABLED
      ,
      loro_doc (), loro_seeded (false), loro_applying_remote (false),
      loro_routing (false)
#endif
{
#ifdef LORO_ENABLED
  loro_doc->on_local_update(local_update_cb, this);
#endif
}
edit_modify_rep::~edit_modify_rep () {}

#ifdef LORO_ENABLED
// Phase 2：把一个 modification 镜像到 shadow LoroDoc。
// mod->p 是 the_et 根路径，用 mod/rp 转成 buffer 相对；首次镜像时 lazy seed。

void
edit_modify_rep::ensure_loro_seeded () {
  if (loro_applying_remote) return;
  if (!loro_seeded) {
    // 如果 shadow 已有内容（从远端 import 而来），复用其 TreeID 血统（不创建新根），
    // 避免 A/B 各自 seed 产生两个根 → to_tree 只读 roots[0] → 对端编辑不可见。
    if (!loro_doc->sync_id_map_from_shadow (the_buffer ()))
      loro_doc->seed (the_buffer ()); // shadow 为空 → 本端是创建者 → seed
    loro_seeded = true;
  }
}

void
edit_modify_rep::mirror_loro (const modification& mod) {
  if (loro_applying_remote) return; // 远端应用期间不回灌镜像，避免循环
  if (const_cast<modification&>(mod)->k == MOD_SET_CURSOR) return;
  ensure_loro_seeded(); // Fallback in case not called from edit_announce
  loro_doc->mirror_mod (the_buffer (), mod / rp);
#ifdef LORO_DEBUG
  // debug_loro 验证：镜像后 buffer 应与 Loro 状态一致。不一致则告警（说明镜像链路有 bug）。
  // 这是安全模式——mirror_loro 在 edit_done(post-apply) 里，不调 edit_announce，无递归风险。
  tree lt= loro_doc->to_tree ();
  if (!(lt == the_buffer ()))
    cout << "[loro-verify] MISMATCH: buffer != Loro after mirror\n";
#endif
}

// Phase 3：导入远端 update，把 diff 出的 mods 经 edit_announce 应用到 buffer。
// loro_applying_remote 守卫使这些应用的 edit_done→mirror_loro 被跳过（versioning）。
void
edit_modify_rep::apply_remote (string bytes) {
  cout << "[Loro] Applying remote update of size " << N(bytes) << "\n";
  loro_applying_remote   = true;
  list<modification> mods= loro_doc->remote_diff_mods (bytes, the_buffer ());
  cout << "[Loro] Diff produced " << N(mods) << " modifications.\n";
  for (list<modification> l= mods; !is_nil (l); l= l->next) {
    cout << "[Loro] Applying remote mod: " << l->item << "\n";
    apply (et, rp * l->item);
  }
  loro_applying_remote= false;

  // 关键：apply_remote 通过 edit_announce 改了 buffer（新 tree_rep*），
  // 但这些新 rep 不在 id_map 里，因此下一次本地编辑 mirror_mod 会 id_map miss -> 块级重 seed
  // -> TreeID 被重洗 -> 远端 update 引用旧 TreeID -> 永久 Diff 0。
  // 因此，这里重建 id_map，把 buffer 当前状态关联到 shadow 的 TreeID。
  if (!is_nil (mods))
    loro_doc->sync_id_map_from_shadow (the_buffer ());

  if (!is_nil(mods)) {
    cout << "[Loro] Forcing typeset invalidation and repaint...\n";
    notify_change (THE_TREE);
    typeset_invalidate_all();
    send_invalidate_all(this);
  }
}

// debug_loro：把 mod 经 Loro round-trip 后再应用（不直接应用原 mod）。
// - 文本精确路径：mirror 后 Loro 已含 op → diff 回译出 mod → 应用时跳过 mirror_loro（防重复）。
// - 结构兜底：mirror 未改变 Loro（重 seed 自 before）→ diff 为空 → 正常应用原 mod
//   （mirror_loro 会从 post-edit buffer 重 seed，使 Loro 与 buffer 一致）。
bool
edit_modify_rep::route_through_loro (const modification& mod) {
  if (loro_routing) return false; // 重入（应用回译/原 mod）→ 正常路径
  loro_routing= true;
  if (!loro_seeded) {
    loro_doc->seed (the_buffer ());
    loro_seeded= true;
  }
  tree               before= the_buffer ();
  loro_doc->mirror_mod (before, mod / rp);
  list<modification> mods= loro_doc->diff_from_current (before);
  if (is_nil (mods)) {
    edit_announce (this, mod); // 结构兜底：正常应用（mirror_loro 重 seed 自 post-edit）
  }
  else {
    loro_applying_remote= true; // 文本精确：Loro 已有 op，应用回译 mod 时跳过 mirror_loro
    for (list<modification> l= mods; !is_nil (l); l= l->next)
      edit_announce (this, rp * l->item);
    loro_applying_remote= false;
  }
  loro_routing= false;
  return true;
}
#endif

/******************************************************************************
 * Notification of changes in document
 ******************************************************************************/

void
edit_modify_rep::notify_assign (path p, tree u) {
  (void) u;
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_assign (get_typesetter (), p / rp, u);
}

void
edit_modify_rep::notify_insert (path p, tree u) {
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_insert (get_typesetter (), p / rp, u);
}

void
edit_modify_rep::notify_remove (path p, int nr) {
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_remove (get_typesetter (), p / rp, nr);
}

void
edit_modify_rep::notify_split (path p) {
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_split (get_typesetter (), p / rp);
}

void
edit_modify_rep::notify_join (path p) {
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_join (get_typesetter (), p / rp);
}

void
edit_modify_rep::notify_assign_node (path p, tree_label op) {
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_assign_node (get_typesetter (), p / rp, op);
}

void
edit_modify_rep::notify_insert_node (path p, tree t) {
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_insert_node (get_typesetter (), p / rp, t);
}

void
edit_modify_rep::notify_remove_node (path p) {
  if (!(rp <= p)) return;
  cur_pos= position_new (tp);
  ::notify_remove_node (get_typesetter (), p / rp);
}

void
edit_modify_rep::notify_set_cursor (path p, tree data) {
  if (!(rp <= p)) return;
  if (data[0] == as_string (author)) {
    if (is_compound (data, "cursor", 1) ||
        is_compound (data, "cursor-clear", 1)) {
      if (tp != p) {
        tp= p;
        go_to_correct (tp);
      }
      if (is_compound (data, "cursor-clear", 1)) {
        // cout << "Clear selection\n";
        select (tp, tp);
      }
    }
    else if (is_compound (data, "start", 1)) {
      if (selection_get_start () != p) {
        // cout << "Set start selection: " << p << "\n";
        select (p, p);
      }
    }
    else if (is_compound (data, "end", 1)) {
      if (selection_get_end () != p) {
        // cout << "Set end selection: " << p << "\n";
        selection_set_end (p);
      }
    }
  }
}

void
edit_modify_rep::post_notify (path p) {
  // cout << "Post notify\n";
  if (!(rp <= p)) return;
  selection_cancel ();
  cancel_alt_selections ();
  notify_change (THE_TREE);
  tp= position_get (cur_pos);
  position_delete (cur_pos);
  cur_pos= nil_observer;
  go_to_correct (tp);
  /*
  cout << "et= " << et << "\n";
  cout << "tp= " << tp << "\n\n";
  */
}

/******************************************************************************
 * Hooks / notify changes to editor
 ******************************************************************************/

// FIXME: the notification might be slow when we have many
// open buffers. In the future, we might obtain the relevant editors
// from all possible prefixes of p using a hashtable

// FIXME: the undo system is not safe when a change is made inside
// a buffer which has no editor attached to it

void
edit_assign (editor_rep* ed, path pp, tree u) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  ed->notify_assign (p, u);
}

void
edit_insert (editor_rep* ed, path pp, tree u) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  ed->notify_insert (p, u);
}

void
edit_remove (editor_rep* ed, path pp, int nr) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  if (nr <= 0) return;
  ed->notify_remove (p, nr);
}

void
edit_split (editor_rep* ed, path pp) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  ed->notify_split (p);
}

void
edit_join (editor_rep* ed, path pp) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  if (N (p) < 1) TM_FAILED ("path too short in join");
  ed->notify_join (p);
}

void
edit_assign_node (editor_rep* ed, path pp, tree_label op) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  ed->notify_assign_node (p, op);
}

void
edit_insert_node (editor_rep* ed, path pp, tree t) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  ed->notify_insert_node (p, t);
}

void
edit_remove_node (editor_rep* ed, path pp) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  ed->notify_remove_node (p);
}

void
edit_set_cursor (editor_rep* ed, path pp, tree data) {
  path p= copy (pp);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  ed->notify_set_cursor (p, data);
}

void
edit_announce (editor_rep* ed, modification mod) {
  // NOTE: 曾尝试在此拦截做 debug_loro round-trip，但 edit_observer 对每个 mod 配对调用
  // edit_announce(应用)+edit_done(mirror)，在 announce 里 mirror 后外层 done 会重复 mirror
  // （文本路径插两次）。盲写难以安全区分内/外层 done，故暂不激活。debug_loro 改用验证模式。
#ifdef LIII_DEBUG
  if (mod->k != MOD_SET_CURSOR) cout << "[loro-mod] " << mod << "\n";
#endif
  if (mod->k != MOD_SET_CURSOR) ed->ensure_loro_seeded();

  switch (mod->k) {
  case MOD_ASSIGN:
    edit_assign (ed, mod->p, mod->t);
    break;
  case MOD_INSERT:
    edit_insert (ed, mod->p, mod->t);
    break;
  case MOD_REMOVE:
    edit_remove (ed, path_up (mod->p), last_item (mod->p));
    break;
  case MOD_SPLIT:
    edit_split (ed, mod->p);
    break;
  case MOD_JOIN:
    edit_join (ed, mod->p);
    break;
  case MOD_ASSIGN_NODE:
    edit_assign_node (ed, mod->p, L (mod));
    break;
  case MOD_INSERT_NODE:
    edit_insert_node (ed, mod->p, mod->t);
    break;
  case MOD_REMOVE_NODE:
    edit_remove_node (ed, mod->p);
    break;
  case MOD_SET_CURSOR:
    edit_set_cursor (ed, mod->p, mod->t);
    break;
  default:
    TM_FAILED ("invalid modification type");
  }
}

void
edit_done (editor_rep* ed, modification mod) {
  path p= copy (mod->p);
  ASSERT (ed->the_buffer_path () <= p, "invalid modification");
  if (mod->k != MOD_SET_CURSOR) {
    ed->post_notify (p);
    ed->mirror_loro (mod); // Phase 2：镜像到 shadow LoroDoc
  }
}

void
edit_touch (editor_rep* ed, path p) {
  // cout << "Touch " << p << "\n";
  ASSERT (ed->the_buffer_path () <= p, "invalid touch");
  ed->typeset_invalidate (p);
}

/******************************************************************************
 * undo and redo handling
 ******************************************************************************/

void
edit_modify_rep::clear_undo_history () {
  global_clear_history ();
}

double
edit_modify_rep::this_author () {
  return author;
}

void
edit_modify_rep::archive_state () {
  path sp1= selection_get_start ();
  path sp2= selection_get_end ();
  if (path_less (sp1, sp2)) {
    // cout << "Selection: " << sp1 << "--" << sp2 << "\n";
    set_cursor (sp2, compound ("end", as_string (author)));
    set_cursor (sp1, compound ("start", as_string (author)));
    set_cursor (tp, compound ("cursor", as_string (author)));
  }
  else set_cursor (tp, compound ("cursor-clear", as_string (author)));
}

void
edit_modify_rep::start_editing () {
  // cout << "Start editing" << LF << INDENT;
  set_author (this_author ());
}

void
edit_modify_rep::end_editing () {
  // cout << UNINDENT << "End editing" << LF;
  global_confirm ();
}

void
edit_modify_rep::cancel_editing () {
  // cout << UNINDENT << "Cancel editing" << LF;
  global_cancel ();
}

void
edit_modify_rep::start_slave (double a) {
  arch->start_slave (a);
}

void
edit_modify_rep::mark_start (double a) {
  // cout << "Mark start " << a << LF << INDENT;
  arch->mark_start (a);
}

bool
edit_modify_rep::mark_cancel (double a) {
  // cout << UNINDENT << "Mark cancel " << a << LF;
  return arch->mark_cancel (a);
}

void
edit_modify_rep::mark_end (double a) {
  // cout << UNINDENT << "Mark end " << a << LF;
  arch->mark_end (a);
}

void
edit_modify_rep::add_undo_mark () {
  // cout << "Add undo mark" << LF;
  arch->confirm ();
}

void
edit_modify_rep::remove_undo_mark () {
  // cout << "Remove undo mark" << LF;
  arch->retract ();
}

int
edit_modify_rep::undo_possibilities () {
  return arch->undo_possibilities ();
}

void
edit_modify_rep::undo (bool redoable) {
  interrupt_shortcut ();
  arch->forget_cursor ();
  if (inside_graphics () && !as_bool (eval ("graphics-undo-enabled"))) {
    eval ("(graphics-reset-context 'undo)");
    return;
  }
  if (arch->undo_possibilities () == 0) {
    set_message ("No more undo information available", "undo");
    return;
  }
  if (redoable) {
    path p= arch->undo ();
    if (!is_nil (p)) go_to (p);
  }
  else arch->forget ();
  if (arch->conform_save ()) {
    set_message ("Your document is back in its original state", "undo");
    beep ();
  }
  if (inside_graphics ()) eval ("(graphics-reset-context 'undo)");
}

void
edit_modify_rep::unredoable_undo () {
  undo (false);
}

void
edit_modify_rep::undo (int i) {
  ASSERT (i == 0, "invalid undo");
  undo (true);
}

int
edit_modify_rep::redo_possibilities () {
  return arch->redo_possibilities ();
}

void
edit_modify_rep::redo (int i) {
  interrupt_shortcut ();
  arch->forget_cursor ();
  if (arch->redo_possibilities () == 0) {
    set_message ("No more redo information available", "redo");
    return;
  }
  path p= arch->redo (i);
  if (!is_nil (p)) go_to (p);
  if (arch->conform_save ()) {
    set_message ("Your document is back in its original state", "undo");
    beep ();
  }
}

void
edit_modify_rep::require_save () {
  arch->require_autosave ();
  arch->require_save ();
}

void
edit_modify_rep::notify_save (bool real_save) {
  arch->confirm ();
  arch->notify_autosave ();
  if (real_save) arch->notify_save ();
}

bool
edit_modify_rep::need_save (bool real_save) {
  if (arch->conform_save ()) return false;
  if (real_save) return true;
  return !arch->conform_autosave ();
}

void
edit_modify_rep::show_history () {
  arch->show_all ();
}

/******************************************************************************
 * handling multiple cursor positions
 ******************************************************************************/

observer
edit_modify_rep::position_new (path p) {
  tree     st   = subtree (et, path_up (p));
  int      index= last_item (p);
  observer o    = tree_position (st, index);
  attach_observer (st, o);
  return o;
}

void
edit_modify_rep::position_delete (observer o) {
  tree st;
  int  index;
  if (o->get_position (st, index)) detach_observer (st, o);
}

void
edit_modify_rep::position_set (observer o, path p) {
  tree st   = subtree (et, path_up (p));
  int  index= last_item (p);
  o->set_position (st, index);
}

path
edit_modify_rep::position_get (observer o) {
  // return super_correct (et, obtain_position (o));
  return correct_cursor (et, obtain_position (o));
}
