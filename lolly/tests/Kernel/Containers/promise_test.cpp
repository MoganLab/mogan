/** \file promise_test.cpp
 *  \copyright GPLv3
 *  \details A unitest for promise.
 *  \author Paradisuman
 *  \date   2023
 */

#include "a_lolly_test.hpp"
#include "promise.hpp"
#include "string.hpp"

class promise_rep_int_add : public promise_rep<int> {
  int x;
  int y;

public:
  promise_rep_int_add (int a, int b) : x (a), y (b) {};
  virtual int eval () override { return x + y; }
};

bool
test_same (tm_ostream& a, tm_ostream& b) {
  string sa= a.unbuffer ();
  string sb= b.unbuffer ();

  return sa == sb;
}

TEST_CASE ("function eval") {
  promise<int> t1;
  t1.rep= tm_new<promise_rep_int_add> (1, 2);
  promise<int> t2;
  t2.rep= tm_new<promise_rep_int_add> (-1, 2);

  CHECK (t1 () == 3);
  CHECK (t2 () == 1);
}

TEST_CASE ("function print") {
  promise<int> t;
  t.rep= tm_new<promise_rep_int_add> (1, 2);
  tm_ostream _ostream1;
  _ostream1.buffer ();

  SUBCASE ("if tm_ostream empty") {
    tm_ostream _ostream2;
    _ostream2.buffer ();
    _ostream2 << "promise";
    t->print (_ostream1);

    CHECK (test_same (_ostream1, _ostream2));
  }

  SUBCASE ("if tm_ostream not empty") {
    tm_ostream _ostream2;
    _ostream2.buffer ();
    _ostream2 << "testpromise";
    _ostream1 << "test";

    t->print (_ostream1);

    CHECK (test_same (_ostream1, _ostream2));
  }
}

TEST_CASE ("operator <<") {
  promise<int> t;

  SUBCASE ("if promise null") {
    tm_ostream _ostream1;
    tm_ostream _ostream2;
    _ostream1.buffer ();
    _ostream2.buffer ();
    _ostream2 << "(null)";
    _ostream1 << t;

    CHECK (test_same (_ostream1, _ostream2));
  }

  SUBCASE ("if promise not null") {
    t.rep= tm_new<promise_rep_int_add> (1, 2);
    tm_ostream _ostream1;
    tm_ostream _ostream2;
    _ostream1.buffer ();
    _ostream2.buffer ();
    _ostream2 << "promise";
    _ostream1 << t;

    CHECK (test_same (_ostream1, _ostream2));
  }
}