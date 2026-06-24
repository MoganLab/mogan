/** \file fast_alloc_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for memory allocator
 *  \author jingkaimori
 *  \date   2024
 */

#include "fast_alloc.hpp"
#include "sys_utils.hpp"
#include <nanobench.h>
static ankerl::nanobench::Bench bench;

struct Complex {
public:
  double re, im;
  Complex (double re_, double im_) : re (re_), im (im_) {}
  Complex () : re (0.5), im (1.0) {}
  ~Complex () {}
};

int
main () {
  lolly::init_tbox ();

#ifdef OS_WASM
  constexpr int NUM= 100;
#else
  constexpr int NUM= 10000;
#endif

  char*   ch[NUM];
  int*    in[NUM];
  long*   lo[NUM];
  double* dou[NUM];

  bench.batch (NUM * 4)
      .unit ("alloc & free")
      .run ("basic type, fast collect", [&] {
        for (int i= 0; i < NUM; i++) { // for gprof
          ch[i]= tm_new<char> ();
          tm_delete (ch[i]);

          in[i]= tm_new<int> ();
          tm_delete (in[i]);

          lo[i]= tm_new<long> ();
          tm_delete (lo[i]);

          dou[i]= tm_new<double> ();
          tm_delete (dou[i]);
        }
      });

  bench.run ("basic type, bulk collect", [&] {
    for (int i= 0; i < NUM; i++) {
      ch[i] = tm_new<char> ();
      in[i] = tm_new<int> ();
      lo[i] = tm_new<long> ();
      dou[i]= tm_new<double> ();
    }

    for (int i= 0; i < NUM; i++) {
      tm_delete (ch[i]);
      tm_delete (in[i]);
      tm_delete (lo[i]);
      tm_delete (dou[i]);
    }
  });
  bench.batch (NUM).run ("char, bulk collect", [&] {
    for (int i= 0; i < NUM; i++) {
      ch[i]= tm_new<char> ();
    }

    for (int i= 0; i < NUM; i++) {
      tm_delete (ch[i]);
    }
  });
  bench.run ("int, bulk collect", [&] {
    for (int i= 0; i < NUM; i++) {
      in[i]= tm_new<int> ();
    }

    for (int i= 0; i < NUM; i++) {
      tm_delete (in[i]);
    }
  });
  bench.run ("long, bulk collect", [&] {
    for (int i= 0; i < NUM; i++) {
      lo[i]= tm_new<long> ();
    }

    for (int i= 0; i < NUM; i++) {
      tm_delete (lo[i]);
    }
  });
  bench.run ("double, bulk collect", [&] {
    for (int i= 0; i < NUM; i++) {
      dou[i]= tm_new<double> ();
    }

    for (int i= 0; i < NUM; i++) {
      tm_delete (dou[i]);
    }
  });
  bench.batch (1).run ("struct", [&] {
    Complex* p_complex= tm_new<Complex> (35.8, 26.2);
    tm_delete (p_complex);
  });
  uint8_t* p_complex= tm_new_array<uint8_t> (100);
  tm_delete_array (p_complex);
#ifdef OS_WASM
  const size_t size_prim= 200, size_complex= 100;
#else
  const size_t size_prim= 20000000, size_complex= 5000000;
#endif
  bench.batch (size_prim).run ("large array of basic type", [&] {
    p_complex= tm_new_array<uint8_t> (size_prim);
    tm_delete_array (p_complex);
  });
  bench.batch (size_complex).run ("large array of complex structure", [&] {
    Complex* p_wide= tm_new_array<Complex> (size_complex);
    tm_delete_array (p_wide);
  });

  Complex* volume[NUM];
  bench.batch (NUM).run ("frequent allocation of array, space reused", [&] {
    for (int i= 0; i < NUM; i++) {
      volume[i]= tm_new_array<Complex> (9);
    }
    for (int i= 0; i < NUM; i++) {
      tm_delete_array (volume[i]);
    }
  });
  return 0;
}
