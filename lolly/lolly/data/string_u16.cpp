
/** \file string_u16.cpp
 *  \copyright GPLv3
 *  \details Strings with different type of char, and corresponding readonly
 *           view of string. Zero-characters are allowed in strings.
 *  \author jingkaimori
 *  \date   2024
 */

#include "string_u16.hpp"
#include "minmax.hpp"

namespace lolly {
namespace data {

static inline constexpr int
round_length (int n) {
  n= (n + 3) & (0xfffffffc);
  if (n < 24) return n;
  int i= 32;
  while (n > i)
    i<<= 1;
  return i;
}

string_u16_rep::string_u16_rep (int n2)
    : n (n2), a_N (round_length (n)),
      a ((n == 0) ? ((char16_t*) NULL) : tm_new_array<char16_t> (a_N)) {}

void
string_u16_rep::resize (int m) {
  int nn= round_length (n);
  int mm= round_length (m);
  if (mm != nn) {
    if (mm != 0) {
      if (nn != 0) {
        a_N= mm;
        a  = tm_resize_array<char16_t> (mm, a);
      }
      else {
        a_N= mm;
        a  = tm_new_array<char16_t> (mm);
      }
    }
    else if (nn != 0) {
      tm_delete_array (a);
      a_N= 0;
      a  = NULL;
    };
  }
  n= m;
}

int
string_u16_rep::expand_or_shrink_by (int delta) {
  int old_n= n;
  n+= delta;
  reserve (n);
  return old_n;
}

void
string_u16_rep::reserve (int new_n) {
  int old_size= a_N;
  if (new_n != 0) {
    if (old_size == 0) {
      a_N= round_length (new_n);
      a  = tm_new_array<char16_t> (a_N);
    }
    else if (old_size < new_n) {
      a_N= round_length (new_n);
      a  = tm_resize_array<char16_t> (a_N, a);
    }
  }
  else {
    if (old_size != 0) {
      tm_delete_array (a);
      a  = NULL;
      a_N= 0;
    };
  }
}

string_u16::string_u16 (char16_t c) : rep (tm_new<string_u16_rep> (1)) {
  rep->a[0]= c;
};

string_u16::string_u16 (const string_u16_view& c)
    : rep (tm_new<string_u16_rep> (c.N)) {
  for (int i= 0; i < c.N; i++)
    rep->a[i]= c.a[i];
};

string_u16::string_u16 (char16_t c, int n) : rep (tm_new<string_u16_rep> (n)) {
  for (int i= 0; i < n; i++)
    rep->a[i]= c;
};

string_u16
copy (const string_u16_view& a) {
  int        i;
  string_u16 r (a.N);
  for (i= 0; i < a.N; i++)
    r[i]= a.a[i];
  return r;
};

string_u16
copy (string_u16 a) {
  return copy ((string_u16_view) a);
};

string_u16&
operator<< (string_u16& a, char16_t ch) {
  int old_a_N= a->expand_or_shrink_by (1);
  a[old_a_N] = ch;
  return a;
};

string_u16&
operator<< (string_u16& a, string_u16 b) {
  int b_N    = N (b);
  int old_a_N= a->expand_or_shrink_by (b_N);
  for (int i= 0; i < b_N; i++)
    a[i + old_a_N]= b[i];
  return a;
};

string_u16
operator* (string_u16 a, const string_u16_view& b) {
  int        a_N= N (a), b_N= b.N;
  string_u16 c (a_N + b_N);
  for (int i= 0; i < a_N; i++) {
    c[i]= a[i];
  }
  for (int i= 0; i < b_N; i++) {
    c[i + a_N]= b.a[i];
  }
  return c;
};

} // namespace data
} // namespace lolly
