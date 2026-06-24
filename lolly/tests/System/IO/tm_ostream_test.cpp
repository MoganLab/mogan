/** \file tm_ostream_test.cpp
 *  \copyright GPLv3
 *  \details A unitest for tm_ostream.
 *  \author Paradisuman
 *  \date   2023
 */

#include "a_lolly_test.hpp"
#include "string.hpp"
#include "tm_ostream.hpp"

bool
test_same (tm_ostream& a, tm_ostream& b) {
  string sa= a.unbuffer ();
  string sb= b.unbuffer ();

  return sa == sb;
}

bool
test_same (tm_ostream& a, string b) {
  string sa= a.unbuffer ();

  return sa == b;
}

TEST_MEMORY_LEAK_INIT

TEST_CASE ("function is_writable") {
  tm_ostream t;
  CHECK (t->is_writable () == true);

  t.buffer ();
  CHECK (t->is_writable () == true);
}

TEST_CASE ("function write and unbuffer") {
  tm_ostream t;

  t.buffer ();
  t->write ("abc");
  CHECK (test_same (t, "abc"));

  t.buffer ();
  t->write ("abc");
  t->write ("1234");
  CHECK (test_same (t, "abc1234"));
}

TEST_CASE ("function redirect") {
  tm_ostream t;

  SUBCASE ("redirect itself") {
    auto tem= t;
    t.redirect (t);
    CHECK (t.rep == tem.rep);
  }

  SUBCASE ("redirect others") {
    tm_ostream t2;
    t.redirect (t2);

    CHECK (t.rep == t2.rep);
  }
}

TEST_CASE ("operator <<") {
  tm_ostream t1;
  t1.buffer ();
  tm_ostream t2;
  t2.buffer ();

  SUBCASE ("test bool") {
    t1 << true;
    t2 << false;
    CHECK (test_same (t1, "true"));
    CHECK (test_same (t2, "false"));
  }

  SUBCASE ("test char") {
    char val = 'A';
    char val2= '1';
    t1 << val;
    t2 << val2;
    CHECK (test_same (t1, "A"));
    CHECK (test_same (t2, "1"));
  }

  SUBCASE ("test short") {
    short val= 123;
    t1 << val;
    CHECK (test_same (t1, "123"));

    short val_neg= -123;
    t2 << val_neg;
    CHECK (test_same (t2, "-123"));
  }

  SUBCASE ("test float") {
    float val= 3.14f;
    t1 << val;
    CHECK (test_same (t1, "3.14"));

    float val_neg= -3.14f;
    t2 << val_neg;
    CHECK (test_same (t2, "-3.14"));
  }

  SUBCASE ("test double") {
    double val= 3.14159265;
    t1 << val;
    CHECK (test_same (t1, "3.14159"));

    double val_neg= -3.14159265;
    t2 << val_neg;
    CHECK (test_same (t2, "-3.14159"));
  }

  SUBCASE ("test int") {
    int val= 1000;
    t1 << val;
    CHECK (test_same (t1, "1000"));

    int val_neg= -1000;
    t2 << val_neg;
    CHECK (test_same (t2, "-1000"));
  }

  SUBCASE ("test unsigned int") {
    unsigned int val= 2000;
    t1 << val;
    CHECK (test_same (t1, "2000"));
  }

  SUBCASE ("test long") {
    long val= 3000000L;
    t1 << val;
    CHECK (test_same (t1, "3000000"));

    long val_neg= -3000000L;
    t2 << val_neg;
    CHECK (test_same (t2, "-3000000"));
  }

  SUBCASE ("test unsigned long") {
    unsigned long val= 4000000UL;
    t1 << val;
    CHECK (test_same (t1, "4000000"));
  }

  SUBCASE ("test long long int") {
    long long int val= 50000000000LL;
    t1 << val;
    CHECK (test_same (t1, "50000000000"));

    long long int val_neg= -50000000000LL;
    t2 << val_neg;
    CHECK (test_same (t2, "-50000000000"));
  }

  SUBCASE ("test unsigned long long int") {
    unsigned long long int val= 60000000000ULL;
    t1 << val;
    CHECK (test_same (t1, "60000000000"));
  }

  SUBCASE ("test long double") {
    long double val= 3.141592653589793238L;
    t1 << val;
    CHECK (test_same (t1, "3.14159"));

    long double val_neg= -3.141592653589793238L;
    t2 << val_neg;
    CHECK (test_same (t2, "-3.14159"));
  }

  SUBCASE ("test const char*") {
    const char* val= "Hello, world!";
    t1 << val;
    CHECK (test_same (t1, "Hello, world!"));
  }
}

TEST_CASE ("cout/cerr") {
  cout << "Lolly" << LF;
  cout << "棒棒糖" << LF;
  cerr << "Lolly" << LF;
  cerr << "棒棒糖" << LF;
}

TEST_MEMORY_LEAK_ALL
