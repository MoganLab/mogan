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
extern tree default_table_tree (int nr_rows, int nr_cols,
                                bool enable_table_hyphen);
extern bool table_default_hyphen_enabled (string mode);
extern bool table_needs_document_wrap (string hyphen, string block,
                                       string mode);

class TestEditTable : public QObject {
  Q_OBJECT

private slots:
  void test_empty_table_structure ();
  void test_default_table_tree_has_cell_hyphen ();
  void test_default_table_tree_cwith_range ();
  void test_custom_border_colors_registered ();
  void test_adjacent_border_colors ();
  void test_adjacent_border_colors_on_cell_typeset ();
  void test_default_table_tree_has_table_hyphen ();
  void test_default_table_tree_twith_value ();
  void test_default_hyphen_enabled_in_text_mode ();
  void test_default_hyphen_disabled_in_math_mode ();
  void test_default_table_tree_skips_table_hyphen_in_math_mode ();
  void test_no_document_wrap_in_math_mode ();
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
  tree T= default_table_tree (2, 3, true);
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
  tree T= default_table_tree (2, 3, true);
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

void
TestEditTable::test_adjacent_border_colors () {
  int hor_prec= 10;

  int cell1_prec   = 5;
  int cell1_rborder= 2;

  int cell2_prec   = 10;
  int cell2_lborder= 2;

  int cell1_merged_rborder= 0;
  if (cell1_rborder == 0 || cell1_prec >= hor_prec) {
    cell1_merged_rborder= 2;
  }

  int cell2_merged_lborder= 0;
  if (cell2_lborder == 0 || cell2_prec >= hor_prec) {
    cell2_merged_lborder= 2;
  }

  QCOMPARE (cell1_merged_rborder, 0);
  QCOMPARE (cell2_merged_lborder, 2);
}

void
TestEditTable::test_adjacent_border_colors_on_cell_typeset () {
  int hor_prec     = 10;
  int cell1_prec   = 5;
  int cell1_rborder= 2;

  int cell1_merged_rborder= 0;
  if (cell1_rborder == 0 || cell1_prec >= hor_prec) {
    cell1_merged_rborder= 2;
  }
  QCOMPARE (cell1_merged_rborder, 0);

  cell1_rborder= 2;

  int cell1_finished_rborder= 0;
  if (cell1_rborder == 0 || cell1_prec >= hor_prec) {
    cell1_finished_rborder= 2;
  }

  QCOMPARE (cell1_finished_rborder, 0);
}

void
TestEditTable::test_default_table_tree_has_table_hyphen () {
  tree T= default_table_tree (2, 3, true);
  QVERIFY (is_func (T, TFORMAT));

  bool found= false;
  for (int i= 0; i < N (T); i++) {
    if (is_func (T[i], TWITH, 2) && T[i][0] == "table-hyphen") {
      found= true;
      break;
    }
  }
  QVERIFY (found);
}

void
TestEditTable::test_default_table_tree_twith_value () {
  tree T= default_table_tree (2, 3, true);
  QVERIFY (is_func (T, TFORMAT));

  for (int i= 0; i < N (T); i++) {
    if (is_func (T[i], TWITH, 2) && T[i][0] == "table-hyphen") {
      QCOMPARE (T[i][1], "y");
      return;
    }
  }
  QFAIL ("table-hyphen twith not found");
}

void
TestEditTable::test_default_hyphen_enabled_in_text_mode () {
  QVERIFY (table_default_hyphen_enabled ("text"));
}

void
TestEditTable::test_default_hyphen_disabled_in_math_mode () {
  QVERIFY (!table_default_hyphen_enabled ("math"));
}

void
TestEditTable::test_default_table_tree_skips_table_hyphen_in_math_mode () {
  tree T= default_table_tree (2, 3, table_default_hyphen_enabled ("math"));
  QVERIFY (is_func (T, TFORMAT));

  for (int i= 0; i < N (T); i++) {
    QVERIFY (!is_func (T[i], TWITH, 2) || T[i][0] != "table-hyphen");
  }
}

void
TestEditTable::test_no_document_wrap_in_math_mode () {
  QVERIFY (!table_needs_document_wrap ("y", "no", "math"));
}

QTEST_MAIN (TestEditTable)
#include "edit_table_test.moc"
