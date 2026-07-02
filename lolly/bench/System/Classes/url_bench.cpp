/** \file url_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for url
 *  \author Darcy Shen
 *  \date   2024
 */

#include "sys_utils.hpp"
#include "url.hpp"
#include <nanobench.h>

static ankerl::nanobench::Bench bench;

url
make_deep_url (int depth) {
  std::string s;
  s.reserve (depth * 2);
  for (int i= 0; i <= depth; ++i) {
    if (i) s.push_back ('/');
    s.push_back ('x');
  }
  return url (s.c_str ());
}

int
main () {
  lolly::init_tbox ();
  bench.run ("url construct 8", [&] { url ("a/b/c/d/e/f/g/h"); });
  bench.run ("url descends equal",
             [&] { descends (url ("a/b/c"), url ("a/b/c")); });
  bench.run ("url descends concat 2",
             [&] { descends (url ("a/b/c/d/e/f"), url ("a/b")); });
  bench.run ("url descends concat 8", [&] {
    descends (url ("a/b/c/d/e/f/g/h/i/j"), url ("a/b/c/d/e/f/g/h"));
  });

  bench.run ("as_string deep url (depth 1000)", [&] {
    url    u= make_deep_url (1000);
    string s= as_string (u, URL_STANDARD);
    ankerl::nanobench::doNotOptimizeAway (s);
  });
  bench.run ("as_string deep url (depth 200)", [&] {
    url    u= make_deep_url (200);
    string s= as_string (u, URL_STANDARD);
    ankerl::nanobench::doNotOptimizeAway (s);
  });

  return 0;
}
