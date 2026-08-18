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
  bench.run ("remove() all entries", [&] {
    hashset<int> h2= hs;
    for (auto k : keys)
      h2->remove (k);
  });
  bench.run ("copy()", [&] {
    auto c= copy (hs);
    ankerl::nanobench::doNotOptimizeAway (c);
  });
  bench.run ("operator<=() subset", [&] {
    hashset<int> sub;
    for (int i= 0; i < N; i+= 2)
      sub->insert (keys[i]);
    volatile bool le= (sub <= hs);
    (void) le;
  });
  bench.run ("single resize op", [&] {
    hashset<int> h;
    for (int i= 0; i < 2; ++i)
      h->insert (i);
    h->insert (2);
  });

  return 0;
}
