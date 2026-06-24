#include "hashmap.hpp"
#include <iostream>
#include <nanobench.h>
#include <vector>

using namespace lolly;
using KeyType  = int;
using ValueType= int;

void
collect_key (KeyType k) {
  // no-op collector for generate()
  (void) k;
}

int
main () {
  ankerl::nanobench::Bench bench;
  const int                N= 200000;

  std::vector<KeyType> keys;
  keys.reserve (N);
  for (int i= 0; i < N; ++i) {
    keys.push_back (i);
  }
  bench.run ("insert [operator()]", [&] {
    hashmap<KeyType, ValueType> h;
    for (auto k : keys) {
      h (k)= 42;
    }
  });

  hashmap<KeyType, ValueType> hm;
  for (auto k : keys) {
    hm (k)= 42;
  }

  bench.run ("contains() hits", [&] {
    for (auto k : keys) {
      volatile bool found= hm->contains (k);
      (void) found;
    }
  });

  bench.run ("lookup [operator[]] hit", [&] {
    for (auto k : keys) {
      volatile ValueType v= hm[k];
      (void) v;
    }
  });
  bench.run ("reset() all entries", [&] {
    hashmap<KeyType, ValueType> h2= hm;
    for (auto k : keys) {
      h2->reset (k);
    }
  });
  bench.run ("generate()", [&] { hm->generate (collect_key); });
  bench.run ("join() two maps", [&] {
    hashmap<KeyType, ValueType> other;
    for (auto k : keys) {
      other (k)= 7;
    }
    hashmap<KeyType, ValueType> copy= hm;
    copy->join (other);
  });
  bench.run ("operator==() equal", [&] {
    hashmap<KeyType, ValueType> a = hm;
    hashmap<KeyType, ValueType> b = hm;
    volatile bool               eq= (a == b);
    (void) eq;
  });
  bench.run ("operator==() not equal", [&] {
    hashmap<KeyType, ValueType> a= hm;
    hashmap<KeyType, ValueType> b= hm;
    b (keys.back ())             = 99;
    volatile bool eq             = (a == b);
    (void) eq;
  });

  auto make_chain= [&] (int M) {
    list<hashentry<int, int>> tmp;
    for (int i= 0; i < M; ++i) {
      tmp= list<hashentry<int, int>> (
          hashentry<int, int> (0, /*code*/ i, /*im*/ 42), tmp);
    }
    return tmp;
  };

  auto chain= make_chain (10000);
  bench.run ("copy_list", [&] {
    auto c= copy_list (chain);
    ankerl::nanobench::doNotOptimizeAway (c);
  });
  bench.run ("single resize op", [&] {
    hashmap<KeyType, ValueType> h;
    for (int i= 0; i < 8; ++i)
      h (i)= 1;
    h (8)= 1;
  });

  return 0;
}
