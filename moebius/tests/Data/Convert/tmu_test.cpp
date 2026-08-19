/** \file tmu_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for TMU serialization roundtrips
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "tmu.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::CONCAT;
using moebius::DOCUMENT;
using moebius::drd::init_std_drd;

TEST_SUITE ("tmu") {

  TEST_CASE ("roundtrip plain words") {
    tree doc (DOCUMENT, tree ("hello"), tree ("world"));
    tree back= tmu_to_tree (tree_to_tmu (doc));
    REQUIRE (N (back) == 2);
    CHECK (back[0] == tree ("hello"));
    CHECK (back[1] == tree ("world"));
  }

  TEST_CASE ("roundtrip spaces inside words") {
    tree doc (DOCUMENT, tree ("a b  c"));
    tree back= tmu_to_tree (tree_to_tmu (doc));
    REQUIRE (N (back) == 1);
    CHECK (back[0] == tree ("a b  c"));
  }

  TEST_CASE ("roundtrip escaped specials") {
    // 反斜杠与标记字符需要转义后往返还原
    tree doc (DOCUMENT, tree ("a\\b"), tree ("x<y"), tree ("p|q"),
              tree ("m>n"));
    tree back= tmu_to_tree (tree_to_tmu (doc));
    REQUIRE (N (back) == 4);
    CHECK (back[0] == tree ("a\\b"));
    CHECK (back[1] == tree ("x<y"));
    CHECK (back[2] == tree ("p|q"));
    CHECK (back[3] == tree ("m>n"));
  }

  TEST_CASE ("roundtrip utf8 text") {
    // 多字节 utf8 字符在词元扫描中不可被单字节截断
    tree doc (DOCUMENT, tree ("caf\xC3\xA9 ok"));
    tree back= tmu_to_tree (tree_to_tmu (doc));
    REQUIRE (N (back) == 1);
    CHECK (back[0] == tree ("caf\xC3\xA9 ok"));
  }

  TEST_CASE ("roundtrip concat with markup") {
    init_std_drd (); // 注册内置标签名,否则 as_string(RIGID) 为 "?"
    tree par (CONCAT, tree ("ab"), tree (RIGID, tree ("cd")), tree ("ef"));
    tree doc (DOCUMENT, par);
    tree back= tmu_to_tree (tree_to_tmu (doc));
    REQUIRE (N (back) == 1);
    tree bp= back[0];
    CHECK (is_concat (bp));
    REQUIRE (N (bp) == 3);
    CHECK (bp[0] == tree ("ab"));
    // 未知标记经 `<?|...>` 形式往返,按名字比较而非标签编号
    CHECK (is_compound (bp[1], string ("rigid")));
    CHECK (bp[1][0] == tree ("cd"));
    CHECK (bp[2] == tree ("ef"));
  }

  TEST_CASE ("roundtrip long word with backslashes") {
    // 密集转义场景:转义后字符与后续 utf8 序列一起复制
    string s ("\\\\\\\\x");
    tree   doc (DOCUMENT, tree (s));
    tree   back= tmu_to_tree (tree_to_tmu (doc));
    REQUIRE (N (back) == 1);
    CHECK (back[0] == tree (s));
  }

} // TEST_SUITE
