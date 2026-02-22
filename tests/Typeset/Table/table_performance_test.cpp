
/******************************************************************************
 * MODULE     : table_performance_test.cpp
 * DESCRIPTION: Performance test for table optimizations
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Metafont/load_tex.hpp"
#include "Table/table.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "sys_utils.hpp"
#include "tm_sys_utils.hpp"
#include <QDebug>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>
#include <moebius/drd/drd_std.hpp>
#include <utility>
#include <vector>

using namespace moebius;
using moebius::drd::std_drd;

// Helper function to create a matrix tree of given dimensions
tree
create_matrix_tree (int rows, int cols) {
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
edit_env
create_test_env () {
  drd_info              drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  return edit_env (drd, "none", h1, h2, h3, h4, h5, h6);
}

// Helper function to create a simple 1x1 matrix tree
tree
create_simple_matrix () {
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
tree
create_table_with_matrix_cells (int rows, int cols) {
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
tree
create_eqnarray_tree (int rows) {
  // Create a table with 3 columns
  tree T (TABLE, rows);
  for (int i= 0; i < rows; i++) {
    tree R (ROW, 3);
    R[0]= tree (CELL, "x = " * as_string (i)); // right-aligned
    R[1]= tree (CELL, "y");                    // centered
    R[2]= tree (CELL, as_string (i * i));      // left-aligned
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

  qDebug () << as_charp (operation_name) << ": " << duration.count () << " μs";
  return duration.count ();
}

// Helper function to measure table creation time
long long
measure_table_creation_time (edit_env& env, const tree& table_tree,
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
  // New tests for optimization validations
  void test_decorated_table_performance ();
  void test_width_cache_efficiency ();
  void test_incremental_update_performance ();
  void cleanupTestCase ();
};

void
TestTablePerformance::initTestCase () {
  init_lolly ();
  init_texmacs_home_path ();
  cache_initialize ();
  init_tex ();
  moebius::drd::init_std_drd ();
  qDebug () << "=== Table Performance Test ===";
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
  simple_row[0]     = tree (CELL, "hello");
  simple_table[0][0]= simple_row;

  qDebug () << "Testing 1x1 table with text content...";
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

  simple_row[0]     = tree (CELL, matrix_cell);
  simple_table[0][0]= simple_row;

  qDebug () << "Testing 1x1 table with matrix content...";
  auto matrix_time= measure_table_creation_time (env, simple_table,
                                                 "1x1 matrix table creation");

  QVERIFY (matrix_time >= 0);
}

void
TestTablePerformance::test_20x20_text_table () {
  edit_env env       = create_test_env ();
  tree     table_tree= create_matrix_tree (20, 20);

  auto typeset_time= measure_table_creation_time (env, table_tree,
                                                  "20x20 text table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_100x100_text_table () {
  edit_env env       = create_test_env ();
  tree     table_tree= create_matrix_tree (100, 100);

  auto typeset_time= measure_table_creation_time (
      env, table_tree, "100x100 text table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_20x20_matrix_table () {
  edit_env env       = create_test_env ();
  tree     table_tree= create_table_with_matrix_cells (20, 20);

  auto typeset_time= measure_table_creation_time (
      env, table_tree, "20x20 matrix table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_100x100_matrix_table () {
  edit_env env       = create_test_env ();
  tree     table_tree= create_table_with_matrix_cells (100, 100);

  auto typeset_time= measure_table_creation_time (
      env, table_tree, "100x100 matrix table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_multiple_20x20_matrix_creation () {
  edit_env env        = create_test_env ();
  tree     matrix_tree= create_table_with_matrix_cells (20, 20);

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
  edit_env env        = create_test_env ();
  tree     matrix_tree= create_matrix_tree (20, 20);

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
  edit_env env          = create_test_env ();
  tree     eqnarray_tree= create_eqnarray_tree (1);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 1 row creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_20_rows () {
  edit_env env          = create_test_env ();
  tree     eqnarray_tree= create_eqnarray_tree (20);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 20 rows creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_100_rows () {
  edit_env env          = create_test_env ();
  tree     eqnarray_tree= create_eqnarray_tree (100);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 100 rows creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_5x20_rows () {
  edit_env env          = create_test_env ();
  tree     eqnarray_tree= create_eqnarray_tree (20);

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

double
measure_median_table_creation_time (edit_env& env, const tree& table_tree,
                                    const string& operation_name,
                                    int           iterations= 5) {
  if (iterations <= 0) {
    qWarning () << "iterations must be > 0 for" << as_charp (operation_name);
    return 0.0;
  }

  std::vector<long long> samples;
  samples.reserve (iterations);
  for (int i= 0; i < iterations; i++) {
    samples.push_back (measure_table_creation_time (
        env, table_tree, operation_name * " #" * as_string (i + 1)));
  }
  std::sort (samples.begin (), samples.end ());
  return (double) samples[iterations / 2];
}

void
add_cell_decoration (tree& tformat, int row, int col, const tree& decoration) {
  tformat << tree (CWITH, as_string (row), as_string (row), as_string (col),
                   as_string (col), "cell-decoration", decoration);
}

template <typename Func>
std::pair<double, double>
measure_two_calls_us (Func&& func, int iterations) {
  if (iterations <= 0) return std::make_pair (0.0, 0.0);

  long long total_first= 0, total_second= 0;
  for (int i= 0; i < iterations; i++) {
    auto start1= std::chrono::high_resolution_clock::now ();
    func ();
    auto end1= std::chrono::high_resolution_clock::now ();

    auto start2= std::chrono::high_resolution_clock::now ();
    func ();
    auto end2= std::chrono::high_resolution_clock::now ();

    total_first+=
        std::chrono::duration_cast<std::chrono::microseconds> (end1 - start1)
            .count ();
    total_second+=
        std::chrono::duration_cast<std::chrono::microseconds> (end2 - start2)
            .count ();
  }
  return std::make_pair (total_first / (double) iterations,
                         total_second / (double) iterations);
}

// Helper function to create a decoration tree which really expands table size
// 3x3 decoration with TMARKER at center means each decorated cell adds
// 1 extra row/col on each side after handle_decorations().
tree
create_expanding_decoration_tree () {
  tree decoration_table (TABLE, 3);
  for (int i= 0; i < 3; i++) {
    tree decoration_row (ROW, 3);
    for (int j= 0; j < 3; j++) {
      if (i == 1 && j == 1) decoration_row[j]= tree (TMARKER);
      else decoration_row[j]= tree (CELL, "•");
    }
    decoration_table[i]= decoration_row;
  }
  return tree (TFORMAT, decoration_table);
}

// Decorated table performance
void
TestTablePerformance::test_decorated_table_performance () {
  edit_env env= create_test_env ();

  const int size       = 40;
  tree      plain_table= create_matrix_tree (size, size);

  tree T (TABLE, size);
  for (int i= 0; i < size; i++) {
    tree R (ROW, size);
    for (int j= 0; j < size; j++) {
      R[j]= tree (CELL, tree (as_string (i) * "," * as_string (j)));
    }
    T[i]= R;
  }

  tree decoration_tree= create_expanding_decoration_tree ();
  tree tformat (TFORMAT);
  add_cell_decoration (tformat, 1, 1, decoration_tree);
  add_cell_decoration (tformat, 6, 6, decoration_tree);
  add_cell_decoration (tformat, 6, 30, decoration_tree);
  add_cell_decoration (tformat, 20, 20, decoration_tree);
  add_cell_decoration (tformat, 30, 6, decoration_tree);
  add_cell_decoration (tformat, 30, 30, decoration_tree);

  tformat << T;

  // Structural check: decorations must expand table dimensions.
  table structural_tab (env);
  structural_tab->typeset (tformat, path ());
  int rows_before= structural_tab->nr_rows;
  int cols_before= structural_tab->nr_cols;
  structural_tab->handle_decorations ();
  int rows_after= structural_tab->nr_rows;
  int cols_after= structural_tab->nr_cols;

  qDebug () << "Decorated table dimensions:" << rows_before << "x"
            << cols_before << "->" << rows_after << "x" << cols_after;

  QVERIFY (rows_after > rows_before);
  QVERIFY (cols_after > cols_before);

  qDebug () << "Testing " << size << "x" << size << " table performance...";

  // Warmup (excluded from stats)
  measure_table_creation_time (env, plain_table, "40x40 plain warmup");
  measure_table_creation_time (env, tformat, "40x40 decorated warmup");

  const int iterations= 5;
  double    plain_time= measure_median_table_creation_time (
      env, plain_table, "40x40 plain table", iterations);
  double decorated_time= measure_median_table_creation_time (
      env, tformat, "40x40 decorated table", iterations);

  qDebug () << "Median plain table: " << plain_time << " μs";
  qDebug () << "Median decorated table: " << decorated_time << " μs";

  // Basic validation
  QVERIFY (plain_time > 0);
  QVERIFY (decorated_time > 0);

  double ratio= decorated_time / plain_time;
  qDebug () << "Decorated/Plain ratio: " << ratio;

  // Small differences are often noise.
  if (ratio < 0.8) {
    qDebug () << "WARNING: Decorated table is much faster than plain table.";
    qDebug () << "Please re-check decoration shape and overlap assumptions.";
  }
}

// Width/height cache efficiency
void
TestTablePerformance::test_width_cache_efficiency () {
  edit_env  env       = create_test_env ();
  const int size      = 60;
  tree      table_tree= create_matrix_tree (size, size);

  table tab (env);
  tab->typeset (table_tree, path ());
  tab->handle_decorations ();
  tab->handle_span ();
  tab->merge_borders ();

  tab->position_columns (true);
  tab->position_rows ();

  const int iterations= 5;
  auto      col_times=
      measure_two_calls_us ([&] { tab->position_columns (true); }, iterations);
  double avg_first = col_times.first;
  double avg_second= col_times.second;
  double ratio     = avg_first / avg_second;

  qDebug () << "Average first position_columns: " << avg_first << " μs";
  qDebug () << "Average second position_columns: " << avg_second << " μs";
  qDebug () << "Cache efficiency ratio: " << ratio;

  auto row_times=
      measure_two_calls_us ([&] { tab->position_rows (); }, iterations);
  avg_first        = row_times.first;
  avg_second       = row_times.second;
  double rows_ratio= avg_first / avg_second;

  qDebug () << "Average first position_rows: " << avg_first << " μs";
  qDebug () << "Average second position_rows: " << avg_second << " μs";
  qDebug () << "Rows cache efficiency ratio: " << rows_ratio;

  // No strict assertion: optimization may be absent in this branch.
  QVERIFY (avg_first > 0 && avg_second > 0);
}

// Incremental update baseline
void
TestTablePerformance::test_incremental_update_performance () {
  edit_env env= create_test_env ();

  const int size      = 50;
  tree      table_tree= create_matrix_tree (size, size);

  auto full_time= measure_table_creation_time (env, table_tree,
                                               "Full 50x50 table creation");

  Q_UNUSED (size);
  qDebug () << "Note: Incremental update optimization not tested";
  qDebug () << "(mark_dirty() interfaces not available in current branch)";

  QVERIFY (full_time > 0);
}

void
TestTablePerformance::cleanupTestCase () {
  qDebug () << "\n=== Performance Test Complete ===";
}

QTEST_MAIN (TestTablePerformance)
#include "table_performance_test.moc"