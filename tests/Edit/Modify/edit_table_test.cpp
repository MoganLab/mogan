/******************************************************************************
 * MODULE     : edit_table_test.cpp
 * DESCRIPTION: Test default table insertion behavior
 * COPYRIGHT  : (C) 2026
 ******************************************************************************/

#include "base.hpp"
#include "env.hpp"
#include <QtTest/QtTest>
#include <moebius/tree_label.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

// Declared in src/Edit/Modify/edit_table.cpp
extern tree empty_table (int nr_rows, int nr_cols);
extern tree default_table_tree (int nr_rows, int nr_cols);

class TestEditTable : public QObject {
  Q_OBJECT

private slots:
  void test_empty_table_structure ();
  void test_default_table_tree_has_cell_hyphen ();
  void test_default_table_tree_cwith_range ();
  void test_custom_border_colors_registered ();
};

void
TestEditTable::test_empty_table_structure () {
  tree T= empty_table (2, 3);
  QCOMPARE (N (T), 2);    // 2 rows
  QCOMPARE (N (T[0]), 3); // 3 cols in first row
  QVERIFY (is_func (T, TABLE));
  QVERIFY (is_func (T[0], ROW));
  QVERIFY (is_func (T[0][0], CELL));
}

void
TestEditTable::test_default_table_tree_has_cell_hyphen () {
  tree T= default_table_tree (2, 3);
  QVERIFY (is_func (T, TFORMAT));

  // TFORMAT should contain: cwith, TABLE
  // Last child is TABLE
  QVERIFY (is_func (T[N (T) - 1], TABLE));

  // Find cwith for cell-hyphen
  bool found= false;
  for (int i= 0; i < N (T); i++) {
    if (is_func (T[i], CWITH, 6) && T[i][4] == "cell-hyphen" &&
        T[i][5] == "t") {
      found= true;
      break;
    }
  }
  QVERIFY (found);
}

void
TestEditTable::test_default_table_tree_cwith_range () {
  tree T= default_table_tree (2, 3);
  QVERIFY (is_func (T, TFORMAT));

  for (int i= 0; i < N (T); i++) {
    if (is_func (T[i], CWITH, 6) && T[i][4] == "cell-hyphen") {
      QCOMPARE (T[i][0], "1");  // row-start
      QCOMPARE (T[i][1], "-1"); // row-end
      QCOMPARE (T[i][2], "1");  // col-start
      QCOMPARE (T[i][3], "-1"); // col-end
      QCOMPARE (T[i][5], "t");  // value
      return;
    }
  }
  QFAIL ("cell-hyphen cwith not found");
}

void
TestEditTable::test_custom_border_colors_registered () {
  QCOMPARE (CELL_BORDER_COLOR, "cell-border-color");
  QCOMPARE (TABLE_BORDER_COLOR, "table-border-color");
}

QTEST_MAIN (TestEditTable)
#include "edit_table_test.moc"
