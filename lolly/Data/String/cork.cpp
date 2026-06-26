
/******************************************************************************
 * MODULE     : cork.cpp
 * DESCRIPTION: Routines on Cork Encoding
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "cork.hpp"
#include "analyze.hpp"

bool
contains_unicode_char (string s) {
  int i= 0, n= N (s);
  while (i + 1 < n) {
    if (s[i] == '<' && s[i + 1] == '#') return true;
    tm_char_forwards (s, i);
  }
  return false;
}

/******************************************************************************
 * Routines for the TeXmacs encoding
 ******************************************************************************/

string
tm_encode (string s) {
  // verbatim to TeXmacs encoding
  string r;
  for (const auto ch : s) {
    if (ch == '<') r << "<less>";
    else if (ch == '>') r << "<gtr>";
    else r << ch;
  }
  return r;
}

string
tm_var_encode (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++) {
    if (s[i] == '<') {
      if (i + 1 < n && s[i + 1] == '#') {
        while (i < n && s[i] != '>')
          r << s[i++];
        if (i < n) r << s[i];
      }
      else r << "<less>";
    }
    else if (s[i] == '>') r << "<gtr>";
    else r << s[i];
  }
  return r;
}

string
tm_decode (string s) {
  // TeXmacs encoding to verbatim
  string r;
  int    s_N= N (s);
  for (int i= 0; i < s_N; i++) {
    if (s[i] == '<') {
      int j;
      for (j= i + 1; j < s_N; j++)
        if (s[j] == '>') break;
      if (j < s_N) j++;
      if (s (i, j) == "<less>") r << "<";
      else if (s (i, j) == "<gtr>") r << ">";
      else if (i + 7 == j && s[i + 1] == '#' && s[j - 1] == '>') r << s (i, j);
      i= j - 1;
      if (s[i] != '>') return r;
    }
    else if (s[i] != '>') r << s[i];
  }
  return r;
}

string
tm_correct (string s) {
  int    i;
  string r;
  for (i= 0; i < N (s); i++) {
    if (s[i] == '<') {
      bool flag= true;
      int  j, k;
      for (j= i + 1; j < N (s); j++)
        if (s[j] == '>') break;
      if (j == N (s)) return r;
      for (k= i + 1; k < j; k++)
        if (s[k] == '<') flag= false;
      if (flag) r << s (i, j + 1);
      i= j;
    }
    else if (s[i] != '>') r << s[i];
  }
  return r;
}

void
tm_char_forwards (string s, int& pos) {
  ASSERT (pos >= 0 && pos <= N (s), "out of range");
  int n= N (s);
  if (pos == n)
    ;
  else if (s[pos] != '<') pos++;
  else {
    while (pos < n && s[pos] != '>')
      pos++;
    if (pos < n) pos++;
  }
}

void
tm_char_backwards (string s, int& pos) {
  ASSERT (pos >= 0 && pos <= N (s), "out of range");
  if (pos == 0)
    ;
  else if (s[pos - 1] != '>') pos--;
  else {
    while (pos > 0 && s[pos - 1] != '<')
      pos--;
    if (pos > 0) pos--;
  }
}

int
tm_char_next (string s, int pos) {
  tm_char_forwards (s, pos);
  return pos;
}

int
tm_char_previous (string s, int pos) {
  tm_char_backwards (s, pos);
  return pos;
}

string
tm_forward_access (string s, int k) {
  int pos= 0;
  for (int i= 0; i < k; i++)
    tm_char_forwards (s, pos);
  int start= pos;
  tm_char_forwards (s, pos);
  return s (start, pos);
}

string
tm_backward_access (string s, int k) {
  int pos= N (s);
  for (int i= 0; i < k; i++)
    tm_char_backwards (s, pos);
  int end= pos;
  tm_char_backwards (s, pos);
  return s (pos, end);
}

int
tm_string_length (string s) {
  int i= 0, pos= 0, s_N= N (s);
  while (pos < s_N) {
    tm_char_forwards (s, pos);
    i++;
  }
  return i;
}

array<string>
tm_tokenize (string s) {
  array<string> r;
  int           pos= 0, s_N= N (s);
  while (pos < s_N) {
    int start= pos;
    tm_char_forwards (s, pos);
    r << s (start, pos);
  }
  return r;
}

string
tm_recompose (array<string> a) {
  string r;
  int    a_N= N (a);
  for (int i= 0; i < a_N; i++)
    r << a[i];
  return r;
}

int
tm_search_forwards (string s, int pos, string in) {
  int k= N (s), n= N (in);
  if (k == 0) return pos;
  char c= s[0];
  while (pos + k <= n) {
    if (in[pos] == c && test (in, pos, s)) return pos;
    tm_char_forwards (in, pos);
  }
  return -1;
}

int
tm_search_backwards (string s, int pos, string in) {
  while (pos >= 0) {
    if (test (in, pos, s)) return pos;
    tm_char_backwards (in, pos);
  }
  return -1;
}

static array<string>
tm_string_split_between_words (string s) {
  int           i= 0, j= -1, n= N (s);
  char          status= 'o';
  array<string> r;
  while (i < n && j < n / 2) {
    char c= s[i];
    if (is_numeric (c) && status == 'c')
      ;
    else if (is_iso_alpha (c) && status == 'a')
      ;
    else {
      if (is_numeric (c)) status= 'c';
      else if (is_iso_alpha (c)) status= 'a';
      else status= 'x';
      j= i;
    }
    tm_char_forwards (s, i);
  }
  if (j > 0 && j < n) r << s (0, j) << s (j, n);
  else r << s;
  return r;
}

static array<string>
tm_string_split_at_spaces (string s) {
  int           i= 0, j= 0, n= N (s);
  array<string> r;
  while (i >= 0 && j < n / 2) {
    i= tm_search_forwards (" ", i, s);
    if (i == -1) break;
    j= i++;
  }
  if (j < 1 || j >= n) r << s;
  else if (j == n - 1) r << s (0, j) << s (j, n);
  else r << s (0, j) << s (j, j + 1) << s (j + 1, n);
  return r;
}

array<string>
tm_string_split (string s) {
  array<string> r;
  r= tm_string_split_at_spaces (s);
  if (N (r) > 1) return r;
  r= tm_string_split_between_words (s);
  if (N (r) > 1) return r;
  /* else split anywhere */
  int i= 0, n= N (s);
  while (i < n / 2)
    tm_char_forwards (s, i);
  return array<string> (s (0, i), s (i, n));
}

string
downgrade_math_letters (string s) {
  string r= "";
  for (int i= 0; i < N (s);) {
    int start= i;
    tm_char_forwards (s, i);
    if (i == start + 1) r << s[start];
    else {
      string ss= s (start, i);
      if (starts (ss, "<b-")) ss= "<" * ss (3, N (ss));
      if (starts (ss, "<up-")) ss= "<" * ss (4, N (ss));
      if (starts (ss, "<cal-")) ss= "<" * ss (5, N (ss));
      if (starts (ss, "<bbb-")) ss= "<" * ss (5, N (ss));
      if (starts (ss, "<frak-")) ss= "<" * ss (6, N (ss));
      if (N (ss) == 3) ss= ss (1, 2);
      r << ss;
    }
  }
  return r;
}
