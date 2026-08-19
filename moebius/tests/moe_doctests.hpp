#ifndef MOE_TBOX_MACROS_H
#define MOE_TBOX_MACROS_H

#include "doctest/doctest.h"
#include "point.hpp"
#include "string.hpp"
#include "url.hpp"

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

/** \brief 浮点容差比较两个 point（各分量差的绝对值均不超过 eps） */
inline bool
almost_eq (point p, point q, double eps= 1e-9) {
  if (N (p) != N (q)) return false;
  for (int i= 0; i < N (p); i++)
    if (fabs (p[i] - q[i]) > eps) return false;
  return true;
}

#define TEST_MEMORY_LEAK_ALL                                                   \
  TEST_CASE ("test memory leak above") { CHECK_EQ (mem_used (), mem_lolly); }

#define TEST_MEMORY_LEAK_INIT                                                  \
  int mem_lolly= 0;                                                            \
  TEST_CASE ("read before test") { mem_lolly= mem_used (); }

#define TEST_MEMORY_LEAK_RESET                                                 \
  TEST_CASE ("reset test of memory leak") { mem_lolly= mem_used (); }

#endif
