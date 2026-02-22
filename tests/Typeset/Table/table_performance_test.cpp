
/******************************************************************************
 * MODULE     : table_performance_test.cpp
 * DESCRIPTION: Performance test for table optimizations
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Table/table.hpp"
#include "base.hpp"
#include "env.hpp"
#include "sys_utils.hpp"
#include "tm_sys_utils.hpp"
#include "data_cache.hpp"
#include "Metafont/load_tex.hpp"
#include <moebius/drd/drd_std.hpp>
#include <QtTest/QtTest>
#include <QDebug>
#include <chrono>

using namespace moebius;
using moebius::drd::std_drd;

// Helper function to create a matrix tree of given dimensions
tree create_matrix_tree (int rows, int cols) {
  tree T (TABLE, rows);
  for (int i= 0; i < rows; i++) {
    tree R (ROW, cols);
    for (int j= 0; j < cols; j++) {
      // Create cell content: simple text "cell i,j"
      R[j]= tree (CELL, tree (as_string (i) * "," * as_string (j)));
    }
    T[i]= R;
  }
  // Wrap in TFORMAT as expected by table typesetter
  return tree (TFORMAT, T);
}

// Helper function to create a proper edit_env for testing
edit_env create_test_env () {
  drd_info drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  return edit_env (drd, "none", h1, h2, h3, h4, h5, h6);
}

// Helper function to create a simple 1x1 matrix tree
tree create_simple_matrix () {
  tree matrix_tree (CONCAT);
  matrix_tree << tree (BEGIN, "matrix");

  tree matrix_row (ROW, 1);
  matrix_row[0]= "a";

  tree matrix_table (TABLE, 1);
  matrix_table[0]= matrix_row;

  matrix_tree << matrix_table;
  matrix_tree << tree (END, "matrix");
  return matrix_tree;
}

// Helper function to create a table tree with matrix cells
tree create_table_with_matrix_cells (int rows, int cols) {
  tree T (TABLE, rows);
  tree matrix_cell= create_simple_matrix ();

  for (int i= 0; i < rows; i++) {
    tree R (ROW, cols);
    for (int j= 0; j < cols; j++) {
      // Each cell contains the same simple matrix
      R[j]= tree (CELL, matrix_cell);
    }
    T[i]= R;
  }
  // Wrap in TFORMAT as expected by table typesetter
  return tree (TFORMAT, T);
}

// Helper function to create an eqnarray tree with given number of rows
// Eqnarray is essentially a table with 3 columns (r, c, l)
tree create_eqnarray_tree (int rows) {
  // Create a table with 3 columns
  tree T (TABLE, rows);
  for (int i= 0; i < rows; i++) {
    tree R (ROW, 3);
    R[0]= tree (CELL, "x = " * as_string (i));  // right-aligned
    R[1]= tree (CELL, "y");                     // centered
    R[2]= tree (CELL, as_string (i * i));       // left-aligned
    T[i]= R;
  }
  // Wrap in TFORMAT with specific column alignment (r, c, l)
  tree tformat (TFORMAT);
  // Add column alignment specifications
  tformat << tree (CWITH, "1", "1", CELL_HALIGN, "r");
  tformat << tree (CWITH, "1", "2", CELL_HALIGN, "c");
  tformat << tree (CWITH, "1", "3", CELL_HALIGN, "l");
  tformat << T;
  return tformat;
}

// Helper function to measure execution time
template <typename Func>
long long
measure_time (Func&& func, const string& operation_name) {
  auto start= std::chrono::high_resolution_clock::now ();
  func ();
  auto end= std::chrono::high_resolution_clock::now ();
  auto duration=
      std::chrono::duration_cast<std::chrono::microseconds> (end - start);

  qDebug() << as_charp (operation_name) << ": " << duration.count () << " μs";
  return duration.count ();
}

// Helper function to measure table creation time
long long measure_table_creation_time (edit_env& env, const tree& table_tree,
                                       const string& operation_name) {
  return measure_time (
      [&] {
        table tab (env);
        tab->typeset (table_tree, path ());
        tab->handle_decorations ();
        tab->handle_span ();
        tab->merge_borders ();
        tab->position_columns (true);
        tab->finish_horizontal ();
        tab->position_rows ();
        tab->finish ();
        Q_UNUSED (tab);
      },
      operation_name);
}

class TestTablePerformance : public QObject {
  Q_OBJECT

private slots:
  void initTestCase ();
  void test_table_optimization_status ();
  void test_1x1_text_table ();
  void test_1x1_matrix_table ();
  void test_20x20_text_table ();
  void test_20x20_matrix_table ();
  void test_100x100_text_table ();
  void test_100x100_matrix_table ();
  void test_multiple_20x20_creation ();
  void test_multiple_20x20_matrix_creation ();
  void test_eqnarray_1_row ();
  void test_eqnarray_20_rows ();
  void test_eqnarray_100_rows ();
  void test_eqnarray_5x20_rows ();
  void cleanupTestCase ();
};

void
TestTablePerformance::initTestCase () {
  init_lolly ();
  init_texmacs_home_path ();
  cache_initialize ();
  init_tex ();
  moebius::drd::init_std_drd ();
  qDebug() << "=== Table Performance Test ===";
}

void
TestTablePerformance::test_table_optimization_status () {
  // Optimization is always enabled
  QVERIFY (true);
}

void
TestTablePerformance::test_1x1_text_table () {
  edit_env env= create_test_env ();

  tree simple_table (TFORMAT, tree (TABLE, 1));
  tree simple_row (ROW, 1);
  simple_row[0]= tree (CELL, "hello");
  simple_table[0][0]= simple_row;

  qDebug() << "Testing 1x1 table with text content...";
  auto simple_time= measure_table_creation_time (env, simple_table,
                                                 "1x1 text table creation");

  QVERIFY (simple_time >= 0);
}

void
TestTablePerformance::test_1x1_matrix_table () {
  edit_env env= create_test_env ();

  // Create a 1x1 table with a matrix in the cell
  tree simple_table (TFORMAT, tree (TABLE, 1));
  tree simple_row (ROW, 1);

  // Use the helper function to create a simple matrix
  tree matrix_cell= create_simple_matrix ();

  simple_row[0]= tree (CELL, matrix_cell);
  simple_table[0][0]= simple_row;

  qDebug() << "Testing 1x1 table with matrix content...";
  auto matrix_time= measure_table_creation_time (env, simple_table,
                                                 "1x1 matrix table creation");

  QVERIFY (matrix_time >= 0);
}

void
TestTablePerformance::test_20x20_text_table () {
  edit_env env= create_test_env ();
  tree table_tree= create_matrix_tree (20, 20);

  auto typeset_time= measure_table_creation_time (env, table_tree,
                                                  "20x20 text table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_100x100_text_table () {
  edit_env env= create_test_env ();
  tree table_tree= create_matrix_tree (100, 100);

  auto typeset_time= measure_table_creation_time (env, table_tree,
                                                  "100x100 text table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_20x20_matrix_table () {
  edit_env env= create_test_env ();
  tree table_tree= create_table_with_matrix_cells (20, 20);

  auto typeset_time= measure_table_creation_time (env, table_tree,
                                                  "20x20 matrix table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_100x100_matrix_table () {
  edit_env env= create_test_env ();
  tree table_tree= create_table_with_matrix_cells (100, 100);

  auto typeset_time= measure_table_creation_time (env, table_tree,
                                                  "100x100 matrix table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_multiple_20x20_matrix_creation () {
  edit_env env= create_test_env ();
  tree matrix_tree= create_table_with_matrix_cells (20, 20);

  auto total_time= measure_time (
      [&] {
        // Create 5 tables of 20x20 with matrix cells
        for (int i= 0; i < 5; i++) {
          table tab (env);
          tab->typeset (matrix_tree, path ());
          tab->handle_decorations ();
          tab->handle_span ();
          tab->merge_borders ();
          tab->position_columns (true);
          tab->finish_horizontal ();
          tab->position_rows ();
          tab->finish ();
          Q_UNUSED (tab);
        }
      },
      "5x 20x20 matrix table creation");

  QVERIFY (total_time >= 0);
}

void
TestTablePerformance::test_multiple_20x20_creation () {
  edit_env env= create_test_env ();
  tree matrix_tree= create_matrix_tree (20, 20);

  auto total_time= measure_time (
      [&] {
        // Create 5 tables of 20x20
        for (int i= 0; i < 5; i++) {
          table tab (env);
          tab->typeset (matrix_tree, path ());
          tab->handle_decorations ();
          tab->handle_span ();
          tab->merge_borders ();
          tab->position_columns (true);
          tab->finish_horizontal ();
          tab->position_rows ();
          tab->finish ();
          Q_UNUSED (tab);
        }
      },
      "5x 20x20 table creation");

  QVERIFY (total_time >= 0);
}

void
TestTablePerformance::test_eqnarray_1_row () {
  edit_env env= create_test_env ();
  tree eqnarray_tree= create_eqnarray_tree (1);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 1 row creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_20_rows () {
  edit_env env= create_test_env ();
  tree eqnarray_tree= create_eqnarray_tree (20);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 20 rows creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_100_rows () {
  edit_env env= create_test_env ();
  tree eqnarray_tree= create_eqnarray_tree (100);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 100 rows creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_5x20_rows () {
  edit_env env= create_test_env ();
  tree eqnarray_tree= create_eqnarray_tree (20);

  auto total_time= measure_time (
      [&] {
        // Create 5 eqnarrays of 20 rows each
        for (int i= 0; i < 5; i++) {
          table tab (env);
          tab->typeset (eqnarray_tree, path ());
          tab->handle_decorations ();
          tab->handle_span ();
          tab->merge_borders ();
          tab->position_columns (true);
          tab->finish_horizontal ();
          tab->position_rows ();
          tab->finish ();
          Q_UNUSED (tab);
        }
      },
      "5x Eqnarray 20 rows creation");

  QVERIFY (total_time >= 0);
}

void
TestTablePerformance::cleanupTestCase () {
  qDebug() << "\n=== Performance Test Complete ===";
}

QTEST_MAIN (TestTablePerformance)
#include "table_performance_test.moc"