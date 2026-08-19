/******************************************************************************
 * MODULE     : tree_observer_test.cpp
 * DESCRIPTION: Tests for raw tree modifications and refcount integrity
 * COPYRIGHT  : (C) 2026 Mogan developers
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "tree_helper.hpp"
#include "tree_observer.hpp"

using namespace moebius;

class TestTreeObserver : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_raw_insert ();
  void test_raw_remove ();
  void test_insert_remove_refcount ();
};

void
TestTreeObserver::test_raw_insert () {
  tree doc (DOCUMENT, tree ("a"), tree ("b"), tree ("d"));
  // 在头部插入
  raw_insert (doc, 0, tree (DOCUMENT, tree ("x")));
  QVERIFY (N (doc) == 4);
  QVERIFY (doc[0] == tree ("x"));
  QVERIFY (doc[1] == tree ("a"));
  // 在中部插入多个
  raw_insert (doc, 2, tree (DOCUMENT, tree ("m"), tree ("n")));
  QVERIFY (N (doc) == 6);
  QVERIFY (doc[1] == tree ("a"));
  QVERIFY (doc[2] == tree ("m"));
  QVERIFY (doc[3] == tree ("n"));
  QVERIFY (doc[4] == tree ("b"));
  QVERIFY (doc[5] == tree ("d"));
  // 在尾部插入
  raw_insert (doc, 6, tree (DOCUMENT, tree ("z")));
  QVERIFY (N (doc) == 7);
  QVERIFY (doc[6] == tree ("z"));
}

void
TestTreeObserver::test_raw_remove () {
  tree doc (DOCUMENT, tree ("a"), tree ("b"), tree ("c"), tree ("d"),
            tree ("e"));
  // 头部删除
  raw_remove (doc, 0, 1);
  QVERIFY (N (doc) == 4);
  QVERIFY (doc[0] == tree ("b"));
  // 中部删除多个
  raw_remove (doc, 1, 2);
  QVERIFY (N (doc) == 2);
  QVERIFY (doc[0] == tree ("b"));
  QVERIFY (doc[1] == tree ("e"));
  // 尾部删除
  raw_remove (doc, 1, 1);
  QVERIFY (N (doc) == 1);
  QVERIFY (doc[0] == tree ("b"));
}

void
TestTreeObserver::test_insert_remove_refcount () {
  // 交叉插入/删除并共享子树：若数组块移动的引用计数记账有误，
  // 后续拷贝比较或进程退出时会崩溃或得到错误内容
  tree shared (CONCAT, tree ("s1"), tree ("s2"));
  tree doc (DOCUMENT);
  for (int round= 0; round < 50; round++) {
    tree ins (DOCUMENT);
    ins << shared << tree ("t") << shared;
    raw_insert (doc, 0, ins);
    QVERIFY (doc[0] == shared);
    QVERIFY (doc[2] == shared);
    if (round % 2 == 0) raw_remove (doc, 0, 1);
  }
  QVERIFY (N (doc) == 125);
  // 大数组跨容量桶的插入/删除
  tree big (DOCUMENT);
  for (int i= 0; i < 1000; i++)
    big << tree ("p" * as_string (i));
  for (int i= 0; i < 40; i++)
    raw_insert (big, 500, tree (DOCUMENT, tree ("q")));
  QVERIFY (N (big) == 1040);
  QVERIFY (big[500] == tree ("q"));
  QVERIFY (big[499] == tree ("p499"));
  QVERIFY (big[539] == tree ("q"));
  QVERIFY (big[540] == tree ("p500"));
  raw_remove (big, 500, 40);
  QVERIFY (N (big) == 1000);
  QVERIFY (big[500] == tree ("p500"));
}

#ifdef QTTEXMACS
QTEST_MAIN (TestTreeObserver)
#else
int
main () {
  return 0;
}
#endif
#include "tree_observer_test.moc"
