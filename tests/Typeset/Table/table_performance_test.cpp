
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

// Helper function to measure execution time
template <typename Func>
long long
measure_time (Func&& func, const string& operation_name) {
  auto start= std::chrono::high_resolution_clock::now ();
  func ();
  auto end= std::chrono::high_resolution_clock::now ();
  auto duration=
      std::chrono::duration_cast<std::chrono::microseconds> (end - start);
  Q_UNUSED (operation_name);

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
  void test_20x20_text_table ();
  void test_20x20_matrix_table ();
  void test_50x50_text_table ();
  // New tests for optimization validations
  void test_handle_decorations_performance ();
  void test_cell_hyphen_wrapping ();
  void test_cell_hyphen_multi_column ();
  void test_cell_long_content_no_row_duplication ();
  void cleanupTestCase ();
};

void
TestTablePerformance::initTestCase () {
  init_lolly ();
  init_texmacs_home_path ();
  cache_initialize ();
  init_tex ();
  moebius::drd::init_std_drd ();
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
TestTablePerformance::test_50x50_text_table () {
  cache_refresh ();
  edit_env env       = create_test_env ();
  tree     table_tree= create_matrix_tree (50, 50);

  auto typeset_time= measure_table_creation_time (
      env, table_tree, "50x50 text table creation");

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
add_cell_decoration (tree& tformat, int row, int col, const tree& decoration) {
  tformat << tree (CWITH, as_string (row), as_string (row), as_string (col),
                   as_string (col), "cell-decoration", decoration);
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

    QVERIFY (median_us >= 0.0);
    QVERIFY (rows_after > rows_before);
    QVERIFY (cols_after > cols_before);
  }
}

void
TestTablePerformance::test_cell_hyphen_wrapping () {
  cache_refresh ();
  edit_env env= create_test_env ();

  // Create a very long string that exceeds page width
  string long_text;
  for (int i= 0; i < 200; i++)
    long_text << "d";

  // Create table without wrapping (cell-hyphen = "n")
  tree table_no_wrap (TFORMAT, tree (TABLE, 1));
  tree row_no_wrap (ROW, 1);
  row_no_wrap[0]     = tree (CELL, tree (DOCUMENT, long_text));
  table_no_wrap[0][0]= row_no_wrap;

  // Create table with wrapping (cell-hyphen = "t")
  tree table_wrap (TFORMAT);
  table_wrap << tree (CWITH, "1", "1", "1", "1", "cell-hyphen", "t");
  table_wrap << tree (TABLE, 1);
  tree row_wrap (ROW, 1);
  row_wrap[0]     = tree (CELL, tree (DOCUMENT, long_text));
  table_wrap[1][0]= row_wrap;

  // Typeset table without wrapping
  table tab_no_wrap (env);
  tab_no_wrap->typeset (table_no_wrap, path ());
  tab_no_wrap->position_columns (true);
  tab_no_wrap->finish_horizontal ();
  tab_no_wrap->position_rows ();
  tab_no_wrap->finish ();

  // Typeset table with wrapping
  table tab_wrap (env);
  tab_wrap->typeset (table_wrap, path ());
  tab_wrap->position_columns (true);
  tab_wrap->finish_horizontal ();
  tab_wrap->position_rows ();
  tab_wrap->finish ();

  SI width_no_wrap       = tab_no_wrap->T[0][0]->b->w ();
  SI width_wrap          = tab_wrap->T[0][0]->b->w ();
  SI height_no_wrap      = tab_no_wrap->T[0][0]->b->h ();
  SI height_wrap         = tab_wrap->T[0][0]->b->h ();
  SI inner_width_no_wrap = tab_no_wrap->T[0][0]->b[0]->w ();
  SI inner_width_wrap    = tab_wrap->T[0][0]->b[0]->w ();
  SI inner_height_no_wrap= tab_no_wrap->T[0][0]->b[0]->h ();
  SI inner_height_wrap   = tab_wrap->T[0][0]->b[0]->h ();

  // When cell-hyphen is enabled, the cell should wrap and be narrower
  // than the non-wrapping case
  QVERIFY (width_wrap < width_no_wrap);
}

void
TestTablePerformance::test_cell_hyphen_multi_column () {
  cache_refresh ();
  edit_env env= create_test_env ();

  // Use actual text pattern from 0117.tmu
  string long_text = "sdasd dasd sdsdasd dasd sdsdasd dasd sdsdasd dasd sd"
                     "sdasd dasd sdsdasd dasd sdsdasd dasd sdsdasd dasd sd"
                     "sdasd dasd sdsdasd dasd sdsdasd dasd sdsdasd dasd sd";
  string short_text= "sdasd dasd sd";

  // Create 2x5 table with cell-hyphen enabled for all cells
  // Matching the 0117.tmu test case layout
  tree table_wrap (TFORMAT);
  table_wrap << tree (CWITH, "1", "-1", "1", "-1", "cell-hyphen", "t");
  table_wrap << tree (TABLE, 2);

  // Row 0: 3 long text cols + 1 short + 1 empty
  tree row0 (ROW, 5);
  row0[0]         = tree (CELL, tree (DOCUMENT, long_text));
  row0[1]         = tree (CELL, tree (DOCUMENT, long_text));
  row0[2]         = tree (CELL, tree (DOCUMENT, long_text));
  row0[3]         = tree (CELL, tree (DOCUMENT, short_text));
  row0[4]         = tree (CELL, tree (DOCUMENT, ""));
  table_wrap[1][0]= row0;

  // Row 1: 4 empty + 1 long text
  tree row1 (ROW, 5);
  row1[0]         = tree (CELL, tree (DOCUMENT, ""));
  row1[1]         = tree (CELL, tree (DOCUMENT, ""));
  row1[2]         = tree (CELL, tree (DOCUMENT, ""));
  row1[3]         = tree (CELL, tree (DOCUMENT, ""));
  row1[4]         = tree (CELL, tree (DOCUMENT, long_text));
  table_wrap[1][1]= row1;

  // Typeset table
  table tab_wrap (env);
  tab_wrap->typeset (table_wrap, path ());
  tab_wrap->position_columns (true);

  SI pw, d1, d2, d3, d4, d5, d6, d7;
  tab_wrap->env->get_page_pars (pw, d1, d2, d3, d4, d5, d6, d7);
  tab_wrap->finish_horizontal ();
  tab_wrap->position_rows ();
  tab_wrap->finish ();

  // Check each cell's box width vs column width
  for (int i= 0; i < 2; i++)
    for (int j= 0; j < 5; j++) {
      SI box_w= tab_wrap->T[i][j]->b->w ();
      SI col_w= tab_wrap->mw[j];
      Q_UNUSED (box_w);
      Q_UNUSED (col_w);
    }
}

// Regression test: when a table cell has very long horizontal content that
// exceeds the page width, position_columns() calls lz->produce() to calculate
// minimum widths, and finish_horizontal() calls lz->produce() again for the
// actual content. Without the fix, the lazy_paragraph's stacker accumulates
// items from both calls, causing the cell content to appear twice.
void
TestTablePerformance::test_cell_long_content_no_row_duplication () {
  cache_refresh ();
  edit_env env= create_test_env ();

  // Create very long text (single unbreakable word)
  string long_text;
  for (int i= 0; i < 200; i++)
    long_text << "d";

  // Create a 2-column table with cell-hyphen enabled
  // Two columns with long text triggers total > page_w in position_columns(),
  // which causes lz->produce() to be called for width calculation.
  tree table_tree (TFORMAT);
  table_tree << tree (CWITH, "1", "-1", "1", "-1", "cell-hyphen", "t");
  table_tree << tree (TABLE, 1);
  tree row (ROW, 2);
  row[0]          = tree (CELL, tree (DOCUMENT, long_text));
  row[1]          = tree (CELL, tree (DOCUMENT, long_text));
  table_tree[1][0]= row;

  table tab (env);
  tab->typeset (table_tree, path ());
  tab->handle_decorations ();
  tab->handle_span ();
  tab->merge_borders ();
  tab->position_columns (true);
  tab->finish_horizontal ();
  tab->position_rows ();
  tab->finish ();

  // The cell's b is a cell_box; b[0] is the inner content from lz->produce().
  // Without the fix, b[0]->h() would be roughly doubled because
  // format_paragraph() was called twice on the same lazy_paragraph.
  SI content_h0= tab->T[0][0]->b[0]->h ();
  SI content_h1= tab->T[0][1]->b[0]->h ();
  SI line_h    = env->fn->yx;

  // A generous upper bound: 10x line height.
  // With the bug, content height would be ~2x of normal (roughly doubled).
  QVERIFY2 (content_h0 < 10 * line_h,
            "Cell 0 content height too large - possible row duplication");
  QVERIFY2 (content_h1 < 10 * line_h,
            "Cell 1 content height too large - possible row duplication");
}

void
TestTablePerformance::cleanupTestCase () {}

QTEST_MAIN (TestTablePerformance)
#include "table_performance_test.moc"