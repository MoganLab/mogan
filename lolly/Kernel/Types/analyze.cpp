
/******************************************************************************
 * MODULE     : analyze.cpp
 * DESCRIPTION: Properties of characters and strings
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "lolly/data/numeral.hpp"
#include "ntuple.hpp"

/******************************************************************************
 * Tests for characters
 ******************************************************************************/

bool
is_iso_alpha (char c) {
  int i= ((int) ((unsigned char) c));
  return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
         ((i >= 128) && (i != 159) && (i != 189) && (i != 190) && (i != 191));
}

bool
is_iso_locase (char c) {
  int code= (int) ((unsigned char) c);
  return ((c >= 'a') && (c <= 'z')) || ((code >= 160) && (code < 189)) ||
         (code >= 224);
}

bool
is_iso_upcase (char c) {
  int code= (int) ((unsigned char) c);
  return ((c >= 'A') && (c <= 'Z')) || ((code >= 128) && (code < 159)) ||
         ((code >= 192) && (code < 224));
}

/******************************************************************************
 * Tests for strings
 ******************************************************************************/

bool
is_alpha (string s) {
  int i;
  if (N (s) == 0) return false;
  for (i= 0; i < N (s); i++)
    if (!is_alpha (s[i])) return false;
  return true;
}

bool
is_alphanum (string s) {
  int i;
  if (N (s) == 0) return false;
  for (i= 0; i < N (s); i++)
    if (!(is_alpha (s[i]) || is_digit (s[i]))) return false;
  return true;
}

bool
is_locase_alpha (string s) {
  int i;
  if (N (s) == 0) return false;
  for (i= 0; i < N (s); i++)
    if (s[i] < 'a' || s[i] > 'z') return false;
  return true;
}

bool
is_iso_alpha (string s) {
  int i;
  if (N (s) == 0) return false;
  for (i= 0; i < N (s); i++)
    if (!is_iso_alpha (s[i])) return false;
  return true;
}

bool
is_numeric (string s) {
  int i;
  if (N (s) == 0) return false;
  for (i= 0; i < N (s); i++)
    if (!is_numeric (s[i])) return false;
  return true;
}

/******************************************************************************
 * Changing cases
 ******************************************************************************/

char
upcase (char c) {
  if (is_iso_locase (c)) return (char) (((int) ((unsigned char) c)) - 32);
  else return c;
}

char
locase (char c) {
  if (is_iso_upcase (c)) return (char) (((int) ((unsigned char) c)) + 32);
  else return c;
}

char
closing_delimiter (char c) {
  if (c == '{') return '}';
  if (c == '(') return ')';
  if (c == '[') return ']';
  return c;
}

string
upcase_first (string s) {
  if ((N (s) == 0) || (!is_iso_locase (s[0]))) return s;
  return string ((char) (((int) ((unsigned char) s[0])) - 32)) * s (1, N (s));
}

string
locase_first (string s) {
  if ((N (s) == 0) || (!is_iso_upcase (s[0]))) return s;
  return string ((char) (((int) ((unsigned char) s[0])) + 32)) * s (1, N (s));
}

string
upcase_all (string s) {
  int    i;
  string r (N (s));
  for (i= 0; i < N (s); i++)
    if (!is_iso_locase (s[i])) r[i]= s[i];
    else r[i]= (char) (((int) ((unsigned char) s[i])) - 32);
  return r;
}

string
locase_all (string s) {
  int    i;
  string r (N (s));
  for (i= 0; i < N (s); i++)
    if (!is_iso_upcase (s[i])) r[i]= s[i];
    else r[i]= (char) (((int) ((unsigned char) s[i])) + 32);
  return r;
}

/******************************************************************************
 * Inserting or removing a character into a string as a set of characters
 ******************************************************************************/

string
string_union (string s1, string s2) {
  return string_minus (s1, s2) * s2;
}

string
string_minus (string s1, string s2) {
  string r;
  int    i1, n1= N (s1), i2, n2= N (s2);
  for (i1= 0; i1 < n1; i1++) {
    for (i2= 0; i2 < n2; i2++)
      if (s1[i1] == s2[i2]) break;
    if (i2 == n2) r << s1[i1];
  }
  return r;
}

string
remove_prefix (string s, string prefix) {
  if (is_empty (s) || is_empty (prefix)) return s;
  if (starts (s, prefix)) return s (N (prefix), N (s));
  return s;
}

string
remove_suffix (string s, string suffix) {
  if (is_empty (s) || is_empty (suffix)) return s;
  if (ends (s, suffix)) return s (0, N (s) - N (suffix));
  return s;
}

/******************************************************************************
 * Spanish in relation with ispell
 ******************************************************************************/

string
ispanish_to_spanish (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if ((s[i] == '\'') && ((i + 1) < n)) {
      switch (s[i + 1]) {
      case 'A':
        r << '\301';
        break;
      case 'E':
        r << '\311';
        break;
      case 'I':
        r << '\315';
        break;
      case 'N':
        r << '\321';
        break;
      case 'O':
        r << '\323';
        break;
      case 'U':
        r << '\332';
        break;
      case 'Y':
        r << '\335';
        break;
      case 'a':
        r << '\341';
        break;
      case 'e':
        r << '\351';
        break;
      case 'i':
        r << '\355';
        break;
      case 'n':
        r << '\361';
        break;
      case 'o':
        r << '\363';
        break;
      case 'u':
        r << '\372';
        break;
      case 'y':
        r << '\375';
        break;
      default:
        r << '\'' << s[i + 1];
      }
      i++;
    }
    else r << s[i];
  return r;
}

string
spanish_to_ispanish (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    switch (s[i]) {
    case '\301':
      r << "'A";
      break;
    case '\311':
      r << "'E";
      break;
    case '\315':
      r << "'I";
      break;
    case '\321':
      r << "'N";
      break;
    case '\323':
      r << "'O";
      break;
    case '\332':
      r << "'U";
      break;
    case '\335':
      r << "'Y";
      break;
    case '\341':
      r << "'a";
      break;
    case '\351':
      r << "'e";
      break;
    case '\355':
      r << "'i";
      break;
    case '\361':
      r << "'n";
      break;
    case '\363':
      r << "'o";
      break;
    case '\372':
      r << "'u";
      break;
    case '\375':
      r << "'y";
      break;
    default:
      r << s[i];
    }
  return r;
}

string
igerman_to_german (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if (s[i] == '\337') r << '\377';
    else r << s[i];
  return r;
}

string
german_to_igerman (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if (s[i] == '\377') r << '\337';
    else r << s[i];
  return r;
}

/******************************************************************************
 * Iso latin 2 encoding for polish and czech
 ******************************************************************************/

static string il2_to_cork_string=
    "\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221"
    "\222\223\224\225\226\227\230\231\232\233\234\235\236\237 \20\212 "
    "\211\221\237¨\222\223\224\231‐\232\233 "
    "\241˛\252´\251\261ˇ¸\262\263\264\271˝\272\273\217\301\302\200\304\210\202"
    "\307\203\311\206\313\205\315\316\204\320\213\214\323\324\216\326."
    "\220\227\332\226\334\335\225\377\257\341\342\240\344\250\242\347\243\351"
    "\246\353\245\355\356\244\236\253\254\363\364\256\366/"
    "\260\267\372\266\374\375\265 ";
static string cork_to_il2_string=
    "\303\241\306\310\317\314\312G\305\245\243\321\322 "
    "\325\300\330\246\251\252\253\336\333\331Y\254\256\257II\360\247\343\261"
    "\346\350\357\354\352g\345\265\263\361\362 "
    "\365\340\370\266\271\272\273\376\373\371y\274\276\277i!?"
    "LA\301\302A\304AA\307E\311E\313I\315\316I\320NO\323\324O\326OOU\332U\334"
    "\335 "
    "Sa\341\342a\344aa\347e\351e\353i\355\356i\360no\363\364o\366oou\372u\374"
    "\375 \337";

static char
il2_to_cork (char c) {
  int i= (int) ((unsigned char) c);
  if (i < 128) return c;
  return il2_to_cork_string[i - 128];
}

static char
cork_to_il2 (char c) {
  int i= (int) ((unsigned char) c);
  if (i < 128) return c;
  return cork_to_il2_string[i - 128];
}

string
il2_to_cork (string s) {
  int    i, n= N (s);
  string r (n);
  for (i= 0; i < n; i++)
    r[i]= il2_to_cork (s[i]);
  return r;
}

string
cork_to_il2 (string s) {
  int    i, n= N (s);
  string r (n);
  for (i= 0; i < n; i++)
    r[i]= cork_to_il2 (s[i]);
  return r;
}

/******************************************************************************
 * Roman and alpha numbers
 ******************************************************************************/

string
alpha_nr (int nr) {
  if (nr < 0) return "-" * alpha_nr (-nr);
  if (nr == 0) return "0";
  if (nr <= 26) return string ((char) (((int) 'a') + nr - 1));
  return alpha_nr ((nr - 1) / 26) * alpha_nr (((nr - 1) % 26) + 1);
}

string
Alpha_nr (int nr) {
  return upcase_all (alpha_nr (nr));
}

string
fnsymbol_nr (int nr) {
  if (nr < 0) nr= -nr;
  string sym, r;
  int    i, m= (nr - 1) % 6, n= ((nr - 1) / 6) + 1;
  switch (m) {
  case 0:
    sym= "<asterisk>";
    break;
  case 1:
    sym= "<dag>";
    break;
  case 2:
    sym= "<ddag>";
    break;
  case 3:
    sym= "<paragraph>";
    break;
  case 4:
    sym= "<endofline>";
    break;
  case 5:
    sym= "||";
    break;
  }
  for (i= 0; i < n; i++)
    r << sym;
  return r;
}

string
raw_quote (string s) {
  // Mark the label of a STRING tree as representing a string and not a symbol.
  return "\"" * s * "\"";
}

string
raw_unquote (string s) {
  // Get the string value of a STRING tree label representing a string.
  if (is_quoted (s)) return s (1, N (s) - 1);
  else return s;
}

/******************************************************************************
 * Handling escape characters
 ******************************************************************************/

string
escape_sh (string s) {
#if (defined OS_MINGW || defined OS_WIN)
  return raw_quote (s);
#else
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    switch (s[i]) {
    case '(':
    case ')':
    case '<':
    case '>':
    case '?':
    case '&':
    case '$':
    case '`':
    case '\"':
    case '\\':
    case ' ':
      r << '\\' << s[i];
      break;
    case '\n':
      r << "\\n";
      break;
    default:
      r << s[i];
    }
  return r;
#endif
}

string
escape_generic (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++) {
    if ((s[i] == '\2') || (s[i] == '\5') || (s[i] == '\33')) r << '\33';
    r << s[i];
  }
  return r;
}

string
escape_verbatim (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++) {
    unsigned char c= (unsigned char) s[i];
    if ((c == '\n') || (c == '\t')) r << ' ';
    else if (((int) c) >= 32) r << s[i];
  }
  return r;
}

string
escape_spaces (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++) {
    unsigned char c= (unsigned char) s[i];
    if (c == ' ') r << '\\';
    r << c;
  }
  return r;
}

string
dos_to_better (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if (s[i] == '\015')
      ;
    else r << s[i];
  return r;
}

string
unescape_guile (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++) {
    if (s[i] == '\\') {
      if (i + 1 < n && s[i + 1] == '\\') {
        r << "\\\\\\\\";
        i+= 1;
      }
      else if (i + 3 < n && s[i + 1] == 'x' && is_hex_digit (s[i + 2]) &&
               is_hex_digit (s[i + 3])) {
        string e= s (i + 2, i + 4);
        r << (unsigned char) lolly::data::from_hex (e);
        i+= 3;
      }
      else r << s[i];
    }
    else r << s[i];
  }
  return r;
}

/******************************************************************************
 * Reading input from a string
 ******************************************************************************/

bool
test (string s, int i, const char* test) {
  int n= N (s), j= 0;
  while (test[j] != '\0') {
    if (i >= n) return false;
    if (s[i] != test[j]) return false;
    i++;
    j++;
  }
  return true;
}

bool
test (string s, int i, string test) {
  int n= N (s), m= N (test), j= 0;
  while (j < m) {
    if (i >= n) return false;
    if (s[i] != test[j]) return false;
    i++;
    j++;
  }
  return true;
}

bool
starts (string s, const char* what) {
  return test (s, 0, what);
}

bool
starts (string s, const string what) {
  return test (s, 0, what);
}

bool
ends (string s, const char* what) {
  string r (what);
  if (N (r) > N (s)) return false;
  return s (N (s) - N (r), N (s)) == r;
}

bool
ends (string s, const string r) {
  if (N (r) > N (s)) return false;
  return s (N (s) - N (r), N (s)) == r;
}

bool
read (string s, int& i, const char* test) {
  int n= N (s), j= 0, k= i;
  while (test[j] != '\0') {
    if (k >= n) return false;
    if (s[k] != test[j]) return false;
    j++;
    k++;
  }
  i= k;
  return true;
}

bool
read (string s, string test) {
  int i= 0;
  return read (s, i, test);
}

bool
read (string s, int& i, string test) {
  int n= N (s), m= N (test), j= 0, k= i;
  while (j < m) {
    if (k >= n) return false;
    if (s[k] != test[j]) return false;
    j++;
    k++;
  }
  i= k;
  return true;
}

bool
read_line (string s, int& i, string& result) {
  int start= i;
  for (; i < N (s); i++) {
    if (s[i] == '\n') {
      result= s (start, i++);
      return true;
    }
  }
  result= s (start, i);
  return false;
}

bool
read_int (string s, int& i, int& result) {
  int n= N (s), start= i;
  result= 0;
  if (i == n) return false;
  if (s[i] == '-') {
    if (i + 1 == n) return false;
    if (!is_digit (s[i + 1])) return false;
    i++;
  }
  else if (!is_digit (s[i])) return false;
  while ((i < n) && is_digit (s[i]))
    i++;
  result= as_int (s (start, i));
  return true;
}

bool
read_double (string s, int& i, double& result) {
  int n= N (s), start= i;
  result= 0.0;
  if (i == n) return false;
  if (s[i] == '-') {
    if (i + 1 == n) return false;
    if (!is_numeric (s[i + 1])) return false;
    i++;
  }
  else if (!is_numeric (s[i])) return false;
  while ((i < n) && is_digit (s[i]))
    i++;
  if ((i < n) && (s[i] == '.')) i++;
  while ((i < n) && is_digit (s[i]))
    i++;
  if ((i < n) && ((s[i] == 'e') || (s[i] == 'E'))) {
    i++;
    if ((i < n) && (s[i] == '-')) i++;
    if ((i == n) || (!is_digit (s[i]))) {
      i= start;
      return false;
    }
    while ((i < n) && is_digit (s[i]))
      i++;
  }
  result= as_double (s (start, i));
  return true;
}

bool
read_word (string s, int& i, string& result) {
  int opos= i;
  while (i < N (s) && is_alpha (s[i])) {
    i++;
  }
  result= s (opos, i);
  return i > opos;
}

bool
is_whitespace (string s) {
  for (int i= 0; i < N (s); i++)
    if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n') return false;
  return true;
}

void
skip_spaces (string s, int& i) {
  int n= N (s);
  while ((i < n) && ((s[i] == ' ') || (s[i] == '\t')))
    i++;
}

void
skip_whitespace (string s, int& i) {
  int n= N (s);
  while ((i < n) && ((s[i] == ' ') || (s[i] == '\t') || (s[i] == '\n')))
    i++;
}

void
skip_line (string s, int& i) {
  int n= N (s);
  while ((i < n) && (s[i] != '\n'))
    i++;
  if (i < n) i++;
}

void
skip_symbol (string s, int& i) {
  int n= N (s);
  if (i < n) {
    if (s[i] == '<') {
      for (i++; i < n; i++)
        if (s[i - 1] == '>') break;
    }
    else i++;
  }
}

string
convert_tabs_to_spaces (string s, int tw) {
  int    i= 0, ts= 0, n= N (s);
  string r= "";
  while (i < n) {
    if (s[i] == '\t') {
      r << string (' ', tw - ((i - ts) % tw));
      ts= i + 1;
    }
    else if (s[i] == '\n') {
      ts= i + 1;
      r << s[i];
    }
    else r << s[i];
    i++;
  }
  return r;
}

/******************************************************************************
 * Parsing binary data
 ******************************************************************************/

void
parse (string s, int& pos, QI& ret) {
  ret= (QI) s[pos++];
}

void
parse (string s, int& pos, QN& ret) {
  ret= (QN) s[pos++];
}

void
parse (string s, int& pos, HI& ret) {
  QI c1= (QI) s[pos++];
  QN c2= (QN) s[pos++];
  ret  = (((HI) c1) << 8) + c2;
}

void
parse (string s, int& pos, HN& ret) {
  QN c1= (QN) s[pos++];
  QN c2= (QN) s[pos++];
  ret  = (((HN) c1) << 8) + c2;
}

void
parse (string s, int& pos, SI& ret) {
  QI c1= (QI) s[pos++];
  QN c2= (QN) s[pos++];
  QN c3= (QN) s[pos++];
  QN c4= (QN) s[pos++];
  ret  = (((((((SI) c1) << 8) + ((SI) c2)) << 8) + ((SI) c3)) << 8) + c4;
}

void
parse (string s, int& pos, SI*& a, int len) {
  int i;
  a= tm_new_array<int> (len);
  for (i= 0; i < len; i++)
    parse (s, pos, a[i]);
}

/******************************************************************************
 * Searching, replacing and pattern matching
 ******************************************************************************/

int
index_of (string s, char c) {
  int s_N= N (s);
  for (int i= 0; i < s_N; i++) {
    if (s[i] == c) {
      return i;
    }
  }
  return -1;
}

int
search_forwards (array<string> a, int pos, string in) {
  int n= N (in), na= N (a);
  while (pos <= n) {
    for (int i= 0; i < na; i++)
      if (N (a[i]) > 0 && in[pos] == a[i][0] && test (in, pos, a[i]))
        return pos;
    pos++;
  }
  return -1;
}

int
search_forwards (string s, int pos, string in) {
  int k= N (s), n= N (in);
  if (k == 0) return pos;
  char c= s[0];
  while (pos + k <= n) {
    if (in[pos] == c && test (in, pos, s)) return pos;
    pos++;
  }
  return -1;
}

int
search_forwards (string s, string in) {
  return search_forwards (s, 0, in);
}

bool
occurs (string what, string in) {
  return search_forwards (what, 0, in) >= 0;
}

bool
contains (string s, string what) {
  return search_forwards (what, 0, s) >= 0;
}

bool
contains (string s, char c) {
  int s_N= N (s);
  for (int i= 0; i < s_N; i++) {
    if (s[i] == c) {
      return true;
    }
  }
  return false;
}

int
search_backwards (string s, int pos, string in) {
  while (pos >= 0) {
    if (test (in, pos, s)) return pos;
    pos--;
  }
  return -1;
}

int
search_backwards (string s, string in) {
  return search_backwards (s, N (in) - N (s), in);
}

int
count_occurrences (string s, string in) {
  int count= 0;
  int i= 0, next, n= N (in);
  while (i < n) {
    next= search_forwards (s, i, in);
    if (next == -1) break;
    count++;
    i= next + 1;
  }
  return count;
}

int
overlapping (string s1, string s2) {
  // return the longuest string being suffix of s1 and prefix of s2
  int i= min (N (s1), N (s2)), n= N (s1);
  while (i > 0) {
    if (s1 (n - i, n) == s2 (0, i)) return i;
    i--;
  }
  return 0;
}

string
replace (string s, string what, string by) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n;)
    if (test (s, i, what)) {
      r << by;
      i+= N (what);
    }
    else {
      r << s[i];
      i++;
    }
  return r;
}

static bool
match_wildcard (string s, int spos, string w, int wpos) {
  if (wpos == N (w)) return spos == N (s);
  if (w[wpos] != '*')
    return (spos < N (s)) && (s[spos] == w[wpos]) &&
           match_wildcard (s, spos + 1, w, wpos + 1);
  while ((wpos < N (w)) && (w[wpos] == '*'))
    wpos++;
  while (spos <= N (s)) {
    if (match_wildcard (s, spos, w, wpos)) return true;
    spos++;
  }
  return false;
}

bool
match_wildcard (string s, string w) {
  return match_wildcard (s, 0, w, 0);
}

int
find_non_alpha (string s, int pos, bool forward) {
  if (forward) {
    for (; pos < N (s); pos++)
      if (!is_alpha (s[pos])) return pos;
  }
  else {
    for (; pos > 0; pos--)
      if (!is_alpha (s[pos - 1])) return pos - 1;
  }
  return -1;
}

array<string>
tokenize (string s, string sep) {
  int           start= 0;
  array<string> a;
  for (int i= 0; i < N (s);)
    if (test (s, i, sep)) {
      a << s (start, i);
      i+= N (sep);
      start= i;
    }
    else i++;
  a << s (start, N (s));
  return a;
}

string
recompose (array<string> a, string sep) {
  string r;
  for (int i= 0; i < N (a); i++) {
    if (i != 0) r << sep;
    r << a[i];
  }
  return r;
}

string
trim_spaces_left (string s) {
  int start;
  for (start= 0; start < N (s) && is_space (s[start]); start++)
    ;
  return s (start, N (s));
}

string
trim_spaces_right (string s) {
  int end;
  for (end= N (s) - 1; end >= 0 && is_space (s[end]); end--)
    ;
  return s (0, end + 1);
}

string
trim_spaces (string s) {
  return trim_spaces_left (trim_spaces_right (s));
}

array<string>
trim_spaces (array<string> a) {
  array<string> b (N (a));
  for (int i= 0; i < N (a); i++)
    b[i]= trim_spaces (a[i]);
  return b;
}

/******************************************************************************
 * Differences between two strings
 ******************************************************************************/

static int
find_longest (string s1, string s2, int& c1, int& c2) {
  int n1= N (s1), n2= N (s2), bc= 0, bl= 0, br= 0;
  for (c2= 0; c2 < n2; c2++)
    if (s1[c1] == s2[c2]) {
      int l= 0, r= 0;
      while (c1 + r < n1 && c2 + r < n2 && s1[c1 + r] == s2[c2 + r])
        r++;
      while (l < c1 && l < c2 && s1[c1 - l - 1] == s2[c2 - l - 1])
        l++;
      if (l + r > bl + br) {
        bc= c2;
        bl= l;
        br= r;
      }
    }
  if (bl + br > 0) {
    c1= c1 - bl;
    c2= bc - bl;
  }
  return bl + br;
}

static void
find_common (string s1, string s2, int& c1, int& c2) {
  int best_len= 0;
  c1= c2= 0;
  int n1= N (s1), n2= N (s2);
  if (n1 == 0 || n2 == 0) return;
  int t= min (min (n1, n2), 6);
  for (int k= 1; k < t; k++) {
    int a1= (k * n1) / t, a2= (k * n2) / t;
    int len= find_longest (s1, s2, a1, a2);
    if (len > best_len) {
      best_len= len;
      c1      = a1;
      c2      = a2;
    }
  }
}

array<int>
differences (string s1, string s2) {
  int n1= N (s1), n2= N (s2);
  int i1= 0, i2= 0, j1= n1, j2= n2;
  while (i1 < j1 && i2 < j2 && s1[i1] == s2[i2]) {
    i1++;
    i2++;
  }
  while (i1 < j1 && i2 < j2 && s1[j1 - 1] == s2[j2 - 1]) {
    j1--;
    j2--;
  }
  if (i1 == i2 && j1 == j2) return array<int> ();
  if (i1 > 0 || i2 > 0 || j1 < n1 || j2 < n2) {
    array<int> r= differences (s1 (i1, j1), s2 (i2, j2));
    for (int k= 0; k < N (r); k+= 4) {
      r[k]+= i1;
      r[k + 1]+= i1;
      r[k + 2]+= i2;
      r[k + 3]+= i2;
    }
    return r;
  }
  else {
    int c1, c2;
    find_common (s1, s2, c1, c2);
    if (c1 == 0 && c2 == 0) {
      array<int> r;
      r << i1 << j1 << i2 << j2;
      return r;
    }
    else {
      array<int> r1= differences (s1 (0, c1), s2 (0, c2));
      array<int> r2= differences (s1 (c1, n1), s2 (c2, n2));
      for (int k= 0; k < N (r2); k+= 4) {
        r2[k]+= c1;
        r2[k + 1]+= c1;
        r2[k + 2]+= c2;
        r2[k + 3]+= c2;
      }
      r1 << r2;
      return r1;
    }
  }
}

int
distance (string s1, string s2) {
  int        d= 0;
  array<int> r= differences (s1, s2);
  for (int k= 0; k < N (r); k+= 4)
    d+= max (r[k + 1] - r[k], r[k + 3] - r[k + 2]);
  return d;
}

/******************************************************************************
 * Parse length
 ******************************************************************************/

void
parse_length (string s, double& len, string& unit) {
  int start= 0;
  int i, n= N (s);
  for (i= start; i < n && !is_locase (s[i]); i++) {
  }
  string s1= s (start, i);
  string s2= s (i, n);
  if (is_double (s1) && (is_locase_alpha (s2) || is_empty (s2))) {
    len = as_double (s1);
    unit= s2;
  }
  else {
    len = 0.0;
    unit= "error";
  }
}

/******************************************************************************
 * Fuzzy matching score (VSCode-style algorithm)
 ******************************************************************************/

// VSCode-style scoring constants
static const int NO_MATCH                   = 0;
static const int NO_SCORE                   = -1;
static const int BASE_MATCH_SCORE           = 1;
static const int CONSECUTIVE_BONUS_BASE     = 6;
static const int CONSECUTIVE_BONUS_REMAINDER= 3;
static const int SAME_CASE_BONUS            = 1;
static const int START_OF_WORD_BONUS        = 8;
static const int AFTER_SEPARATOR_BONUS_HIGH = 5; // path separators
static const int AFTER_SEPARATOR_BONUS_LOW  = 4; // other separators
static const int CAMEL_CASE_BONUS           = 2;

static bool
consider_as_equal (char a, char b) {
  if (a == b) return true;
  // Special case path separators
  if ((a == '/' || a == '\\') && (b == '/' || b == '\\')) return true;
  return false;
}

static int
score_separator_at_pos (char prev_char) {
  switch (prev_char) {
  case '/':
  case '\\':
    return AFTER_SEPARATOR_BONUS_HIGH; // prefer path separators
  case '_':
  case '-':
  case '.':
  case ' ':
  case '\'':
  case '"':
  case ':':
    return AFTER_SEPARATOR_BONUS_LOW; // other separators
  default:
    return 0;
  }
}

static int
compute_char_score (char query_char, char query_lower_char, string target,
                    string target_lower, int target_index,
                    int matches_sequence_length) {
  int score= 0;

  if (!consider_as_equal (query_lower_char, target_lower[target_index])) {
    return score; // no match of characters
  }

  // Character match bonus
  score+= BASE_MATCH_SCORE;

  // Consecutive match bonus
  if (matches_sequence_length > 0) {
    score+=
        (min (matches_sequence_length, 3) * CONSECUTIVE_BONUS_BASE) +
        (max (0, matches_sequence_length - 3) * CONSECUTIVE_BONUS_REMAINDER);
  }

  // Same case bonus
  if (query_char == target[target_index]) {
    score+= SAME_CASE_BONUS;
  }

  // Start of word bonus
  if (target_index == 0) {
    score+= START_OF_WORD_BONUS;
  }
  else {
    // After separator bonus
    int separator_bonus= score_separator_at_pos (target[target_index - 1]);
    if (separator_bonus) {
      score+= separator_bonus;
    }
    // Camel case bonus
    else if (is_upcase (target[target_index]) && matches_sequence_length == 0) {
      score+= CAMEL_CASE_BONUS;
    }
  }

  return score;
}

int
fuzzy_match_score (string pattern, string target) {
  int pattern_len= N (pattern);
  int target_len = N (target);

  // Early exit conditions
  if (pattern_len == 0) return 0;
  if (target_len == 0 || pattern_len > target_len) return NO_SCORE;

  // Convert to lowercase
  string pattern_lower= locase_all (pattern);
  string target_lower = locase_all (target);

  // Build scorer matrix using dynamic programming
  array<int> scores (pattern_len * target_len);
  array<int> matches (pattern_len * target_len);

  // Initialize matrices
  for (int i= 0; i < pattern_len * target_len; i++) {
    scores[i] = 0;
    matches[i]= NO_MATCH;
  }

  // Fill matrix
  for (int query_idx= 0; query_idx < pattern_len; query_idx++) {
    int query_idx_offset= query_idx * target_len;
    int prev_offset     = query_idx_offset - target_len;

    char query_char      = pattern[query_idx];
    char query_lower_char= pattern_lower[query_idx];
    bool first_char      = (query_idx == 0);

    for (int target_idx= 0; target_idx < target_len; target_idx++) {
      int current= query_idx_offset + target_idx;

      // Left (previous target char)
      int left_score= (target_idx > 0) ? scores[current - 1] : 0;

      // Diagonal (previous char in both)
      int diag_score= 0;
      int seq_length= 0;
      if (query_idx > 0 && target_idx > 0) {
        diag_score= scores[prev_offset + target_idx - 1];
        seq_length= matches[prev_offset + target_idx - 1];
      }

      // Compute current char score
      int char_score= 0;
      if (diag_score > 0 || first_char) {
        char_score= compute_char_score (query_char, query_lower_char, target,
                                        target_lower, target_idx, seq_length);
      }

      // Check if we can use this match
      bool use_match=
          (char_score > 0) && (diag_score + char_score >= left_score);
      if (use_match) {
        scores[current] = diag_score + char_score;
        matches[current]= seq_length + 1;
      }
      else {
        scores[current] = left_score;
        matches[current]= NO_MATCH;
      }
    }
  }

  // Find best score in last row
  int best_score= 0;
  int last_row  = (pattern_len - 1) * target_len;
  for (int j= 0; j < target_len; j++) {
    if (scores[last_row + j] > best_score) {
      best_score= scores[last_row + j];
    }
  }

  // return best_score > 0 ? best_score : NO_SCORE;
  return best_score;
}
