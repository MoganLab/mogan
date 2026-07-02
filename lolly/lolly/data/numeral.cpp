
/******************************************************************************
 * MODULE     : numberal.hpp
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2013  Francois Poulain
 *                  2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "numeral.hpp"
#include "analyze.hpp"
#include "unicode.hpp"

namespace lolly {
namespace data {
static const string roman_ones[10]    = {"",  "i",  "ii",  "iii",  "iv",
                                         "v", "vi", "vii", "viii", "ix"};
static const string roman_tens[10]    = {"",  "x",  "xx",  "xxx",  "xl",
                                         "l", "lx", "lxx", "lxxx", "xc"};
static const string roman_hundreds[10]= {"",  "c",  "cc",  "ccc",  "cd",
                                         "d", "dc", "dcc", "dccc", "cm"};
static const string roman_thousands[4]= {"", "m", "mm", "mmm"};

// 0 should not be used as index of this array, or bug occurs. because digit 0
// is handled specially according to position of digit.
static const string chars_han[10]= {"?",  "一", "二", "三", "四",
                                    "五", "六", "七", "八", "九"};

static const char* hex_string= "0123456789ABCDEF";

string
to_roman (int32_t nr) {
  if (nr == 0) return "o";
  if (nr > 3999 || nr < -3999) return "?";
  if (nr < 0) return "-" * to_roman (-nr);
  return roman_thousands[(nr / 1000) % 10] * roman_hundreds[(nr / 100) % 10] *
         roman_tens[(nr / 10) % 10] * roman_ones[nr % 10];
}

string
to_Roman (int32_t nr) {
  return upcase_all (to_roman (nr));
}

static string
to_circle_positive (uint64_t nr) {
  if (nr == 0) return encode_as_utf8 (0x24EA);              // ⓪
  if (nr <= 20) return encode_as_utf8 (0x2460 + (nr - 1));  // ①-⑳
  if (nr <= 35) return encode_as_utf8 (0x3251 + (nr - 21)); // ㉑-㉟
  if (nr <= 50) return encode_as_utf8 (0x32B1 + (nr - 36)); // ㊱-㊿
  return as_string ((int64_t) nr);
}

string
to_circle (int32_t nr) {
  if (nr < 0) {
    uint64_t positive= (uint64_t) (-((int64_t) nr));
    return "-" * to_circle_positive (positive);
  }
  return to_circle_positive ((uint64_t) nr);
}

string
hanzi_sub (int16_t nr, bool leading_zero) {
  short thousand= (nr % 10000) / 1000, hundred= (nr % 1000) / 100,
        ten= (nr % 100) / 10, one= nr % 10;
  short cases= (leading_zero << 4) | ((thousand == 0) << 3) |
               ((hundred == 0) << 2) | ((ten == 0) << 1) | (one == 0);
  switch (cases) {
  case 0x0:
  case 0x10:
    return chars_han[thousand] * "千" * chars_han[hundred] * "百" *
           chars_han[ten] * "十" * chars_han[one];
  case 0x1:
  case 0x11:
    return chars_han[thousand] * "千" * chars_han[hundred] * "百" *
           chars_han[ten] * "十";
  case 0x2:
  case 0x12:
    return chars_han[thousand] * "千" * chars_han[hundred] * "百零" *
           chars_han[one];
  case 0x3:
  case 0x13:
    return chars_han[thousand] * "千" * chars_han[hundred] * "百";
  case 0x4:
  case 0x14:
    return chars_han[thousand] * "千零" * chars_han[ten] * "十" *
           chars_han[one];
  case 0x5:
  case 0x15:
    return chars_han[thousand] * "千零" * chars_han[ten] * "十";
  case 0x6:
  case 0x16:
    return chars_han[thousand] * "千零" * chars_han[one];
  case 0x7:
  case 0x17:
    return chars_han[thousand] * "千";
  case 0x8:
    return chars_han[hundred] * "百" * chars_han[ten] * "十" * chars_han[one];
  case 0x18:
    return "零" * chars_han[hundred] * "百" * chars_han[ten] * "十" *
           chars_han[one];
  case 0x9:
    return chars_han[hundred] * "百" * chars_han[ten] * "十";
  case 0x19:
    return "零" * chars_han[hundred] * "百" * chars_han[ten] * "十";
  case 0xA:
    return chars_han[hundred] * "百零" * chars_han[one];
  case 0x1A:
    return "零" * chars_han[hundred] * "百零" * chars_han[one];
  case 0xB:
    return chars_han[hundred] * "百";
  case 0x1B:
    return "零" * chars_han[hundred] * "百";
  case 0xC:
    if (ten == 1) {
      return "十" * chars_han[one];
    }
    else {
      return chars_han[ten] * "十" * chars_han[one];
    }
  case 0x1C:
    return "零" * chars_han[ten] * "十" * chars_han[one];
  case 0xD:
    if (ten == 1) {
      return "十";
    }
    else {
      return chars_han[ten] * "十";
    }
  case 0x1D:
    return "零" * chars_han[ten] * "十";
  case 0xE:
    return chars_han[one];
  case 0x1E:
    return "零" * chars_han[one];
  case 0xF:
  case 0x1F:
    return "";
  default:
    return "?" * as_string (cases);
  }
}

string
to_hanzi (int32_t nr) {
  if (nr == 0) return "零";
  if (nr == 0x80000000) return "负二十一亿四千七百四十八万三千六百四十八";
  if (nr < 0) return "负" * to_hanzi (-nr);
  if (nr >= 100000000) {
    return hanzi_sub (nr / 100000000, false) * "亿" *
           hanzi_sub ((nr / 10000) % 10000, true) * "万" *
           hanzi_sub (nr % 10000, true);
  }
  if (nr >= 10000) {
    return hanzi_sub (nr / 10000, false) * "万" * hanzi_sub (nr % 10000, true);
  }
  return hanzi_sub (nr, false);
}

string
to_padded_Hex (uint8_t i) {
  uint8_t i_low = i & 15;
  uint8_t i_high= i >> 4;
  return string (hex_string[i_high]) * string (hex_string[i_low]);
}

string
to_padded_hex (uint8_t i) {
  return locase_all (to_padded_Hex (i));
}

/**
 * @brief Handle positive number separately to avoid unnecessary check of sign.
 * string is passed into the function as reference, thus no reference counting
 * is performed.
 * @tparam T unsigned integral type is expected.
 */
template <typename T>
std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>,
                 void>
to_Hex_positive (T i, string& s) {
  if (i >= 16) {
    to_Hex_positive (i >> 4, s);
  }
  s << hex_string[i & 15];
}

string
to_Hex (int32_t i) {
  if (i == INT32_MIN) return "-80000000";
  if (i < 0) {
    string result ("-");
    to_Hex_positive ((uint32_t) (-i), result);
    return result;
  }
  else {
    string result;
    to_Hex_positive ((uint32_t) (i), result);
    return result;
  };
}

string
to_hex (int32_t i) {
  return locase_all (to_Hex (i));
}

string
to_Hex (pointer ptr) {
  intptr_t i= (intptr_t) ptr;
  if (i < 0) {
    string result ("-");
    to_Hex_positive ((uintptr_t) (-i), result);
    return result;
  }
  else {
    string result;
    to_Hex_positive ((uintptr_t) (i), result);
    return result;
  };
}

string
to_hex (pointer ptr) {
  return locase_all (to_Hex (ptr));
}

int
from_hex (string s) {
  int i, n= N (s), res= 0;
  if ((n > 0) && (s[0] == '-')) return -from_hex (s (1, n));
  for (i= 0; i < n; i++) {
    res= res << 4;
    if (is_digit (s[i])) res+= (int) (s[i] - '0');
    if ((s[i] >= 'A') && (s[i] <= 'F')) res+= (int) (s[i] + 10 - 'A');
    if ((s[i] >= 'a') && (s[i] <= 'f')) res+= (int) (s[i] + 10 - 'a');
  }
  return res;
}

/**
 * @brief Handle positive number separately to avoid unnecessary check of sign.
 * string is passed into the function as reference, thus no reference counting
 * is performed.
 * Because length of string s is known here, use index instead of appending
 * operator can avoid cost of reallocation. Firstly a string of given length is
 * constructed, then digits is filled according to index rather than appending
 * to the tail.
 * @tparam T unsigned integral type is expected.
 */
template <unsigned int cur, typename T>
std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>,
                 void>
as_hexadecimal_sub (T i, string& s) {
  if constexpr (cur > 0) {
    as_hexadecimal_sub<cur - 1> (i >> 4, s);
  }
  s[cur]= hex_string[i & 15];
}

string
as_hexadecimal (int i, int len) {
  string result (len);
  switch (len) {
  case 1:
    as_hexadecimal_sub<0> ((unsigned int) i, result);
    break;
  case 2:
    as_hexadecimal_sub<1> ((unsigned int) i, result);
    break;
  case 3:
    as_hexadecimal_sub<2> ((unsigned int) i, result);
    break;
  case 4:
    as_hexadecimal_sub<3> ((unsigned int) i, result);
    break;
  case 5:
    as_hexadecimal_sub<4> ((unsigned int) i, result);
    break;
  case 6:
    as_hexadecimal_sub<5> ((unsigned int) i, result);
    break;
  case 7:
    as_hexadecimal_sub<6> ((unsigned int) i, result);
    break;
  case 8:
    as_hexadecimal_sub<7> ((unsigned int) i, result);
    break;
  case 9:
    as_hexadecimal_sub<8> ((unsigned int) i, result);
    break;
  case 10:
    as_hexadecimal_sub<9> ((unsigned int) i, result);
    break;
  case 11:
    as_hexadecimal_sub<10> ((unsigned int) i, result);
    break;
  case 12:
    as_hexadecimal_sub<11> ((unsigned int) i, result);
    break;
  case 13:
    as_hexadecimal_sub<12> ((unsigned int) i, result);
    break;
  case 14:
    as_hexadecimal_sub<13> ((unsigned int) i, result);
    break;
  case 15:
    as_hexadecimal_sub<14> ((unsigned int) i, result);
    break;
  case 16:
    as_hexadecimal_sub<15> ((unsigned int) i, result);
    break;
  default:
    TM_FAILED ("len is too large");
    break;
  }
  return result;
}

string
uint32_to_Hex (uint32_t i) {
  string result;
  to_Hex_positive (i, result);
  return result;
}

string
binary_to_hexadecimal (string bin) {
  string res ((int) (N (bin) * 2));
  int    cur= 0;
  for (unsigned char ch : bin) {
    res[cur]    = hex_string[ch >> 4];
    res[cur + 1]= hex_string[ch & 0x0f];
    cur+= 2;
  }
  return res;
}

} // namespace data
} // namespace lolly
