
/** \file string_view.hpp
 *  \copyright GPLv3
 *  \details Strings with different type of char, and corresponding readonly
 *           view of string. Zero-characters are allowed in strings.
 *  \author jingkaimori
 *  \date   2024
 */

#pragma once

#include "classdef.hpp"
#include "fast_alloc.hpp"
#include "minmax.hpp"
#include <string>

namespace lolly {
namespace data {

/**
 * Readonly view of string, in which char is store in a continuous space. NUL is
 * allowed in string. this view can be construct from lolly_string et al.
 */
template <typename T> class string_view {
public:
  const int      N;
  const T* const a;
  /**
   * construct from arbitary const pointer with length. NUL can occurs in given
   * array
   */
  string_view (const T* a_, int N_) : N (N_), a (a_) {};
  /**
   * construct from string literal
   */
  template <size_t N_>
  constexpr string_view (const T (&null_end_str)[N_])
      : N (N_ - 1), a (null_end_str){};

  string_view (string_view<T>&&)= default;

  constexpr string_view (const std::basic_string<T>& str)
      : N (str.size ()), a (str.data ()) {};
  operator std::basic_string<T> () { return std::basic_string<T> (a, N); };

  string_view<T> operator() (int start, int end) const;

  /**
   * empty view is not allowed.
   */
  string_view ()                                = delete;
  string_view (const string_view<T>&)           = delete;
  string_view& operator= (const string_view<T>&)= delete;
  string_view& operator= (string_view<T>&&)     = delete;
};

template <class T>
string_view<T>
string_view<T>::operator() (int start, int end) const {
  if (end <= start) {
    return string_view<T> (nullptr, 0);
  }
  start= max (min (N, start), 0);
  end  = max (min (N, end), 0);
  return string_view<T> (a + start, end - start);
}

template <typename T>
bool
operator== (const string_view<T>& a, const string_view<T>& b) {
  if (a.N != b.N) return false;
  const T *Sa= a.a, *Sb= b.a;
  int      n= a.N;
  for (int i= 0; i < n; i++)
    if (Sa[i] != Sb[i]) return false;
  return true;
};

template <typename T, size_t Na>
bool
operator== (const T (&a)[Na], const string_view<T>& b) {
  return string_view<T> (a) == b;
}

template <typename T, size_t Nb>
bool
operator== (const string_view<T>& a, const T (&b)[Nb]) {
  return a == string_view<T> (b);
};

template <typename T>
bool
operator!= (const string_view<T>& a, const string_view<T>& b) {
  if (a.N != b.N) return true;
  const T *Sa= a.a, *Sb= b.a;
  int      n= a.N;
  for (int i= 0; i < n; i++)
    if (Sa[i] != Sb[i]) return true;
  return false;
};

template <typename T, size_t Nb>
bool
operator!= (const string_view<T>& a, const T (&b)[Nb]) {
  return a != string_view<T> (b);
};

template <typename T, size_t Na>
bool
operator!= (const T (&a)[Na], const string_view<T>& b) {
  return string_view<T> (a) != b;
};

template <typename T>
bool
operator< (const string_view<T>& a, const string_view<T>& b) {
  int      i, na= a.N, nb= b.N, nmin= min (na, nb);
  const T *Sa= a.a, *Sb= b.a;
  for (i= 0; i < nmin; i++) {
    if (Sa[i] < Sb[i]) return true;
    if (Sb[i] < Sa[i]) return false;
  }
  return na < nb;
};

template <typename T, size_t Nb>
bool
operator< (const string_view<T>& a, const T (&b)[Nb]) {
  return a < string_view<T> (b);
};

template <typename T, size_t Na>
bool
operator< (const T (&a)[Na], const string_view<T>& b) {
  return string_view<T> (a);
};

template <typename T>
bool
operator<= (const string_view<T>& a, const string_view<T>& b) {
  int      i, na= a.N, nb= b.N, nmin= min (na, nb);
  const T *Sa= a.a, *Sb= b.a;
  for (i= 0; i < nmin; i++) {
    if (Sa[i] < Sb[i]) return true;
    if (Sb[i] < Sa[i]) return false;
  }
  return na <= nb;
};

template <typename T, size_t Nb>
bool
operator<= (const string_view<T>& a, const T (&b)[Nb]) {
  return a <= string_view<T> (b);
};

template <typename T, size_t Na>
bool
operator<= (const T (&a)[Na], const string_view<T>& b) {
  return string_view<T> (a) <= b;
};

} // namespace data
} // namespace lolly
