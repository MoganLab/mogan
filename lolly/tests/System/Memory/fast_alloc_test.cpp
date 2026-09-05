#include "a_lolly_test.hpp"
#include "fast_alloc.hpp"
#include "tm_timer.hpp"

struct Complex {
public:
  double re, im;
  Complex (double re_, double im_) : re (re_), im (im_) {}
  Complex () : re (0.5), im (1.0) {}
  ~Complex () {}
};

TEST_CASE ("test for memory leaks") {
  char* p_char= tm_new<char> ('a');
  CHECK (*p_char == 'a');

  p_char= tm_new<char> ('b');
  CHECK (*p_char == 'b');

  *p_char= 'c';
  CHECK (*p_char == 'c');

  tm_delete (p_char);

  char* q_char= tm_new<char> ('z'); // here p_char is modified to 'z'

  // *p_char= 'c'; // behavior of this code is unspecified, DO NOT DO THIS!
}

TEST_MEMORY_LEAK_INIT

TEST_CASE ("test basic data types") {

  char*   ch;
  int*    in;
  long*   lo;
  double* dou;

  ch = tm_new<char> ();
  in = tm_new<int> ();
  lo = tm_new<long> ();
  dou= tm_new<double> ();

  tm_delete (ch);
  tm_delete (in);
  tm_delete (lo);
  tm_delete (dou);
}

TEST_CASE ("test class") {
  Complex* p_complex= tm_new<Complex> (35.8, 26.2);
  tm_delete (p_complex);
}

TEST_CASE ("test tm_*_array") {
  uint8_t* p_complex= tm_new_array<uint8_t> (100);
  tm_delete_array (p_complex);
#ifdef OS_WASM
  const size_t size_prim= 200, size_complex= 100;
#else
  const size_t size_prim= 20000000, size_complex= 5000000;
#endif
  p_complex= tm_new_array<uint8_t> (size_prim);
  tm_delete_array (p_complex);
  Complex* p_wide= tm_new_array<Complex> (size_complex);
  tm_delete_array (p_wide);
}

#ifndef OS_WASM
TEST_CASE ("test large bunch of tm_*") {
  const int bnum= 100000;
  int*      volume[bnum];
  for (int i= 0; i < bnum; i++) {
    volume[i]= tm_new<int> (35);
  }
  for (int i= 0; i < bnum; i++) {
    tm_delete (volume[i]);
  }
}
TEST_MEMORY_LEAK_ALL
TEST_MEMORY_LEAK_RESET
#endif

TEST_CASE ("test large bunch of tm_*_array with class") {

#ifdef OS_WASM
#define NUM 100
#else
#define NUM 10000
#endif
  Complex* volume[NUM];
  for (int i= 0; i < NUM; i++) {
    volume[i]= tm_new_array<Complex> (9);
  }
  for (int i= 0; i < NUM; i++) {
    tm_delete_array (volume[i]);
  }
}

namespace {
// 存活计数类型：构造 +1、析构 -1，用于断言槽位析构是否发生
struct CountedSlot {
  static int live;
  int        dummy;
  CountedSlot () : dummy (0) { live++; }
  CountedSlot (const CountedSlot& other) : dummy (other.dummy) { live++; }
  ~CountedSlot () { live--; }
};
int CountedSlot::live= 0;
} // namespace

TEST_CASE ("tm_resize_array: grow constructs new slots") {
  CountedSlot::live= 0;
  CountedSlot* a   = tm_new_array<CountedSlot> (4);
  CHECK_EQ (CountedSlot::live, 4);
  a= tm_resize_array<CountedSlot> (8, a);
  CHECK_EQ (CountedSlot::live, 8);
  tm_delete_array (a);
  CHECK_EQ (CountedSlot::live, 0);
}

TEST_CASE ("tm_resize_array: shrink destructs truncated slots") {
  CountedSlot::live= 0;
  CountedSlot* a   = tm_new_array<CountedSlot> (8);
  CHECK_EQ (CountedSlot::live, 8);
  a= tm_resize_array<CountedSlot> (3, a);
  // [3, 8) 被截断的槽位必须析构，否则引用计数句柄在此泄漏
  CHECK_EQ (CountedSlot::live, 3);
  tm_delete_array (a);
  CHECK_EQ (CountedSlot::live, 0);
}

TEST_MEMORY_LEAK_ALL
