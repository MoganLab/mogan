
/******************************************************************************
 * MODULE     : json.cpp
 * DESCRIPTION: Json Data Type
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "unicode.hpp"
#include "tbox/tbox.h"

namespace lolly {
namespace data {

string_u8
encode_as_utf8 (uint32_t code) {
  if (/* 0x0 <= code && */ code <= 0x7F) {
    // 0x0ddddddd
    return string ((char) code);
  }
  else if (0x80 <= code && code <= 0x7FF) {
    // 0x110ddddd 0x10dddddd
    string str (2);
    str[0]= ((code >> 6) & 0x1F) | 0xC0;
    str[1]= (code & 0x3F) | 0x80;
    return str;
  }
  else if (0x800 <= code && code <= 0xFFFF) {
    // 0x1110dddd 0x10dddddd 0x10dddddd
    string str (3);
    str[0]= ((code >> 12) & 0x0F) | 0xE0;
    str[1]= ((code >> 6) & 0x3F) | 0x80;
    str[2]= (code & 0x3F) | 0x80;
    return str;
  }
  else if (0x10000 <= code && code <= 0x1FFFFF) {
    // 0x11110uuu 0x10zzzzzz 0x10yyyyyy 0x10xxxxxx
    string str (4);
    str[0]= ((code >> 18) & 0x07) | 0xF0;
    str[1]= ((code >> 12) & 0x3F) | 0x80;
    str[2]= ((code >> 6) & 0x3F) | 0x80;
    str[3]= (code & 0x3F) | 0x80;
    return str;
  }
  else return "";
}

uint32_t
decode_from_utf8 (string_u8 s, int& i) {
  unsigned char c= s[i];
  if ((0x80 & c) == 0) {
    // 0x0ddddddd
    i++;
    return (uint32_t) c;
  }
  uint32_t code;
  int      trail;
  if ((0xE0 & c) == 0xC0) {
    // 0x110ddddd 0x10dddddd
    trail= 1;
    code = c & 0x1F;
  }
  else if ((0xF0 & c) == 0xE0) {
    // 0x1110dddd 0x10dddddd 0x10dddddd
    trail= 2;
    code = c & 0x0F;
  }
  else if ((0xF8 & c) == 0xF0) {
    // 0x11110dddd 0x10dddddd 0x10dddddd 0x10dddddd
    trail= 3;
    code = c & 0x07;
  }
  else {
    // failsafe
    // cout << "failsafe: " << c << " (" << (unsigned int)(c) << ")\n";
    i++;
    return (uint32_t) c;
  }
  int start= i - 1;
  for (; trail > 0; trail--) {
    i++;
    if (i >= N (s)) i= N (s) - 1;
    c= s[i];
    if ((0xC0 & c) == 0x80) code= (code << 6) | (c & 0x3F);
    else {
      i= start + 1;
      c= s[i++];
      return c;
    }
  }
  i++;
  return code;
}

string
unicode_get_range (int code) {
  if (code <= 0x7f) return "ascii";
  else if (code >= 0x80 && code <= 0x37f) return "latin";
  else if (code >= 0x380 && code <= 0x3ff) return "greek";
  else if (code >= 0x400 && code <= 0x4ff) return "cyrillic";
  else if (code >= 0x2460 && code <= 0x24ff) return "enclosed_alphanumerics";
  else if (code >= 0x3000 && code <= 0x303f) return "cjk";
  else if (code >= 0x4e00 && code <= 0x9fcc) return "cjk";
  else if (code >= 0xff00 && code <= 0xffef) return "cjk";
  else if (code >= 0x3040 && code <= 0x309F) return "hiragana";
  else if (code >= 0xac00 && code <= 0xd7af) return "hangul";
  else if (code >= 0x2000 && code <= 0x23ff) return "mathsymbols";
  else if (code >= 0x2900 && code <= 0x2e7f) return "mathextra";
  else if (code >= 0x1d400 && code <= 0x1d7ff) return "mathletters";
  else if ((code >= 0x1F600 && code <= 0x1F64F) || // Emoticons
           (code >= 0x1F300 &&
            code <= 0x1F5FF) || // Misc Symbols and Pictographs
           (code >= 0x1F680 && code <= 0x1F6FF) || // Transport and Map Symbols
           (code >= 0x1F700 && code <= 0x1F77F) || // Alchemical Symbols
           (code >= 0x1F780 && code <= 0x1F7FF) || // Geometric Shapes Extended
           (code >= 0x1F800 && code <= 0x1F8FF) || // Supplemental Arrows-C
           (code >= 0x1F900 &&
            code <= 0x1F9FF) || // Supplemental Symbols and Pictographs
           (code >= 0x1FA00 && code <= 0x1FA6F) || // Chess Symbols
           (code >= 0x1FA70 &&
            code <= 0x1FAFF) || // Symbols and Pictographs Extended-A
           (code >= 0x2600 && code <= 0x26FF) ||   // Miscellaneous Symbols
           (code >= 0x2700 && code <= 0x27BF) ||   // Dingbats
           (code >= 0xFE00 && code <= 0xFE0F) ||   // Variation Selectors
           (code >= 0x1F1E6 && code <= 0x1F1FF) || // Regional Indicator Symbols
           code == 0x200D)                         // Zero Width Joiner
    return "emoji";
  else return "";
}

bool
is_cjk_unified_ideographs (string s) {
  int n= N (s);
  for (int i= 0; i < n; i++)
    if (s[i] == '<' && i + 1 < n && s[i + 1] == '#') {
      int start= i + 2;
      i        = i + 2;
      while (i < n && s[i] != '>')
        i++;
      string r= s (start, i);
      if ("4E00" <= r && r <= "9FBF") continue;
      else return false;
    }
    else {
      return false;
    }
  return true;
}

bool
has_cjk_unified_ideographs (string s) {
  int n= N (s);
  for (int i= 0; i < n; i++)
    if (s[i] == '<' && i + 1 < n && s[i + 1] == '#') {
      int start= i + 2;
      i        = i + 2;
      while (i < n && s[i] != '>')
        i++;
      string r= s (start, i);
      if ("4E00" <= r && r <= "9FBF") return true;
      else continue;
    }
    else {
      continue;
    }
  return false;
}

string
utf16_to_utf8 (string s_u16) {
  tb_size_t  isize= N (s_u16);
  tb_byte_t* idata= tb_malloc_bytes (isize);
  tb_long_t  osize= (tb_long_t) (isize << 2);
  tb_byte_t* odata= tb_malloc_bytes ((tb_size_t) osize);

  for (tb_size_t i= 0; i < isize; i++) {
    idata[i]= (tb_byte_t) s_u16[i];
  }

  osize= tb_charset_conv_data (TB_CHARSET_TYPE_UTF16, TB_CHARSET_TYPE_UTF8,
                               idata, isize, odata, osize);

  string ret ((int) osize);
  for (tb_size_t i= 0; i < osize; i++) {
    ret[i]= (char) odata[i];
  }
  if (idata) tb_free (idata);
  if (odata) tb_free (odata);
  return ret;
}

#if defined(OS_MINGW) || defined(OS_WIN)
string
wchar_to_utf8 (const wchar_t* s_u16) {
  tb_size_t  wchar_size= tb_wcslen (s_u16);
  tb_size_t  isize     = wchar_size * 2;
  tb_byte_t* idata     = tb_malloc_bytes (isize);
  tb_long_t  osize     = (tb_long_t) (isize << 2);
  tb_byte_t* odata     = tb_malloc_bytes ((tb_size_t) osize);

  for (tb_size_t i= 0; i < wchar_size; i++) {
    uint16_t  bytes = (uint16_t) s_u16[i];
    tb_byte_t high  = (tb_byte_t) (bytes >> 8);
    tb_byte_t low   = (tb_byte_t) (bytes & 0x00FF);
    idata[2 * i]    = high;
    idata[2 * i + 1]= low;
  }

  osize= tb_charset_conv_data (TB_CHARSET_TYPE_UTF16, TB_CHARSET_TYPE_UTF8,
                               idata, isize, odata, osize);

  string ret ((int) osize);
  for (tb_size_t i= 0; i < osize; i++) {
    ret[i]= (char) odata[i];
  }
  if (idata) tb_free (idata);
  if (odata) tb_free (odata);
  return ret;
}
#endif

string
utf8_to_utf16 (string s_u8) {
  tb_long_t  osize= (tb_long_t) (N (s_u8) << 2);
  tb_byte_t* odata= tb_malloc_bytes ((tb_size_t) osize);

  osize= tb_charset_conv_cstr (TB_CHARSET_TYPE_UTF8, TB_CHARSET_TYPE_UTF16,
                               c_string (s_u8), odata, osize);

  string ret ((int) osize);
  for (tb_size_t i= 0; i < osize; i++) {
    ret[i]= odata[i];
  }
  if (odata) tb_free (odata);
  return ret;
}

} // namespace data
} // namespace lolly
