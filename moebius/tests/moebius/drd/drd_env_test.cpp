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
