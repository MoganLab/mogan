/** \file loro_shadow_test.cpp
 *  \copyright GPLv3
 *  \details Phase 2 Step 3 gate：seed 一棵树到 shadow live doc，验证
 *            to_tree / export_snapshot(→loro_to_tree)
 * 都与原树深度相等，且身份表覆盖各节点。 仅 LORO_ENABLED 下编译用例。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro.hpp"
#include "loro_shadow.hpp"
#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
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

#endif // LORO_ENABLED
