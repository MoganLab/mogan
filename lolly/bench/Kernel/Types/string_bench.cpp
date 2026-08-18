/** \file string_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for string
 *  \author jingkaimori
 *  \date   2024
 */

#include "string.hpp"
#include "sys_utils.hpp"
#include <nanobench.h>

static ankerl::nanobench::Bench bench;

int
main () {
  lolly::init_tbox ();
  bench.run ("construct string", [&] {
    string ("abc");
    string ();
  });
  bench.run ("equality of string", [&] {
    static string a ("abc"), b;
    a == b;
  });
  bench.run ("equality of larger string", [&] {
    static string a ("equality of larger string"),
        b ("equality of larger strinG");
    a == b;
  });
  bench.run ("compare string", [&] {
    static string a ("ab"), b ("b");
    a <= b;
  });
  bench.run ("compare larger string", [&] {
    static string a ("compare larger string"), b ("compare LARGER string");
    a <= b;
  });
  bench.run ("slice string", [&] {
    static string a ("abcdefgh");
    a (2, 3);
  });
  bench.run ("slice string with larger range", [&] {
    static string a ("abcdefgh");
    a (1, 6);
  });
  bench.minEpochIterations (40000);
  bench.run ("concat string", [&] {
    static string a ("abc"), b ("de");
    a*            b;
  });
  bench.run ("append string", [&] {
    static string a ("abc"), b ("de");
    a << b;
  });
  bench.run ("hash of string", [&] {
    static string a ("accde");
    hash (a);
  });
  bench.run ("hash of larger string", [&] {
    static string a ("compare larger string ,compute hash of LARGER string");
    hash (a);
  });
  bench.run ("is quoted", [&] {
    static string a ("H\"ello TeXmacs\"");
    is_quoted (a);
  });
  bench.run ("construct from long char*", [&] {
    const char* s= "construct from long char*: the quick brown fox jumps over "
                   "the lazy dog";
    string      a (s);
    ankerl::nanobench::doNotOptimizeAway (a);
  });
  bench.run ("copy", [&] {
    static string a ("copy this string, copy this string, copy this string");
    auto          c= copy (a);
    ankerl::nanobench::doNotOptimizeAway (c);
  });
  bench.run ("append char", [&] {
    static string a ("abcdefg");
    a << 'x';
    a->resize (7);
  });
  // 填充移到计时区外,避免每轮迭代重复 1KB 写入
  static string a_1k (1024);
  for (int i= 0; i < N (a_1k); i++)
    a_1k[i]= (char) ('a' + (i % 26));
  bench.run ("slice 1KB string", [&] {
    auto s= a_1k (100, 900);
    ankerl::nanobench::doNotOptimizeAway (s);
  });
  bench.run ("concat 1KB strings", [&] {
    static string a (1024, 'a'), b (1024, 'b');
    auto          c= a * b;
    ankerl::nanobench::doNotOptimizeAway (c);
  });
  bench.run ("equality with char*", [&] {
    static string a ("equality with char* string");
    (void) (a == "equality with char* strinG");
  });
  bench.run ("hash 1KB string", [&] {
    static string a (1024, 'a');
    auto          h= hash (a);
    ankerl::nanobench::doNotOptimizeAway (h);
  });
}
