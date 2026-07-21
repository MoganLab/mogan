
/******************************************************************************
 * MODULE     : edit_modify.hpp
 * DESCRIPTION: Main routines for the modification of the edit tree
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef EDIT_MODIFY_H
#define EDIT_MODIFY_H
#include "archiver.hpp"
#include "editor.hpp"
#ifdef LORO_ENABLED
#include "loro_shadow.hpp"
#endif

path inner_paragraph (tree t, path p);

class edit_modify_rep : virtual public editor_rep {
protected:
  observer cur_pos; // tree_position corresponding to tp
  double   author;  // the author identifier associated to this view
  archiver arch;    // archiver attached to the editor
#ifdef LORO_ENABLED
  loro_shadow loro_doc; // Phase 2 shadow LoroDoc（随本地编辑镜像）
  bool        loro_seeded         = false; // 是否已 seed 当前 buffer
  bool        loro_applying_remote= false; // 远端应用期间，跳过镜像回灌
  bool        loro_routing        = false; // debug_loro round-trip 中，防递归
  bool        loro_collab_on= false; // 协作会话开启前，本地编辑不 seed/不上行
#endif

public:
  edit_modify_rep ();
  ~edit_modify_rep ();
  double this_author () override;

  void notify_assign (path p, tree u) override;
  void notify_insert (path p, tree u) override;
  void notify_remove (path p, int nr) override;
  void notify_split (path p) override;
  void notify_join (path p);
  void notify_assign_node (path p, tree_label op);
  void notify_insert_node (path p, tree t);
  void notify_remove_node (path p);
  void notify_set_cursor (path p, tree data);
  void post_notify (path p);
#ifdef LORO_ENABLED
  void ensure_loro_seeded () override;
  void mirror_loro (const modification& mod) override;
  // Phase 3：导入远端 update，diff 出把 buffer 变到新状态所需的 mods，经
  // edit_announce 应用到 buffer（versioning：loro_applying_remote 守卫使
  // mirror_loro 跳过，避免回灌）。
  void apply_remote (string bytes) override;
  // 协作会话开关：打开前本地编辑不 seed/不上行（loro_collab
  // 在加入成功后置位）。
  void collab_enable () override;
  bool collab_enabled () override;
  void collab_resync () override;
  // debug_loro：把 mod 经 Loro round-trip（mirror→diff_from_current）后再应用。
  bool route_through_loro (const modification& mod) override;
#endif

  void clear_undo_history ();
  void archive_state ();
  void start_editing ();
  void end_editing ();
  void cancel_editing ();
  void start_slave (double a);
  void mark_start (double a);
  bool mark_cancel (double a);
  void mark_end (double a);
  void add_undo_mark ();
  void remove_undo_mark ();
  int  undo_possibilities ();
  void undo (bool redoable);
  void unredoable_undo ();
  void undo (int i);
  int  redo_possibilities ();
  void redo (int i);
  void require_save ();
  void notify_save (bool real_save= true);
  bool need_save (bool real_save= true);
  void show_history ();

  observer position_new (path p);
  void     position_delete (observer o);
  void     position_set (observer o, path p);
  path     position_get (observer o);
};

#endif // defined EDIT_MODIFY_H
