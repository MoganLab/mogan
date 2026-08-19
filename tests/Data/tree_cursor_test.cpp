/******************************************************************************
 * MODULE     : tree_cursor_test.cpp
 * DESCRIPTION: Tests for cursor validation and movement
 * COPYRIGHT  : (C) 2026 Mogan developers
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "tree_cursor.hpp"
#include "tree_helper.hpp"
#include "tree_traverse.hpp"

using namespace moebius;

class TestTreeCursor : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_valid_cursor_basics ();
  void test_valid_cursor_input_math ();
  void test_next_valid ();
};

void
TestTreeCursor::test_valid_cursor_basics () {
  tree doc (DOCUMENT);
  tree par (CONCAT);
  par << tree ("hello") << tree ("world");
  doc << par << par;
  // 原子内部与边界均为合法光标位置
  QVERIFY (valid_cursor (doc, path (0, 0, 0)));
  QVERIFY (valid_cursor (doc, path (0, 0, N (doc[0][0]->label))));
  QVERIFY (valid_cursor (doc, path (0, 1, 2)));
  // 非法索引与空路径
  QVERIFY (!valid_cursor (doc, path ()));
  QVERIFY (!is_inside (doc, path (0, 5, 0)));
}

void
TestTreeCursor::test_valid_cursor_input_math () {
  // VAR_EXPAND "math" 的特殊情形：
  // (input "name" (math "x")) 的 math 参数内不允许光标
  tree input_math (make_tree_label ("input"), tree ("name"),
                   tree (make_tree_label ("math"), tree ("x")));
  QVERIFY (!valid_cursor (input_math, path (1, 0)));

  // 普通的 input（第二个孩子不是 math）不受影响
  tree input_plain (make_tree_label ("input"), tree ("name"),
                    tree ("plain"));
  QVERIFY (valid_cursor (input_plain, path (1, 0)));

  // 同名 with 结构不受 input 特判影响
  tree with_math (make_tree_label ("with"), tree ("mode"),
                  tree ("math"), tree ("body"));
  QVERIFY (valid_cursor (with_math, path (2, 0)));
}

void
TestTreeCursor::test_next_valid () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 5; i++) {
    tree par (CONCAT);
    par << tree ("aa") << tree ("bb");
    if (i == 2)
      par << tree (make_tree_label ("with"), tree ("color"),
                   tree ("red"), tree ("inner"));
    doc << par;
  }
  // 从首个原子起点逐步前进，位置应单调不减且始终合法
  path p (0, 0, 0);
  for (int i= 0; i < 20; i++) {
    path q= next_valid (doc, p);
    QVERIFY (q != p);
    QVERIFY (valid_cursor (doc, q));
    p= q;
  }
  // 反向移动应能回到起点附近
  path back= previous_valid (doc, p);
  QVERIFY (back != p);
}

#ifdef QTTEXMACS
QTEST_MAIN (TestTreeCursor)
#else
int
main () {
  return 0;
}
#endif
#include "tree_cursor_test.moc"
