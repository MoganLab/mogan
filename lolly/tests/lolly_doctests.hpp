#ifndef LOLLY_TBOX_MACROS_H
#define LOLLY_TBOX_MACROS_H

#include "doctest/doctest.h"
#include "lolly/data/string_u16.hpp"
#include "lolly/data/string_view.hpp"
#include "string.hpp"
#include "url.hpp"

using lolly::data::string_u16;
using lolly::data::string_u16_view;

inline void
string_eq (string left, string right) {
  if (left != right) {
    cout << "left : " << left << LF;
    cout << "right: " << right << LF;
  }
  CHECK_EQ (left == right, true);
}

inline void
string_neq (string left, string right) {
  if (left == right) {
    cout << "same: " << left << LF;
  }
  CHECK_EQ (left != right, true);
}

inline void
url_eq (url left, url right) {
  if (left != right) {
    cout << "left : " << left << LF;
    cout << "right: " << right << LF;
  }
  CHECK_EQ (left == right, true);
}

inline void
url_neq (url left, url right) {
  if (left == right) {
    cout << "same: " << left << LF;
  }
  CHECK_EQ (left != right, true);
}

#define TM_CHECK_THROWS(statements)                                            \
  CHECK_THROWS (statements);                                                   \
  handle_exceptions ();

#define TEST_MEMORY_LEAK_ALL                                                   \
  TEST_CASE ("test memory leak above") { CHECK_EQ (mem_used (), mem_lolly); }

#define TEST_MEMORY_LEAK_INIT                                                  \
  int mem_lolly= 0;                                                            \
  TEST_CASE ("read before test") { mem_lolly= mem_used (); }

#define TEST_MEMORY_LEAK_RESET                                                 \
  TEST_CASE ("reset test of memory leak") { mem_lolly= mem_used (); }

#endif
