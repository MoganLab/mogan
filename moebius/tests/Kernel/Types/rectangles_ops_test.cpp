/******************************************************************************
 * MODULE     : rectangles_ops_test.cpp
 * DESCRIPTION: Unit tests for rectangle/rectangles operations
 *              (complement to rectangles_test.cpp which covers disjoint_union)
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "moe_doctests.hpp"
#include "rectangles.hpp"

TEST_CASE ("test equality") {
  rectangle a (0, 0, 10, 10);
  rectangle b (0, 0, 10, 10);
  rectangle c (1, 1, 10, 10);
  CHECK (a == b);
  CHECK (a != c);
}

TEST_CASE ("test area") {
  CHECK_EQ (area (rectangle (0, 0, 4, 5)), 20.0);
  CHECK_EQ (area (rectangle (5, 5, 5, 5)), 0.0);
}

TEST_CASE ("test is_zero") {
  CHECK (is_zero (rectangle (0, 0, 0, 0)));
  CHECK (!is_zero (rectangle (1, 0, 0, 0)));
}

TEST_CASE ("test intersect") {
  rectangle r1 (0, 0, 10, 10);
  rectangle r2 (5, 5, 15, 15);
  rectangle r3 (20, 20, 30, 30);
  CHECK (intersect (r1, r2));
  CHECK (!intersect (r1, r3));
}

TEST_CASE ("test subset") {
  rectangle r1 (0, 0, 10, 10);
  rectangle small (2, 2, 8, 8);
  rectangle r2 (5, 5, 15, 15);
  CHECK (small <= r1);
  CHECK (!(r2 <= r1));
}

TEST_CASE ("test translate") {
  rectangle r (0, 0, 10, 10);
  CHECK (translate (r, 5, 7) == rectangle (5, 7, 15, 17));
  CHECK (translate (r, 0, 0) == r);
  CHECK (translate (r, -3, -4) == rectangle (-3, -4, 7, 6));
}

TEST_CASE ("test translate rectangles") {
  CHECK (translate (rectangles (), 5, 7) == rectangles ());
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (5, 5, 8, 8), rectangles ()));
  rectangles t= translate (l, 1, -1);
  CHECK (t == rectangles (rectangle (1, -1, 3, 1),
                          rectangles (rectangle (6, 4, 9, 7), rectangles ())));
  CHECK (l == rectangles (rectangle (0, 0, 2, 2),
                          rectangles (rectangle (5, 5, 8, 8), rectangles ())));
}

TEST_CASE ("test thicken") {
  rectangle r (0, 0, 10, 10);
  CHECK (thicken (r, 2, 3) == rectangle (-2, -3, 12, 13));
  CHECK (thicken (r, 0, 0) == r);
}

TEST_CASE ("test thicken rectangles") {
  CHECK (thicken (rectangles (), 2, 3) == rectangles ());
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (5, 5, 8, 8), rectangles ()));
  rectangles t= thicken (l, 1, 2);
  CHECK (t == rectangles (rectangle (-1, -2, 3, 4),
                          rectangles (rectangle (4, 3, 9, 10), rectangles ())));
  CHECK (l == rectangles (rectangle (0, 0, 2, 2),
                          rectangles (rectangle (5, 5, 8, 8), rectangles ())));
}

TEST_CASE ("test scaling") {
  rectangle r (1, 1, 4, 4);
  CHECK ((r * 3) == rectangle (3, 3, 12, 12));
  CHECK ((r / 2) == rectangle (0, 0, 2, 2));
}

TEST_CASE ("test lub two") {
  rectangle r1 (0, 0, 5, 5);
  rectangle r2 (3, 3, 10, 10);
  CHECK (least_upper_bound (r1, r2) == rectangle (0, 0, 10, 10));
}

TEST_CASE ("test lub list") {
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (5, 5, 8, 8), rectangles ()));
  CHECK (least_upper_bound (l) == rectangle (0, 0, 8, 8));
}

TEST_CASE ("test area list") {
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (0, 0, 3, 3), rectangles ()));
  CHECK_EQ (area (l), 13.0);
}

TEST_CASE ("test union and difference") {
  rectangle  r1 (0, 0, 4, 4);
  rectangle  r2 (2, 0, 6, 4);
  rectangles l1= rectangles (r1, rectangles ());
  rectangles l2= rectangles (r2, rectangles ());
  rectangles u = l1 | l2;
  CHECK (least_upper_bound (u) == rectangle (0, 0, 6, 4));
  rectangles d= l1 - l2;
  CHECK_EQ (area (d), 8.0);
}

TEST_CASE ("test difference full coverage") {
  // 被完全覆盖的矩形被整块丢弃,结果为空
  rectangles l1= rectangles (rectangle (0, 0, 10, 10), rectangles ());
  rectangles l2= rectangles (rectangle (-1, -1, 11, 11), rectangles ());
  rectangles d = l1 - l2;
  CHECK (is_nil (d));
}

TEST_CASE ("test difference disjoint keeps order") {
  // 与被减列表全不交的元素原样保留且保序
  rectangles l1=
      rectangles (rectangle (0, 0, 2, 2),
                  rectangles (rectangle (5, 5, 8, 8), rectangles ()));
  rectangles l2= rectangles (rectangle (20, 20, 30, 30), rectangles ());
  CHECK ((l1 - l2) == l1);
}

TEST_CASE ("test difference splits pieces") {
  // 中间被挖掉一条,切成上下两块,面积守恒
  rectangles l1= rectangles (rectangle (0, 0, 10, 10), rectangles ());
  rectangles l2= rectangles (rectangle (0, 4, 10, 6), rectangles ());
  rectangles d = l1 - l2;
  CHECK_EQ (N (d), 2);
  CHECK_EQ (area (d), 80.0);
  CHECK (least_upper_bound (d) == rectangle (0, 0, 10, 10));
}

TEST_CASE ("test difference multiple subtrahends") {
  // 依次被两个减数切割:先挖右侧,再从剩余碎片中挖一块
  rectangles l1= rectangles (rectangle (0, 0, 10, 10), rectangles ());
  rectangles l2=
      rectangles (rectangle (6, 0, 12, 10),
                  rectangles (rectangle (0, 0, 2, 4), rectangles ()));
  rectangles d= l1 - l2;
  CHECK_EQ (area (d), 60.0 - 8.0);
}

// 逐对校验碎片列表互不相交(差集结果的基本不变量)
static bool
pairwise_disjoint (rectangles l) {
  for (rectangles p= l; !is_nil (p); p= p->next)
    for (rectangles q= p->next; !is_nil (q); q= q->next)
      if (intersect (p->item, q->item)) return false;
  return true;
}

TEST_CASE ("test difference cross cuts") {
  // 十字切缝:先竖一刀再横一刀,切成 4 块,面积守恒且互不相交
  // (挖去 20+20-4,两条切缝的 2x2 交集只计一次)
  rectangles l1= rectangles (rectangle (0, 0, 10, 10), rectangles ());
  rectangles l2=
      rectangles (rectangle (4, 0, 6, 10),
                  rectangles (rectangle (0, 4, 10, 6), rectangles ()));
  rectangles d= l1 - l2;
  CHECK (pairwise_disjoint (d));
  CHECK_EQ (area (d), 64.0);
  CHECK (least_upper_bound (d) == rectangle (0, 0, 10, 10));
}

TEST_CASE ("test difference scattered cuts") {
  // 大矩形内散布多个减数,每个减数只切到部分碎片;
  // 结果互不相交且面积守恒(总面积减去被挖面积)
  rectangles l1= rectangles (rectangle (0, 0, 30, 30), rectangles ());
  rectangles l2;
  for (int i= 4; i >= 0; i--)
    l2= rectangles (rectangle (i * 6, i * 6, i * 6 + 4, i * 6 + 4), l2);
  rectangles d= l1 - l2;
  CHECK (pairwise_disjoint (d));
  CHECK_EQ (area (d), 900.0 - 5 * 16.0);
}

TEST_CASE ("test difference partial hit keeps untouched fragment") {
  // 第二个减数只切到部分碎片:未命中碎片原样保留(元素级相等)
  rectangles l1= rectangles (rectangle (0, 0, 10, 10), rectangles ());
  rectangles l2= rectangles (
      rectangle (4, 0, 6, 10), // 竖切:左 4x10 + 右 4x10
      rectangles (rectangle (0, 0, 2, 4), rectangles ())); // 再挖左块一角
  rectangles d= l1 - l2;
  CHECK (pairwise_disjoint (d));
  CHECK_EQ (area (d), 100.0 - 20.0 - 8.0);
  bool has_right= false;
  for (rectangles p= d; !is_nil (p); p= p->next)
    if (p->item == rectangle (6, 0, 10, 10)) has_right= true;
  CHECK (has_right);
}

TEST_CASE ("test difference mixed elements") {
  // 多元素 l1:一个不交、一个被完全覆盖、一个被切,均独立处理
  rectangles l1= rectangles (
      rectangle (0, 0, 2, 2),
      rectangles (rectangle (5, 5, 8, 8),
                  rectangles (rectangle (20, 20, 30, 30), rectangles ())));
  rectangles l2=
      rectangles (rectangle (4, 4, 9, 9),
                  rectangles (rectangle (22, 22, 24, 24), rectangles ()));
  rectangles d= l1 - l2;
  CHECK_EQ (area (d), 4.0 + (100.0 - 4.0));
  CHECK (pairwise_disjoint (d));
  CHECK (d->item == rectangle (0, 0, 2, 2)); // 不交元素保序直挂
}

TEST_CASE ("test correct drops degenerate") {
  CHECK (correct (rectangles ()) == rectangles ());
  // 零宽/零高/负宽高的矩形均被剔除，其余顺序保持
  rectangles l=
      rectangles (rectangle (0, 0, 2, 2),
                  rectangles (rectangle (5, 5, 5, 9),
                              rectangles (rectangle (0, 0, 2, 2),
                                          rectangles (rectangle (7, 2, 4, 8),
                                                      rectangles ()))));
  rectangles c= correct (l);
  CHECK (c == rectangles (rectangle (0, 0, 2, 2),
                          rectangles (rectangle (0, 0, 2, 2), rectangles ())));
}

TEST_CASE ("test simplify merges adjacent") {
  // 左右相邻的两个矩形合并为一个
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (2, 0, 4, 2), rectangles ()));
  rectangles s= simplify (l);
  CHECK_EQ (N (s), 1);
  CHECK (least_upper_bound (s) == rectangle (0, 0, 4, 2));
  // 不相邻的矩形保持不变
  rectangles d= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (3, 0, 5, 2), rectangles ()));
  CHECK_EQ (N (simplify (d)), 2);
}

TEST_CASE ("test simplify long list returns copy") {
  // 超过 25 个元素时直接返回副本，不做合并
  rectangles l= rectangles ();
  for (int i= 0; i < 26; i++)
    l= rectangles (rectangle (i, i, i + 1, i + 1), l);
  rectangles s= simplify (l);
  CHECK_EQ (N (s), 26);
  CHECK (s == l);
}

TEST_CASE ("test union merges adjacent in place") {
  // l2 元素与累积列表相邻时被吸收合并,不产生重复
  rectangles l1= rectangles (rectangle (0, 0, 2, 2), rectangles ());
  rectangles l2= rectangles (rectangle (2, 0, 4, 2), rectangles ());
  rectangles u = l1 | l2;
  CHECK_EQ (N (u), 1);
  CHECK (u->item == rectangle (0, 0, 4, 2));
}

TEST_CASE ("test union appends disjoint") {
  // 与累积列表全不相邻的元素追加到尾部,原有元素原样保留
  rectangles l1=
      rectangles (rectangle (0, 0, 2, 2),
                  rectangles (rectangle (5, 5, 8, 8), rectangles ()));
  rectangles l2= rectangles (rectangle (20, 20, 30, 30), rectangles ());
  rectangles u = l1 | l2;
  CHECK_EQ (N (u), 3);
  CHECK (u->item == rectangle (0, 0, 2, 2));
  CHECK ((u->next)->item == rectangle (5, 5, 8, 8));
  CHECK ((u->next->next)->item == rectangle (20, 20, 30, 30));
}

TEST_CASE ("test union absorbs overlapping l2") {
  // l2 与 l1 重叠:先做 l1 - l2 去重叠,再并入 l2,面积等于并集面积
  rectangles l1= rectangles (rectangle (0, 0, 4, 4), rectangles ());
  rectangles l2= rectangles (rectangle (2, 0, 6, 4), rectangles ());
  rectangles u = l1 | l2;
  CHECK_EQ (N (u), 1);
  CHECK (u->item == rectangle (0, 0, 6, 4));
}

TEST_CASE ("test union chain merges multiple") {
  // 链式相邻:l2 一次并入时把累积列表中多块相邻碎片逐个吸收
  rectangles l1= rectangles (
      rectangle (0, 0, 2, 2),
      rectangles (rectangle (2, 0, 4, 2),
                  rectangles (rectangle (10, 0, 12, 2), rectangles ())));
  rectangles l2= rectangles (rectangle (4, 0, 10, 2), rectangles ());
  rectangles u = l1 | l2;
  CHECK_EQ (N (u), 2);
  CHECK (u->item == rectangle (0, 0, 2, 2));
  CHECK ((u->next)->item == rectangle (2, 0, 12, 2));
}

TEST_CASE ("test union area conservation") {
  // 不相交并集面积守恒
  rectangles l1= rectangles (rectangle (0, 0, 10, 10), rectangles ());
  rectangles l2= rectangles (rectangle (0, 20, 10, 30), rectangles ());
  CHECK_EQ (area (l1 | l2), 200.0);
}
