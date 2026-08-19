/** \file drd_env_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for drd_info::get_env_child fast paths
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/drd/drd_info.hpp>
#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::drd::init_std_drd;
using moebius::drd::the_drd;

TEST_SUITE ("drd_env") {

  TEST_CASE ("get_env_child empty binding returns default") {
    init_std_drd ();
    tree doc (DOCUMENT, tree ("a"), tree ("b"));
    // 无环境绑定的标签:返回调用方缺省值而非空树
    CHECK (the_drd->get_env_child (doc, 0, "mode", tree ("text")) ==
           tree ("text"));
    CHECK (the_drd->get_env_child (doc, 0, "mode", tree ("")) == tree (""));
  }

  TEST_CASE ("get_env_child invalid index returns default") {
    init_std_drd ();
    tree doc (DOCUMENT, tree ("a"));
    CHECK (the_drd->get_env_child (doc, 5, "mode", tree ("d")) == tree ("d"));
  }

  TEST_CASE ("get_env_child WITH reads binding") {
    init_std_drd ();
    tree w (WITH, tree ("mode"), tree ("src"), tree ("body"));
    // WITH 的最后一个孩子继承绑定值
    CHECK (the_drd->get_env_child (w, 2, "mode", tree ("text")) ==
           tree ("src"));
    // 非最后孩子不受 WITH 绑定影响
    CHECK (the_drd->get_env_child (w, 0, "mode", tree ("text")) ==
           tree ("text"));
  }

  TEST_CASE ("get_env_child env variant merges") {
    init_std_drd ();
    tree w (WITH, tree ("mode"), tree ("src"), tree ("body"));
    tree env (ATTR);
    tree r= the_drd->get_env_child (w, 2, env);
    CHECK (drd::drd_env_read (r, "mode", tree ("")) == tree ("src"));
    // 无绑定的复合节点透传传入的 env
    tree doc (DOCUMENT, tree ("a"));
    tree r2= the_drd->get_env_child (doc, 0, env);
    CHECK (drd::drd_env_read (r2, "mode", tree ("x")) == tree ("x"));
  }

  TEST_CASE ("get_env_descendant through WITH") {
    init_std_drd ();
    tree w (WITH, tree ("mode"), tree ("src"), tree ("body"));
    CHECK (the_drd->get_env_descendant (w, path (2), "mode", tree ("text")) ==
           tree ("src"));
    CHECK (the_drd->get_env_descendant (w, path (), "mode", tree ("text")) ==
           tree ("text"));
  }

} // TEST_SUITE

TEST_CASE ("extern derived label memoized consistently") {
  init_std_drd ();
  // EXTERN 节点派生 "extern:<name>" 标签,备忘缓存不改变判定结果
  tree ex1 (EXTERN, tree ("hlink"));
  ex1 << tree ("a") << tree ("b");
  tree ex2 (EXTERN, tree ("hlink"));
  ex2 << tree ("c") << tree ("d");
  tree ex3 (EXTERN, tree ("other"));
  ex3 << tree ("e");
  bool r1= the_drd->is_accessible_child (ex1, 0);
  // 同名宏重复出现(命中备忘)与首次(未命中)结果一致
  CHECK (the_drd->is_accessible_child (ex2, 0) == r1);
  // 不同宏名(备忘失效)也返回确定的布尔值
  bool r3= the_drd->is_accessible_child (ex3, 0);
}
