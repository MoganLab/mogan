#include "a_lolly_test.hpp"
#include "hashfunc.hpp"

TEST_CASE ("hashfunc") {
  // Define the hash function for integers, where hash(x) = x % 7
  int                init= 0;
  hashfunc<int, int> hashf ([] (int x) { return x % 7; }, init);

  // Test the hash function for several input values
  CHECK (hashf[0] == 0);
  CHECK (hashf[1] == 1);
  CHECK (hashf[2] == 2);
  CHECK (hashf[3] == 3);
  CHECK (hashf[4] == 4);
  CHECK (hashf[5] == 5);
  CHECK (hashf[6] == 6);
  CHECK (hashf[7] == 0);
  CHECK (hashf[8] == 1);
  CHECK (hashf[9] == 2);
}