/******************************************************************************
 * MODULE     : edit_collab.cpp
 * DESCRIPTION: Collaboration
 * COPYRIGHT  : (C) 2026 JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Editor/edit_main.hpp"
#include "modification.hpp"
#include "observers.hpp"
#include "tm_debug.hpp"
#include "tm_window.hpp"
#include "tree_cursor.hpp"
#include "tree_helper.hpp"
#include "tree_observer.hpp"
#include <moebius/vars.hpp>

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
    // 重建 meta 账本（JOIN 时从导入的 shadow 填充），再补 seed 本端缺失的
    // section
    loro_doc->sync_meta_from_shadow ();
    loro_seed_local_meta ();
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
  // 输入法 pre-edit 期间的插入（<pre-edit|s> 节点）与提交时 mark_cancel 的回撤
  // 都属本地临时显示，不应同步给对端；提交后的正式文本在 pre_edit_mark 清零后
  // 插入，仍会正常镜像。
  if (is_pre_editing ()) return;
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
  bench_start ("apply_remote");
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

  loro_applying_remote= true;
  if (DEBUG_LORO) {
    tree buf= the_buffer ();
    debug_loro << "apply_remote: rp=" << as_string (rp)
               << " buffer is_comp=" << is_compound (buf)
               << " op=" << (int) buf->op << " L=" << as_string (L (buf))
               << " N=" << N (buf) << " | et op=" << (int) et->op
               << " L=" << as_string (L (et)) << " N=" << N (et) << "\n";
  }
  list<modification> mods= loro_doc->remote_diff_mods (bytes, the_buffer ());
  if (DEBUG_LORO)
    debug_loro << "Diff produced " << N (mods) << " modifications.\n";
  for (list<modification> l= mods; !is_nil (l); l= l->next) {
    if (DEBUG_LORO) debug_loro << "Applying remote mod: " << l->item << "\n";
    if (DEBUG_LORO) {
      path rp2= rp * root (l->item);
      debug_loro << "  target: rp*root=" << as_string (rp2)
                 << " has_subtree=" << has_subtree (et, rp2)
                 << (has_subtree (et, rp2)
                         ? string (" st_comp=") *
                               as_string (is_compound (subtree (et, rp2))) *
                               string (" st_N=") *
                               as_string (N (subtree (et, rp2))) *
                               string (" st_L=") *
                               as_string (L (subtree (et, rp2)))
                         : string (" (no subtree)"))
                 << "\n";
    }
    apply (et, rp * l->item);
  }
  // applying_remote 暂不关闭：apply_remote_meta 的回写 setter 也要在守卫内

  // 恢复游标与选中（observer 已随 apply 的树编辑更新位置）。恢复期间保持
  // loro_applying_remote=true，使 go_to/select 的 collab_cursor_moved_hook
  // 被抑制
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
  // 远端 mod 改变了树结构，上面的 go_to_correct/go_to_start/select 只是把
  // 错位后的游标「按位恢复」，并非用户主动移动；但 go_to 会置位 user_active，
  // 导致下一帧 apply_changes（edit_interface.cpp）误判为用户操作而调用
  // cursor_visible()->scroll_to()，把视口强行拉回光标处。这里清除 user_active，
  // 让本次远端更新被视为程序化编辑、视口保持原位——与 session-edit.scm 中
  // session-output / field-process-input 插入后 (set-user-active #f) 一致
  // （LLM/会话流式输出同样靠它避免视图跳动）。
  set_user_active (false);
  loro_applying_remote= false;

  // 关键：apply_remote 通过 edit_announce 改了 buffer（新 tree_rep*），
  // 但这些新 rep 不在 id_map 里，因此下一次本地编辑 mirror_mod 会 id_map miss
  // -> 块级重 seed
  // -> TreeID 被重洗 -> 远端 update 引用旧 TreeID -> 永久 Diff 0。
  // 因此，这里重建 id_map，把 buffer 当前状态关联到 shadow 的 TreeID。
  if (!is_nil (mods)) loro_doc->sync_id_map_from_shadow (the_buffer ());

  // 远端 meta section（style/initial/...）回写 buf->data：仍处于
  // applying_remote 守卫，回写触发的 setter 钩子会被守卫跳过，避免回环。body 与
  // meta 都处理完后再 关闭守卫。
  apply_remote_meta ();
  loro_applying_remote= false;

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
  }
  bench_cumul ("apply_remote");
  if (DEBUG_BENCH) lolly::system::bench_print (std_bench);
  bench_reset ("apply_remote");
}

/******************************************************************************
 * meta section（body 之外的文档部分）同步
 *
 * style/initial/final/project/attachments 作为带 __section__ 标签的独立 root 与
 * body 共享同一 LoroDoc。这些方法实现在 edit_main_rep：它同时持有
 *edit_modify_rep 的 loro_doc 与 edit_typeset_rep 的 style/init getter/setter。
 ******************************************************************************/

// 与 attach_data 一致：initial 同步前剔除显示专用变量（屏幕宽高/缩放/全屏），
// 避免本端显示设置被同步到对端。返回剔除后的
// hashmap（顺序无关，便于等值比较）。
static hashmap<string, tree>
doc_init_map (hashmap<string, tree> init) {
  hashmap<string, tree> h= copy (init);
  h->reset (moebius::PAGE_SCREEN_WIDTH);
  h->reset (moebius::PAGE_SCREEN_HEIGHT);
  if (!h->contains ("no-zoom")) h->reset (moebius::ZOOM_FACTOR);
  h->reset ("full-screen-mode");
  return h;
}

// 本端缺失的 meta section 灌入 CRDT（CREATE 首发场景）。
void
edit_main_rep::loro_seed_local_meta () {
  if (!loro_doc->has_meta ("style"))
    loro_doc->seed_meta ("style", get_style ());
  if (!loro_doc->has_meta ("initial"))
    loro_doc->seed_meta ("initial",
                         make_collection (doc_init_map (get_init ())));
  if (!loro_doc->has_meta ("final"))
    loro_doc->seed_meta ("final", make_collection (get_fin ()));
  if (!loro_doc->has_meta ("attachments"))
    loro_doc->seed_meta ("attachments", make_collection (get_att ()));
  if (!loro_doc->has_meta ("project"))
    loro_doc->seed_meta ("project", buf->data->project);
}

// 远端 update → 各 meta section 回写 buf->data。用 hashmap 等值比较（顺序无关，
// 因 make_collection 遍历 hashmap 的顺序不确定）；initial 保留本端显示变量。
// 写回经 setter，受 loro_applying_remote 守卫，触发的镜像钩子会被跳过。
void
edit_main_rep::apply_remote_meta () {
  loro_doc->sync_meta_from_shadow ();
  array<string> secs   = loro_doc->list_meta_sections ();
  bool          changed= false;
  for (int i= 0; i < N (secs); i++) {
    string name  = secs[i];
    tree   remote= loro_doc->meta_to_tree (name);
    if (name == "style") {
      if (get_style () != remote) {
        change_style (remote);
        changed= true;
      }
    }
    else if (name == "initial") {
      hashmap<string, tree> rmap= tree_hashmap (UNINIT, remote);
      if (doc_init_map (get_init ()) != rmap) {
        // 保留本端显示变量，文档变量取远端
        hashmap<string, tree> loc= get_init ();
        if (loc->contains (moebius::PAGE_SCREEN_WIDTH))
          rmap (moebius::PAGE_SCREEN_WIDTH)= loc[moebius::PAGE_SCREEN_WIDTH];
        if (loc->contains (moebius::PAGE_SCREEN_HEIGHT))
          rmap (moebius::PAGE_SCREEN_HEIGHT)= loc[moebius::PAGE_SCREEN_HEIGHT];
        if (loc->contains (moebius::ZOOM_FACTOR))
          rmap (moebius::ZOOM_FACTOR)= loc[moebius::ZOOM_FACTOR];
        if (loc->contains ("full-screen-mode"))
          rmap ("full-screen-mode")= loc["full-screen-mode"];
        set_init (rmap);
        changed= true;
      }
    }
    else if (name == "final") {
      hashmap<string, tree> rmap= tree_hashmap (UNINIT, remote);
      if (get_fin () != rmap) {
        set_fin (rmap);
        changed= true;
      }
    }
    else if (name == "attachments") {
      hashmap<string, tree> rmap= tree_hashmap (UNINIT, remote);
      if (get_att () != rmap) {
        set_att (rmap);
        changed= true;
      }
    }
    else if (name == "project") {
      if (buf->data->project != remote) {
        buf->data->project= remote;
        changed           = true;
      }
    }
  }
  if (changed) {
    notify_change (THE_ENVIRONMENT);
    typeset_invalidate_env ();
  }
}

// 本地 meta setter 钩子入口：按 section 名取当前工作副本树，镜像到 CRDT 并经
// local-update 广播。远端应用期间（loro_applying_remote）或协作未开启时跳过。
void
edit_main_rep::mirror_meta_if_active (string section) {
  if (loro_applying_remote) return;
  if (!loro_collab_on) return;
  ensure_loro_seeded ();
  tree t;
  if (section == "style") t= get_style ();
  else if (section == "initial")
    t= make_collection (doc_init_map (get_init ()));
  else if (section == "final") t= make_collection (get_fin ());
  else if (section == "attachments") t= make_collection (get_att ());
  else if (section == "project") t= buf->data->project;
  else return;
  loro_doc->mirror_meta_replace (section, t);
}

/******************************************************************************
 * 多光标：本地光标序列化（path -> TreeID + 稳定位置）与远程光标接收/解析。
 * 位置组格式 "peerhex:counter:off_field"（peer 为 u64 的 16 位 hex——lolly 无 64
 * 位整数字符串转换）；off_field = "T<hex>"（文本内 Loro 稳定位置 Cursor 的
 * postcard 字节 hex，op-id 锚定、并发编辑下自动跟随——CRDT 级稳定）或 "I<int>"
 * （结构/整数偏移兜底）。payload = "caret sel_start sel_end" 三组空格分隔，
 * 传输层不解析，原样收发。
 ******************************************************************************/

static string
u64_to_hex (uint64_t v) {
  string r;
  for (int i= 15; i >= 0; i--) {
    int nib= (int) ((v >> (4 * i)) & 0xf);
    r << (char) (nib < 10 ? '0' + nib : 'a' + nib - 10);
  }
  return r;
}

static uint64_t
hex_to_u64 (string s) {
  uint64_t v= 0;
  for (int i= 0; i < N (s); i++) {
    char c  = s[i];
    int  nib= (c >= '0' && c <= '9')   ? c - '0'
              : (c >= 'a' && c <= 'f') ? c - 'a' + 10
              : (c >= 'A' && c <= 'F') ? c - 'A' + 10
                                       : 0;
    v       = (v << 4) | (uint64_t) nib;
  }
  return v;
}

static string
format_group (mogan_tree_id tid, string off_field) {
  return u64_to_hex (tid.peer) * ":" * as_string (tid.counter) * ":" *
         off_field;
}

static bool
parse_group (string g, mogan_tree_id& tid, string& off_field) {
  int c1= search_forwards (":", g);
  if (c1 < 0) return false;
  string rest= g (c1 + 1, N (g));
  int    c2  = search_forwards (":", rest);
  if (c2 < 0) return false;
  tid.peer   = hex_to_u64 (g (0, c1));
  tid.counter= as_int (rest (0, c2));
  off_field  = rest (c2 + 1, N (rest)); // "T<hex>" 或 "I<int>"，无冒号/空格
  return true;
}

static array<string>
split_spaces (string s) {
  array<string> out;
  int           start= 0;
  for (int i= 0; i <= N (s); i++)
    if (i == N (s) || s[i] == ' ') {
      if (i > start) out << s (start, i);
      start= i + 1;
    }
  return out;
}

// 绝对 path -> 纯 id 上传（CRDT 级稳定）：解析到光标所在节点，上传其 TreeID +
// 节点内偏移。原子节点内偏移用 **Loro 稳定位置**（op-id 锚定，并发编辑下自动
// 跟随）；复合节点上的子位置下钻到该子节点、上传子节点 TreeID（结构偏移 0），
// 不传 mogan 树子索引。不可定位（nil/path 越界）回退 0:0:I0。
static string
encode_path (tree& et, loro_shadow loro_doc, path p) {
  if (is_nil (p) || is_nil (path_up (p)) || !has_subtree (et, path_up (p)))
    return format_group (mogan_tree_id{0, 0}, "I0");
  tree node= subtree (et, path_up (p));
  int  off = last_item (p);
  if (is_atomic (node)) {
    mogan_tree_id tid= loro_doc->get_id (node);
    string        hex= loro_doc->encode_cursor_hex (tid, off);
    // 文本内：稳定位置（hex 非空）；编码失败回落整数偏移
    return format_group (tid, (hex != "") ? string ("T") * hex
                                          : string ("I") * as_string (off));
  }
  // 复合节点：光标在子位置 off——下钻到该子节点，上传子节点 id（结构偏移 0）
  if (has_subtree (et, p))
    return format_group (loro_doc->get_id (subtree (et, p)), "I0");
  return format_group (loro_doc->get_id (node),
                       "I" * as_string (off)); // 越界回落
}

string
edit_modify_rep::collab_cursor_payload () {
  if (!loro_collab_on) return "";
  // pre-edit 期间光标落在未同步的临时预编辑节点内，其位置对对端无意义：返回空
  // 载荷，使本帧不上行光标（提交后 pre_edit_mark 清零，恢复正常上行）。
  if (is_pre_editing ()) return "";
  path cp= tp;
  path sp= cp, ep= cp;
  if (selection_active_any ()) selection_get (sp, ep);
  string cg= encode_path (et, loro_doc, cp);
  // 光标未就绪（如 JOIN 刚完成、tp 尚在 buffer 根/未定位 → path_up(tp) 为 nil）
  // 则不发本帧，避免对端把远程光标渲染成 {0,0} 而缺失。
  if (cg == format_group (mogan_tree_id{0, 0}, "I0")) return "";
  return cg * " " * encode_path (et, loro_doc, sp) * " " *
         encode_path (et, loro_doc, ep);
}

extern void (*g_loro_cursor_flush) ();

void
edit_modify_rep::collab_cursor_moved_hook () {
  if (loro_applying_remote) return; // 远端应用期间恢复本地光标，不回灌
  if (!loro_collab_on) return;
  if (g_loro_cursor_flush) g_loro_cursor_flush ();
}

void
edit_modify_rep::set_remote_cursor (string peer, string payload) {
  // 远程光标到达：重建 id_map/rev_id_map。本地结构化编辑后增量映射可能漂移，
  loro_doc->sync_id_map_from_shadow (the_buffer ());
  array<string> g= split_spaces (payload);
  if (N (g) < 3) return;
  remote_cursor_entry e;
  e.peer= peer;
  if (!parse_group (g[0], e.c_tid, e.c_off)) return;
  if (!parse_group (g[1], e.s_tid, e.s_off)) return;
  if (!parse_group (g[2], e.e_tid, e.e_off)) return;
  if (DEBUG_LORO)
    debug_loro << "set_remote_cursor " << peer << " c=" << e.c_tid.counter
               << "/" << e.c_off << " s=" << e.s_tid.counter << "/" << e.s_off
               << " e=" << e.e_tid.counter << "/" << e.e_off << "\n";
  for (int i= 0; i < N (remote_cursors); i++)
    if (remote_cursors[i].peer == peer) {
      remote_cursors[i]= e;
      send_invalidate_all (this); // 远程光标变化：触发重绘
      return;
    }
  remote_cursors << e;
  send_invalidate_all (this);
}

array<editor_rep::remote_cursor_view>
edit_modify_rep::get_remote_cursors () {
  array<remote_cursor_view> out;
  // off_field "T<hex>"→按当前 doc 解析稳定位置（并发编辑下自动跟随）；
  // "I<int>"→结构/整数偏移。延迟解析保证 CRDT
  // 级稳定。失败/节点缺失→nil（不渲染）。 原子节点若位于 concat
  // 等内联容器中，box 树会打平：有效光标 path 是 [concat, 跨原子偏移]，而非
  // [concat, 原子索引, 原子内偏移]。此处把后者转成前者， 使 find_check_cursor /
  // find_check_selection 接受（否则 caret missing、选区退化为整行）。
  auto resolve= [&] (mogan_tree_id tid, string off_field) -> path {
    if (N (off_field) == 0) return path ();
    char   head= off_field[0];
    string rest= off_field (1, N (off_field));
    int    off = head == 'T'   ? loro_doc->decode_cursor_hex (rest)
                 : head == 'I' ? as_int (rest)
                               : -1;
    if (off < 0) return path ();
    path node_path=
        loro_doc->node_path_of (tid); // 节点 buffer-相对 path（如 0.0）
    if (is_nil (node_path)) return path ();
    tree buf= the_buffer ();
    path pp = path_up (node_path);
    if (!is_nil (pp) && has_subtree (buf, pp) &&
        is_concat (subtree (buf, pp))) {
      // 父是 concat：合并到 concat 级偏移 = 前置原子长度之和 + 原子内偏移
      int idx       = last_item (node_path);
      int concat_off= off;
      for (int j= 0; j < idx; j++) {
        tree sib= subtree (buf, pp * path (j));
        if (is_atomic (sib)) concat_off+= N (sib->label);
        else { // 非原子兄弟（如数学）：偏移不连续，回退到原子级 path
          concat_off= -1;
          break;
        }
      }
      if (concat_off >= 0) {
        path rp2= pp * path (concat_off);
        if (DEBUG_LORO)
          debug_loro << "  resolve tid_counter=" << tid.counter << " "
                     << off_field << " -> off=" << off
                     << " concat_path=" << as_string (rp2) << "\n";
        return rp2;
      }
    }
    path p= node_path * path (off);
    if (DEBUG_LORO)
      debug_loro << "  resolve tid_counter=" << tid.counter << " " << off_field
                 << " -> off=" << off << " path=" << as_string (p) << "\n";
    return p;
  };
  for (int i= 0; i < N (remote_cursors); i++) {
    remote_cursor_entry& e= remote_cursors[i];
    remote_cursor_view   v;
    v.peer     = e.peer;
    path crel  = resolve (e.c_tid, e.c_off);
    path srel  = resolve (e.s_tid, e.s_off);
    path erel  = resolve (e.e_tid, e.e_off);
    v.caret    = is_nil (crel) ? path () : rp * crel;
    v.sel_start= is_nil (srel) ? path () : rp * srel;
    v.sel_end  = is_nil (erel) ? path () : rp * erel;
    out << v;
  }
  return out;
}
#endif
