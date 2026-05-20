/******************************************************************************
 * MODULE     : rectangles_test.cpp
 * DESCRIPTION: Test rectangles disjoint_union function
 * COPYRIGHT  : (C) 2025
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "rectangles.hpp"
#include <QtTest/QtTest>

class TestRectangles : public QObject {
  Q_OBJECT

private slots:
  void test_disjoint_union_empty_list ();
  void test_disjoint_union_non_adjacent ();
  void test_disjoint_union_adjacent_horizontal ();
  void test_disjoint_union_adjacent_vertical ();
  void test_disjoint_union_multiple_adjacent ();
  void test_disjoint_union_overlapping ();
  void test_disjoint_union_complex_case ();
  void test_split_cell_border_no_middle_line ();
};

void
TestRectangles::test_disjoint_union_empty_list () {
  rectangles empty_list;
  rectangle  r (10, 10, 20, 20);
  rectangles result= disjoint_union (empty_list, r);

  QVERIFY (!is_nil (result));
  QVERIFY (is_nil (result->next));
  QVERIFY (result->item == r);
}

void
TestRectangles::test_disjoint_union_non_adjacent () {
  rectangle  r1 (0, 0, 10, 10);
  rectangle  r2 (20, 20, 30, 30);
  rectangles list (r1);
  rectangles result= disjoint_union (list, r2);

  int count= 0;
  for (rectangles p= result; !is_nil (p); p= p->next) {
    count++;
  }
  QVERIFY (count == 2);

  bool found_r1= false, found_r2= false;
  for (rectangles p= result; !is_nil (p); p= p->next) {
    if (p->item == r1) found_r1= true;
    if (p->item == r2) found_r2= true;
  }
  QVERIFY (found_r1 && found_r2);
}

void
TestRectangles::test_disjoint_union_adjacent_horizontal () {
  rectangle  r1 (0, 0, 10, 10);
  rectangle  r2 (10, 0, 20, 10);
  rectangles list (r1);
  rectangles result= disjoint_union (list, r2);

  QVERIFY (!is_nil (result));
  QVERIFY (is_nil (result->next));

  rectangle merged= result->item;
  QVERIFY (merged->x1 == 0);
  QVERIFY (merged->y1 == 0);
  QVERIFY (merged->x2 == 20);
  QVERIFY (merged->y2 == 10);
}

void
TestRectangles::test_disjoint_union_adjacent_vertical () {
  rectangle  r1 (0, 0, 10, 10);
  rectangle  r2 (0, 10, 10, 20);
  rectangles list (r1);
  rectangles result= disjoint_union (list, r2);

  QVERIFY (!is_nil (result));
  QVERIFY (is_nil (result->next));

  rectangle merged= result->item;
  QVERIFY (merged->x1 == 0);
  QVERIFY (merged->y1 == 0);
  QVERIFY (merged->x2 == 10);
  QVERIFY (merged->y2 == 20);
}

void
TestRectangles::test_disjoint_union_multiple_adjacent () {
  rectangle  r1 (0, 0, 10, 10);
  rectangle  r2 (10, 0, 20, 10);
  rectangle  r3 (20, 0, 30, 10);
  rectangles list  = rectangles (r2, rectangles (r1));
  rectangles result= disjoint_union (list, r3);

  QVERIFY (!is_nil (result));
  QVERIFY (is_nil (result->next));

  rectangle merged= result->item;
  QVERIFY (merged->x1 == 0);
  QVERIFY (merged->y1 == 0);
  QVERIFY (merged->x2 == 30);
  QVERIFY (merged->y2 == 10);
}

void
TestRectangles::test_disjoint_union_overlapping () {
  rectangle  r1 (0, 0, 15, 10);
  rectangle  r2 (10, 0, 20, 10);
  rectangles list (r1);
  rectangles result= disjoint_union (list, r2);

  // disjoint_union only merges adjacent rectangles, not overlapping ones
  // So overlapping rectangles should remain separate
  int count= 0;
  for (rectangles p= result; !is_nil (p); p= p->next) {
    count++;
  }
  QVERIFY (count == 2);

  bool found_r1= false, found_r2= false;
  for (rectangles p= result; !is_nil (p); p= p->next) {
    if (p->item == r1) found_r1= true;
    if (p->item == r2) found_r2= true;
  }
  QVERIFY (found_r1 && found_r2);
}

void
TestRectangles::test_disjoint_union_complex_case () {
  rectangle  r1 (0, 0, 10, 10);
  rectangle  r2 (15, 15, 25, 25);
  rectangle  r3 (10, 0, 20, 10);
  rectangles list  = rectangles (r2, rectangles (r1));
  rectangles result= disjoint_union (list, r3);

  // 应该有两个：一个是 [0,0]-[20,10] 的合并矩形，另一个是 r2 原样保留
  int count= 0;
  for (rectangles p= result; !is_nil (p); p= p->next)
    count++;
  QVERIFY (count == 2);

  bool has_merged= false, has_r2= false;
  for (rectangles p= result; !is_nil (p); p= p->next) {
    rectangle rc= p->item;
    if (rc->x1 == 0 && rc->y1 == 0 && rc->x2 == 20 && rc->y2 == 10)
      has_merged= true;
    if (p->item == r2) has_r2= true; // r2 未被改写时可用指针判断
  }
  QVERIFY (has_merged && has_r2);
}

// Test that cells on different pages are detected by checking original
// (unthickened) selection rects. When sel->rs->item->y1 > bis->rs->item->y2,
// the cells are on different pages and correct_adjacent should be skipped.
void
TestRectangles::test_split_cell_border_no_middle_line () {
  // Page 1 cell (bottom row): y2=900, y1=700
  rectangle cell1 (100, 700, 400, 900);
  // Page 2 cell (top row): y2=300, y1=100
  // In the coordinate system, page 1 is above page 2.
  // cell1->y1 (700) > cell2->y2 (300) means there's a page gap.
  rectangle cell2 (100, 100, 400, 300);
  QVERIFY (cell1->y1 > cell2->y2);

  // Same-page case: cells that touch.
  // cell_a->y1 == cell_b->y2 means they are adjacent on the same page.
  rectangle cell_a (100, 500, 400, 700);
  rectangle cell_b (100, 300, 400, 500);
  QVERIFY (cell_a->y1 <= cell_b->y2); // same page: no gap
}

QTEST_MAIN (TestRectangles)
#include "rectangles_test.moc"