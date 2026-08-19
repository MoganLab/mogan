/******************************************************************************
 * MODULE     : tree_modify_test.cpp
 * DESCRIPTION: Tests for tree correction routines
 * COPYRIGHT  : (C) 2026 Mogan developers
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "tree_helper.hpp"
#include "tree_modify.hpp"

#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;

class TestTreeModify : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_label_name_bijection ();
  void test_correct_node ();
};

void
TestTreeModify::test_label_name_bijection () {
  // contains(L(t)) 与 contains(as_string(L(t))) 等价的前提：
  // 标签名与标签编号一一对应
  array<string> names= get_all_primitives ();
  for (int i= 0; i < N (names); i++) {
    string      n= names[i];
    tree_label l= as_tree_label (n);
    QVERIFY (existing_tree_label (n));
    QVERIFY (l != UNKNOWN);
    QVERIFY (to_string (l) == n);
  }
  // 扩展标签同样满足
  tree_label ext= make_tree_label ("test-bijection-ext");
  QVERIFY (as_tree_label ("test-bijection-ext") == ext);
  QVERIFY (to_string (ext) == string ("test-bijection-ext"));
  QVERIFY (make_tree_label ("test-bijection-ext") == ext);
}

void
TestTreeModify::test_correct_node () {
  // 原子节点不做校正
  tree a ("x");
  correct_node (a);
  QVERIFY (is_atomic (a));

  // DRD 未知的标签不校正
  tree unknown (make_tree_label ("test-unknown-tag-zzz"), tree ("x"));
  tree unknown2= unknown;
  correct_node (unknown2);
  QVERIFY (unknown2 == unknown);

  // DRD 已知但 arity 错误的标签被替换为空串
  tree_label bad_l= make_tree_label ("test-arity-tag-zzz");
  drd::the_drd->set_arity (bad_l, 1, 0, ARITY_NORMAL, CHILD_DETAILED);
  tree bad (bad_l, tree ("a"), tree ("b"));
  correct_node (bad);
  QVERIFY (bad == tree (""));
  // arity 正确则保留
  tree_label good_l= make_tree_label ("test-arity-tag2-zzz");
  drd::the_drd->set_arity (good_l, 1, 0, ARITY_NORMAL, CHILD_DETAILED);
  tree good (good_l, tree ("a"));
  correct_node (good);
  QVERIFY (good == tree (good_l, tree ("a")));
}

#ifdef QTTEXMACS
QTEST_MAIN (TestTreeModify)
#else
int
main () {
  return 0;
}
#endif
#include "tree_modify_test.moc"
