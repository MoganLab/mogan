/** \file loro_diff_bench.cpp
 *  \copyright GPLv3
 *  \details 测量 remote_diff_mods / to_tree 在类删除场景下的耗时，定位
 *            apply_remote 性能瓶颈（diff_walk 深比较 vs to_tree 解析）。
 *            运行：xmake b loro_diff_bench && xmake r loro_diff_bench
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_shadow.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

#include "basic.hpp"
#include "nanobench.h"

using namespace moebius;

#ifdef LORO_ENABLED

static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

// document(para*P)，每个 para 内 concat 含 N 个原子
static tree
make_doc (int npara, int nwords) {
  tree doc (DOCUMENT, npara);
  for (int p= 0; p < npara; p++) {
    tree para (PARA, 1);
    tree con (CONCAT, nwords);
    for (int w= 0; w < nwords; w++)
      con[w]= tree (string ("word") * as_string (w));
    para[0]= con;
    doc[p] = para;
  }
  return doc;
}

int
main () {
  ensure_labels ();
  tree init= make_doc (50, 20); // 50 段 × 20 原子

  loro_shadow a;
  a->seed (init);
  string      s0= a->export_snapshot ();
  loro_shadow b;
  tree        tB;
  b->import_and_build (s0, tB);

  // b 删一个原子（模拟删除），导出 update
  tB[0][0][0]->label= string ("");
  b->mirror_mod (tB, mod_remove (path (0) * 0 * 0, 0, 4));
  string update= b->export_snapshot ();

  ankerl::nanobench::Bench bench;
  bench.title ("apply_remote 拆解 (50 段 × 20 原子)");

  bench.run ("to_tree", [&] {
    tree t= a->to_tree ();
    ankerl::nanobench::doNotOptimizeAway (t);
  });

  bench.run ("remote_diff_mods (import+diff)", [&] {
    tree               buf = init;
    list<modification> mods= a->remote_diff_mods (update, buf);
    ankerl::nanobench::doNotOptimizeAway (mods);
  });

  return 0;
}

#else
int
main () {
  return 0;
}
#endif
