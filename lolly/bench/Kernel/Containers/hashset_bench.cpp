#include "hashset.hpp"
#include <iostream>
#include <nanobench.h>
#include <vector>

using namespace lolly;

int
main () {
  ankerl::nanobench::Bench bench;
  const int                N= 200000;

  std::vector<int> keys;
  keys.reserve (N);
  for (int i= 0; i < N; ++i)
    keys.push_back (i);

  bench.run ("insert()", [&] {
    hashset<int> h;
    for (auto k : keys)
      h->insert (k);
  });

  hashset<int> hs;
  for (auto k : keys)
    hs->insert (k);

  bench.run ("contains() hits", [&] {
    for (auto k : keys) {
      volatile bool found= hs->contains (k);
      (void) found;
    }
  });
  bench.run ("contains() misses", [&] {
    for (auto k : keys) {
      volatile bool found= hs->contains (k + N);
      (void) found;
    }
  });
  // remove 会破坏数据,每个 epoch 都须深拷贝一份(hashset 赋值是共享 rep
  // 的浅拷贝),故本用例计时含 copy
  bench.run ("copy()+remove() all entries", [&] {
    hashset<int> h2= copy (hs);
    for (auto k : keys)
      h2->remove (k);
  });
  bench.run ("copy()", [&] {
    auto c= copy (hs);
    ankerl::nanobench::doNotOptimizeAway (c);
  });
  hashset<int> sub;
  for (int i= 0; i < N; i+= 2)
    sub->insert (keys[i]);

  bench.run ("operator<=() subset", [&] {
    bool le= true;
    // 交替包含/不包含的键扰动输入,防止编译器把结果当成不变量折叠
    for (int i= 0; i < 1000; ++i) {
      sub->insert (keys[0] + N + i);
      le= le && (sub <= hs);
      sub->remove (keys[0] + N + i);
    }
    ankerl::nanobench::doNotOptimizeAway (le);
  });

  return 0;
}
