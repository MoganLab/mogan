#include "modification.hpp"
#include "moe_doctests.hpp"
#include "tree.hpp"

#include <moebius/tree_label.hpp>

using moebius::CONCAT;
using moebius::DOCUMENT;

TEST_CASE ("test construct func") {
  modification m1 (MOD_ASSIGN, path (), tree ());
  modification m2 (MOD_ASSIGN, path (), tree ());
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_ASSIGN, true);
  CHECK_EQ (m1->p == path (), true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_ASSIGN, true);
  CHECK_EQ (m2->p == path (), true);
  CHECK_EQ (m2->t == tree (), true);
}

TEST_CASE ("test mod_assign") {
  modification m1= mod_assign (path (), tree ());
  modification m2= mod_assign (path (), tree ());
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_ASSIGN, true);
  CHECK_EQ (m1->p == path (), true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_ASSIGN, true);
  CHECK_EQ (m2->p == path (), true);
  CHECK_EQ (m2->t == tree (), true);
}

TEST_CASE ("test mod_insert") {
  modification m1= mod_insert (path (), 0, tree ());
  modification m2= mod_insert (path (), 0, tree ());
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_INSERT, true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_INSERT, true);
  CHECK_EQ (m2->t == tree (), true);
}

TEST_CASE ("test mod_remove") {
  modification m1= mod_remove (path (), 1, 2);
  modification m2= mod_remove (path (), 1, 2);
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_REMOVE, true);
  CHECK_EQ (m1->p[0] == 1, true);
  CHECK_EQ (m1->p[1] == 2, true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_REMOVE, true);
  CHECK_EQ (m2->t == tree (), true);
}

TEST_CASE ("test mod_split") {
  modification m1= mod_split (path (), 1, 2);
  modification m2= mod_split (path (), 1, 2);
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_SPLIT, true);
  CHECK_EQ (m1->p[0] == 1, true);
  CHECK_EQ (m1->p[1] == 2, true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_SPLIT, true);
  CHECK_EQ (m2->t == tree (), true);
}

TEST_CASE ("test mod_join") {
  modification m1= mod_join (path (), 1);
  modification m2= mod_join (path (), 1);
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_JOIN, true);
  CHECK_EQ (m1->p[0] == 1, true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_JOIN, true);
  CHECK_EQ (m2->t == tree (), true);
  modification m3= mod_join (path (1), 2);
  CHECK_EQ (m3->k == MOD_JOIN, true);
  CHECK_EQ (m3->p[0] == 1, true);
  CHECK_EQ (m3->p[1] == 2, true);
}

TEST_CASE ("test mod_assign_node") {
  modification m1= mod_assign_node (path (), 1);
  modification m2= mod_assign_node (path (), 1);
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_ASSIGN_NODE, true);
  CHECK_EQ (m1->p == path (), true);
  CHECK_EQ (m1->t == tree (1), true);
  CHECK_EQ (m2->k == MOD_ASSIGN_NODE, true);
  CHECK_EQ (m2->p == path (), true);
  CHECK_EQ (m2->t == tree (1), true);
}

TEST_CASE ("test mod_insert_node") {
  modification m1= mod_insert_node (path (), 1, tree ());
  modification m2= mod_insert_node (path (), 1, tree ());
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_INSERT_NODE, true);
  CHECK_EQ (m1->p[0] == 1, true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_INSERT_NODE, true);
  CHECK_EQ (m2->p[0] == 1, true);
  CHECK_EQ (m2->t == tree (), true);
  modification m3= mod_insert_node (path (), 2, tree ("string"));
  CHECK_EQ (m3->k == MOD_INSERT_NODE, true);
  CHECK_EQ (m3->p[0] == 2, true);
  CHECK_EQ (m3->t == tree ("string"), true);
}

TEST_CASE ("test mod_remove_node") {
  modification m1= mod_remove_node (path (), 1);
  modification m2= mod_remove_node (path (), 1);
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_REMOVE_NODE, true);
  CHECK_EQ (m1->p[0] == 1, true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_REMOVE_NODE, true);
  CHECK_EQ (m2->p[0] == 1, true);
  CHECK_EQ (m2->t == tree (), true);
}

TEST_CASE ("test mod_set_cursor") {
  modification m1= mod_set_cursor (path (), 1, tree ());
  modification m2= mod_set_cursor (path (), 1, tree ());
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_SET_CURSOR, true);
  CHECK_EQ (m1->p[0] == 1, true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_SET_CURSOR, true);
  CHECK_EQ (m2->p[0] == 1, true);
  CHECK_EQ (m2->t == tree (), true);
  modification m3= mod_set_cursor (path (), 2, tree ("string"));
  CHECK_EQ (m3->k == MOD_SET_CURSOR, true);
  CHECK_EQ (m3->p[0] == 2, true);
  CHECK_EQ (m3->t == tree ("string"), true);
}

TEST_CASE ("test operator*/") {
  modification m1= mod_assign (path (), tree ());
  modification m2= 1 * m1;
  modification m3= path (1) * m1;
  modification m4= m1 * 1;
  modification m5= mod_assign (path (1, 2, 3), tree ()) / path (1);
  CHECK_EQ (m2->k == MOD_ASSIGN, true);
  CHECK_EQ (m2->p[0] == 1, true);
  CHECK_EQ (m2->t == tree (), true);
  CHECK_EQ (m3->k == MOD_ASSIGN, true);
  CHECK_EQ (m3->p[0] == 1, true);
  CHECK_EQ (m3->t == tree (), true);
  CHECK_EQ (m4->k == MOD_ASSIGN, true);
  CHECK_EQ (m4->p[0] == 1, true);
  CHECK_EQ (m4->t == tree (), true);
  CHECK_EQ (m5->k == MOD_ASSIGN, true);
  CHECK_EQ (m5->p[0] == 2, true);
  CHECK_EQ (m5->t == tree (), true);
}

TEST_CASE ("test operator==") {
  modification m1= mod_assign (path (), tree ());
  modification m2= mod_assign (path (), tree ());
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  modification m3= mod_assign (path (), tree ("string"));
  CHECK_EQ (m1 == m3, false);
  CHECK_EQ (m1 != m3, true);
}

TEST_CASE ("test copy") {
  modification m1= mod_assign (path (), tree ());
  modification m2= copy (m1);
  CHECK_EQ (m1 == m2, true);
  CHECK_EQ (m1 != m2, false);
  CHECK_EQ (m1->k == MOD_ASSIGN, true);
  CHECK_EQ (m1->p == path (), true);
  CHECK_EQ (m1->t == tree (), true);
  CHECK_EQ (m2->k == MOD_ASSIGN, true);
  CHECK_EQ (m2->p == path (), true);
  CHECK_EQ (m2->t == tree (), true);
}

TEST_CASE ("test root") {
  modification m1= mod_assign (path (), tree ());
  modification m2= mod_assign (path (1), tree ());
  modification m3= mod_assign (path (1, 2), tree ());
  CHECK_EQ (root (m1) == path (), true);
  CHECK_EQ (root (m2) == path (1), true);
  CHECK_EQ (root (m3) == path (1, 2), true);
}

TEST_CASE ("test index") {
  modification m1= mod_insert (path (1, 2), 3, tree ());
  modification m2= mod_remove (path (), 3, 4);
  modification m3= mod_split (path (), 3, 4);
  modification m4= mod_join (path (), 3);
  modification m5= mod_remove_node (path (), 3);
  modification m6= mod_set_cursor (path (), 3, tree ());
  CHECK_EQ (index (m1) == 3, true);
  CHECK_EQ (index (m2) == 3, true);
  CHECK_EQ (index (m3) == 3, true);
  CHECK_EQ (index (m4) == 3, true);
  CHECK_EQ (index (m5) == 3, true);
  CHECK_EQ (index (m6) == 3, true);
}

TEST_CASE ("test argument") {
  modification m1= mod_remove (path (), 3, 4);
  modification m2= mod_split (path (), 3, 4);
  modification m3= mod_insert_node (path (), 3, tree ());
  CHECK_EQ (argument (m1) == 4, true);
  CHECK_EQ (argument (m2) == 4, true);
  CHECK_EQ (argument (m3) == 3, true);
}

TEST_CASE ("test make_modification") {
  make_modification ("assign", path (), tree ());
  make_modification ("insert", path (), tree ());
  make_modification ("remove", path (), tree ());
  make_modification ("split", path (), tree ());
  make_modification ("join", path (), tree ());
  make_modification ("assign-node", path (), tree ());
  make_modification ("insert-node", path (), tree ());
  make_modification ("remove-node", path (), tree ());
  make_modification ("set-cursor", path (), tree ());
}

TEST_CASE ("test get_type") {
  modification m1= make_modification ("assign", path (), tree ());
  modification m2= make_modification ("insert", path (), tree ());
  modification m3= make_modification ("remove", path (), tree ());
  modification m4= make_modification ("split", path (), tree ());
  modification m5= make_modification ("join", path (), tree ());
  modification m6= make_modification ("assign-node", path (), tree ());
  modification m7= make_modification ("insert-node", path (), tree ());
  modification m8= make_modification ("remove-node", path (), tree ());
  modification m9= make_modification ("set-cursor", path (), tree ());
  CHECK_EQ (get_type (m1) == "assign", true);
  CHECK_EQ (get_type (m2) == "insert", true);
  CHECK_EQ (get_type (m3) == "remove", true);
  CHECK_EQ (get_type (m4) == "split", true);
  CHECK_EQ (get_type (m5) == "join", true);
  CHECK_EQ (get_type (m6) == "assign-node", true);
  CHECK_EQ (get_type (m7) == "insert-node", true);
  CHECK_EQ (get_type (m8) == "remove-node", true);
  CHECK_EQ (get_type (m9) == "set-cursor", true);
}

TEST_CASE ("test get_path") {
  modification m1= mod_assign (path (), tree ());
  modification m2= mod_assign (path (1), tree ());
  modification m3= mod_assign (path (1, 2), tree ());
  CHECK_EQ (get_path (m1) == path (), true);
  CHECK_EQ (get_path (m2) == path (1), true);
  CHECK_EQ (get_path (m3) == path (1, 2), true);
}

TEST_CASE ("test get_tree") {
  modification m1= mod_assign (path (), tree ());
  modification m2= mod_assign (path (), tree ("string"));
  modification m3= mod_assign (path (), tree (1));
  CHECK_EQ (get_tree (m1) == tree (), true);
  CHECK_EQ (get_tree (m2) == tree ("string"), true);
  CHECK_EQ (get_tree (m3) == tree (1), true);
}

TEST_CASE ("test is_applicable") {
  modification m1= mod_assign (path (), tree ());
  modification m2= mod_assign (path (1), tree ());
  modification m3= mod_assign (path (1, 2), tree ());
  CHECK_EQ (is_applicable (tree (), m1), true);
  CHECK_EQ (is_applicable (tree (), m2), false);
  CHECK_EQ (is_applicable (tree (), m3), false);
  CHECK_EQ (is_applicable (tree (1), m1), true);
  CHECK_EQ (is_applicable (tree (1), m2), false);
  CHECK_EQ (is_applicable (tree (1), m3), false);
  CHECK_EQ (is_applicable (tree (1, 2), m1), true);
  CHECK_EQ (is_applicable (tree (1, 2), m2), true);
  CHECK_EQ (is_applicable (tree (1, 2), m3), false);
}
TEST_CASE ("test is_applicable insert") {
  tree doc (DOCUMENT, tree ("a"), tree ("b"));
  CHECK (is_applicable (doc,
                        mod_insert (path (), 0, tree (DOCUMENT, tree ("x")))));
  CHECK (!is_applicable (
      doc, mod_insert (path (), 0, tree ("x")))); // 复合位置插原子
  CHECK (!is_applicable (
      doc, mod_insert (path (5), 0, tree (DOCUMENT, tree ("x"))))); // 越界
  CHECK (!is_applicable (
      doc, mod_insert (path (), 9, tree (DOCUMENT, tree ("x"))))); // pos 越界
}

TEST_CASE ("test is_applicable atomic insert") {
  tree doc (DOCUMENT, tree ("abc"));
  CHECK (is_applicable (doc, mod_insert (path (0), 1, tree ("xy"))));
  CHECK (!is_applicable (
      doc, mod_insert (path (0), 1, tree (DOCUMENT, tree ("x")))));
}

TEST_CASE ("test is_applicable remove") {
  tree doc (DOCUMENT, tree ("a"), tree ("b"));
  CHECK (is_applicable (doc, mod_remove (path (), 1, 1)));
  CHECK (!is_applicable (doc, mod_remove (path (), 1, 2)));
  CHECK (!is_applicable (doc, mod_remove (path (3), 0, 1)));
}

TEST_CASE ("test is_applicable join") {
  tree doc (DOCUMENT, tree ("a"), tree ("b"));
  CHECK (is_applicable (doc, mod_join (path (), 0)));
  CHECK (!is_applicable (doc, mod_join (path (), 1))); // pos+1 越界
  tree mixed (DOCUMENT, tree ("a"), tree (CONCAT, tree ("b")));
  CHECK (!is_applicable (mixed, mod_join (path (), 0))); // 原子+复合
}

TEST_CASE ("test is_applicable assign and node ops") {
  tree doc (DOCUMENT, tree ("a"));
  CHECK (is_applicable (doc, mod_assign (path (0), tree ("z"))));
  CHECK (!is_applicable (doc, mod_assign (path (2), tree ("z"))));
  CHECK (is_applicable (doc, mod_assign_node (path (), CONCAT)));
  CHECK (is_applicable (doc, mod_insert_node (path (0), 0, tree (CONCAT))));
  // remove_node 需要目标是复合节点的孩子
  tree nested (DOCUMENT, tree (CONCAT, tree ("a")));
  CHECK (is_applicable (nested, mod_remove_node (path (0), 0)));
}
