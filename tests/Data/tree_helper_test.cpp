/******************************************************************************
 * MODULE     : tree_helper_test.cpp
 * DESCRIPTION: Tests for tree_helper predicates
 * COPYRIGHT  : (C) 2026 Mogan developers
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "tree_helper.hpp"

using namespace moebius;

class TestTreeHelper : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_is_empty ();
  void test_is_snippet ();
};

void
TestTreeHelper::test_is_empty () {
  QVERIFY (is_empty (tree ("")));
  QVERIFY (!is_empty (tree ("a")));
  // 空段落/空 concat 视为空
  QVERIFY (is_empty (tree (DOCUMENT)));
  QVERIFY (is_empty (tree (DOCUMENT, tree (""))));
  QVERIFY (!is_empty (tree (DOCUMENT, tree ("a"))));
  QVERIFY (is_empty (tree (CONCAT)));
  // concat 只要有一个非空孩子即非空
  QVERIFY (!is_empty (tree (CONCAT, tree (""), tree ("b"))));
  // suppressed 标签视为空
  QVERIFY (is_empty (tree (make_tree_label ("suppressed"))));
  QVERIFY (!is_empty (tree (make_tree_label ("kept"), tree ("x"))));
}

void
TestTreeHelper::test_is_snippet () {
  // 非文档一律是 snippet
  QVERIFY (is_snippet (tree ("x")));
  QVERIFY (is_snippet (tree (CONCAT, tree ("x"))));
  // 不含 TeXmacs 根标记的文档也是 snippet
  tree doc (DOCUMENT, tree ("body"));
  QVERIFY (is_snippet (doc));
  // 首层含 (TeXmacs ...) 的文档不是 snippet
  tree full (DOCUMENT, tree (make_tree_label ("TeXmacs"), tree ("1.0.2")),
             tree ("body"));
  QVERIFY (!is_snippet (full));
}

#ifdef QTTEXMACS
QTEST_MAIN (TestTreeHelper)
#else
int
main () {
  return 0;
}
#endif
#include "tree_helper_test.moc"
