
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
  loro_shadow loro_doc;
  bool        loro_seeded         = false; // 是否已 seed 当前 buffer
  bool        loro_applying_remote= false; // 远端应用期间，跳过镜像回灌
  bool        loro_routing        = false; // debug_loro round-trip 中，防递归
  bool        loro_collab_on= false; // 协作会话开启前，本地编辑不 seed/不上行
  bool        loro_vv_initialized=
      false; // 首次 import 远端数据后推进 export vv，避免回传
  // 远程 peer 光标列表：peer 数量少，线性查找/更新即可；重绘时遍历它，经
  // cursor_path_of 把 TreeID+偏移解析回当前 buffer path。
  struct remote_cursor_entry {
    string        peer;
    mogan_tree_id c_tid;
    int           c_off;
    mogan_tree_id s_tid;
    int           s_off;
    mogan_tree_id e_tid;
    int           e_off;
  };
  array<remote_cursor_entry> remote_cursors;
#endif

public:
  edit_modify_rep ();
  ~edit_modify_rep ();
  double this_author ();

  void notify_assign (path p, tree u);
  void notify_insert (path p, tree u);
  void notify_remove (path p, int nr);
  void notify_split (path p);
  void notify_join (path p);
  void notify_assign_node (path p, tree_label op);
  void notify_insert_node (path p, tree t);
  void notify_remove_node (path p);
  void notify_set_cursor (path p, tree data);
  void post_notify (path p);
#ifdef LORO_ENABLED
  void ensure_loro_seeded () override;
  void mirror_loro (const modification& mod) override;
  void apply_remote (string bytes) override;
  void set_remote_cursor (string peer, string payload) override;
  array<remote_cursor_view> get_remote_cursors () override;
  string                    collab_cursor_payload () override;
  void                      collab_cursor_moved_hook () override;
#endif
  void collab_enable () override;
  bool collab_enabled () override;

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
