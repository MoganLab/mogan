/******************************************************************************
 * MODULE     : edit_collab_cursor_test.cpp
 * DESCRIPTION: Unit tests for collaborative cursor payload resolution
 * COPYRIGHT  : (C) 2026
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifdef LORO_ENABLED
#include "../../../src/Edit/Modify/edit_collab.cpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestEditCollabCursor : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void test_empty_core_off ();
};

void
TestEditCollabCursor::test_empty_core_off () {
  mogan_tree_id      tid{0, 0};
  tree               buf (moebius::DOCUMENT);
  loro_shadow        doc;
  array<linear_item> items;

  QVERIFY (is_nil (resolve_cursor (tid, "@O", buf, doc, items)));
  QVERIFY (is_nil (resolve_cursor (tid, "@", buf, doc, items)));
}

QTEST_MAIN (TestEditCollabCursor)
#else
#include <QtTest/QtTest>

class DummyTestEditCollabCursor : public QObject {
  Q_OBJECT
};

QTEST_MAIN (DummyTestEditCollabCursor)
#endif // LORO_ENABLED

#include "edit_collab_cursor_test.moc"
