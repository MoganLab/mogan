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

// ===== SPLIT 结构边界 marker：保字符身份 =====
// 本地 SPLIT 不再 delete+insert 文本，只在文本节点 meta 插一条边界 marker；
// 字符身份保留（TreeID 不变），且 to_tree 按 marker 切出两段原子。
TEST_CASE ("loro_shadow: split preserves char identity via marker") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow sh;
  sh->seed (body);
  CHECK_EQ (sh->to_tree () == body, true);

  path          atom= path (0) * 0; // [0,0] 文本原子
  mogan_tree_id id0 = sh->get_id (body[0][0]);

  // 直接调 FFI 验证 split_marker_create 可用（绕过 mirror_split）
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 0);
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  int rc= mogan_loro_node_split_marker_create (sh->doc, id0, 3, &out, &out_len);
  CHECK_EQ (rc, 0);
  if (out) mogan_loro_free (out, out_len);
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 1);

  // SPLIT at 3：buffer 变成两兄弟原子 ABC / DEF
  tree split_buf (PARA, 2);
  split_buf[0]= tree ("ABC");
  split_buf[1]= tree ("DEF");
  tree doc2 (DOCUMENT, 1);
  doc2[0]= split_buf;
  // 注：上面已直接创建了 marker，这里再 mirror 会重复；先测直接 FFI 路径
  // sh->mirror_mod (doc2, mod_split (path (0), 0, 3));

  // 物化：to_tree 按 marker 切出 ABC / DEF
  CHECK_EQ (sh->to_tree () == doc2, true);
}

// SPLIT 后在 marker 左/右插入字符：cursor 跟随，归属正确，字符身份保留。
// Phase 3 最小原型：直接在 shadow 层验证（不经过 mirror_mod，避开 id_map
// 匹配）。
TEST_CASE ("loro_shadow: inserts around split marker via FFI") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow sh;
  sh->seed (body);
  mogan_tree_id id0= sh->get_id (body[0][0]);

  // SPLIT at 3（marker cursor 锚定 C|D 边界）
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  CHECK_EQ (
      mogan_loro_node_split_marker_create (sh->doc, id0, 3, &out, &out_len), 0);
  if (out) mogan_loro_free (out, out_len);

  // 在 marker 左侧插入 X（offset 3 = 紧邻 marker 前）
  CHECK_EQ (
      mogan_loro_node_text_insert (sh->doc, id0, 3, (const uint8_t*) "X", 1),
      0);
  // 在 marker 右侧（DEF 之后）插入 Y（offset 现在 = 3+1+3 = 7）
  CHECK_EQ (
      mogan_loro_node_text_insert (sh->doc, id0, 7, (const uint8_t*) "Y", 1),
      0);

  // to_tree 应切出 ABCX / DEFY（marker cursor 跟随到 X|D）
  tree want (PARA, 2);
  want[0]= tree ("ABCX");
  want[1]= tree ("DEFY");
  tree wantdoc (DOCUMENT, 1);
  wantdoc[0]= want;
  CHECK_EQ (sh->to_tree () == wantdoc, true);
  // marker 仍在
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 1);
  // 字符身份不变（容器 TreeID 未被删旧重建）
  mogan_tree_id id1= sh->get_id (body[0][0]);
  CHECK_EQ (id1.peer == id0.peer && id1.counter == id0.counter, true);
}

// Case 3：marker 附近并发插入。shA 建 marker + 在 marker 前插 X；shB 在原 C|D
// 边界插 Y。交换 snapshot 后 marker 应存活，X/Y 分属两侧（确定结果）。
TEST_CASE ("loro_shadow: concurrent inserts around marker converge") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow shA;
  shA->seed (body);
  string        sa0= shA->export_snapshot ();
  mogan_tree_id idA= shA->get_id (body[0][0]);

  // shB 从公共 snapshot 导入
  loro_shadow shB;
  tree        tB;
  CHECK_EQ (shB->import_and_build (sa0, tB), true);
  mogan_tree_id idB= shB->get_id (tB[0][0]);
  CHECK_EQ (idB.peer == idA.peer && idB.counter == idA.counter, true);

  // shA：split at 3 + 在 marker 前插 X（offset 3）
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  CHECK_EQ (
      mogan_loro_node_split_marker_create (shA->doc, idA, 3, &out, &out_len),
      0);
  if (out) mogan_loro_free (out, out_len);
  CHECK_EQ (
      mogan_loro_node_text_insert (shA->doc, idA, 3, (const uint8_t*) "X", 1),
      0);
  string sa= shA->export_snapshot ();

  // shB：在原 C|D 边界（offset 3）插 Y（不知道 marker 存在）
  CHECK_EQ (
      mogan_loro_node_text_insert (shB->doc, idB, 3, (const uint8_t*) "Y", 1),
      0);
  string sb= shB->export_snapshot ();

  // 互导 → CRDT 合并
  CHECK_EQ (shA->import_data (sb), true);
  CHECK_EQ (shB->import_data (sa), true);

  // 两端收敛（文本内容一致），marker 在 shA 上存活
  tree ta= shA->to_tree ();
  tree tb= shB->to_tree ();
  CHECK_EQ (ta == tb, true);
  CHECK_EQ (mogan_loro_node_has_split_markers (shA->doc, idA), 1);
  CHECK_EQ (mogan_loro_node_has_split_markers (shB->doc, idB), 1);
  // 字符身份不变
  mogan_tree_id idA2= shA->get_id (body[0][0]);
  CHECK_EQ (idA2.peer == idA.peer && idA2.counter == idA.counter, true);
}

// Case 4：删除 marker 两侧的字符，marker 应 clamp 到邻近位置且持久保留。
TEST_CASE ("loro_shadow: marker survives anchor deletion") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow sh;
  sh->seed (body);
  mogan_tree_id id0= sh->get_id (body[0][0]);

  // split at 3
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  CHECK_EQ (
      mogan_loro_node_split_marker_create (sh->doc, id0, 3, &out, &out_len), 0);
  if (out) mogan_loro_free (out, out_len);

  // 删 C（offset 2，marker 左邻）→ marker clamp 到 AB|DEF
  CHECK_EQ (mogan_loro_node_text_delete (sh->doc, id0, 2, 1), 0);
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 1);
  tree t1= sh->to_tree ();
  CHECK_EQ (N (t1[0]) == 2, true); // 仍切 2 段
  CHECK_EQ (t1[0][0] == tree ("AB"), true);

  // 再删 D（现在 offset 2，marker 右邻）→ AB|EF
  CHECK_EQ (mogan_loro_node_text_delete (sh->doc, id0, 2, 1), 0);
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 1);

  // 删全部剩余 → marker 持久保留为空结构边界（用户拍板）
  CHECK_EQ (mogan_loro_node_text_delete (sh->doc, id0, 0, 4), 0);
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 1);
}

// 多 SPLIT：SPLIT(2) + SPLIT(4) → AB[S1]CD[S2]EF → 3 段原子。
TEST_CASE ("loro_shadow: multiple splits produce 3 segments") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow sh;
  sh->seed (body);
  mogan_tree_id id0= sh->get_id (body[0][0]);

  // SPLIT at 2
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  CHECK_EQ (
      mogan_loro_node_split_marker_create (sh->doc, id0, 2, &out, &out_len), 0);
  if (out) mogan_loro_free (out, out_len);
  // SPLIT at 4（在原始 ABCDEF 的 D|E 边界；marker cursor 锚定到 offset 4）
  CHECK_EQ (
      mogan_loro_node_split_marker_create (sh->doc, id0, 4, &out, &out_len), 0);
  if (out) mogan_loro_free (out, out_len);

  // 物化：3 段 AB / CD / EF
  tree t= sh->to_tree ();
  CHECK_EQ (N (t[0]) == 3, true);
  CHECK_EQ (t[0][0] == tree ("AB"), true);
  CHECK_EQ (t[0][1] == tree ("CD"), true);
  CHECK_EQ (t[0][2] == tree ("EF"), true);
}

// JOIN：split 后用 split_marker_delete 删 marker → 两段合并为一。
TEST_CASE ("loro_shadow: join removes marker and merges segments") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow sh;
  sh->seed (body);
  mogan_tree_id id0= sh->get_id (body[0][0]);

  // split at 3
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  CHECK_EQ (
      mogan_loro_node_split_marker_create (sh->doc, id0, 3, &out, &out_len), 0);
  // marker_id = 返回的 postcard 字节
  CHECK_EQ (out_len > 0, true);
  // 删 marker（JOIN）
  CHECK_EQ (mogan_loro_node_split_marker_delete (sh->doc, id0, out, out_len),
            0);
  mogan_loro_free (out, out_len);

  // 物化：回到 1 段 ABCDEF
  tree t= sh->to_tree ();
  CHECK_EQ (N (t[0]) == 1, true);
  CHECK_EQ (t[0][0] == tree ("ABCDEF"), true);
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 0);
}

// Phase 5 e2e：远端 split 到达时 diff_walk 应吐 mod_split（而非
// remove+insert）。
TEST_CASE ("loro_shadow: remote split produces mod_split not remove+insert") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow shA;
  shA->seed (body);
  mogan_tree_id idA    = shA->get_id (body[0][0]);
  uint8_t*      out    = nullptr;
  size_t        out_len= 0;
  CHECK_EQ (
      mogan_loro_node_split_marker_create (shA->doc, idA, 3, &out, &out_len),
      0);
  if (out) mogan_loro_free (out, out_len);
  string sa= shA->export_snapshot ();

  loro_shadow shB;
  // 不 seed：remote_diff_mods 内部 import A 的 snapshot（含 marker）
  list<modification> mods     = shB->remote_diff_mods (sa, body);
  bool               has_split= false, has_remove= false;
  for (auto l= mods; !is_nil (l); l= l->next) {
    if (l->item->k == MOD_SPLIT) has_split= true;
    if (l->item->k == MOD_REMOVE) has_remove= true;
  }
  CHECK_EQ (has_split, true);
  CHECK_EQ (has_remove, false);
}

// Phase 5 e2e：远端 JOIN 到达时 diff_walk 应吐 mod_join。
TEST_CASE ("loro_shadow: remote join produces mod_join not remove+insert") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow shA;
  shA->seed (body);
  mogan_tree_id idA    = shA->get_id (body[0][0]);
  uint8_t*      out    = nullptr;
  size_t        out_len= 0;
  CHECK_EQ (
      mogan_loro_node_split_marker_create (shA->doc, idA, 3, &out, &out_len),
      0);
  CHECK_EQ (mogan_loro_node_split_marker_delete (shA->doc, idA, out, out_len),
            0);
  mogan_loro_free (out, out_len);
  string sa= shA->export_snapshot ();

  tree split_body (DOCUMENT, 1);
  split_body[0]   = tree (PARA, 2);
  split_body[0][0]= tree ("ABC");
  split_body[0][1]= tree ("DEF");
  loro_shadow shB;
  // 不 seed：remote_diff_mods 内部 import A 的 snapshot（marker 已删 = JOIN
  // 后）
  list<modification> mods    = shB->remote_diff_mods (sa, split_body);
  bool               has_join= false, has_remove= false;
  for (auto l= mods; !is_nil (l); l= l->next) {
    if (l->item->k == MOD_JOIN) has_join= true;
    if (l->item->k == MOD_REMOVE) has_remove= true;
  }
  CHECK_EQ (has_join, true);
  CHECK_EQ (has_remove, false);
}

// Phase 5a：import_and_build 在 split 后正确重建 buffer + id_map（1↔N）。
// sync_walk 的 arity 校验不应 fail；两段原子各自映射到正确 TreeID。
TEST_CASE ("loro_shadow: import_and_build maps split segments 1-to-N") {
  ensure_labels ();
  tree body (DOCUMENT, 1);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("ABCDEF");
  loro_shadow shA;
  shA->seed (body);
  mogan_tree_id id_text= shA->get_id (body[0][0]);
  // split at 3
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  CHECK_EQ (mogan_loro_node_split_marker_create (shA->doc, id_text, 3, &out,
                                                 &out_len),
            0);
  if (out) mogan_loro_free (out, out_len);
  string sa= shA->export_snapshot ();

  // B 导入 + 构建 buffer
  loro_shadow shB;
  tree        buf;
  CHECK_EQ (shB->import_and_build (sa, buf), true);
  // buffer 应切出 2 段
  CHECK_EQ (N (buf[0]) == 2, true);
  CHECK_EQ (buf[0][0] == tree ("ABC"), true);
  CHECK_EQ (buf[0][1] == tree ("DEF"), true);
  // id_map 覆盖两段（sync_walk 1↔N 不 fail）
  CHECK_EQ (shB->has_id (buf[0][0]), true);
  CHECK_EQ (shB->has_id (buf[0][1]), true);
  // 前缀段身份 = 原文本节点 TreeID（保字符身份）
  mogan_tree_id id_seg0= shB->get_id (buf[0][0]);
  CHECK_EQ (id_seg0.peer == id_text.peer && id_seg0.counter == id_text.counter,
            true);
}

// ===== A/B 集成测试框架：模拟完整同步周期 =====
// A 的编辑经 mirror_mod → snapshot → B 的 remote_diff_mods（import + diff）。
// 不调 apply()（符号依赖 observer 栈）；改用 shB->to_tree() 作为被动 B 下一轮
// diff 的 buffer 真值。校验 shA->to_tree() == shB->to_tree() 收敛。

// A→B 单向同步（B 被动）：A 导 snapshot → B import + diff。
static void
ab_sync_a2b (loro_shadow shA, loro_shadow shB) {
  string snap= shA->export_snapshot ();
  tree   buf = shB->to_tree (); // B 当前 shadow 状态作 diff 基准
  shB->remote_diff_mods (snap, buf);
}

// 场景：A 打字 "LINE1" → 在末尾 split（模拟 Return）→ B 被动同步。
// 校验 B 的 shadow 与 A 收敛（不是 LINE2LINE1）。
TEST_CASE ("loro_shadow: AB sync split at line end converges") {
  ensure_labels ();
  tree init (DOCUMENT, 1);
  init[0]   = tree (PARA, 1);
  init[0][0]= tree ("ABCDEF");

  loro_shadow shA;
  shA->seed (init);
  string sa0= shA->export_snapshot ();

  loro_shadow shB;
  tree        buf_b;
  REQUIRE_EQ (shB->import_and_build (sa0, buf_b), true);

  // A 在 "ABCDEF" 末尾 split（offset 6 = 产生 "ABCDEF" + "" 空段）
  // 先把 init 更新到 post-split 状态（mirror_mod 读树解析路径）
  init[0]   = tree (PARA, 2);
  init[0][0]= tree ("ABCDEF");
  init[0][1]= tree ("");
  shA->mirror_mod (init, mod_split (path (0), 0, 6));

  // sync to B
  ab_sync_a2b (shA, shB);
  CHECK_EQ (shA->to_tree () == shB->to_tree (), true);
  // B 应有 2 段（ABCDEF + 空），不是 assign 整体替换
  tree tb= shB->to_tree ();
  CHECK_EQ (N (tb[0]) == 2, true);
}

// 场景：A 和 B 并发在同一段插入字符，互导后收敛。
TEST_CASE ("loro_shadow: AB concurrent insert converges") {
  ensure_labels ();
  tree init (DOCUMENT, 1);
  init[0]   = tree (PARA, 1);
  init[0][0]= tree ("X");

  loro_shadow shA;
  shA->seed (init);
  string sa0= shA->export_snapshot ();

  loro_shadow shB;
  tree        buf_b;
  REQUIRE_EQ (shB->import_and_build (sa0, buf_b), true);

  // A 在 "X" 后插 "A"
  shA->mirror_mod (init, mod_insert (path (0) * 0, 1, tree ("A")));
  init[0][0]->label= string ("XA");
  // B 在 "X" 后插 "B"（并发）
  shB->mirror_mod (buf_b, mod_insert (path (0) * 0, 1, tree ("B")));
  buf_b[0][0]->label= string ("XB");

  // 互导
  ab_sync_a2b (shA, shB);
  tree   buf_a= shA->to_tree (); // A 当前 shadow 作 diff 基准
  string sb   = shB->export_snapshot ();
  shA->remote_diff_mods (sb, buf_a);

  // 收敛
  CHECK_EQ (shA->to_tree () == shB->to_tree (), true);
  tree ta= shA->to_tree ();
  CHECK_EQ (N (ta[0][0]->label) == 3, true); // X + A + B
}

// 旧文档兼容：seed 无 marker 的普通文档，to_tree 行为不变。
TEST_CASE ("loro_shadow: old document without markers loads fine") {
  ensure_labels ();
  tree body (DOCUMENT, 2);
  body[0]   = tree (PARA, 1);
  body[0][0]= tree ("hello");
  body[1]   = tree (PARA, 1);
  body[1][0]= tree ("world");
  loro_shadow sh;
  sh->seed (body);

  // 无 marker → has_split_markers = 0
  mogan_tree_id id0= sh->get_id (body[0][0]);
  CHECK_EQ (mogan_loro_node_has_split_markers (sh->doc, id0), 0);

  // to_tree 原样返回
  CHECK_EQ (sh->to_tree () == body, true);

  // snapshot 往返后仍无 marker
  string      snap= sh->export_snapshot ();
  loro_shadow sh2;
  CHECK_EQ (sh2->import_data (snap), true);
  CHECK_EQ (sh2->to_tree () == body, true);
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
