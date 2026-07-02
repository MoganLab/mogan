/** \file numeral_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for converter between numeral and string
 *  \author jingkaimori
 *  \date   2024
 */

#include "file.hpp"
#include "lolly/data/numeral.hpp"
#include <nanobench.h>

using namespace lolly::data;

namespace lolly {
extern void init_tbox ();
} // namespace lolly

int
main () {
  lolly::init_tbox ();
  ankerl::nanobench::Bench bench;
#ifdef OS_WASM
  bench.minEpochIterations (2000);
#else
  bench.minEpochIterations (400000);
#endif

  string hex_string;
  for (int d= 1; d < 6; d++) {
    hex_string << ('a' + d % 6);
    bench.complexityN (d);
    bench.run ("convert hexadecimal string to int",
               [&] { from_hex (hex_string); });
    int hex_number= (0x1 << ((d + 1) * 4)) - 1;
    bench.run ("convert signed int to hexadecimal string",
               [&] { to_Hex (hex_number); });
    bench.run ("convert unsigned int to hexadecimal string",
               [&] { as_hexadecimal (hex_number, d); });
  }
  string                   content= string_load (url_pwd () * "LICENSE");
  ankerl::nanobench::Bench bench2;
  bench2.relative (true)
      .minEpochTime (std::chrono::milliseconds (10))
      .run ("convert document to hexadecimal string, legacy",
            [&] {
              string result;
              for (int i= 0; i < N (content); i++)
                result << as_hexadecimal ((unsigned char) content[i], 2);
            })
      .run (
          "convert document to hexadecimal string, using binary_to_hexadecimal",
          [&] { string result= binary_to_hexadecimal (content); });
}
