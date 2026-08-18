/** \file analyze_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for string algorithms
 *  \author jingkaimori
 *  \date   2024
 */

#include "analyze.hpp"
#include "sys_utils.hpp"
#include <nanobench.h>

void
bench_string_join (string base_name, array<string> str) {
  ankerl::nanobench::Bench bench;
  bench.relative (true).minEpochIterations (1000).run (
      c_string (base_name * " by append"), [&] {
        string res;
        int    lth= N (str);
        for (size_t i= 0; i < lth; i++) {
          res << str[i];
        }
      });
  bench.run (c_string (base_name * " by concat"), [&] {
    string res;
    int    lth= N (str);
    for (size_t i= 0; i < lth; i++) {
      res= str[i] * res;
    }
  });
  bench.run (c_string (base_name * " by recompose"),
             [&] { recompose (str, ""); });
}

/**
 * @brief 对 replace 做微基准：覆盖无命中、多命中、首字符干扰三种典型负载
 */
void
bench_string_replace (string base_name, string s, string what, string by) {
  ankerl::nanobench::Bench ().title ("replace").relative (true).run (
      c_string (base_name), [&] {
        auto r= replace (s, what, by);
        ankerl::nanobench::doNotOptimizeAway (r);
      });
}

/**
 * @brief 对 tokenize 做微基准：覆盖多命中、无命中、首字符干扰三种典型负载
 */
void
bench_string_tokenize (string base_name, string s, string sep) {
  ankerl::nanobench::Bench ()
      .title ("tokenize")
      .relative (true)
      .run (c_string (base_name), [&] {
        auto r= tokenize (s, sep);
        ankerl::nanobench::doNotOptimizeAway (r);
      });
}

/**
 * @brief 对 search_forwards
 * 做微基准：覆盖多命中、无命中、首字符干扰三种典型负载
 */
void
bench_string_search (string base_name, string s, string what) {
  ankerl::nanobench::Bench ()
      .title ("search_forwards")
      .relative (true)
      .run (c_string (base_name), [&] {
        auto r= search_forwards (what, s);
        ankerl::nanobench::doNotOptimizeAway (r);
      });
}

int
main () {
  lolly::init_tbox ();
  bench_string_join ("join regular string",
                     array<string> ("long string", "short", "", "<#ABCD>"));
  bench_string_join ("join two string", array<string> ("long string", "short"));
  bench_string_join ("join empty ones", array<string> (5));

  ankerl::nanobench::Rng rng;
  int                    total= 20;
  array<string>          test_case;
  for (int i= 0; i < total; i++) {
    test_case << string ('!' + i, rng.bounded (30));
  }
  bench_string_join ("join a bunch of random string", test_case);

  string line= "the quick brown fox jumps over the lazy dog\n";
  string text;
  for (int i= 0; i < 2000; i++)
    text << line;
  // 多命中：每行一次替换
  bench_string_replace ("replace hit each line", text, "quick", "brisk");
  // 无命中：纯扫描开销
  bench_string_replace ("replace no hit", text, "QUICK", "BRISK");
  // 首字符干扰：模式首字符大量出现但整体不匹配，考验快速筛选
  bench_string_replace ("replace first-char noise", text, "totally", "missed");

  // 多命中：每行拆出多个 token
  bench_string_tokenize ("tokenize hit each line", text, " ");
  // 无命中：纯扫描开销
  bench_string_tokenize ("tokenize no hit", text, "::;");
  // 首字符干扰：分隔符首字符大量出现但整体不匹配，考验快速筛选
  bench_string_tokenize ("tokenize first-char noise", text, "the!");

  // 多命中：每行两次命中
  bench_string_search ("search hit each line", text, "the");
  // 无命中：纯扫描开销
  bench_string_search ("search no hit", text, "QUICK");
  // 首字符干扰：模式首字符大量出现但整体不匹配，考验快速筛选
  bench_string_search ("search first-char noise", text, "totally");

  return 0;
}