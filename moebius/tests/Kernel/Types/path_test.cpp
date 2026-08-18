#include "moe_doctests.hpp"
#include "path.hpp"

static const list<int> p1= list<int> ();
static const list<int> p2= list<int> (1);
static const list<int> p3= list<int> (1, 2);

static path
gen_path (int n) {
  auto normal= list<int> ();
  for (int i= 0; i < n; i++)
    normal << i;
  return normal;
}

TEST_CASE ("test zero_path") {
  CHECK_EQ (zero_path (p1), true);
  CHECK_EQ (zero_path (p2), false);
  CHECK_EQ (zero_path (p3), false);
}

TEST_CASE ("test hash") {
  CHECK_EQ (hash (p1), 0);
  CHECK_EQ (hash (p2), 1);
  CHECK_EQ (hash (p3), 257);
}

TEST_CASE ("test as_string") {
  string_eq (as_string (p1), "");
  string_eq (as_string (p2), "1");
  string_eq (as_string (p3), "1.2");
  string_eq (as_string (gen_path (4)), "0.1.2.3");
}

TEST_CASE ("test as_path") {
  CHECK_EQ (as_path (""), list<int> ());
  CHECK_EQ (as_path ("1"), list<int> (1));
  CHECK_EQ (as_path ("1.2"), list<int> (1, 2));
  CHECK_EQ (as_path ("0.1.2.3"), gen_path (4));
}

TEST_CASE ("test version_inf_eq") {
  CHECK_EQ (version_inf_eq ("1.2", "1.2"), true);
  CHECK_EQ (version_inf_eq ("1.2", "1.2.3"), true);
  CHECK_EQ (version_inf_eq ("1.2.3", "1.2"), false);
  CHECK_EQ (version_inf_eq ("1.2.3", "1.2.3"), true);
  CHECK_EQ (version_inf_eq ("1.2.3", "1.2.4"), true);
  CHECK_EQ (version_inf_eq ("1.2.4", "1.2.3"), false);
}

TEST_CASE ("test version_inf") {
  CHECK_EQ (version_inf ("1.2", "1.2"), false);
  CHECK_EQ (version_inf ("1.2", "1.2.3"), true);
  CHECK_EQ (version_inf ("1.2.3", "1.2"), false);
  CHECK_EQ (version_inf ("1.2.3", "1.2.3"), false);
  CHECK_EQ (version_inf ("1.2.3", "1.2.4"), true);
}

TEST_CASE ("test path_up") {
  CHECK_EQ (path_up (p2), list<int> ());
  CHECK_EQ (path_up (p3), list<int> (1));
  CHECK_EQ (path_up (gen_path (4)), gen_path (3));
  CHECK_EQ (path_up (p3, 1), list<int> (1));
  CHECK_EQ (path_up (gen_path (4), 1), gen_path (3));
}

TEST_CASE ("test path_add") {
  CHECK_EQ (path_add (p2, 1), list<int> (2));
  CHECK_EQ (path_add (p3, 1), list<int> (1, 3));
  CHECK_EQ (path_add (gen_path (4), 1), list<int> (0, 1, 2, 4));
  CHECK_EQ (path_add (p3, 2, 0), list<int> (3, 2));
  CHECK_EQ (path_add (gen_path (4), 2, 1), list<int> (0, 3, 2, 3));
}

TEST_CASE ("test path_inf") {
  CHECK_EQ (path_inf (p3, list<int> (1, 4)), true);
  CHECK_EQ (path_inf (p3, p2), false);
  CHECK_EQ (path_inf (p3, p3), false);
  CHECK_EQ (path_inf (gen_path (4), path_add (gen_path (4), 1)), true);
  CHECK_EQ (path_inf (gen_path (4), gen_path (4)), false);
}

TEST_CASE ("test path_inf_eq") { CHECK_EQ (path_inf_eq (p2, p3), false); }

TEST_CASE ("test path_less") {
  CHECK_EQ (path_less (p2, p3), false);
  CHECK_EQ (path_less (p3, p2), true);
  CHECK_EQ (path_less (gen_path (4), path_add (gen_path (4), 1)), true);
  CHECK_EQ (path_less (gen_path (4), gen_path (4)), false);
}

TEST_CASE ("test operator/") {
  CHECK_EQ (p3 / p2, list<int> (2));
  CHECK_EQ (gen_path (4) / gen_path (3), list<int> (3));
  CHECK_EQ (gen_path (4) / gen_path (4), list<int> ());
}

TEST_CASE ("test common") {
  CHECK_EQ (common (p3, list<int> (1, 4)), list<int> (1));
  CHECK_EQ (common (p3, p3), p3);
  CHECK_EQ (common (gen_path (4), path_add (gen_path (4), 1)), gen_path (3));
  CHECK_EQ (common (gen_path (4), gen_path (4)), gen_path (4));
}

TEST_CASE ("test strip") {
  CHECK_EQ (strip (p3, p3), list<int> ());
  CHECK_EQ (strip (gen_path (4), gen_path (4)), list<int> ());
}

TEST_CASE ("test has_subtree") {
  CHECK_EQ (
      has_subtree (tree (2, tree (3, tree ()), tree (4, tree ())), path (1)),
      true);
}

static tree
mk_test_tree () {
  // op=0 表示原子节点，compound 的 op 必须为正： (1 (2 (3 "x")) (4 "y"))
  return tree (1, tree (2, tree (3, tree ("x"))), tree (4, tree ("y")));
}

TEST_CASE ("test subtree") {
  tree t= mk_test_tree ();
  CHECK_EQ (subtree (t, path ()), t);
  CHECK_EQ (subtree (t, path (0)), tree (2, tree (3, tree ("x"))));
  CHECK_EQ (subtree (t, path (0, 0)), tree (3, tree ("x")));
  CHECK_EQ (subtree (t, path (0, 0, 0)), tree ("x"));
  CHECK_EQ (subtree (t, path (1, 0)), tree ("y"));
  // 越界与命中原子节点：打印诊断并返回原树
  CHECK_EQ (subtree (t, path (2)), t);
  CHECK_EQ (subtree (t, path (0, 0, 0, 0)), t);
  CHECK_EQ (subtree (t, path (-1)), t);
}

TEST_CASE ("test parent_subtree") {
  tree t= mk_test_tree ();
  CHECK_EQ (parent_subtree (t, path (0)), t);
  CHECK_EQ (parent_subtree (t, path (0, 0)), tree (2, tree (3, tree ("x"))));
  CHECK_EQ (parent_subtree (t, path (1, 0)), tree (4, tree ("y")));
}

TEST_CASE ("test has_subtree miss") {
  tree t= mk_test_tree ();
  CHECK_EQ (has_subtree (t, path (2)), false);
  CHECK_EQ (has_subtree (t, path (-1)), false);
  CHECK_EQ (has_subtree (t, path (0, 0, 0, 0)), false);
  CHECK_EQ (has_subtree (t, path ()), true);
  CHECK_EQ (has_subtree (tree ("leaf"), path (0)), false);
}