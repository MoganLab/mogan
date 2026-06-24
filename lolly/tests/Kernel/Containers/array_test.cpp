#include "a_lolly_test.hpp"
#include "array.hpp"
#include "string.hpp"

static array<int>
gen_array (int n) {
  auto normal= array<int> ();
  for (auto i= 1; i <= n; i++) {
    normal << i;
  }
  return normal;
}

auto zero_elem = array<int> ();
auto one_elem  = append (1, zero_elem);
auto two_elem  = array<int> (1, 2);
auto three_elem= array<int> (1, 2, 3);
auto four_elem = array<int> (1, 2, 3, 4);
auto five_elem = array<int> (1, 2, 3, 4, 5);

TEST_CASE ("test access") {
  CHECK_EQ (five_elem[0], 1);
  CHECK_EQ (five_elem[1], 2);
  CHECK_EQ (five_elem[2], 3);
  CHECK_EQ (five_elem[3], 4);
  CHECK_EQ (five_elem[4], 5);
  CHECK_EQ (one_elem[0], 1);
}

TEST_CASE ("test multiply") {
  auto mulu4= array<int> ();
  for (auto i= 1; i < 6; i++) {
    mulu4 << (i * 4);
  }
  CHECK_EQ (mulu4, five_elem * 4);
}

TEST_CASE ("test divide") {
  auto mulu4= array<int> ();
  for (auto i= 1; i < 6; i++) {
    mulu4 << (i * 4);
  }
  CHECK_EQ (mulu4 / 4, five_elem);
}

TEST_CASE ("test range") {
  CHECK_EQ (range (gen_array (10), 0, 5), five_elem);
  CHECK_EQ (range (five_elem, 0, 4), four_elem);
}

TEST_CASE ("test size") {
  CHECK_EQ (N (zero_elem), 0);
  CHECK_EQ (N (one_elem), 1);
  CHECK_EQ (N (two_elem), 2);
  CHECK_EQ (N (three_elem), 3);
  CHECK_EQ (N (four_elem), 4);
  CHECK_EQ (N (five_elem), 5);
  for (auto i= 6; i < 200; i++) {
    auto array_test= gen_array (i);
    CHECK_EQ (N (array_test), i);
  }
}

TEST_CASE ("test copy") {
  CHECK_EQ (copy (zero_elem), zero_elem);
  CHECK_EQ (copy (one_elem), one_elem);
  CHECK_EQ (copy (two_elem), two_elem);
  CHECK_EQ (copy (three_elem), three_elem);
  CHECK_EQ (copy (four_elem), four_elem);
  CHECK_EQ (copy (five_elem), five_elem);
}

TEST_CASE ("test append") {
  auto zero_appended= append (1, zero_elem);
  CHECK_EQ (zero_appended, one_elem);
  auto one_appended= append (one_elem, append (2, zero_elem));
  CHECK_EQ (one_appended, two_elem);
  auto three_appended= append (three_elem, append (4, zero_elem));
  CHECK_EQ (three_appended, four_elem);

  auto one2ten= gen_array (10);
  auto six2ten= array<int> (6, 7, 8, 9, 10);
  for (auto i= 1; i <= 10; i++) {
    CHECK_EQ (one2ten[i - 1], i);
  }
  CHECK_EQ (one2ten, append (five_elem, six2ten));
}

TEST_CASE ("test reverse") {
  CHECK_EQ (reverse (zero_elem), zero_elem);
  CHECK_EQ (reverse (one_elem), one_elem);
  auto rev_five= array<int> (5, 4, 3, 2, 1);
  CHECK_EQ (reverse (five_elem), rev_five);
}

TEST_CASE ("test contains") {
  CHECK_EQ (contains (1, zero_elem), false);
  CHECK_EQ (contains (1, one_elem), true);
  CHECK_EQ (contains (3, two_elem), false);
  CHECK_EQ (contains (1, five_elem), true);
  CHECK_EQ (contains (2, five_elem), true);
  CHECK_EQ (contains (3, five_elem), true);
}

TEST_CASE ("array of string") {
  auto arr= array<string> ();
  arr << "Hello"
      << "1"
      << "2"
      << "3";
  CHECK (contains (string ("Hello"), arr));
  arr << "】";
  CHECK (contains (string ("】"), arr));
}

TEST_CASE ("test iteration") {
  int expectedIndex= 0;
  for (const auto element : five_elem) {
    CHECK_EQ (element, five_elem[expectedIndex]);
    ++expectedIndex;
  }
  CHECK_EQ (expectedIndex, 5);

  expectedIndex= 0;
  for (const auto element : zero_elem) {
    CHECK_EQ (element, zero_elem[expectedIndex]);
    ++expectedIndex;
  }
  CHECK_EQ (expectedIndex, 0);
}
