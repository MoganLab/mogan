/******************************************************************************
 * MODULE     : cork_test.cpp
 * DESCRIPTION: tests on UTF-8 <-> Cork encoding conversions
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "lolly/data/cork.hpp"

using lolly::data::cork_to_utf8;
using lolly::data::strict_cork_to_utf8;
using lolly::data::utf8_to_cork;

// Helper: build a single-byte Cork string, even for the NUL byte.
static string
cork_byte (int byte) {
  return string ((char) byte, 1);
}

// Helper: build a UTF-8 byte string from a codepoint. Used so the test source
// stays ASCII even when exercising non-ASCII codepoints.
static string
cp (uint32_t code) {
  string r;
  if (code < 0x80) r << string ((char) code, 1);
  else if (code < 0x800) {
    r << string ((char) (0xC0 | (code >> 6)), 1);
    r << string ((char) (0x80 | (code & 0x3F)), 1);
  }
  else if (code < 0x10000) {
    r << string ((char) (0xE0 | (code >> 12)), 1);
    r << string ((char) (0x80 | ((code >> 6) & 0x3F)), 1);
    r << string ((char) (0x80 | (code & 0x3F)), 1);
  }
  else {
    r << string ((char) (0xF0 | (code >> 18)), 1);
    r << string ((char) (0x80 | ((code >> 12) & 0x3F)), 1);
    r << string ((char) (0x80 | ((code >> 6) & 0x3F)), 1);
    r << string ((char) (0x80 | (code & 0x3F)), 1);
  }
  return r;
}

/******************************************************************************
 * utf8_to_cork
 ******************************************************************************/

TEST_CASE ("utf8_to_cork_ascii_passthrough") {
  // Visible ASCII round-trips to its own byte.
  string_eq (utf8_to_cork ("A"), cork_byte (0x41));
  string_eq (utf8_to_cork ("z"), cork_byte (0x7A));
  string_eq (utf8_to_cork (" "), cork_byte (0x20));
  string_eq (utf8_to_cork ("~"), cork_byte (0x7E));
}

TEST_CASE ("utf8_to_cork_ascii_control_passthrough") {
  // Control bytes without an entry pass through verbatim (copy_unmatched).
  string_eq (utf8_to_cork (cp (0x0A)), cork_byte (0x0A));
  string_eq (utf8_to_cork (cp (0x1B)), cork_byte (0x1B));
  string_eq (utf8_to_cork (cp (0x7F)), cork_byte (0x7F));
}

TEST_CASE ("utf8_to_cork_lt_gt_named_entities") {
  // ASCII '<' and '>' have no 1-byte Cork form; they map to the named
  // entities registered via tmuniversaltounicode ("<less>" -> U+003C, etc).
  string_eq (utf8_to_cork ("<"), "<less>");
  string_eq (utf8_to_cork (">"), "<gtr>");
}

TEST_CASE ("utf8_to_cork_backtick_to_cork0") {
  // U+0060 backtick maps to Cork 0x00 (corktounicode has Cork 0x00 <-> U+0060).
  string_eq (utf8_to_cork ("`"), cork_byte (0x00));
}

TEST_CASE ("utf8_to_cork_latin1_letter") {
  // U+00C1 (Á) maps to Cork 0xC1 (identity in the Latin-1 range).
  string_eq (utf8_to_cork (cp (0xC1)), cork_byte (0xC1));
  // U+00DF (ß) maps to Cork 0xFF (not 0xDF; Cork 0xDF is sharp-s one-way only).
  string_eq (utf8_to_cork (cp (0xDF)), cork_byte (0xFF));
}

TEST_CASE ("utf8_to_cork_accented_upper") {
  // Cork 0x80..0x9F cover Eastern European uppercase (corktounicode).
  string_eq (utf8_to_cork (cp (0x0102)), cork_byte (0x80));
  string_eq (utf8_to_cork (cp (0x017D)), cork_byte (0x9A));
}

TEST_CASE ("utf8_to_cork_nbsp_to_varspace") {
  // U+00A0 (NBSP) maps to <varspace>: tmuniversaltounicode is loaded *after*
  // unicode-cork-oneway and overwrites the earlier " " mapping.
  string_eq (utf8_to_cork (cp (0x00A0)), "<varspace>");
}

TEST_CASE ("utf8_to_cork_cjk_escape") {
  // CJK codepoints with no Cork byte or named entity escape to <#XXXX>.
  string_eq (utf8_to_cork (cp (0x4E2D)), "<#4E2D>");
  string_eq (utf8_to_cork (cp (0x1F600)), "<#1F600>");
}

TEST_CASE ("utf8_to_cork_named_symbol") {
  // Math symbols map to their TeXmacs entity names via tmuniversaltounicode.
  string_eq (utf8_to_cork (cp (0x03B1)), "<alpha>"); // U+03B1 Greek alpha
  string_eq (utf8_to_cork (cp (0x221E)), "<infty>"); // U+221E infinity
}

TEST_CASE ("utf8_to_cork_empty") { string_eq (utf8_to_cork (""), ""); }

TEST_CASE ("utf8_to_cork_invalid_utf8_passthrough") {
  // Lone continuation byte 0x80 and lead byte 0xC3 (no continuation) are
  // invalid UTF-8; decode_from_utf8 returns the byte itself, and the 1-byte
  // Cork table has no entry, so it passes through unchanged.
  string_eq (utf8_to_cork (string ((char) 0x80, 1)), cork_byte (0x80));
  string_eq (utf8_to_cork (string ((char) 0xC3, 1)), cork_byte (0xC3));
}

/******************************************************************************
 * cork_to_utf8
 ******************************************************************************/

TEST_CASE ("cork_to_utf8_cork_byte_roundtrip") {
  // 1-byte Cork -> UTF-8 for the Latin-1 identity range.
  string_eq (cork_to_utf8 (cork_byte (0xC1)), cp (0xC1));
  string_eq (cork_to_utf8 (cork_byte (0xFF)), cp (0xDF)); // Cork 0xFF -> U+00DF
}

TEST_CASE ("cork_to_utf8_named_entity") {
  string_eq (cork_to_utf8 ("<less>"), "<");
  string_eq (cork_to_utf8 ("<gtr>"), ">");
  string_eq (cork_to_utf8 ("<alpha>"), cp (0x03B1));
  string_eq (cork_to_utf8 ("<infty>"), cp (0x221E));
}

TEST_CASE ("cork_to_utf8_lt_gt_passthrough") {
  // A bare '<' / '>' (no matching entity) passes through unchanged. The
  // original hashtree cannot complete a prefix match here, so copy_unmatched
  // emits the byte verbatim.
  string_eq (cork_to_utf8 ("<"), "<");
  string_eq (cork_to_utf8 (">"), ">");
  string_eq (cork_to_utf8 ("<xyz>"), "<xyz>");
}

TEST_CASE ("cork_to_utf8_escape") {
  string_eq (cork_to_utf8 ("<#4E2D>"), cp (0x4E2D));
  string_eq (cork_to_utf8 ("<#1F600>"), cp (0x1F600));
}

TEST_CASE ("cork_to_utf8_special_multibyte_keys") {
  // The 3 non-entity multi-byte Cork keys: "%\x18", "%\x18\x18", "...".
  // Built manually to avoid source-file encoding issues.
  string permille= string ("%", 1) * cork_byte (0x18);
  string perten  = permille * cork_byte (0x18);
  string ellipsis= string ("...", 3);
  string_eq (cork_to_utf8 (permille), cp (0x2030)); // per mille sign
  string_eq (cork_to_utf8 (perten), cp (0x2031));   // per ten thousand sign
  string_eq (cork_to_utf8 (ellipsis), cp (0x2026)); // horizontal ellipsis
}

TEST_CASE ("cork_to_utf8_empty") { string_eq (cork_to_utf8 (""), ""); }

TEST_CASE ("cork_to_utf8_mixed") {
  // Cork byte + escape + named entity, all in one string.
  string input= cork_byte (0x41) * string ("<#2019>") * cork_byte (0x42);
  string_eq (cork_to_utf8 (input), string ("A") * cp (0x2019) * string ("B"));
}

/******************************************************************************
 * strict_cork_to_utf8
 ******************************************************************************/

TEST_CASE ("strict_cork_to_utf8_matches_cork_for_non_fallback") {
  // For any entity that is NOT in symbol-unicode-fallback, strict and
  // non-strict produce identical output.
  string_eq (strict_cork_to_utf8 ("<less>"), "<");
  string_eq (strict_cork_to_utf8 ("<alpha>"), cp (0x03B1));
  string_eq (strict_cork_to_utf8 ("<#4E2D>"), cp (0x4E2D));
}

TEST_CASE ("strict_cork_to_utf8_excludes_long_arrow_fallbacks") {
  // The 8 long-arrow fallbacks come from symbol-unicode-fallback, which the
  // strict variant does not load. They pass through verbatim.
  string_eq (strict_cork_to_utf8 ("<longuparrow>"), "<longuparrow>");
  string_eq (strict_cork_to_utf8 ("<longtwoheadrightarrow>"),
             "<longtwoheadrightarrow>");
  // cork_to_utf8 (non-strict) decodes them:
  string_eq (cork_to_utf8 ("<longuparrow>"), cp (0x2191));
}
