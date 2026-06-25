
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

// Helper: build a single-byte Herk string, even for the NUL byte.
static string
herk_byte (int byte) {
  return string ((char) byte, 1);
}

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

TEST_CASE ("utf8_to_herk") {
  // Empty string.
  string_eq (utf8_to_herk (""), "");

  // ASCII printable characters are mostly preserved.
  string_eq (utf8_to_herk ("Hello"), "Hello");

  // U+007F is unmapped but stays as a single byte.
  string_eq (utf8_to_herk ("\x7F"), "\x7F");

  // Some Latin-1 characters with direct Herk mappings.
  string_eq (utf8_to_herk ("\xC3\xA0"), herk_byte (0xE0)); // U+00E0
  string_eq (utf8_to_herk ("\xC3\x9F"), herk_byte (0xFF)); // U+00DF

  // Some special punctuation with Herk mappings.
  string_eq (utf8_to_herk ("\xE2\x80\x94"), herk_byte (0x16)); // U+2014
  string_eq (utf8_to_herk ("\xE2\x80\x9C"), herk_byte (0x10)); // U+201C

  // Characters without Herk mappings fall back to <#XXXX> escapes.
  string_eq (utf8_to_herk ("\xE2\x80\x99"), "<#2019>"); // U+2019
  string_eq (utf8_to_herk ("\xE4\xB8\xAD"), "<#4E2D>"); // U+4E2D
  string_eq (utf8_to_herk ("\xE3\x80\x82"), "<#3002>"); // U+3002

  // Control characters also use escapes.
  string_eq (utf8_to_herk ("\x01"), "<#01>");

  // <#XXXX> and named escapes pass through unchanged.
  string_eq (utf8_to_herk ("<#FF>"), "<#FF>");
  string_eq (utf8_to_herk ("<#0FF>"), "<#0FF>");
  string_eq (utf8_to_herk ("<#00FF>"), "<#00FF>");
  string_eq (utf8_to_herk ("<less>"), "<less>");

  // Mixed strings.
  string_eq (utf8_to_herk ("A"
                           "\xE4\xB8\xAD"
                           "B"),
             herk_byte (0x41) * string ("<#4E2D>") * herk_byte (0x42));
}

TEST_CASE ("herk_to_utf8") {
  // Empty string.
  string_eq (herk_to_utf8 (""), "");

  // ASCII Herk bytes decode back to ASCII.
  string_eq (herk_to_utf8 (herk_byte (0x41)), "A");
  string_eq (herk_to_utf8 (herk_byte (0x20)), " ");

  // Herk byte 0x7F maps to U+00AD (soft hyphen).
  string_eq (herk_to_utf8 (herk_byte (0x7F)), "\xC2\xAD");

  // Some non-ASCII Herk bytes decode to expected UTF-8.
  string_eq (herk_to_utf8 (herk_byte (0x00)),
             utf8_from_code (0x0060)); // U+0060
  string_eq (herk_to_utf8 (herk_byte (0xE0)),
             utf8_from_code (0x00E0)); // U+00E0
  string_eq (herk_to_utf8 (herk_byte (0xFF)),
             utf8_from_code (0x00DF)); // U+00DF
  string_eq (herk_to_utf8 (herk_byte (0x10)),
             utf8_from_code (0x201C)); // U+201C

  // <#XXXX> escapes are parsed literally as Unicode code points.
  string_eq (herk_to_utf8 ("<#0F>"), "\x0F");
  string_eq (herk_to_utf8 ("<#FF>"), utf8_from_code (0x00FF));
  string_eq (herk_to_utf8 ("<#0FF>"), utf8_from_code (0x00FF));
  string_eq (herk_to_utf8 ("<#00FF>"), utf8_from_code (0x00FF));
  string_eq (herk_to_utf8 ("<#2019>"), utf8_from_code (0x2019));
  string_eq (herk_to_utf8 ("<#4E2D>"), utf8_from_code (0x4E2D));

  // NUL escape produces a single NUL byte.
  string r= herk_to_utf8 ("<#00>");
  CHECK_EQ (N (r), 1);
  CHECK_EQ ((int) (unsigned char) r[0], 0);

  // Named escapes pass through unchanged.
  string_eq (herk_to_utf8 ("<less>"), "<less>");

  // Mixed internal bytes and escapes.
  string_eq (
      herk_to_utf8 (herk_byte (0x41) * string ("<#2019>") * herk_byte (0x42)),
      "A" * utf8_from_code (0x2019) * "B");
}

TEST_CASE ("herk_round_trip") {
  // utf8 -> herk -> utf8 recovers any UTF-8 string (unmapped chars are
  // preserved via <#XXXX> escapes).
  string_eq (herk_to_utf8 (utf8_to_herk ("Hello")), "Hello");
  string_eq (herk_to_utf8 (utf8_to_herk ("\xC3\xA0\xC3\x9F")),
             "\xC3\xA0\xC3\x9F");
  string_eq (herk_to_utf8 (utf8_to_herk ("Hello \xE4\xB8\xAD")),
             "Hello \xE4\xB8\xAD");
  string_eq (
      herk_to_utf8 (utf8_to_herk ("\xE2\x80\x94\"\xE4\xB8\xAD\xE3\x80\x82")),
      "\xE2\x80\x94\"\xE4\xB8\xAD\xE3\x80\x82");

  // herk -> utf8 -> herk recovers valid Herk strings.
  string_eq (utf8_to_herk (herk_to_utf8 ("ABC")), "ABC");
  string_eq (utf8_to_herk (herk_to_utf8 (herk_byte (0x00) * herk_byte (0xFF))),
             herk_byte (0x00) * herk_byte (0xFF));
  string_eq (utf8_to_herk (herk_to_utf8 ("A<#4E2D>B")), "A<#4E2D>B");
  string_eq (utf8_to_herk (herk_to_utf8 ("<less>")), "<less>");
}
