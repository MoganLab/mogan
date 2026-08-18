/** \file rectangles_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for rectangle/rectangles translate operations
 *  \date   2026
 */

#include "nanobench.h"
#include "rectangles.hpp"

static rectangles
mk_rects (int n) {
  rectangles l= rectangles ();
  for (int i= 0; i < n; i++)
    l= rectangles (rectangle (i, i, i + 10, i + 10), l);
  return l;
}

// 优化前的递归按值实现,用于同二进制 A/B 对比
static rectangles
old_translate (rectangles l, SI x, SI y) {
  if (is_nil (l)) return l;
  rectangle& r= l->item;
  return rectangles (rectangle (r->x1 + x, r->y1 + y, r->x2 + x, r->y2 + y),
                     old_translate (l->next, x, y));
}

// 优化前的递归按值实现,用于同二进制 A/B 对比
static rectangles
old_thicken (rectangles l, SI width, SI height) {
  if (is_nil (l)) return l;
  rectangle& r= l->item;
  return rectangles (
      rectangle (r->x1 - width, r->y1 - height, r->x2 + width, r->y2 + height),
      old_thicken (l->next, width, height));
}

// 优化前的递归按值实现,用于同二进制 A/B 对比
static rectangles
old_correct (rectangles l) {
  if (is_nil (l)) return l;
  if ((l->item->x1 >= l->item->x2) || (l->item->y1 >= l->item->y2))
    return old_correct (l->next);
  return rectangles (l->item, old_correct (l->next));
}

// 生产代码中的单矩形差分辅助(外部链接,rectangles.cpp 未导出到 hpp)
void complement (rectangle r1, rectangle r2, rectangles& l);

// 优化前的按值实现,用于同二进制 A/B 对比
static rectangles
old_subtract (rectangles l1, rectangles l2) {
  rectangles a= l1;
  for (; !is_nil (l2); l2= l2->next) {
    rectangles b;
    for (rectangles p= a; !is_nil (p); p= p->next)
      complement (p->item, l2->item, b);
    a= b;
  }
  return a;
}

// 第八轮实现:单矩形独立求差,但被切过后每个相交 q 仍整表重建碎片表,
// 用于同二进制 A/B 对比
static void
r8_append_difference (rectangle r, rectangles l2, rectangles*& tail) {
  rectangles cur;
  bool       cut= false;
  for (rectangles q= l2; !is_nil (q); q= q->next) {
    if (!intersect (r, q->item)) continue;
    if (!cut) {
      complement (r, q->item, cur);
      cut= true;
      continue;
    }
    rectangles next;
    for (rectangles p= cur; !is_nil (p); p= p->next)
      complement (p->item, q->item, next);
    cur= next;
  }
  if (!cut) rectangles::append (tail, r);
  else
    for (rectangles p= cur; !is_nil (p); p= p->next)
      rectangles::append (tail, p->item);
}

static rectangles
r8_subtract (rectangles l1, rectangles l2) {
  rectangles  out;
  rectangles* tail= &out;
  for (; !is_nil (l1); l1= l1->next)
    r8_append_difference (l1->item, l2, tail);
  return out;
}

// 优化前的实现:每个 l2 元素经 disjoint_union 克隆整个前缀,用于同二进制 A/B 对比
static rectangles
old_union (rectangles l1, rectangles l2) {
  rectangles l (l1 - l2);
  while (!is_nil (l2)) {
    l = disjoint_union (l, l2->item);
    l2= l2->next;
  }
  return l;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (1000).unit ("op");

  rectangle  r (0, 0, 10, 10);
  rectangles large= mk_rects (1024);

  // l2 取 large 的稀疏子集,每个被减矩形切到约 11 个相邻元素
  rectangles sparse;
  for (int i= 1024 - 64; i >= 0; i-= 64)
    sparse= rectangles (rectangle (i, i, i + 10, i + 10), sparse);
  // 与 large 完全不相交的减数列表(全走直挂快路径)
  rectangles faraway;
  for (int i= 0; i < 16; i++)
    faraway= rectangles (rectangle (100000 + i, 100000, 100010 + i, 100010),
                         faraway);

  bench.run ("translate rectangle", [&] {
    ankerl::nanobench::doNotOptimizeAway (translate (r, 5, 7));
  });
  bench.run ("rev+reverse translate x1024", [&] {
    rectangles rev;
    for (rectangles p= large; !is_nil (p); p= p->next) {
      rectangle& r= p->item;
      rev= rectangles (rectangle (r->x1 + 5, r->y1 + 7, r->x2 + 5, r->y2 + 7),
                       rev);
    }
    ankerl::nanobench::doNotOptimizeAway (reverse (rev));
  });
  bench.run ("old translate rectangles x1024", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_translate (large, 5, 7));
  });
  bench.run ("translate rectangles x1024", [&] {
    ankerl::nanobench::doNotOptimizeAway (translate (large, 5, 7));
  });
  bench.run ("thicken rectangle",
             [&] { ankerl::nanobench::doNotOptimizeAway (thicken (r, 5, 7)); });
  bench.run ("old thicken rectangles x1024", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_thicken (large, 5, 7));
  });
  bench.run ("thicken rectangles x1024", [&] {
    ankerl::nanobench::doNotOptimizeAway (thicken (large, 5, 7));
  });
  bench.run ("old correct x1024", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_correct (large));
  });
  bench.run ("correct x1024",
             [&] { ankerl::nanobench::doNotOptimizeAway (correct (large)); });
  bench.run ("old subtract x1024 sparse16", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_subtract (large, sparse));
  });
  bench.run ("subtract x1024 sparse16",
             [&] { ankerl::nanobench::doNotOptimizeAway (large - sparse); });
  bench.run ("old subtract x1024 disjoint16", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_subtract (large, faraway));
  });
  bench.run ("subtract x1024 disjoint16",
             [&] { ankerl::nanobench::doNotOptimizeAway (large - faraway); });
  // 碎片化场景:单个大矩形被 64 个散布小矩形反复切割,碎片表逐轮膨胀,
  // 第八轮实现每个相交减数都整表重建碎片
  rectangles big= rectangles (rectangle (0, 0, 1024, 1024), rectangles ());
  rectangles holes;
  for (int i= 63; i >= 0; i--) {
    int x= (i % 8) * 128 + 32, y= (i / 8) * 128 + 32;
    holes= rectangles (rectangle (x, y, x + 64, y + 64), holes);
  }
  bench.run ("r8 subtract fragmentation64", [&] {
    ankerl::nanobench::doNotOptimizeAway (r8_subtract (big, holes));
  });
  bench.run ("subtract fragmentation64",
             [&] { ankerl::nanobench::doNotOptimizeAway (big - holes); });
  // 并集场景:模拟失效区域逐矩形累积,16 块均不与 large 相邻(全走尾插)
  bench.run ("old union x1024 append16", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_union (large, faraway));
  });
  bench.run ("union x1024 append16",
             [&] { ankerl::nanobench::doNotOptimizeAway (large | faraway); });
  // 并集场景:与 large 部分元素相邻,触发合并路径
  rectangles touchy;
  for (int i= 1024 - 64; i >= 0; i-= 64)
    touchy= rectangles (rectangle (i + 10, i, i + 12, i + 10), touchy);
  bench.run ("old union x1024 adjacent16", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_union (large, touchy));
  });
  bench.run ("union x1024 adjacent16",
             [&] { ankerl::nanobench::doNotOptimizeAway (large | touchy); });
  return 0;
}
