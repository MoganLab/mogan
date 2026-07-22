/** \file loro_tmu_test.cpp
 *  \copyright GPLv3
 *  \details 真实文档保真测试：遍历 TeXmacs/tests/tmu/，对每个夹具做
 *            tmu_document_to_tree -> tree_to_loro ->
 * loro_to_tree，断言与原树深度相等。
 *            覆盖真实文档里的数学、表格、图片(raw-data)、样式等结构，暴露映射盲区。
 *
 *            仅在 LORO_ENABLED（xmake option
 * libloro=y）下编译用例；关闭时为空二进制。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "file.hpp"
#include "loro.hpp"
#include "moe_doctests.hpp"
#include "tmu.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include "url.hpp"
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

#ifdef LORO_ENABLED

static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

// 定位 tmu 夹具目录
static string
resolve_fixture_dir () {
  const char* cands[]= {
      "../../../../TeXmacs/tests/tmu/",
  };
  int n= sizeof (cands) / sizeof (cands[0]);
  for (int i= 0; i < n; i++) {
    string c= cands[i];
    if (is_directory (c)) return c;
  }
  return string ();
}

static bool
ends_with_tmu (string name) {
  int n= N (name);
  return n >= 4 && name (n - 4, n) == ".tmu";
}

TEST_CASE ("loro real: round-trip all tmu fixtures") {
  ensure_labels ();
  string dir= resolve_fixture_dir ();
  if (N (dir) == 0) {
    FAIL ("找不到 TeXmacs/tests/tmu 夹具目录");
    return;
  }

  bool          err   = false;
  array<string> names = read_directory (dir, err);
  int           tested= 0, passed= 0, failed= 0, skipped= 0, shown= 0;
  for (int i= 0; i < N (names); i++) {
    string name= names[i];
    if (!ends_with_tmu (name)) continue;
    string content= string_load (dir * name);
    if (N (content) == 0) continue;

    tree doc= tmu_document_to_tree (content);
    // 解析失败会返回 (ERROR "bad format or data")；跳过不计，避免把 error
    // 树的恒等 round-trip 误当通过。若全部都 ERROR，下方 tested>0 会兜住（提示
    // init 不足）。
    if (is_compound (doc) && L (doc) == ERROR) {
      skipped++;
      continue;
    }
    tree back= loro_to_tree (tree_to_loro (doc));
    tested++;
    if (back == doc) {
      passed++;
      continue;
    }
    failed++;
    if (shown < 3) {
      // 打印前 600 字符的 tmu 表示，定位分歧起点
      string ot  = tree_to_tmu (doc);
      string bt  = tree_to_tmu (back);
      int    upto= N (ot) < 600 ? N (ot) : 600;
      cout << "MISMATCH: " << name << "  (arity orig=" << N (doc)
           << " back=" << N (back) << ")" << LF;
      cout << "  orig: " << ot (0, upto) << (N (ot) > upto ? "..." : "") << LF;
      cout << "  back: " << bt (0, upto) << (N (bt) > upto ? "..." : "") << LF;
      shown++;
    }
  }

  cout << "loro real: tested=" << tested << " passed=" << passed
       << " failed=" << failed << " skipped(parse-error)=" << skipped << LF;
  CHECK_EQ (tested > 0, true);
  CHECK_EQ (failed == 0, true);
}

#endif // LORO_ENABLED
