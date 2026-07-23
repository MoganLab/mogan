/******************************************************************************
 * MODULE     : edit_collab.cpp
 * DESCRIPTION: Collaboration
 * COPYRIGHT  : (C) 2026 JimZhouZZY
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
 * Switches
 ******************************************************************************/

// 协作会话开关：loro_collab 在 CREATE/JOIN 成功建立会话后置位；
// 置位前本地编辑不 seed/不上行（见 ensure_loro_seeded）。
void
edit_modify_rep::collab_enable () {
#ifdef LORO_ENABLED
  loro_collab_on= true;
  if (DEBUG_LORO) debug_loro << "Collaboration enabled\n";
#endif
}

bool
edit_modify_rep::collab_enabled () {
#ifdef LORO_ENABLED
  return loro_collab_on;
#else
  return false;
#endif
}

/******************************************************************************
 * Collaboration related routines
 ******************************************************************************/

#ifdef LORO_ENABLED
void
edit_modify_rep::ensure_loro_seeded () {
  if (loro_applying_remote) return;
  if (!loro_collab_on) return; // 协作未开启：纯本地编辑，不做 Loro 镜像/上行
  if (!loro_seeded) {
    // 如果 shadow 已有内容（从远端 import 而来），复用其 TreeID
    // 血统（不创建新根）， 避免 A/B 各自 seed 产生两个根 → to_tree 只读
    // roots[0] → 对端编辑不可见。
    if (!loro_doc->sync_id_map_from_shadow (the_buffer ()))
      loro_doc->seed (the_buffer ()); // shadow 为空 → 本端是创建者 → seed
    loro_seeded= true;
    if (DEBUG_LORO)
      debug_loro << "ensure_loro_seeded: seeded, preparing to broadcast full "
                 << "doc update\n";
    // seed 创建了整篇文档的 op（新加入端则继承了远端血统）。把当前完整状态
    // 广播出去：否则对端只收到第一条增量编辑，其 shadow 里本端那棵树上一次
    // 提交的内容仍为旧态，diff 结果错误（首编辑表现为 Diff 0）。
    loro_doc->broadcast_update ();
    if (DEBUG_LORO)
      debug_loro << "ensure_loro_seeded: broadcast_update called\n";
  }
}

void
edit_modify_rep::mirror_loro (const modification& mod) {
  if (loro_applying_remote) return; // 远端应用期间不回灌镜像，避免循环
  if (const_cast<modification&> (mod)->k == MOD_SET_CURSOR) return;
  if (!loro_collab_on) {
    static bool warned= false; // 诊断：协作未开启时（首次）提示，定位不广播问题
    if (!warned) {
      warned= true; // 只 Warn 一次防止 log 量爆炸
      if (DEBUG_LORO)
        debug_loro << "mirror_loro skipped: (collab_enable "
                   << "not true in editor)\n";
    }
    return;
  }
  ensure_loro_seeded (); // Fallback in case not called from edit_announce
  if (DEBUG_LORO) debug_loro << "mirror_loro is mirroring mod to shadow\n";
  loro_doc->mirror_mod (the_buffer (), mod / rp);
#ifdef LORO_DEBUG
  // debug_loro 验证：镜像后 buffer 应与 Loro
  // 状态一致。不一致则告警（说明镜像链路有 bug）。 这是安全模式——mirror_loro 在
  // edit_done(post-apply) 里，不调 edit_announce，无递归风险。
  tree lt= loro_doc->to_tree ();
  if (!(lt == the_buffer ()))
    std_error << "MISMATCH: buffer != Loro after mirror\n";
#endif
}

// Phase 3：导入远端 update，把 diff 出的 mods 经 edit_announce 应用到 buffer。
// loro_applying_remote 守卫使这些应用的 edit_done→mirror_loro
// 被跳过（versioning）。
void
edit_modify_rep::apply_remote (string bytes) {
  if (DEBUG_LORO)
    debug_loro << "Applying remote update of size " << N (bytes) << "\n";
  // 远端 mod 经 apply()（tree_observer::raw_apply）改树，游标 tp 与选中路径会
  // 系统性错位（选中区域失效）。先把它们转成 observer 追踪位置（raw_apply 的
  // observer 回调会随树编辑更新 position），应用 mods 后取回错位后的路径恢复。
  observer cur_save      = position_new (tp);
  bool     had_sel       = selection_active_any ();
  observer sel_start_save= nil_observer, sel_end_save= nil_observer;
  if (had_sel) {
    path sp, ep;
    selection_get (sp, ep);
    sel_start_save= position_new (sp);
    sel_end_save  = position_new (ep);
  }

  loro_applying_remote   = true;
  list<modification> mods= loro_doc->remote_diff_mods (bytes, the_buffer ());
  if (DEBUG_LORO)
    debug_loro << "Diff produced " << N (mods) << " modifications.\n";
  for (list<modification> l= mods; !is_nil (l); l= l->next) {
    if (DEBUG_LORO) debug_loro << "Applying remote mod: " << l->item << "\n";
    apply (et, rp * l->item);
  }
  loro_applying_remote= false;

  // 恢复游标与选中（observer 已随 apply 的树编辑更新位置）
  path nc= position_get (cur_save);
  position_delete (cur_save);
  if (!is_nil (nc)) go_to_correct (nc); // 游标按错位后路径恢复
  else go_to_start (rp); // 游标所在节点被远端删除：回落 buffer 起始
  if (had_sel) {
    path ns= position_get (sel_start_save);
    path ne= position_get (sel_end_save);
    position_delete (sel_start_save);
    position_delete (sel_end_save);
    if (!is_nil (ns) && !is_nil (ne) && ns != ne) select (ns, ne);
  }

  // 关键：apply_remote 通过 edit_announce 改了 buffer（新 tree_rep*），
  // 但这些新 rep 不在 id_map 里，因此下一次本地编辑 mirror_mod 会 id_map miss
  // -> 块级重 seed
  // -> TreeID 被重洗 -> 远端 update 引用旧 TreeID -> 永久 Diff 0。
  // 因此，这里重建 id_map，把 buffer 当前状态关联到 shadow 的 TreeID。
  if (!is_nil (mods)) loro_doc->sync_id_map_from_shadow (the_buffer ());

  // 首次 import 远端数据（JOIN 同步）后，把 export vv 推进到当前，标记这些 op
  // "已知"——否则后续 broadcast_update 会把刚收到的 snapshot 当本地增量回传
  // （连接即有一次空上行）。仅首次：重连时若有 pending 本地
  // op，推进会吞掉它们。
  if (!loro_vv_initialized) {
    loro_doc->advance_export_vv ();
    loro_vv_initialized= true;
  }

  if (!is_nil (mods)) {
    if (DEBUG_LORO)
      debug_loro << "Forcing typeset invalidation and repaint...\n";
    notify_change (THE_TREE);
    typeset_invalidate_all ();
    send_invalidate_all (this);
  }
}
#endif
