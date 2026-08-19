/** \file tree_observer_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for raw_split / raw_join / raw_insert / raw_remove
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include "tree_observer.hpp"

#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;

// 生产代码导出但未写入 hpp,测试中补声明
void raw_split (tree& ref, int pos, int at);
void raw_join (tree& ref, int pos);
void raw_remove (tree& ref, int pos, int nr);

// 修改链引用的全局编辑树与 ip 观察者定义在 mogan 主程序侧,
// 测试中给出未挂接的独立桩实现
tree the_et;
path
obtain_ip (tree& ref) {
  (void) ref;
  return path ();
}
bool
ip_attached (path ip) {
  (void) ip;
  return false;
}
observer
list_observer (observer o1, observer o2) {
  (void) o1;
  (void) o2;
  return observer ();
}

TEST_SUITE ("tree_observer") {

  TEST_CASE ("raw_split compound keeps siblings") {
    tree doc (DOCUMENT);
    doc << tree ("p0") << tree ("p1") << tree ("p2") << tree ("p3");
    tree inner (CONCAT, tree ("aaa"), tree ("bbb"));
    doc[2]= inner;
    raw_split (doc, 2, 1); // 把 concat 的第 1 个孩子处切开
    CHECK_EQ (N (doc), 5);
    CHECK (doc[0] == tree ("p0"));
    CHECK (doc[1] == tree ("p1"));
    CHECK_EQ (N (doc[2]), 1);
    CHECK (doc[2][0] == tree ("aaa"));
    CHECK_EQ (N (doc[3]), 1);
    CHECK (doc[3][0] == tree ("bbb"));
    CHECK (doc[4] == tree ("p3"));
  }

  TEST_CASE ("raw_split at tail boundary") {
    tree doc (DOCUMENT);
    doc << tree ("a") << tree ("b") << tree ("c");
    tree inner (CONCAT, tree ("x"), tree ("y"));
    doc[1]= inner;
    raw_split (doc, 1, 2); // 在最后一个孩子边界切,搬移空块
    CHECK_EQ (N (doc), 4);
    CHECK (doc[1] == tree (CONCAT, tree ("x"), tree ("y")));
    CHECK (doc[2] == tree (CONCAT));
  }

  TEST_CASE ("raw_split atomic text") {
    tree doc (DOCUMENT);
    doc << tree ("hello world") << tree ("tail");
    raw_split (doc, 0, 5); // 原子文本中间切开
    CHECK_EQ (N (doc), 3);
    CHECK (doc[0] == tree ("hello"));
    CHECK (doc[1] == tree (" world"));
    CHECK (doc[2] == tree ("tail"));
  }

  TEST_CASE ("raw_join compound merges children") {
    tree doc (DOCUMENT);
    tree c1 (CONCAT, tree ("a"), tree ("b"));
    tree c2 (CONCAT, tree ("c"));
    doc << c1 << c2 << tree ("z");
    raw_join (doc, 0);
    CHECK_EQ (N (doc), 2);
    CHECK (doc[0] == tree (CONCAT, tree ("a"), tree ("b"), tree ("c")));
    CHECK (doc[1] == tree ("z"));
  }

  TEST_CASE ("raw_join at tail boundary") {
    tree doc (DOCUMENT);
    tree c1 (CONCAT, tree ("a"));
    tree c2 (CONCAT, tree ("b"));
    doc << tree ("x") << c1 << c2;
    raw_join (doc, 1); // 尾部合并,搬移空块
    CHECK_EQ (N (doc), 2);
    CHECK (doc[0] == tree ("x"));
    CHECK (doc[1] == tree (CONCAT, tree ("a"), tree ("b")));
  }

  TEST_CASE ("raw_join atomic texts") {
    tree doc (DOCUMENT);
    doc << tree ("foo") << tree ("bar") << tree ("end");
    raw_join (doc, 0); // 两个原子直接合并字符串
    CHECK_EQ (N (doc), 2);
    CHECK (doc[0] == tree ("foobar"));
    CHECK (doc[1] == tree ("end"));
  }

  TEST_CASE ("raw_join compounds keep children") {
    tree doc (DOCUMENT);
    tree c1 (CONCAT, tree ("foo"));
    tree c2 (CONCAT, tree ("bar"));
    doc << c1 << c2 << tree ("end");
    raw_join (doc, 0); // 复合节点 join 时合并孩子序列
    CHECK_EQ (N (doc), 2);
    CHECK_EQ (N (doc[0]), 2);
    CHECK (doc[0][0] == tree ("foo"));
    CHECK (doc[0][1] == tree ("bar"));
    CHECK (doc[1] == tree ("end"));
  }

  TEST_CASE ("split join roundtrip") {
    tree doc (DOCUMENT);
    doc << tree ("p0") << tree ("p1") << tree ("p2");
    tree keep= copy (doc);
    tree c (CONCAT, tree ("alpha"), tree ("beta"));
    doc[1] = c;
    keep[1]= tree (CONCAT, tree ("alpha"), tree ("beta"));
    raw_split (doc, 1, 1);
    raw_join (doc, 1);
    CHECK (doc == keep);
  }

  TEST_CASE ("raw_insert then remove roundtrip") {
    tree doc (DOCUMENT);
    doc << tree ("p0") << tree ("p2");
    // raw_insert 的复合分支取 t 的孩子逐个插入,单孩子需包装一层
    raw_insert (doc, 1, tree (DOCUMENT, tree ("p1")));
    CHECK_EQ (N (doc), 3);
    CHECK (doc[1] == tree ("p1"));
    raw_remove (doc, 1, 1);
    CHECK_EQ (N (doc), 2);
    CHECK (doc[0] == tree ("p0"));
    CHECK (doc[1] == tree ("p2"));
  }

} // TEST_SUITE
