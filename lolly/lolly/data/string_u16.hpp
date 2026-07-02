
/** \file string_u16.hpp
 *  \copyright GPLv3
 *  \details Strings with different type of char, and corresponding readonly
 *           view of string. Zero-characters are allowed in strings.
 *  \author jingkaimori
 *  \date   2024
 */

#pragma once

#include "classdef.hpp"
#include "fast_alloc.hpp"
#include "string_view.hpp"

namespace lolly {
namespace data {

using string_u16_view= lolly::data::string_view<char16_t>;

class string_u16;

class string_u16_rep : concrete_struct {
  int       n;
  int       a_N;
  char16_t* a;

public:
  inline string_u16_rep () : n (0), a_N (0), a (NULL) {}
  string_u16_rep (int n);
  inline ~string_u16_rep () {
    if (n != 0) tm_delete_array (a);
  }
  /**
   * @brief expand (or shrink) string by delta, but do not release memory when
   * string is shrinked.
   *
   * @return string length before expansion
   */
  int expand_or_shrink_by (int delta);

  /**
   * @brief expand (or shrink) string to given length n, and try to release
   * memory when string is shrinked.
   *
   * @note expand_or_shrink_by may be faster if memory space is reserved
   */
  void resize (int n);

  /**
   * @brief reserve memory space to contain at least n word in the whole string.
   * Do not affect length of string, and do not release memory when n is smaller
   * than current space.
   */
  void reserve (int n);

  friend class string_u16;
  friend int N (string_u16 a);
};

class string_u16 {
  CONCRETE (string_u16);

  inline string_u16 () : rep (tm_new<string_u16_rep> ()) {}
  inline explicit string_u16 (int n) : rep (tm_new<string_u16_rep> (n)) {}

  template <size_t N_>
  string_u16 (const char16_t (&s)[N_]) : rep (tm_new<string_u16_rep> (N_ - 1)) {
    constexpr int n= N_ - 1;
    for (int i= 0; i < n; i++)
      rep->a[i]= s[i];
  };

  string_u16 (char16_t c);
  string_u16 (char16_t c, int n);
  string_u16 (const string_u16_view& sv);

  inline char16_t* buffer () { return rep->a; }
  inline char16_t* buffer (int size) {
    rep->resize (size);
    return rep->a;
  }
  char16_t* begin () { return rep->a; }
  char16_t* end () { return rep->a + rep->n; }

  inline operator string_u16_view () {
    return string_u16_view (rep->a, rep->n);
  }
  inline string_u16_view operator() (int start, int end) {
    return ((string_u16_view) * this) (start, end);
  }

  inline char16_t& operator[] (int i) { return rep->a[i]; }
};
CONCRETE_CODE (string_u16);

inline int
N (string_u16 a) {
  return a->n;
}

inline int
hash (string_u16 s) {
  int i, h= 0, n= N (s);
  for (i= 0; i < n; i++) {
    h= (h << 9) + (h >> 23);
    h= h + ((int) s[i]);
  }
  return h;
};

inline bool
operator== (string_u16 a, string_u16 b) {
  return ((string_u16_view) a) == ((string_u16_view) b);
};

inline bool
operator== (string_u16 a, string_u16_view b) {
  return ((string_u16_view) a) == b;
}

inline bool
operator== (string_u16_view a, string_u16 b) {
  return a == ((string_u16_view) b);
}

template <size_t Nb>
bool
operator== (string_u16 a, const char16_t (&b)[Nb]) {
  return ((string_u16_view) a) == string_u16_view (b);
}

template <size_t Na>
bool
operator== (const char16_t (&a)[Na], string_u16 b) {
  return string_u16_view (a) == ((string_u16_view) b);
}

inline bool
operator!= (string_u16 a, string_u16 b) {
  return ((string_u16_view) a) != ((string_u16_view) b);
};

inline bool
operator!= (string_u16 a, string_u16_view b) {
  return ((string_u16_view) a) != b;
}

inline bool
operator!= (string_u16_view a, string_u16 b) {
  return a != ((string_u16_view) b);
}

template <size_t Nb>
bool
operator!= (string_u16 a, const char16_t (&b)[Nb]) {
  return ((string_u16_view) a) != string_u16_view (b);
}

template <size_t Na>
bool
operator!= (const char16_t (&a)[Na], string_u16 b) {
  return string_u16_view (a) != ((string_u16_view) b);
}

inline bool
operator< (string_u16 a, string_u16 b) {
  return ((string_u16_view) a) < ((string_u16_view) b);
}

inline bool
operator<= (string_u16 a, string_u16 b) {
  return ((string_u16_view) a) <= ((string_u16_view) b);
}

string_u16& operator<< (string_u16& a, char16_t c);
string_u16& operator<< (string_u16& a, string_u16 b);

string_u16 operator* (string_u16 a, const string_u16_view& b);

string_u16 copy (const string_u16_view& a);
string_u16 copy (string_u16 a);

} // namespace data
} // namespace lolly
