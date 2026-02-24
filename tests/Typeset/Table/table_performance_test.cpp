
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
  void test_handle_decorations_correctness ();
  void test_handle_decorations_performance ();
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
  cache_refresh ();
  // Optimization is always enabled
  QVERIFY (true);
}

void
TestTablePerformance::test_1x1_text_table () {
  cache_refresh ();
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
  cache_refresh ();
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
  cache_refresh ();
  edit_env env       = create_test_env ();
  tree     table_tree= create_matrix_tree (20, 20);

  auto typeset_time= measure_table_creation_time (env, table_tree,
                                                  "20x20 text table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_100x100_text_table () {
  cache_refresh ();
  edit_env env       = create_test_env ();
  tree     table_tree= create_matrix_tree (100, 100);

  auto typeset_time= measure_table_creation_time (
      env, table_tree, "100x100 text table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_20x20_matrix_table () {
  cache_refresh ();
  edit_env env       = create_test_env ();
  tree     table_tree= create_table_with_matrix_cells (20, 20);

  auto typeset_time= measure_table_creation_time (
      env, table_tree, "20x20 matrix table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_100x100_matrix_table () {
  cache_refresh ();
  edit_env env       = create_test_env ();
  tree     table_tree= create_table_with_matrix_cells (100, 100);

  auto typeset_time= measure_table_creation_time (
      env, table_tree, "100x100 matrix table creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_multiple_20x20_matrix_creation () {
  cache_refresh ();
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
  cache_refresh ();
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
  cache_refresh ();
  edit_env env          = create_test_env ();
  tree     eqnarray_tree= create_eqnarray_tree (1);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 1 row creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_20_rows () {
  cache_refresh ();
  edit_env env          = create_test_env ();
  tree     eqnarray_tree= create_eqnarray_tree (20);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 20 rows creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_100_rows () {
  cache_refresh ();
  edit_env env          = create_test_env ();
  tree     eqnarray_tree= create_eqnarray_tree (100);

  auto typeset_time= measure_table_creation_time (env, eqnarray_tree,
                                                  "Eqnarray 100 rows creation");

  QVERIFY (typeset_time >= 0);
}

void
TestTablePerformance::test_eqnarray_5x20_rows () {
  cache_refresh ();
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

// Test for handle_decorations loop optimization correctness
void
TestTablePerformance::test_handle_decorations_correctness () {
  cache_refresh ();
  edit_env env= create_test_env ();

  const int size= 10;
  tree      T (TABLE, size);
  for (int i= 0; i < size; i++) {
    tree R (ROW, size);
    for (int j= 0; j < size; j++) {
      R[j]= tree (CELL, tree (as_string (i) * "," * as_string (j)));
    }
    T[i]= R;
  }

  // Add decorations at specific positions
  tree decoration_tree= create_expanding_decoration_tree ();
  tree tformat (TFORMAT);
  // Add decorations at (1,1), (5,5), (8,8)
  add_cell_decoration (tformat, 1, 1, decoration_tree);
  add_cell_decoration (tformat, 5, 5, decoration_tree);
  add_cell_decoration (tformat, 8, 8, decoration_tree);
  tformat << T;

  table tab (env);
  tab->typeset (tformat, path ());
  int rows_before= tab->nr_rows;
  int cols_before= tab->nr_cols;
  tab->handle_decorations ();
  int rows_after= tab->nr_rows;
  int cols_after= tab->nr_cols;

  qDebug () << "Table dimensions:" << rows_before << "x" << cols_before << "->"
            << rows_after << "x" << cols_after;

  // Verify that decorations expanded the table
  QVERIFY (rows_after > rows_before);
  QVERIFY (cols_after > cols_before);

  // Verify that the table still has correct number of cells
  // (original cells plus decoration cells minus overlaps)
  // This is a basic sanity check
  int total_cells= 0;
  for (int i= 0; i < rows_after; i++) {
    for (int j= 0; j < cols_after; j++) {
      if (!is_nil (tab->T[i][j])) {
        total_cells++;
      }
    }
  }
  qDebug () << "Non-nil cells after handle_decorations:" << total_cells;
  // At least original size * size cells should be present
  QVERIFY (total_cells >= size * size);
}

// Performance test for handle_decorations loop optimization
void
TestTablePerformance::test_handle_decorations_performance () {
  cache_refresh ();
  edit_env env= create_test_env ();

  // Test different table sizes for real decoration expansion workload.
  // Keep a single test but make decoration size grow with table size,
  // to amplify complexity differences.
  const int sizes[]  = {20, 30, 40, 50};
  const int num_sizes= sizeof (sizes) / sizeof (sizes[0]);

  for (int idx= 0; idx < num_sizes; idx++) {
    const int size= sizes[idx];

    // Base table content
    tree T (TABLE, size);
    for (int i= 0; i < size; i++) {
      tree R (ROW, size);
      for (int j= 0; j < size; j++) {
        R[j]= tree (CELL, tree (as_string (i) * "," * as_string (j)));
      }
      T[i]= R;
    }

    // Make decoration size scale with table size (odd, >=3).
    int d= size / 2;
    if (d < 3) d= 3;
    if ((d % 2) == 0) d++;

    tree decoration_table (TABLE, d);
    int  c= d / 2;
    for (int di= 0; di < d; di++) {
      tree decoration_row (ROW, d);
      for (int dj= 0; dj < d; dj++) {
        if (di == c && dj == c) decoration_row[dj]= tree (TMARKER);
        else decoration_row[dj]= tree (CELL, "•");
      }
      decoration_table[di]= decoration_row;
    }
    tree decoration_tree= tree (TFORMAT, decoration_table);

    tree tformat (TFORMAT);

    // Add real cell-decoration (with TMARKER) so handle_decorations()
    // enters expansion path (status == 1), not only format scanning.
    // NOTE: CWITH row/col indices are 1-based in this codebase.
    int decorations= 0;
    for (int i= 1; i <= size; i+= 2)
      for (int j= 1; j <= size; j+= 2) {
        add_cell_decoration (tformat, i, j, decoration_tree);
        decorations++;
      }

    tformat << T;

    // Measure handle_decorations itself (avoid mixing typeset cost)
    // Use warmup + median to reduce micro-benchmark noise.
    const int              warmup_iterations= 2;
    const int              iterations       = 9; // odd number for stable median
    std::vector<long long> samples;
    samples.reserve (iterations);

    int rows_before= 0, cols_before= 0;
    int rows_after= 0, cols_after= 0;

    // Warmup
    for (int k= 0; k < warmup_iterations; k++) {
      table tab (env);
      tab->typeset (tformat, path ());
      tab->handle_decorations ();
    }

    for (int k= 0; k < iterations; k++) {
      table tab (env);
      tab->typeset (tformat, path ());

      rows_before= tab->nr_rows;
      cols_before= tab->nr_cols;

      auto start= std::chrono::high_resolution_clock::now ();
      tab->handle_decorations ();
      auto end= std::chrono::high_resolution_clock::now ();

      rows_after= tab->nr_rows;
      cols_after= tab->nr_cols;
      samples.push_back (
          std::chrono::duration_cast<std::chrono::microseconds> (end - start)
              .count ());
    }

    std::sort (samples.begin (), samples.end ());
    double    median_us= (double) samples[iterations / 2];
    long long min_us   = samples.front ();
    long long max_us   = samples.back ();

    double per_n2= median_us / ((double) size * (double) size);
    double per_n4= median_us / ((double) size * (double) size * (double) size *
                                (double) size);
    double per_n2d2=
        median_us / ((double) size * (double) size * (double) d * (double) d);

    qDebug () << as_charp (as_string (size) * "x" * as_string (size) *
                           " handle_decorations median")
              << ":" << median_us << "μs"
              << "(d:" << d << "decorations:" << decorations << ")"
              << "(us/n^2:" << per_n2 << "us/n^4:" << per_n4
              << "us/(n^2*d^2):" << per_n2d2 << ")"
              << "(min:" << min_us << "max:" << max_us << ")"
              << "(" << rows_before << "x" << cols_before << "->" << rows_after
              << "x" << cols_after << ")";

    QVERIFY (median_us >= 0.0);
    QVERIFY (rows_after > rows_before);
    QVERIFY (cols_after > cols_before);
  }
}

void
TestTablePerformance::cleanupTestCase () {
  qDebug () << "\n=== Performance Test Complete ===";
}

QTEST_MAIN (TestTablePerformance)
#include "table_performance_test.moc"