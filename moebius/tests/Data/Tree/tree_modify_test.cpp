/** \file tree_modify_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for correct_node / correct_downwards / simplify_concat
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_modify.hpp"
#include "tree_observer.hpp"

#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::drd::init_std_drd;
using moebius::drd::the_drd;

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

TEST_SUITE ("tree_modify") {

TEST_CASE ("drd contains label overload") {
  init_std_drd ();
  CHECK (the_drd->contains (string ("document")));
  CHECK (the_drd->contains (DOCUMENT));
  CHECK (the_drd->contains (CONCAT));
  CHECK (!the_drd->contains (make_tree_label ("no-such-tag-xyz")));
}

TEST_CASE ("correct_node fixes bad arity") {
  init_std_drd ();
  // document 要求至少 1 个孩子(fixed arity 校验失败时清空)
  tree t (tree (RIGID, tree ("a"), tree ("b"))); // rigid 定长 1
  correct_node (t);
  CHECK (t == tree (""));
}

TEST_CASE ("correct_node keeps valid arity") {
  init_std_drd ();
  tree t (tree (RIGID, tree ("a")));
  correct_node (t);
  CHECK (t == tree (RIGID, tree ("a")));
}

TEST_CASE ("correct_node merges adjacent atomics in concat") {
  init_std_drd ();
  tree t (tree (CONCAT, tree ("a"), tree ("b"), tree ("")));
  correct_node (t);
  // 相邻原子合并、空串并走后只剩一个原子;correct_node 不展开单元素 concat
  CHECK (t == tree (CONCAT, tree ("ab")));
}

TEST_CASE ("correct_downwards recursion") {
  init_std_drd ();
  tree inner (tree (RIGID, tree ("x"), tree ("y")));
  tree doc (DOCUMENT, tree ("para"), inner);
  correct_downwards (doc);
  CHECK (doc[0] == tree ("para"));
  CHECK (doc[1] == tree (""));
}

TEST_CASE ("simplify_concat merges atomics") {
  tree t (tree (CONCAT, tree ("a"), tree ("b"), tree (WITH, tree ("u")),
                tree ("c")));
  CHECK (simplify_concat (t) ==
         tree (CONCAT, tree ("ab"), tree (WITH, tree ("u")), tree ("c")));
}

TEST_CASE ("simplify_concat flattens nested concat") {
  tree inner (tree (CONCAT, tree ("x"), tree ("y")));
  tree t (tree (CONCAT, tree ("a"), inner, tree ("z")));
  // 展平后全部原子相邻,合并为单个原子
  CHECK (simplify_concat (t) == tree ("axyz"));
}

TEST_CASE ("simplify_concat empty result") {
  tree t (tree (CONCAT, tree (""), tree ("")));
  CHECK (simplify_concat (t) == tree (""));
}

TEST_CASE ("simplify_document flattens nested document") {
  tree inner (tree (DOCUMENT, tree ("p1"), tree ("p2")));
  tree t (tree (DOCUMENT, inner, tree ("p3")));
  CHECK (simplify_document (t) ==
         tree (DOCUMENT, tree ("p1"), tree ("p2"), tree ("p3")));
}

} // TEST_SUITE
