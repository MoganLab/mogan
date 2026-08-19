/** \file colors_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for named color resolution
 *  \date   2026
 */

#include "nanobench.h"

#include <moebius/data/colors.hpp>

using namespace moebius::data;

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (1000).unit ("x100");

  // 常用命名颜色轮询:渲染期重复解析同一批颜色
  const char* names[]= {"red",   "blue", "green",  "black",
                        "white", "gray", "orange", "pastel"};
  color       acc    = 0;
  bench.run ("named_color x100", [&] {
    for (int i= 0; i < 100; i++)
      acc+= named_color (names[i % 8]);
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("named_color_to_xcolormap x100", [&] {
    int n= 0;
    for (int i= 0; i < 100; i++)
      n+= N (named_color_to_xcolormap (names[i % 8]));
    ankerl::nanobench::doNotOptimizeAway (n);
  });
  return 0;
}
