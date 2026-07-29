/** \file loro_shadow_test.cpp
 *  \copyright GPLv3
 *  \details Phase 2 Step 3 gate：seed 一棵树到 shadow live doc，验证
 *            to_tree / export_snapshot(→loro_to_tree)
 * 都与原树深度相等，且身份表覆盖各节点。 仅 LORO_ENABLED 下编译用例。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro.hpp"
#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include <cstdio>
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

#ifdef LORO_ENABLED

static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

TEST_CASE ("loro_shadow: seed round-trips a document") {
  ensure_labels ();
  // (document (para (concat "hello")) (para "world"))
  tree t (DOCUMENT, 2);
  t[0]      = tree (PARA, 1);
  t[0][0]   = tree (CONCAT, 1);
  t[0][0][0]= tree ("hello");
  t[1]      = tree (PARA, 1);
  t[1][0]   = tree ("world");

  loro_shadow sh;
  sh->seed (t);

  // live doc -> tree（经 to_ir + loro_ir_to_tree）
  tree back= sh->to_tree ();
  CHECK_EQ (back == t, true);

  // live doc -> snapshot -> loro_to_tree（Phase 1 路径）
  string snap = sh->export_snapshot ();
  tree   back2= loro_to_tree (snap);
  CHECK_EQ (back2 == t, true);

  // 身份表覆盖根与各层节点
  CHECK_EQ (sh->has_id (t), true);
  CHECK_EQ (sh->has_id (t[0]), true);
  CHECK_EQ (sh->has_id (t[0][0][0]), true);
}

TEST_CASE ("loro_shadow: atomic text (UTF-8) seeded via LoroText") {
  ensure_labels ();
  tree t (PARA, 1);
  t[0]= tree ("hello 世界");
  loro_shadow sh;
  sh->seed (t);
  CHECK_EQ (sh->to_tree () == t, true);
  CHECK_EQ (sh->has_id (t[0]), true);
}

TEST_CASE ("loro_shadow: mirror text typing and backspace") {
  ensure_labels ();
  // (document (paragraph (concat "")))
  tree t (DOCUMENT, 1);
  t[0]      = tree (PARA, 1);
  t[0][0]   = tree (CONCAT, 1);
  t[0][0][0]= tree (""); // 空原子
  loro_shadow sh;
  sh->seed (t);

  path          atom= path (0) * 0 * 0;        // [0,0,0]
  mogan_tree_id id0 = sh->get_id (t[0][0][0]); // 原子身份（seed 后）
  // 敲 "abc"：只喂 mod 给 mirror_mod（它读 mod
  // 不读树状态），再手工把期望结果赋给 t
  sh->mirror_mod (t, mod_insert (atom, 0, tree ("a")));
  sh->mirror_mod (t, mod_insert (atom, 1, tree ("b")));
  sh->mirror_mod (t, mod_insert (atom, 2, tree ("c")));
  t[0][0][0]->label= string ("abc");
  CHECK_EQ (sh->to_tree () == t, true);

  // 退格删 "c"
  sh->mirror_mod (t, mod_remove (atom, 2, 1));
  t[0][0][0]->label= string ("ab");
  CHECK_EQ (sh->to_tree () == t, true);

  // 原子身份逐字不变 → 走的是精确 LoroText 路径（非兜底重 seed）
  mogan_tree_id id1= sh->get_id (t[0][0][0]);
  CHECK_EQ (id1.peer == id0.peer && id1.counter == id0.counter, true);
}

TEST_CASE ("loro_shadow: structural edit falls back to re-seed") {
  ensure_labels ();
  // (document (paragraph "hi"))
  tree t (DOCUMENT, 1);
  t[0]   = tree (PARA, 1);
  t[0][0]= tree ("hi");
  loro_shadow sh;
  sh->seed (t);
  CHECK_EQ (sh->to_tree () == t, true);

  // 结构改动：document 末尾插入新段落（root=doc 复合 → 兜底整树重 seed）。
  // 手工把 t 改成 post-edit 状态（不用 raw_apply，避免拉入 libmogan 符号），
  // mirror_mod 兜底从 t 重新 seed。
  tree extra (PARA, 1);
  extra[0]= tree ("new");
  tree mutated (DOCUMENT, 2);
  mutated[0]= t[0];
  mutated[1]= extra;
  t         = mutated;
  sh->mirror_mod (t, mod_insert (path (), 1, extra));
  CHECK_EQ (sh->to_tree () == t, true);
  CHECK_EQ (N (t) == 2, true);
}

// 模拟一段真实编辑会话：打字 → 退格 → 结构改动（兜底重 seed）→ 再打字。
// 每步都用期望的真值 buffer 比对 shadow.to_tree()，证明 shadow 始终与 buffer
// 一致。
TEST_CASE ("loro_shadow: consistent across a mixed edit session") {
  ensure_labels ();
  // (document (paragraph (concat "")))
  tree t (DOCUMENT, 1);
  t[0]      = tree (PARA, 1);
  t[0][0]   = tree (CONCAT, 1);
  t[0][0][0]= tree ("");
  loro_shadow sh;
  sh->seed (t);
  path atom= path (0) * 0 * 0; // [0,0,0]

  // 1. 打 "hello"（精确 LoroText）
  const char* word= "hello";
  for (int i= 0; word[i] != 0; i++) {
    char s[2]= {word[i], 0};
    sh->mirror_mod (t, mod_insert (atom, i, tree (s)));
  }
  t[0][0][0]->label= string ("hello");
  CHECK_EQ (sh->to_tree () == t, true);

  // 2. 退格两次 → "hel"
  sh->mirror_mod (t, mod_remove (atom, 4, 1));
  sh->mirror_mod (t, mod_remove (atom, 3, 1));
  t[0][0][0]->label= string ("hel");
  CHECK_EQ (sh->to_tree () == t, true);

  // 3. 结构改动：插入第二段（root=doc 复合 → 兜底整树重 seed）
  tree p2 (PARA, 1);
  p2[0]= tree ("world");
  tree mutated (DOCUMENT, 2);
  mutated[0]= t[0];
  mutated[1]= p2;
  t         = mutated;
  sh->mirror_mod (t, mod_insert (path (), 1, p2));
  CHECK_EQ (sh->to_tree () == t, true);

  // snapshot 可导出且非空
  CHECK_EQ (N (sh->export_snapshot ()) > 0, true);
}

// 正向 e2e：A 本地编辑（mirror）→ snapshot → B 导入 → B 的树 == A 的 buffer。
TEST_CASE ("loro e2e: local edit on A propagates to B via snapshot") {
  ensure_labels ();
  // A 的 buffer: (document (paragraph (concat "x")))
  tree tA (DOCUMENT, 1);
  tA[0]      = tree (PARA, 1);
  tA[0][0]   = tree (CONCAT, 1);
  tA[0][0][0]= tree ("x");
  loro_shadow a;
  a->seed (tA);

  // A 敲 "y" → "xy"（精确 LoroText 镜像）
  a->mirror_mod (tA, mod_insert (path (0) * 0 * 0, 1, tree ("y")));
  tA[0][0][0]->label= string ("xy");
  CHECK_EQ (a->to_tree () == tA, true);

  // 导出 snapshot
  string snap= a->export_snapshot ();
  CHECK_EQ (N (snap) > 0, true);

  // B 导入 snapshot（被动），to_tree 应等于 A 的编辑后 buffer
  loro_shadow b;
  CHECK_EQ (b->import_data (snap), true);
  CHECK_EQ (b->to_tree () == tA, true);
}

// 双向合并 e2e：A/B 从公共初始 snapshot
// 各自并发编辑不同节点，互导后收敛到同一棵树。 关键：B 用 import_and_build 把
// buffer rep 关联到 A 的 TreeID，二者身份对齐才能合并。
TEST_CASE ("loro e2e: bidirectional merge (concurrent edits converge)") {
  ensure_labels ();
  // 公共初始文档 (document (para "a") (para "b"))
  tree init (DOCUMENT, 2);
  init[0]   = tree (PARA, 1);
  init[0][0]= tree ("a");
  init[1]   = tree (PARA, 1);
  init[1][0]= tree ("b");

  loro_shadow a;
  a->seed (init);
  string sa0= a->export_snapshot (); // 公共初始 snapshot（A 编辑前导出）

  // A 编辑 para0："a" -> "aA"（tA 共享 init 的 rep）
  tree tA= init;
  a->mirror_mod (tA, mod_insert (path (0) * 0, 1, tree ("A")));
  tA[0][0]->label= string ("aA");
  string sa      = a->export_snapshot ();

  // B 从 sa0 导入并构建 buffer（id_map 关联到 A 的 TreeID）
  loro_shadow b;
  tree        tB;
  CHECK_EQ (b->import_and_build (sa0, tB), true);
  // B 编辑 para1："b" -> "bB"
  b->mirror_mod (tB, mod_insert (path (1) * 0, 1, tree ("B")));
  tB[1][0]->label= string ("bB");
  string sb      = b->export_snapshot ();

  // 互导 snapshot → CRDT 合并
  CHECK_EQ (a->import_data (sb), true);
  CHECK_EQ (b->import_data (sa), true);

  // 收敛：两端都 == (para "aA") (para "bB")
  tree expected (DOCUMENT, 2);
  expected[0]   = tree (PARA, 1);
  expected[0][0]= tree ("aA");
  expected[1]   = tree (PARA, 1);
  expected[1][0]= tree ("bB");
  CHECK_EQ (a->to_tree () == expected, true);
  CHECK_EQ (b->to_tree () == expected, true);
}

// 事件级增量同步：A 订阅 local-update，编辑产生 delta（仅新增 op）→ B 导入
// delta 收敛。
extern "C" void
loro_test_capture_update (void* ud, const uint8_t* bytes, size_t len) {
  auto* s= static_cast<string*> (ud);
  *s     = string ((const char*) bytes, (int) len);
}

// 逐条捕获每次 commit 的 delta（模拟真实编辑器：每个 local-update 立即发
// WS，B 逐条 import）。
extern "C" void
loro_test_collect_update (void* ud, const uint8_t* bytes, size_t len) {
  auto* v= static_cast<list<string>*> (ud);
  *v     = *v * list<string> (string ((const char*) bytes, (int) len));
}

TEST_CASE ("loro event: incremental sync via local-update events") {
  ensure_labels ();
  tree init (DOCUMENT, 1);
  init[0]       = tree (PARA, 1);
  init[0][0]    = tree (CONCAT, 1);
  init[0][0][0] = tree ("x");
  tree        tA= init; // 持久 buffer（共享 init 的 rep）
  loro_shadow a;
  a->seed (tA);
  string      s0= a->export_snapshot ();
  loro_shadow b;
  CHECK_EQ (b->import_data (s0), true); // B 起点 == init，共享 TreeID

  // A 订阅 local-update（捕获 seed 之后的增量 delta）
  string captured;
  a->on_local_update (loro_test_capture_update, &captured);

  // A 编辑："x" -> "xy"
  a->mirror_mod (tA, mod_insert (path (0) * 0 * 0, 1, tree ("y")));
  tA[0][0][0]->label= string ("xy");

  CHECK_EQ (N (captured) > 0, true); // 事件触发，捕获到 delta（非整 snapshot）

  // B 导入 delta（仅新增 op）→ 收敛到 A 的编辑后状态
  CHECK_EQ (b->import_data (captured), true);
  CHECK_EQ (b->to_tree () == tA, true);
}

// 远端 delta → modification：B 收到 A 的编辑后，生成把 buffer 变到新状态所需的
// mod， 手工应用到 buf（模拟 edit_modify），验证 buf 与 A 的编辑结果一致。
TEST_CASE ("loro reverse: remote edit -> modification (apply to buffer)") {
  ensure_labels ();
  tree init (DOCUMENT, 1);
  init[0]      = tree (PARA, 1);
  init[0][0]   = tree (CONCAT, 1);
  init[0][0][0]= tree ("x");

  // A: seed、编辑 "x"->"xy"、导出
  loro_shadow a;
  tree        tA= init;
  a->seed (tA);
  string s0= a->export_snapshot ();
  a->mirror_mod (tA, mod_insert (path (0) * 0 * 0, 1, tree ("y")));
  tA[0][0][0]->label= string ("xy");
  string sa         = a->export_snapshot ();

  // B: 从 s0 建共享身份的 buffer
  loro_shadow b;
  tree        buf;
  CHECK_EQ (b->import_and_build (s0, buf), true);
  // 生成把 buf(init) 变到 A 编辑后状态的 mods
  list<modification> mods= b->remote_diff_mods (sa, buf);
  CHECK_EQ (N (mods) >= 1, true);

  // 手工把 mods 应用到 buf（模拟编辑器 edit_modify，不依赖 libmogan）
  for (list<modification> l= mods; !is_nil (l); l= l->next) {
    modification m= l->item;
    if (m->k == MOD_INSERT) {
      tree&  atom= subtree (buf, root (m));
      string s   = m->t->label;
      int    pos = index (m);
      atom->label=
          atom->label (0, pos) * s * atom->label (pos, N (atom->label));
    }
    else if (m->k == MOD_REMOVE) {
      tree& atom= subtree (buf, root (m));
      int   pos = index (m);
      int   nr  = argument (m);
      atom->label=
          atom->label (0, pos) * atom->label (pos + nr, N (atom->label));
    }
    else if (m->k == MOD_ASSIGN) {
      buf= m->t;
    }
  }

  tree expected (DOCUMENT, 1);
  expected[0]      = tree (PARA, 1);
  expected[0][0]   = tree (CONCAT, 1);
  expected[0][0][0]= tree ("xy");
  CHECK_EQ (buf == expected, true);
}

// 粘贴多行回归：A 粘贴含多个 para 的片段并逐行填充，B 经 delta 同步后必须
// 收到全部行（此前 B 只收到第一行）。忠实模拟 edit_announce(clean_apply) →
// edit_done(mirror) 时序——mirror_mod 读的是 post-apply buffer。
TEST_CASE (
    "loro paste: multi-child block insert at document root propagates fully") {
  ensure_labels ();
  // 初始文档 (document (para "line1") ... (para "line4"))。
  tree tA (DOCUMENT, 4);
  for (int i= 0; i < 4; i++) {
    tA[i]   = tree (PARA, 1);
    string s= string ("line") * as_string (i + 1);
    tA[i][0]= tree (s);
  }
  loro_shadow a;
  a->seed (tA);
  string s0= a->export_snapshot ();

  // B 用 import_and_build 建共享身份的 buffer（与真实新加入者一致）
  loro_shadow b;
  tree        tB;
  CHECK_EQ (b->import_and_build (s0, tB), true);
  CHECK_EQ (tB == tA, true);

  // A 逐条捕获 delta（真实编辑器：每个 local-update 立即发 WS）
  list<string> deltas;
  a->on_local_update (loro_test_collect_update, &deltas);

  // 忠实模拟 edit_announce(应用) → edit_done(mirror) 时序：先 clean_apply
  // 推进 buffer 到 post-apply，再 mirror（mirror_mod 读 post-apply buffer）。
  auto apply_mirror= [] (loro_shadow& sh, tree& buf, modification m) {
    buf= clean_apply (buf, m);
    sh->mirror_mod (buf, m);
  };

  // A：末尾粘贴 4 个 para（单个根插入 mod，片段含 4 个 para）
  tree clip (DOCUMENT, 4);
  for (int i= 0; i < 4; i++)
    clip[i]= tree (PARA, 1);
  apply_mirror (a, tA, mod_insert (path (), 4, clip));

  // A：逐行把新 para（索引 4..7）填成 line1..line4（每行一个原子插入 mod）
  for (int i= 0; i < 4; i++) {
    string s= string ("line") * as_string (i + 1);
    apply_mirror (a, tA, mod_insert (path (4 + i) * 0, 0, tree (s)));
  }

  // A 端自洽：shadow == buffer（8 行）
  CHECK_EQ (N (tA) == 8, true);
  CHECK_EQ (a->to_tree () == tA, true);

  // B：逐条导入 delta（与真实时序一致）→ shadow 必须收敛到 8 行
  CHECK_EQ (N (deltas) > 0, true);
  for (list<string> l= deltas; !is_nil (l); l= l->next)
    CHECK_EQ (b->import_data (l->item), true);
  CHECK_EQ (b->to_tree () == tA, true); // B 端必须收到全部 4 个新行
}

// 单根插入 mod 携带多子节点片段（mod->t 是复合、N>1）时，mirror_mod 必须把
// 每个子节点都 seed 进 shadow——否则只有第一个子节点进 shadow，远端丢后续节点。
// 这是粘贴多行/多块的核心缺陷（不依赖具体编辑器的粘贴 mod
// 序列，直接命中分支）。
TEST_CASE ("loro mirror: multi-child insert mod seeds all children") {
  ensure_labels ();
  // 初始 (document (para "a") (para "b"))，B 经 import_and_build 共享身份
  tree tA (DOCUMENT, 2);
  tA[0]   = tree (PARA, 1);
  tA[0][0]= tree ("a");
  tA[1]   = tree (PARA, 1);
  tA[1][0]= tree ("b");
  loro_shadow a;
  a->seed (tA);
  string      s0= a->export_snapshot ();
  loro_shadow b;
  tree        tB;
  CHECK_EQ (b->import_and_build (s0, tB), true);

  list<string> deltas;
  a->on_local_update (loro_test_collect_update, &deltas);

  // 在根插入一个含 3 个 para 的片段（mod->t 是多子节点复合）
  tree frag (DOCUMENT, 3);
  for (int i= 0; i < 3; i++) {
    frag[i]   = tree (PARA, 1);
    frag[i][0]= tree (string ("x") * as_string (i));
  }
  modification m= mod_insert (path (), 2, frag);
  tA            = clean_apply (tA, m); // post-apply: 5 个 para
  a->mirror_mod (tA, m);

  CHECK_EQ (N (tA) == 5, true);
  CHECK_EQ (a->to_tree () == tA, true); // A 端 shadow 应有全部 5 个 para

  for (list<string> l= deltas; !is_nil (l); l= l->next)
    CHECK_EQ (b->import_data (l->item), true);
  CHECK_EQ (b->to_tree () == tA, true); // B 端必须收到全部 3 个新 para
}

TEST_CASE (
    "loro_shadow: sync_id_map_from_shadow binds buffer to imported shadow") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]   = tree (PARA, 1);
  t[0][0]= tree ("sync_test");

  loro_shadow a;
  a->seed (t);
  string snap= a->export_snapshot ();

  loro_shadow b;
  CHECK_EQ (b->import_data (snap), true);

  // 创建一个结构和内容相同的树，但具有新的 rep（没有绑定 id）
  tree tb (DOCUMENT, 1);
  tb[0]   = tree (PARA, 1);
  tb[0][0]= tree ("sync_test");

  CHECK_EQ (b->has_id (tb), false);
  CHECK_EQ (b->sync_id_map_from_shadow (tb), true);
  CHECK_EQ (b->has_id (tb), true);
  CHECK_EQ (b->has_id (tb[0]), true);
  CHECK_EQ (b->has_id (tb[0][0]), true);
}

TEST_CASE (
    "loro_shadow: diff_from_current generates mods to update older buffer") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]   = tree (PARA, 1);
  t[0][0]= tree ("updated");

  loro_shadow sh;
  sh->seed (t);

  // 提供一个旧状态的 buffer
  tree buf (DOCUMENT, 1);
  buf[0]   = tree (PARA, 1);
  buf[0][0]= tree ("old");

  list<modification> mods= sh->diff_from_current (buf);
  CHECK_EQ (N (mods) > 0, true);

  for (list<modification> l= mods; !is_nil (l); l= l->next) {
    buf= clean_apply (buf, l->item);
  }
  CHECK_EQ (buf == t, true);
}

TEST_CASE ("loro_shadow: broadcast_update sends full state and "
           "advance_export_vv prevents echo") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]   = tree (PARA, 1);
  t[0][0]= tree ("init");

  loro_shadow a;
  a->seed (t);

  list<string> updates;
  a->on_local_update (loro_test_collect_update, &updates);

  // 主动广播初始状态
  a->broadcast_update ();
  CHECK_EQ (N (updates) > 0, true);

  // 验证广播的数据能够重建文档
  loro_shadow b;
  for (list<string> l= updates; !is_nil (l); l= l->next) {
    b->import_data (l->item);
  }
  CHECK_EQ (b->to_tree () == t, true);

  // 验证 advance_export_vv
  updates= list<string> (); // 清空
  tree t_new (DOCUMENT, 1);
  t_new[0]   = tree (PARA, 1);
  t_new[0][0]= tree ("remote");
  loro_shadow c;
  c->seed (t_new);
  string remote_update= c->export_snapshot ();

  a->import_data (remote_update);
  // 调用 advance_export_vv 标记已导入的内容为“已知”
  a->advance_export_vv ();

  a->broadcast_update ();
  // 应当不产生新的实质性更新，返回的内容为空或者只有极少的 metadata 字节
  // 为了稳定测试，不严格要求完全为空，但我们可以确定如果是纯净环境，不会发送刚导入的数据作为本地修改。
  // 在 loro-c 的表现中，可能返回空的 array (即 len == 0) 或不触发回调。
  if (!is_nil (updates)) {
    // 如果有输出，其长度应当很小（不包含整个文档的创建逻辑）
    CHECK_EQ (N (updates->item) < 50, true);
  }
}

TEST_CASE ("loro_shadow: mirror_mod with structural MOD_ASSIGN") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]   = tree (PARA, 1);
  t[0][0]= tree ("old");

  loro_shadow sh;
  sh->seed (t);

  tree new_para (PARA, 1);
  new_para[0]= tree ("new");

  // 使用 assign 替换整个段落
  t[0]= new_para; // 先在树上应用改动
  sh->mirror_mod (t, mod_assign (path (0), new_para));

  CHECK_EQ (sh->to_tree () == t, true);
}

TEST_CASE ("loro_shadow: mirror_mod with structural MOD_REMOVE") {
  ensure_labels ();
  tree t (DOCUMENT, 3);
  t[0]   = tree (PARA, 1);
  t[0][0]= tree ("1");
  t[1]   = tree (PARA, 1);
  t[1][0]= tree ("2");
  t[2]   = tree (PARA, 1);
  t[2][0]= tree ("3");

  loro_shadow sh;
  sh->seed (t);

  // 移除中间和末尾的段落
  tree mutated (DOCUMENT, 1);
  mutated[0]= t[0];
  t         = mutated; // 先在树上应用改动
  sh->mirror_mod (t, mod_remove (path (), 1, 2));

  CHECK_EQ (sh->to_tree () == t, true);
}

// 诊断：remove 一个 compound 的全部子、再 insert 到它（模拟 JOIN 时 et[2] 的
// 序列），用 clean_apply 看结果是否正确（不入 et，不触发 detach/IP）。

// ===== 身份对账（0778）：跨 merge 后 buffer 与 shadow 顺序错位时，reconcile
// 按 TreeID（而非位置）删/移，绝不错删并发节点。复现「<alpha> 被吞」的病根：
// 位置型 diff_walk 会把 remove 落到错下标，身份对账则按 TreeID 精确删除。 =====

static tree
mk_para (string s) {
  tree p (PARA, 1);
  p[0]= tree (s);
  return p;
}

// 诊断：buffer=document(空para("")) vs after=document(para("a"))，看 reconcile
// 产 的 mod（尤其 insert 的 mod->t 是否 atomic）。
TEST_CASE ("loro reconcile: DIAG empty-doc vs doc-with-para mods") {
  ensure_labels ();
  // after: document(para("a"))
  tree body (DOCUMENT, 1);
  body[0]= mk_para ("a");
  loro_shadow a;
  a->seed (body);
  string sa= a->export_snapshot ();
  // buffer: document(空 para(""))（空文档 stub）
  tree buf (DOCUMENT, 1);
  buf[0]   = tree (PARA, 1);
  buf[0][0]= tree ("");
  loro_shadow        b;
  list<modification> mods= b->remote_diff_mods (sa, buf);
  MESSAGE ("diag nmods=", N (mods));
  for (list<modification> l= mods; !is_nil (l); l= l->next) {
    modification m= l->item;
    MESSAGE ("  k=", (int) m->k, " t_atomic=", is_atomic (m->t),
             " t_comp=", is_compound (m->t));
  }
}

// 诊断：buffer/after 都是 DOCUMENT N=1（空文档含1空para vs 新文档含1para），
// reconcile 产 remove([],0,1)+insert([],0,para)。用 clean_apply 应用到 et=TUPLE
// 的 et[2]（rp=[2]），模拟真实崩溃序列。
TEST_CASE (
    "loro reconcile: DIAG same-label empty-doc remove+insert via clean_apply") {
  ensure_labels ();
  // et = TUPLE(空 document x3)，rp=[2]
  tree empty_doc (DOCUMENT, 1);
  empty_doc[0]   = tree (PARA, 1); // document(para(""))
  empty_doc[0][0]= tree ("");
  tree et (TUPLE, 3);
  et[0]= empty_doc;
  et[1]= empty_doc;
  et[2]= empty_doc;
  // remove([2],0,1): 删 et[2] 的 para
  et= clean_apply (et, mod_remove (path (2), 0, 1));
  MESSAGE ("after remove N(et[2])=", N (et[2]));
  // insert([2],0, para): 插入新 para 到 et[2]
  tree newpara (PARA, 1);
  newpara[0]= tree ("x");
  tree frag (DOCUMENT, 1);
  frag[0]= newpara;
  et     = clean_apply (et, mod_insert (path (2), 0, frag));
  MESSAGE ("after insert N(et[2])=", N (et[2]),
           " et[2][0]=", et[2][0][0]->label);
  CHECK_EQ (N (et[2]), 1);
}

// 诊断：buffer=TUPLE(空 document)（模拟 et[n] buffer 容器），after=DOCUMENT。
// 守卫应回退整树 assign（1 个 mod），不产生 remove+insert。
TEST_CASE ("loro reconcile: DIAG buffer=tuple-container vs after=document") {
  ensure_labels ();
  tree body (DOCUMENT, 2);
  body[0]= mk_para ("a");
  body[1]= mk_para ("b");
  loro_shadow a;
  a->seed (body);
  string sa= a->export_snapshot ();
  // buffer = TUPLE(空 document)（容器，label=tuple）
  tree empty_doc (DOCUMENT, 1);
  empty_doc[0]= tree ("");
  tree buf (TUPLE, 1);
  buf[0]= empty_doc;
  loro_shadow        b;
  list<modification> mods= b->remote_diff_mods (sa, buf);
  MESSAGE ("diag nmods=", N (mods));
  for (list<modification> l= mods; !is_nil (l); l= l->next)
    MESSAGE ("  diag mod k=", (int) l->item->k);
  // 期望：1 个 assign（守卫回退）
  CHECK_EQ (N (mods), 1);
  if (N (mods) == 1) CHECK_EQ ((int) mods->item->k, (int) MOD_ASSIGN);
}

// 诊断：模拟 et=TUPLE(空 document,空 document,空
// document)，buffer=et[2]（rp=[2]）。 远端 document body 与 et[2]（空
// document）同 label，对账后 apply(et,[2]*mod)， 看是否复现崩溃/非法（remove
// et[2] 全部子 + insert）。

// 树切片 INSERT（不复制 rep，保持 rep 复用语义）：mod_insert(parent, pos,
// frag)。
static void
test_insert_slice (tree& buf, modification m) {
  path  par   = root (m);
  int   pos   = index (m);
  tree  frag  = m->t;
  tree& parref= subtree (buf, par);
  int   old_n = N (parref);
  int   nr    = N (frag);
  tree  newp (L (parref), old_n + nr);
  for (int i= 0; i < pos; i++)
    newp[i]= parref[i];
  for (int i= 0; i < nr; i++)
    newp[pos + i]= frag[i];
  for (int i= pos; i < old_n; i++)
    newp[i + nr]= parref[i];
  parref= newp;
}

// 树切片 REMOVE：mod_remove(parent, pos, nr)。
static void
test_remove_slice (tree& buf, modification m) {
  path  par   = root (m);
  int   pos   = index (m);
  int   nr    = argument (m);
  tree& parref= subtree (buf, par);
  int   old_n = N (parref);
  tree  newp (L (parref), old_n - nr);
  for (int i= 0; i < pos; i++)
    newp[i]= parref[i];
  for (int i= pos + nr; i < old_n; i++)
    newp[i - nr]= parref[i];
  parref= newp;
}

// 对账把 mods 应用到持久 buffer（模拟编辑器 raw_apply，结构增删用切片、文本
// 插入/删除直接改原子 label）。
static void
apply_mods (tree& buf, list<modification> mods) {
  for (list<modification> l= mods; !is_nil (l); l= l->next) {
    modification m= l->item;
    if (m->k == MOD_INSERT && is_atomic (subtree (buf, root (m)))) {
      tree&  atom= subtree (buf, root (m));
      string s   = m->t->label;
      int    pos = index (m);
      atom->label=
          atom->label (0, pos) * s * atom->label (pos, N (atom->label));
    }
    else if (m->k == MOD_REMOVE && is_atomic (subtree (buf, root (m)))) {
      tree& atom= subtree (buf, root (m));
      int   pos= index (m), nr= argument (m);
      atom->label=
          atom->label (0, pos) * atom->label (pos + nr, N (atom->label));
    }
    else if (m->k == MOD_INSERT) test_insert_slice (buf, m);
    else if (m->k == MOD_REMOVE) test_remove_slice (buf, m);
    else if (m->k == MOD_ASSIGN) subtree (buf, root (m))= m->t;
  }
}

// ===== 身份对账（0778）：跨 merge 后 buffer 与 shadow 顺序错位时，reconcile
// 按 TreeID（而非位置）删/移，绝不错删并发节点。复现「<alpha> 被吞」的病根：
// 位置型 diff_walk 会把 remove 落到错下标，身份对账则按 TreeID 精确删除。 =====

// JOIN 容器回退：本端 buffer 是 CONCAT(空 document,...) 这类多文档容器（label
// 与远端 body 的 document 不同），逐项对账会产生 remove+空/失配 insert（应用到
// et 时 can_insert 非法）。应回退整树 assign。覆盖 0778 JOIN 崩溃。
TEST_CASE (
    "loro reconcile: non-document buffer falls back to whole-tree assign") {
  ensure_labels ();
  // 远端 body: (document (para "a") (para "b"))
  tree body (DOCUMENT, 2);
  body[0]= mk_para ("a");
  body[1]= mk_para ("b");
  loro_shadow a;
  a->seed (body);
  string sa= a->export_snapshot ();

  // 本端 buffer 是多文档容器 CONCAT(空 document, 空 document, 空 document)
  tree empty_doc (DOCUMENT, 1);
  empty_doc[0]= tree ("");
  tree cont (CONCAT, 3);
  cont[0]= empty_doc;
  cont[1]= empty_doc;
  cont[2]= empty_doc;

  loro_shadow        b;
  tree               stub= cont; // 远端先到，本端还是容器 stub
  list<modification> mods= b->remote_diff_mods (sa, stub);
  // 必须是单个整树 assign（不产生 remove/空 insert）
  CHECK_EQ (N (mods), 1);
  if (N (mods) == 1) CHECK_EQ ((int) mods->item->k, (int) MOD_ASSIGN);
  // 应用后 buffer == 远端 body
  apply_mods (stub, mods);
  CHECK_EQ (stub == body, true);
}

// 同 peer 结构编辑：A（创建者）删掉中间 para "b"，B 共享血统后接收。身份
// 对账按 TreeID 删 b，且复用 a/c 的 rep（不整段重排）。
TEST_CASE ("loro reconcile: remote remove deletes correct child by TreeID") {
  ensure_labels ();
  tree t (DOCUMENT, 3);
  t[0]= mk_para ("a");
  t[1]= mk_para ("b");
  t[2]= mk_para ("c");

  loro_shadow a;
  a->seed (t);
  string s0= a->export_snapshot ();
  tree   tA= t;
  a->mirror_mod (tA, mod_remove (path (), 1, 1)); // -> [a, c]
  string sa= a->export_snapshot ();
  CHECK_EQ (N (a->to_tree ()) == 2, true);

  loro_shadow b;
  tree        tB;
  CHECK_EQ (b->import_and_build (s0, tB), true);
  CHECK_EQ (N (tB) == 3, true);

  apply_mods (tB, b->remote_diff_mods (sa, tB));
  CHECK_EQ (N (tB), 2);
  if (N (tB) == 2) {
    CHECK_EQ (tB[0][0]->label, "a");
    CHECK_EQ (tB[1][0]->label, "c");
  }
}

// 同 peer 纯新增：A 插入一个 para，B 数量落后但顺序一致。对账只插入新增项。
TEST_CASE ("loro reconcile: pure insert only adds new child") {
  ensure_labels ();
  tree t (DOCUMENT, 2);
  t[0]= mk_para ("a");
  t[1]= mk_para ("c");

  loro_shadow a;
  a->seed (t);
  string s0= a->export_snapshot ();
  tree   tA= t;
  tree   ins (DOCUMENT, 1);
  ins[0]= mk_para ("alpha");
  // 精确 mirror：doc_root 用 post-apply 的 [a,alpha,c]，parent=root 已在
  // id_map，走 mirror_insert 而非 reseed，root 身份不被换。
  tree tAfter (DOCUMENT, 3);
  tAfter[0]= tA[0];
  tAfter[1]= mk_para ("alpha");
  tAfter[2]= tA[1];
  a->mirror_mod (tAfter, mod_insert (path (), 1, ins));
  string sa= a->export_snapshot ();
  CHECK_EQ (N (a->to_tree ()) == 3, true);

  loro_shadow b;
  tree        tB;
  CHECK_EQ (b->import_and_build (s0, tB), true);
  CHECK_EQ (N (tB) == 2, true);

  apply_mods (tB, b->remote_diff_mods (sa, tB));
  CHECK_EQ (N (tB), 3);
  if (N (tB) == 3) {
    CHECK_EQ (tB[0][0]->label, "a");
    CHECK_EQ (tB[1][0]->label, "alpha");
    CHECK_EQ (tB[2][0]->label, "c");
  }
}

// 同 peer 文本编辑：A 改 para0 文本，B 共享身份接收。对账只改 para0 文本。
TEST_CASE ("loro reconcile: text edit on one para only touches that para") {
  ensure_labels ();
  tree t (DOCUMENT, 2);
  t[0]= mk_para ("x");
  t[1]= mk_para ("y");

  loro_shadow a;
  a->seed (t);
  string s0= a->export_snapshot ();
  tree   tA= t;
  a->mirror_mod (tA,
                 mod_insert (path (0) * 0, 1, tree ("X"))); // para0 "x"->"xX"
  tA[0][0]->label= string ("xX");
  string sa      = a->export_snapshot ();

  loro_shadow b;
  tree        tB;
  CHECK_EQ (b->import_and_build (s0, tB), true);

  apply_mods (tB, b->remote_diff_mods (sa, tB));
  CHECK_EQ (tB[0][0]->label, "xX");
  CHECK_EQ (tB[1][0]->label, "y");
}

// 同 peer 重排：A 删掉中间 para "b"，B 的 buffer 里 a/b/c 的 rep 与 A 共享。
// 身份对账删 b 后，a/c 的 rep 被复用（不销毁重建）。
TEST_CASE (
    "loro reconcile: remote remove preserves surviving tree_rep identity") {
  ensure_labels ();
  tree t (DOCUMENT, 3);
  t[0]= mk_para ("a");
  t[1]= mk_para ("b");
  t[2]= mk_para ("c");

  loro_shadow a;
  a->seed (t);
  string s0= a->export_snapshot ();
  tree   tA= t;
  a->mirror_mod (tA, mod_remove (path (), 1, 1)); // -> [a, c]
  string sa= a->export_snapshot ();

  loro_shadow b;
  tree        tB;
  CHECK_EQ (b->import_and_build (s0, tB), true);
  tree_rep* a_rep= inside (tB[0]); // "a" 的 rep
  tree_rep* c_rep= inside (tB[2]); // "c" 的 rep

  apply_mods (tB, b->remote_diff_mods (sa, tB));
  CHECK_EQ (N (tB), 2);
  if (N (tB) == 2) {
    CHECK_EQ (tB[0][0]->label, "a");
    CHECK_EQ (tB[1][0]->label, "c");
    // 复用 rep：a、c 仍是原来的 rep
    CHECK_EQ (inside (tB[0]) == a_rep, true);
    CHECK_EQ (inside (tB[1]) == c_rep, true);
  }
}

// ===== meta section（body 之外的文档部分）coarse 镜像 =====
// 这些用例验证 body 之外的 section（style/initial/...）作为带 __section__
// 标签的 独立 root 纳入同一 LoroDoc，与 body（roots[0]）共享一条 update 流，且
// body 的 字符级精确镜像不受影响。

// 多 section seed + round-trip，且 body 不受影响。
TEST_CASE ("loro_shadow: metadata section seed and round-trip") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("hello");
  loro_shadow sh;
  sh->seed (body);

  // style: (tuple "generic")
  tree style (TUPLE, 1);
  style[0]= tree ("generic");
  sh->seed_meta ("style", style);
  CHECK_EQ (sh->has_meta ("style"), true);
  CHECK_EQ (sh->meta_to_tree ("style") == style, true);

  // initial: (collection (assoc "page-medium" "paper"))
  tree initial (COLLECTION, 1);
  initial[0]= tree (ASSOCIATE, tree ("page-medium"), tree ("paper"));
  sh->seed_meta ("initial", initial);
  CHECK_EQ (sh->has_meta ("initial"), true);
  CHECK_EQ (sh->meta_to_tree ("initial") == initial, true);

  // body 仍是 roots[0]，to_tree 不含 meta
  CHECK_EQ (sh->to_tree () == body, true);
  CHECK_EQ (sh->has_id (body[0][0]), true);
}

// coarse replace：删旧 root + 重建，list_meta_sections 反映当前 section。
TEST_CASE ("loro_shadow: metadata coarse replace") {
  ensure_labels ();
  loro_shadow sh;
  tree        body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("x");
  sh->seed (body);

  tree s1 (TUPLE, 1);
  s1[0]= tree ("article");
  sh->seed_meta ("style", s1);
  CHECK_EQ (sh->meta_to_tree ("style") == s1, true);

  tree s2 (TUPLE, 2);
  s2[0]= tree ("generic");
  s2[1]= tree ("chinese");
  sh->mirror_meta_replace ("style", s2);
  CHECK_EQ (sh->meta_to_tree ("style") == s2, true);
  CHECK_EQ (sh->has_meta ("style"), true);

  // list_meta_sections 含 style
  array<string> secs = sh->list_meta_sections ();
  bool          found= false;
  for (int i= 0; i < N (secs); i++)
    if (secs[i] == "style") found= true;
  CHECK_EQ (found, true);
}

// e2e：A seed body+meta → snapshot → B import → B 经 sync_meta_from_shadow
// 重建账本，读到与 A 一致的 body 与各 section。
TEST_CASE ("loro e2e: metadata propagates across snapshot import") {
  ensure_labels ();
  loro_shadow a;
  tree        body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("hi");
  a->seed (body);
  tree style (TUPLE, 1);
  style[0]= tree ("beamer");
  a->seed_meta ("style", style);
  tree initial (COLLECTION, 1);
  initial[0]= tree (ASSOCIATE, tree ("page-medium"), tree ("beamer"));
  a->seed_meta ("initial", initial);
  // export_snapshot 内部 commit，把 seed_meta 的 op 一并带进 snapshot
  string snap= a->export_snapshot ();
  CHECK_EQ (N (snap) > 0, true);

  loro_shadow b;
  CHECK_EQ (b->import_data (snap), true);
  b->sync_meta_from_shadow ();
  CHECK_EQ (b->has_meta ("style"), true);
  CHECK_EQ (b->has_meta ("initial"), true);
  CHECK_EQ (b->meta_to_tree ("style") == style, true);
  CHECK_EQ (b->meta_to_tree ("initial") == initial, true);
  CHECK_EQ (b->to_tree () == body, true);
}

// 双向 coarse 替换收敛：A 改 style → 导出 → B import 后读到 A 的值。
// 顺序执行（非真并发），验证删旧 root + 建新 root 在 CRDT 合并后 B
// 端只看到新值。
TEST_CASE ("loro meta: bidirectional coarse replace converges") {
  ensure_labels ();
  loro_shadow a;
  tree        body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("c");
  a->seed (body);
  tree sA (TUPLE, 1);
  sA[0]= tree ("generic");
  a->seed_meta ("style", sA);
  string s0= a->export_snapshot (); // 公共初始（style=sA）

  // A 改 style -> article
  tree sA2 (TUPLE, 1);
  sA2[0]= tree ("article");
  a->mirror_meta_replace ("style", sA2);
  string sa= a->export_snapshot ();

  // B 导入初始后再导入 A 的改动，应收敛到 article
  loro_shadow b;
  CHECK_EQ (b->import_data (s0), true);
  b->sync_meta_from_shadow ();
  CHECK_EQ (b->meta_to_tree ("style") == sA, true);
  CHECK_EQ (b->import_data (sa), true);
  b->sync_meta_from_shadow ();
  CHECK_EQ (b->meta_to_tree ("style") == sA2, true);
}

// 回归：meta section 存在时，body 的字符级精确镜像（身份不变）仍然成立，
// 且 body 编辑不破坏 meta section。
TEST_CASE ("loro_shadow: body editing unaffected by metadata sections") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]      = tree (PARA, 1);
  body[0][0]   = tree (CONCAT, 1);
  body[0][0][0]= tree ("");
  loro_shadow sh;
  sh->seed (body);

  // 预先 seed 若干 meta section
  tree style (TUPLE, 1);
  style[0]= tree ("generic");
  sh->seed_meta ("style", style);
  tree att (COLLECTION, 1);
  att[0]= tree (ASSOCIATE, tree ("k"), tree ("v"));
  sh->seed_meta ("attachments", att);

  // body 字符级编辑：身份应逐字不变（精确 LoroText 路径）
  path          atom= path (0) * 0 * 0;
  mogan_tree_id id0 = sh->get_id (body[0][0][0]);
  sh->mirror_mod (body, mod_insert (atom, 0, tree ("z")));
  body[0][0][0]->label= string ("z");
  mogan_tree_id id1   = sh->get_id (body[0][0][0]);
  CHECK_EQ (id1.peer == id0.peer && id1.counter == id0.counter, true);
  CHECK_EQ (sh->to_tree () == body, true);

  // meta section 仍在且未被破坏
  CHECK_EQ (sh->meta_to_tree ("style") == style, true);
  CHECK_EQ (sh->meta_to_tree ("attachments") == att, true);
}

#endif // LORO_ENABLED
