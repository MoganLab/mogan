#ifndef TM_CODE_WRAP_HPP
#define TM_CODE_WRAP_HPP

#include "basic.hpp"
#include "string.hpp"
// Protect TeXmacs internal escape sequences like "<#4E2D>" (CJK, etc.)
// from being split during automatic line wrapping in code/prog environments.
//
// NOTE:
// - We only protect "<#...>" to avoid affecting normal code like "<tag>".
// - This is a last-resort safety net: even if the line breaker proposes an
//   invalid split position, we snap it to a valid boundary here.
static inline int
tm_atom_end_for_code_wrap (string s, int i) {
  int n= N (s);
  if (i < 0 || i >= n) return i;
  if (s[i] != '<') return i + 1;

  if (i + 1 >= n || s[i + 1] != '#') return i + 1;

  int j= i + 2;
  while (j < n && s[j] != '>')
    j++;
  if (j < n && s[j] == '>') return j + 1;

  return i + 1;
}

static inline int
tm_snap_after_boundary_for_code_wrap (string s, int after) {
  int n= N (s);
  if (after <= 0) return 0;
  if (after >= n) return n;

  int i   = 0;
  int last= 0;
  while (i < n) {
    int j= tm_atom_end_for_code_wrap (s, i);
    if (j > after) break;
    last= j;
    i   = j;
  }
  return last;
}

#endif // TM_CODE_WRAP_HPP
