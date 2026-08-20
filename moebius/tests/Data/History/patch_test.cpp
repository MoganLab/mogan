#include "modification.hpp"
#include "moe_doctests.hpp"
#include "moebius/tree_label.hpp"
#include "patch.hpp"
#include "tree.hpp"

using namespace moebius;

/******************************************************************************
 * Basic patch construction
 ******************************************************************************/

TEST_CASE ("patch modification construction") {
  modification m  = mod_assign (path (), tree ("hello"));
  modification inv= mod_assign (path (), tree (""));
  patch        p (m, inv);
  CHECK (is_modification (p));
  CHECK (get_type (p) == PATCH_MODIFICATION);
  CHECK (get_modification (p) == m);
  CHECK (get_inverse (p) == inv);
}

TEST_CASE ("patch compound construction") {
  modification m1= mod_assign (path (), tree ("a"));
  modification i1= mod_assign (path (), tree (""));
  modification m2= mod_assign (path (), tree ("b"));
  modification i2= mod_assign (path (), tree ("a"));
  patch        p1 (m1, i1);
  patch        p2 (m2, i2);
  patch        compound (p1, p2);
  CHECK (is_compound (compound));
  CHECK (N (compound) == 2);
  CHECK (compound[0] == p1);
  CHECK (compound[1] == p2);
}

TEST_CASE ("patch birth construction") {
  double author= new_author ();
  patch  p (author, true);
  CHECK (is_birth (p));
  CHECK (get_author (p) == author);
  CHECK (get_birth (p) == true);
}

TEST_CASE ("patch author construction") {
  double       author= new_author ();
  modification m     = mod_assign (path (), tree ("x"));
  modification inv   = mod_assign (path (), tree (""));
  patch        inner (m, inv);
  patch        p (author, inner);
  CHECK (is_author (p));
  CHECK (get_author (p) == author);
  CHECK (N (p) == 1);
}

/******************************************************************************
 * Patch application
 ******************************************************************************/

TEST_CASE ("apply modification patch to tree") {
  tree         t  = tree (DOCUMENT, "hello", "world");
  modification m  = mod_assign (path (0), tree ("hi"));
  modification inv= mod_assign (path (0), tree ("hello"));
  patch        p (m, inv);
  tree         result= clean_apply (p, t);
  CHECK (result[0] == tree ("hi"));
  CHECK (result[1] == tree ("world"));
}

TEST_CASE ("apply compound patch to tree") {
  tree         t = tree ("original");
  modification m1= mod_assign (path (), tree ("step1"));
  modification i1= mod_assign (path (), tree ("original"));
  modification m2= mod_assign (path (), tree ("step2"));
  modification i2= mod_assign (path (), tree ("step1"));
  patch        p1 (m1, i1);
  patch        p2 (m2, i2);
  patch        compound (p1, p2);
  tree         result= clean_apply (compound, t);
  CHECK (result == tree ("step2"));
}

TEST_CASE ("apply insert modification") {
  tree         t  = tree (DOCUMENT, "abc");
  modification m  = mod_insert (path (0), 1, tree ("X"));
  modification inv= mod_remove (path (0), 1, 1);
  patch        p (m, inv);
  tree         result= clean_apply (p, t);
  CHECK (result[0] == tree ("aXbc"));
}

TEST_CASE ("apply remove modification") {
  tree         t  = tree (DOCUMENT, "abcde");
  modification m  = mod_remove (path (0), 1, 2);
  modification inv= mod_insert (path (0), 1, tree ("bc"));
  patch        p (m, inv);
  tree         result= clean_apply (p, t);
  CHECK (result[0] == tree ("ade"));
}

TEST_CASE ("apply split modification") {
  tree         t  = tree (DOCUMENT, "abcde");
  modification m  = mod_split (path (), 0, 2);
  modification inv= mod_join (path (), 0);
  patch        p (m, inv);
  tree         result= clean_apply (p, t);
  CHECK (N (result) == 2);
  CHECK (result[0] == tree ("ab"));
  CHECK (result[1] == tree ("cde"));
}

TEST_CASE ("apply join modification") {
  tree         t  = tree (DOCUMENT, "ab", "cde");
  modification m  = mod_join (path (), 0);
  modification inv= mod_split (path (), 0, 2);
  patch        p (m, inv);
  tree         result= clean_apply (p, t);
  CHECK (N (result) == 1);
  CHECK (result[0] == tree ("abcde"));
}

/******************************************************************************
 * Patch inversion
 ******************************************************************************/

TEST_CASE ("invert modification patch") {
  tree         t  = tree ("hello");
  modification m  = mod_assign (path (), tree ("world"));
  modification inv= mod_assign (path (), tree ("hello"));
  patch        p (m, inv);
  patch        p_inv= invert (p, t);
  CHECK (is_modification (p_inv));
  CHECK (get_modification (p_inv) == inv);
  CHECK (get_inverse (p_inv) == m);
}

TEST_CASE ("invert then apply restores original") {
  tree         t  = tree ("original");
  modification m  = mod_assign (path (), tree ("modified"));
  modification inv= mod_assign (path (), tree ("original"));
  patch        p (m, inv);
  tree         t2   = clean_apply (p, t);
  patch        p_inv= invert (p, t);
  tree         t3   = clean_apply (p_inv, t2);
  CHECK (t3 == t);
}

TEST_CASE ("invert compound patch") {
  tree         t = tree (DOCUMENT, "a", "b");
  modification m1= mod_assign (path (0), tree ("x"));
  modification i1= mod_assign (path (0), tree ("a"));
  modification m2= mod_assign (path (1), tree ("y"));
  modification i2= mod_assign (path (1), tree ("b"));
  patch        p1 (m1, i1);
  patch        p2 (m2, i2);
  patch        compound (p1, p2);
  tree         t2 = clean_apply (compound, t);
  patch        inv= invert (compound, t);
  tree         t3 = clean_apply (inv, t2);
  CHECK (t3 == t);
}

/******************************************************************************
 * Patch equality and copy
 ******************************************************************************/

TEST_CASE ("patch equality") {
  modification m  = mod_assign (path (), tree ("a"));
  modification inv= mod_assign (path (), tree (""));
  patch        p1 (m, inv);
  patch        p2 (m, inv);
  CHECK (p1 == p2);
  CHECK_FALSE (p1 != p2);
}

TEST_CASE ("patch copy") {
  modification m  = mod_assign (path (), tree ("a"));
  modification inv= mod_assign (path (), tree (""));
  patch        p1 (m, inv);
  patch        p2= copy (p1);
  CHECK (p1 == p2);
}

/******************************************************************************
 * is_applicable
 ******************************************************************************/

TEST_CASE ("is_applicable for valid modification") {
  tree  t= tree (DOCUMENT, "hello", "world");
  patch p (mod_assign (path (0), tree ("hi")),
           mod_assign (path (0), tree ("hello")));
  CHECK (is_applicable (p, t));
}

TEST_CASE ("is_applicable for birth patch") {
  tree   t= tree ("anything");
  double a= new_author ();
  patch  p (a, true);
  CHECK (is_applicable (p, t));
}

/******************************************************************************
 * Commutation of modifications (swap semantics)
 ******************************************************************************/

// swap 声明在 patch.hpp,验证换位后的索引调整

TEST_CASE ("commute inserts after range shifts back") {
  // 先插 "abc"@5,再插 "xy"@9:可换位,换位后
  // m1*=insert xy@6 (回退 3),m2*=insert abc@5 (不变)
  modification a= mod_insert (path (0, 3), 5, tree ("abc"));
  modification b= mod_insert (path (0, 3), 9, tree ("xy"));
  CHECK (commute (a, b));
  modification s1= mod_insert (path (0, 3), 5, tree ("abc"));
  modification s2= mod_insert (path (0, 3), 9, tree ("xy"));
  CHECK (swap (s1, s2));
  CHECK (index (s1) == 6); // xy 后移到 abc 之后
  CHECK (index (s2) == 5); // abc 保持在 5
}

TEST_CASE ("commute inserts before range shifts forward") {
  // 先插 "abc"@5,再插 "xy"@2(在前):换位后 abc 前移 2
  modification s1= mod_insert (path (0, 3), 5, tree ("abc"));
  modification s2= mod_insert (path (0, 3), 2, tree ("xy"));
  CHECK (swap (s1, s2));
  CHECK (index (s1) == 2); // xy 不动
  CHECK (index (s2) == 7); // abc 前面多了 "xy",插点后移 2
}

TEST_CASE ("commute insert inside range fails") {
  // 后插落在前插区间内部且非同点:不可换位
  modification s1= mod_insert (path (0, 3), 5, tree ("abc"));
  modification s2= mod_insert (path (0, 3), 6, tree ("xy"));
  CHECK (!commute (s1, s2));
}

TEST_CASE ("commute disjoint paths is basic swap") {
  modification s1= mod_insert (path (0, 3), 5, tree ("abc"));
  modification s2= mod_insert (path (1, 7), 2, tree ("xy"));
  CHECK (swap (s1, s2));
  // 互换后内容对调,索引不变
  CHECK (s1->t == tree ("xy"));
  CHECK (s2->t == tree ("abc"));
  CHECK (index (s1) == 2);
  CHECK (index (s2) == 5);
}

TEST_CASE ("commute remove overlapping fails") {
  // 后 remove 的区间覆盖前 remove 起点(非同点):不可换位
  modification s1= mod_remove (path (0, 3), 6, 1);
  modification s2= mod_remove (path (0, 3), 5, 3);
  CHECK (!commute (s1, s2));
}

TEST_CASE ("commute join adjacent fails") {
  // join@i-1 与前序操作冲突
  modification s1= mod_insert (path (0, 3), 5, tree ("abc"));
  modification s2= mod_join (path (0, 3), 4);
  CHECK (!commute (s1, s2));
}
