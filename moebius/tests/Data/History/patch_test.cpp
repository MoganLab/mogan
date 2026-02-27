#include "modification.hpp"
#include "moe_doctests.hpp"
#include "patch.hpp"
#include "tree.hpp"

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
