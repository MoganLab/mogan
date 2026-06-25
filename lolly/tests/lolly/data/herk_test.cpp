
/******************************************************************************
 * MODULE     : herk_test.cpp
 * DESCRIPTION: tests on Herk encoding conversions
 * COPYRIGHT  : (C) 2026  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "lolly/data/herk.hpp"

using lolly::data::herk_to_utf8;
using lolly::data::utf8_to_herk;

// Helper: encode a Unicode code point as a UTF-8 byte string.
static string
utf8_from_code (int code) {
  if (code < 0x80) return string ((char) code);
  else if (code < 0x800) {
    return string ((char) (0xC0 | (code >> 6))) *
           string ((char) (0x80 | (code & 0x3F)));
  }
  else {
    return string ((char) (0xE0 | (code >> 12))) *
           string ((char) (0x80 | ((code >> 6) & 0x3F))) *
           string ((char) (0x80 | (code & 0x3F)));
  }
}

// Helper: build a single-byte Herk string, even for the NUL byte.
static string
herk_byte (int byte) {
  return string ((char) byte, 1);
}

// Helper: build a <#XXXX> hexadecimal escape for a Unicode code point.
static string
hex_escape (int code) {
  static const char* digits= "0123456789ABCDEF";
  if (code < 16) {
    char buf[6]= {'<', '#', '0', digits[code], '>', '\0'};
    return string (buf);
  }
  else if (code < 256) {
    char buf[7]= {'<', '#', digits[code >> 4], digits[code & 0xF], '>', '\0'};
    return string (buf);
  }
  else {
    char buf[9];
    int  n  = 0;
    int  tmp= code;
    while (tmp > 0) {
      buf[n++]= digits[tmp & 0xF];
      tmp>>= 4;
    }
    string r= "<#";
    for (int i= n - 1; i >= 0; i--)
      r << buf[i];
    r << ">";
    return r;
  }
}

// Expected Unicode code point for each Herk byte 0x00..0xFF.
static const int herk_to_utf8_code[256]= {
    0x0060, 0x00B4, 0x02C6, 0x02DC, 0x00A8, 0x02DD, 0x02DA, 0x02C7, 0x02D8,
    0x00AF, 0x02D9, 0x00B8, 0x02DB, 0x201A, 0x2039, 0x203A, 0x201C, 0x201D,
    0x201E, 0x00AB, 0x00BB, 0x2013, 0x2014, 0x200B, 0x2080, 0x0131, 0x0237,
    0xFB00, 0xFB01, 0xFB02, 0xFB03, 0xFB04, 0x0020, 0x0021, 0x0022, 0x0023,
    0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C,
    0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
    0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E,
    0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050,
    0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
    0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x2018, 0x0061, 0x0062,
    0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B,
    0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
    0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D,
    0x007E, 0x00AD, 0x0102, 0x0104, 0x0106, 0x010C, 0x010E, 0x011A, 0x0118,
    0x011E, 0x0139, 0x013D, 0x0141, 0x0143, 0x0147, 0x014A, 0x0150, 0x0154,
    0x0158, 0x015A, 0x0160, 0x015E, 0x0164, 0x0162, 0x0170, 0x016E, 0x0178,
    0x0179, 0x017D, 0x017B, 0x0132, 0x0130, 0x0111, 0x00A7, 0x0103, 0x0105,
    0x0107, 0x010D, 0x010F, 0x011B, 0x0119, 0x011F, 0x013A, 0x013E, 0x0142,
    0x0144, 0x0148, 0x014B, 0x0151, 0x0155, 0x0159, 0x015B, 0x0161, 0x015F,
    0x0165, 0x0163, 0x0171, 0x016F, 0x00FF, 0x017A, 0x017E, 0x017C, 0x0133,
    0x00A1, 0x00BF, 0x00A3, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5,
    0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE,
    0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0152,
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x1E9E, 0x00E0,
    0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9,
    0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F1, 0x00F2,
    0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0153, 0x00F8, 0x00F9, 0x00FA, 0x00FB,
    0x00FC, 0x00FD, 0x00FE, 0x00DF,
};

// Reverse mapping: Unicode code point 0x00..0xFF -> Herk byte, -1 if none.
static const int utf8_to_herk_byte[256]= {
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
    60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
    75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  0,   97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  189, -1,  191, -1,
    -1,  -1,  159, 4,   -1,  -1,  19,  -1,  127, -1,  9,   -1,  -1,  -1,  -1,
    1,   -1,  -1,  -1,  11,  -1,  -1,  20,  -1,  -1,  -1,  190, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, -1,  216, 217, 218, 219, 220, 221, 222, 255, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, -1,  248, 249, 250, 251, 252, 253, 254,
    184,
};

TEST_CASE ("utf8_to_herk") {
  // U+0000..U+00FF are covered exhaustively.
  for (int code= 0; code <= 0xFF; code++) {
    int    mapped= utf8_to_herk_byte[code];
    string input = utf8_from_code (code);
    if (mapped > 0) {
      string_eq (utf8_to_herk (input), herk_byte (mapped));
    }
    else if (mapped == 0) {
      // U+0060 (`) maps to Herk byte 0x00; string_eq cannot check NUL bytes.
      string r= utf8_to_herk (input);
      CHECK_EQ (N (r), 1);
      CHECK_EQ ((int) (unsigned char) r[0], 0);
    }
    else if (code < 32 || code >= 128) {
      string_eq (utf8_to_herk (input), hex_escape (code));
    }
    else {
      // U+007F is the only unmapped code point in 0x20..0x7F.
      string_eq (utf8_to_herk (input), input);
    }
  }

  // Characters outside the Cork range use <#XXXX> escapes.
  string_eq (utf8_to_herk ("—"), herk_byte (0x16)); // U+2014 has a mapping
  string_eq (utf8_to_herk ("’"), "<#2019>");        // U+2019 has no mapping
  string_eq (utf8_to_herk ("中"), "<#4E2D>");       // CJK
  string_eq (utf8_to_herk ("。"), "<#3002>");       // CJK punctuation

  // <#XXXX> and named escapes pass through unchanged.
  string_eq (utf8_to_herk ("<#FF>"), "<#FF>");
  string_eq (utf8_to_herk ("<#0FF>"), "<#0FF>");
  string_eq (utf8_to_herk ("<#00FF>"), "<#00FF>");
  string_eq (utf8_to_herk ("<less>"), "<less>");

  // Mixed strings.
  string_eq (utf8_to_herk ("A中B"),
             herk_byte (0x41) * string ("<#4E2D>") * herk_byte (0x42));
}

TEST_CASE ("herk_to_utf8") {
  // Every Herk byte 0x00..0xFF must decode to the expected Unicode code point.
  for (int byte= 0; byte < 256; byte++)
    string_eq (herk_to_utf8 (herk_byte (byte)),
               utf8_from_code (herk_to_utf8_code[byte]));

  // <#XXXX> escapes are parsed literally as Unicode code points.
  string_eq (herk_to_utf8 ("<#0F>"), utf8_from_code (0x0F));
  string_eq (herk_to_utf8 ("<#10>"), utf8_from_code (0x10));
  string_eq (herk_to_utf8 ("<#1F>"), utf8_from_code (0x1F));
  string_eq (herk_to_utf8 ("<#FF>"), "ÿ");
  string_eq (herk_to_utf8 ("<#0FF>"), "ÿ");
  string_eq (herk_to_utf8 ("<#00FF>"), "ÿ");
  string_eq (herk_to_utf8 ("<#2019>"), "’");
  string_eq (herk_to_utf8 ("<#4E2D>"), "中");

  // NUL cannot be compared with string_eq because as_charp truncates at '\0'.
  string r= herk_to_utf8 ("<#00>");
  CHECK_EQ (N (r), 1);
  CHECK_EQ ((int) (unsigned char) r[0], 0);

  // Named escapes pass through unchanged.
  string_eq (herk_to_utf8 ("<less>"), "<less>");

  // Mixed internal bytes and escapes.
  string_eq (
      herk_to_utf8 (herk_byte (0x41) * string ("<#2019>") * herk_byte (0x42)),
      "A’B");
}
