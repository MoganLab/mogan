#include "a_lolly_test.hpp"
#include "ntuple.hpp"
#include "string.hpp"
#include "tm_ostream.hpp"

bool
test_same (tm_ostream& a, string b) {
  string sa= a.unbuffer ();
  return sa == b;
}

TEST_CASE ("test pair") {
  pair<int, int> p1 (1, 2);
  pair<int, int> p2 (1, 2);
  CHECK (p1 == p2);
  CHECK (p1.x1 == 1);
  CHECK (p1.x2 == 2);
  CHECK (p1 != pair<int, int> (1, 3));
  CHECK (p1 != pair<int, int> (2, 2));
  CHECK (hash (p1) == hash (p2));
  tm_ostream out1;
  out1.buffer ();
  out1 << p1;
  CHECK (test_same (out1, "[ 1, 2 ]"));
}

TEST_CASE ("test triple") {
  triple<int, int, int> t1 (1, 2, 3);
  triple<int, int, int> t2 (1, 2, 3);
  CHECK (t1 == t2);
  CHECK (t1.x1 == 1);
  CHECK (t1.x2 == 2);
  CHECK (t1.x3 == 3);
  CHECK (t1 != triple<int, int, int> (1, 2, 4));
  CHECK (t1 != triple<int, int, int> (1, 3, 3));
  CHECK (t1 != triple<int, int, int> (2, 3, 4));
  CHECK (hash (t1) == hash (t2));
  tm_ostream out1;
  out1.buffer ();
  out1 << t1;
  CHECK (test_same (out1, "[ 1, 2, 3 ]"));
}

TEST_CASE ("test quadruple") {
  quartet<int, int, int, int> q1 (1, 2, 3, 4);
  quartet<int, int, int, int> q2 (1, 2, 3, 4);
  CHECK (q1 == q2);
  CHECK (q1.x1 == 1);
  CHECK (q1.x2 == 2);
  CHECK (q1.x3 == 3);
  CHECK (q1.x4 == 4);
  CHECK (q1 != quartet<int, int, int, int> (1, 2, 3, 5));
  CHECK (q1 != quartet<int, int, int, int> (1, 2, 4, 4));
  CHECK (q1 != quartet<int, int, int, int> (2, 3, 4, 5));
  CHECK (hash (q1) == hash (q2));
  tm_ostream out1;
  out1.buffer ();
  out1 << q1;
  CHECK (test_same (out1, "[ 1, 2, 3, 4 ]"));
}

TEST_CASE ("test quintuple") {
  quintuple<int, int, int, int, int> q1 (1, 2, 3, 4, 5);
  quintuple<int, int, int, int, int> q2 (1, 2, 3, 4, 5);
  CHECK (q1 == q2);
  CHECK (q1.x1 == 1);
  CHECK (q1.x2 == 2);
  CHECK (q1.x3 == 3);
  CHECK (q1.x4 == 4);
  CHECK (q1.x5 == 5);
  CHECK (q1 != quintuple<int, int, int, int, int> (1, 2, 3, 4, 6));
  CHECK (q1 != quintuple<int, int, int, int, int> (1, 2, 3, 5, 5));
  CHECK (q1 != quintuple<int, int, int, int, int> (2, 3, 4, 5, 6));
  CHECK (hash (q1) == hash (q2));
  tm_ostream out1;
  out1.buffer ();
  out1 << q1;
  CHECK (test_same (out1, "[ 1, 2, 3, 4, 5 ]"));
}

TEST_CASE ("test sextuple") {
  sextuple<int, int, int, int, int, int> s1 (1, 2, 3, 4, 5, 6);
  sextuple<int, int, int, int, int, int> s2 (1, 2, 3, 4, 5, 6);
  CHECK (s1 == s2);
  CHECK (s1.x1 == 1);
  CHECK (s1.x2 == 2);
  CHECK (s1.x3 == 3);
  CHECK (s1.x4 == 4);
  CHECK (s1.x5 == 5);
  CHECK (s1.x6 == 6);
  CHECK (s1 != sextuple<int, int, int, int, int, int> (1, 2, 3, 4, 5, 7));
  CHECK (s1 != sextuple<int, int, int, int, int, int> (1, 2, 3, 4, 6, 6));
  CHECK (s1 != sextuple<int, int, int, int, int, int> (2, 3, 4, 5, 6, 7));
  CHECK (hash (s1) == hash (s2));
  tm_ostream out1;
  out1.buffer ();
  out1 << s1;
  CHECK (test_same (out1, "[ 1, 2, 3, 4, 5, 6 ]"));
}