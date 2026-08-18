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

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (1000).unit ("op");

  rectangle  r (0, 0, 10, 10);
  rectangles large= mk_rects (1024);

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
  return 0;
}
