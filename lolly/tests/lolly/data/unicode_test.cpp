
/******************************************************************************
 * MODULE     : unicode_test.cpp
 * DESCRIPTION: tests on unicode
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "lolly/data/unicode.hpp"

using lolly::data::has_cjk_unified_ideographs;
using lolly::data::is_cjk_unified_ideographs;
using lolly::data::unicode_get_range;
using lolly::data::utf16_to_utf8;
using lolly::data::utf8_to_utf16;

#if defined(OS_MINGW) || defined(OS_WIN)
using lolly::data::wchar_to_utf8;
#endif

TEST_CASE ("unicode_get_range") {
  string_eq (unicode_get_range ((int) 'a'), "ascii");
  string_eq (unicode_get_range (0x2460), "enclosed_alphanumerics"); // ①
  string_eq (unicode_get_range (0x24ff), "enclosed_alphanumerics"); // ⓿

  // CJK range tests (including unified ideographs, compatibility, symbols, and
  // half/fullwidth)
  string_eq (unicode_get_range (0x3000),
             "cjk"); // CJK Symbols & Punctuation Start
  string_eq (unicode_get_range (0x303f),
             "cjk"); // CJK Symbols & Punctuation End
  string_eq (unicode_get_range (0xff00),
             "cjk"); // Halfwidth & Fullwidth Forms Start
  string_eq (unicode_get_range (0xffef),
             "cjk"); // Halfwidth & Fullwidth Forms End

  // CJK Unified Ideographs and Extensions
  string_eq (unicode_get_range (0x4e00),
             "cjk"); // Unified Ideographs Base Start
  string_eq (unicode_get_range (0x9fff), "cjk");  // Unified Ideographs Base End
  string_eq (unicode_get_range (0x3400), "cjk");  // Extension A Start
  string_eq (unicode_get_range (0x4dbf), "cjk");  // Extension A End
  string_eq (unicode_get_range (0x20000), "cjk"); // Extension B Start
  string_eq (unicode_get_range (0x2a6df), "cjk"); // Extension B End
  string_eq (unicode_get_range (0x2a700), "cjk"); // Extension C Start
  string_eq (unicode_get_range (0x2b81f), "cjk"); // Extension D End
  string_eq (unicode_get_range (0x2b820), "cjk"); // Extension E Start
  string_eq (unicode_get_range (0x2ee5f), "cjk"); // Extension I End
  string_eq (unicode_get_range (0x30000), "cjk"); // Extension G Start
  string_eq (unicode_get_range (0x3347f),
             "cjk"); // Extension J End (Unicode 17.0)
  string_eq (unicode_get_range (0xf900),
             "cjk"); // Compatibility Ideographs Start
  string_eq (unicode_get_range (0xfaff), "cjk"); // Compatibility Ideographs End
  string_eq (unicode_get_range (0x2f800),
             "cjk"); // Compatibility Ideographs Supplement Start
  string_eq (unicode_get_range (0x2fa1f),
             "cjk"); // Compatibility Ideographs Supplement End

  // Non-CJK boundaries
  string_eq (unicode_get_range (0x33f0), ""); // Outside Extension A / CJK
  string_eq (unicode_get_range (0x4dc0), ""); // Outside Unified Ideographs Base
  string_eq (unicode_get_range (0x2a6e0), ""); // Outside Extension B
  string_eq (unicode_get_range (0x2fa20),
             ""); // Outside Compatibility Supplement
  string_eq (unicode_get_range (0x33480), ""); // Outside Extension J
}

TEST_CASE ("cjk_unified_ideographs") {
  // CJK Unified Ideographs Base (e.g., 中)
  CHECK (is_cjk_unified_ideographs ("<#4E2D>"));
  CHECK (has_cjk_unified_ideographs ("<#4E2D>"));
  CHECK (has_cjk_unified_ideographs ("bib-<#4E2D>"));
  CHECK (!is_cjk_unified_ideographs ("bib-<#4E2D>"));

  // Extension A
  CHECK (is_cjk_unified_ideographs ("<#3400>"));
  CHECK (is_cjk_unified_ideographs ("<#4DBF>"));

  // Extension B (e.g., 𠹌)
  CHECK (is_cjk_unified_ideographs ("<#20E4C>"));
  CHECK (has_cjk_unified_ideographs ("<#20E4C>"));
  CHECK (has_cjk_unified_ideographs ("bib-<#20E4C>"));
  CHECK (!is_cjk_unified_ideographs ("bib-<#20E4C>"));
  CHECK (is_cjk_unified_ideographs ("<#20000>"));
  CHECK (is_cjk_unified_ideographs ("<#2A6DF>"));

  // Extension C & D
  CHECK (is_cjk_unified_ideographs ("<#2A700>"));
  CHECK (is_cjk_unified_ideographs ("<#2B81F>"));

  // Extension E, F & I
  CHECK (is_cjk_unified_ideographs ("<#2B820>"));
  CHECK (is_cjk_unified_ideographs ("<#2EE5F>"));

  // Extension G, H & J
  CHECK (is_cjk_unified_ideographs ("<#30000>"));
  CHECK (is_cjk_unified_ideographs ("<#3347F>"));

  // Compatibility Ideographs
  CHECK (is_cjk_unified_ideographs ("<#F900>"));
  CHECK (is_cjk_unified_ideographs ("<#FAFF>"));
  CHECK (is_cjk_unified_ideographs ("<#2F800>"));
  CHECK (is_cjk_unified_ideographs ("<#2FA1F>"));

  // Non-CJK / Boundaries (Should fail)
  CHECK (!is_cjk_unified_ideographs (
      "<#3000>")); // CJK Symbol/Punctuation is not a unified ideograph
  CHECK (!is_cjk_unified_ideographs (
      "<#FF00>")); // Half/Fullwidth is not a unified ideograph
  CHECK (!is_cjk_unified_ideographs ("<#4DC0>"));  // Outside Base
  CHECK (!is_cjk_unified_ideographs ("<#2A6E0>")); // Outside Ext B
  CHECK (!is_cjk_unified_ideographs (
      "<#2FA20>")); // Outside Compatibility Supplement
  CHECK (!is_cjk_unified_ideographs ("<#33480>")); // Outside Ext J
  CHECK (!is_cjk_unified_ideographs ("<#1FFFF>")); // Invalid
}

#if defined(OS_MINGW) || defined(OS_WIN)
TEST_CASE ("wchar to utf8") { string_eq (wchar_to_utf8 (L"中"), "中"); }
#endif

TEST_CASE ("utf16 to utf8") {
  string t= "";
  string_eq (utf16_to_utf8 ("\x4E\x2D"), "中");
  t << '\x00' << '\x61';
  string_eq (utf16_to_utf8 (t), "a");
  t= "";
  t << '\x00' << '\x61' << '\x00' << '\x62';
  string_eq (utf16_to_utf8 (t), "ab");
  t= "";
  t << '\x00';
  string_eq (utf16_to_utf8 (t), "");
  t= "";
  t << '\x00' << '\x61' << '\x00';
  string_eq (utf16_to_utf8 (t), "a");
}

TEST_CASE ("utf8 to utf16") {
  string t= "";
  t << '\x4E' << '\x2D';
  string_eq (utf8_to_utf16 ("中"), t);
  t= "";
  t << '\x00' << '\x61';
  string_eq (utf8_to_utf16 ("a"), t);
  t= "";
  t << '\x00' << '\x61' << '\x00' << '\x62';
  string_eq (utf8_to_utf16 ("ab"), t);
}
