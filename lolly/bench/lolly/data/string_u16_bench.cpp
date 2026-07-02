/** \file lolly_string_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for string
 *  \author jingkaimori
 *  \date   2024
 */

#include "lolly/data/string_u16.hpp"
#include <nanobench.h>

using lolly::data::string_u16;

static ankerl::nanobench::Bench bench;

namespace lolly {
extern void init_tbox ();
} // namespace lolly

int
main () {
  lolly::init_tbox ();
#ifdef OS_WASM
  bench.minEpochIterations (2000);
#else
  bench.minEpochIterations (200000);
#endif
  bench.run ("construct string_u16", [&] { string_u16 (); });
  bench.run ("equality of string", [&] {
    static string_u16 a (u"abc"), b;
    a == b;
  });
  bench.run ("equality of larger string", [&] {
    static string_u16 a (u"equality of larger string"),
        b (u"equality of larger string");
    a == b;
  });
  bench.run ("compare string", [&] {
    static string_u16 a (u"ab"), b (u"b");
    a <= b;
  });
  bench.run ("compare larger string", [&] {
    static string_u16 a (u"compare larger string"),
        b (u"compare LARGEr string");
    a <= b;
  });
  bench.run ("slice string", [&] {
    static string_u16 a (u"abcdefgh");
    a (2, 3);
  });
  bench.run ("slice string with larger range", [&] {
    static string_u16 a (u"abcdefgh");
    a (1, 6);
  });
  bench.run ("concat string", [&] {
    static string_u16 a (u"abc"), b (u"de");
    a*                b;
  });
  bench.run ("append string", [&] {
    static string_u16 a (u"abc"), b (u"de");
    a << b;
  });
}
