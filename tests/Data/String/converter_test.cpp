/******************************************************************************
 * MODULE     : converter_test.cpp
 * DESCRIPTION: Properties of characters and strings
 * COPYRIGHT  : (C) 2019 Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "converter.hpp"
#include "file.hpp"

// Helper: build a single-byte Cork string.
static string
cork_byte (int byte) {
  return string ((char) byte, 1);
}

class TestConverter : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); };
  void cleanup () { cleanup_qt_top_level_widgets (); }

  // utf8_to_cork: ASCII printable range, organized by high nibble.
  void test_utf8_to_cork_2x ();
  void test_utf8_to_cork_3x ();
  void test_utf8_to_cork_4x ();
  void test_utf8_to_cork_5x ();
  void test_utf8_to_cork_6x ();
  void test_utf8_to_cork_7x ();

  // utf8_to_cork: non-ASCII Cork-mapped codepoints.
  void test_utf8_to_cork_accents ();     // Cork 0x00..0x0C
  void test_utf8_to_cork_punctuation (); // Cork 0x0D..0x17
  void test_utf8_to_cork_specials ();    // Cork 0x19, 0x1B..0x1F
  void test_utf8_to_cork_upper ();       // Cork 0x80..0x9F
  void test_utf8_to_cork_lower ();       // Cork 0xA0..0xBF
  void test_utf8_to_cork_latin1 ();      // Cork 0xC0..0xFF

  // utf8_to_cork: unmapped codepoints (escape as <#XXXX> or named entity).
  void test_utf8_to_cork_unmapped_high ();
  void test_utf8_to_cork_named_unmapped ();

  // cork_to_utf8: organized by high nibble of the Cork byte.
  void test_cork_to_utf8_0x ();
  void test_cork_to_utf8_1x ();
  void test_cork_to_utf8_2x ();
  void test_cork_to_utf8_3x ();
  void test_cork_to_utf8_4x ();
  void test_cork_to_utf8_5x ();
  void test_cork_to_utf8_6x ();
  void test_cork_to_utf8_7x ();
  void test_cork_to_utf8_8x ();
  void test_cork_to_utf8_9x ();
  void test_cork_to_utf8_Ax ();
  void test_cork_to_utf8_Bx ();
  void test_cork_to_utf8_Cx ();
  void test_cork_to_utf8_Dx ();
  void test_cork_to_utf8_Ex ();
  void test_cork_to_utf8_Fx ();

  // <#XXXX> hexadecimal escapes.
  void test_cork_to_utf8_escapes ();

  // Named entities.
  void test_cork_to_utf8_named_entities ();
  void test_utf8_to_cork_named_entities ();

  // Mixed strings, empty input, roundtrip.
  void test_mixed ();
  void test_empty ();
  void test_roundtrip_idempotent ();

  // Named entities (<name>) decoded by cork_to_utf8 / encoded by utf8_to_cork.
  void test_named_text_punctuation ();
  void test_named_currency ();
  void test_named_greek_lower ();
  void test_named_greek_upper ();
  void test_named_greek_variants ();
  void test_named_binary_operators ();
  void test_named_relations ();
  void test_named_arrows ();
  void test_named_set_theory ();
  void test_named_calculus ();
  void test_named_dots_dashes ();
  void test_named_special_letterforms ();
  void test_named_non_identity_roundtrip ();

  // utf8_to_cork: codepoints with no Cork byte map to named entities.
  void test_utf8_to_cork_greek ();
  void test_utf8_to_cork_math_ops ();
  void test_utf8_to_cork_math_syms ();
  void test_utf8_to_cork_text_syms ();
  void test_utf8_to_cork_named_remap ();
};

/******************************************************************************
 * utf8_to_cork
 ******************************************************************************/

void
TestConverter::test_utf8_to_cork_2x () {
  qcompare (utf8_to_cork (" "), cork_byte (0x20));
  qcompare (utf8_to_cork ("!"), cork_byte (0x21));
  qcompare (utf8_to_cork ("\""), cork_byte (0x22));
  qcompare (utf8_to_cork ("#"), cork_byte (0x23));
  qcompare (utf8_to_cork ("$"), cork_byte (0x24));
  qcompare (utf8_to_cork ("%"), cork_byte (0x25));
  qcompare (utf8_to_cork ("&"), cork_byte (0x26));
  qcompare (utf8_to_cork ("'"), cork_byte (0x27));
  qcompare (utf8_to_cork ("("), cork_byte (0x28));
  qcompare (utf8_to_cork (")"), cork_byte (0x29));
  qcompare (utf8_to_cork ("*"), cork_byte (0x2A));
  qcompare (utf8_to_cork ("+"), cork_byte (0x2B));
  qcompare (utf8_to_cork (","), cork_byte (0x2C));
  qcompare (utf8_to_cork ("-"), cork_byte (0x2D));
  qcompare (utf8_to_cork ("."), cork_byte (0x2E));
  qcompare (utf8_to_cork ("/"), cork_byte (0x2F));
}

void
TestConverter::test_utf8_to_cork_3x () {
  qcompare (utf8_to_cork ("0"), cork_byte (0x30));
  qcompare (utf8_to_cork ("1"), cork_byte (0x31));
  qcompare (utf8_to_cork ("2"), cork_byte (0x32));
  qcompare (utf8_to_cork ("3"), cork_byte (0x33));
  qcompare (utf8_to_cork ("4"), cork_byte (0x34));
  qcompare (utf8_to_cork ("5"), cork_byte (0x35));
  qcompare (utf8_to_cork ("6"), cork_byte (0x36));
  qcompare (utf8_to_cork ("7"), cork_byte (0x37));
  qcompare (utf8_to_cork ("8"), cork_byte (0x38));
  qcompare (utf8_to_cork ("9"), cork_byte (0x39));
  qcompare (utf8_to_cork (":"), cork_byte (0x3A));
  qcompare (utf8_to_cork (";"), cork_byte (0x3B));
  // 0x3C '<' and 0x3E '>' are covered by named-entity tests.
  qcompare (utf8_to_cork ("="), cork_byte (0x3D));
  qcompare (utf8_to_cork ("?"), cork_byte (0x3F));
}

void
TestConverter::test_utf8_to_cork_4x () {
  qcompare (utf8_to_cork ("@"), cork_byte (0x40));
  qcompare (utf8_to_cork ("A"), cork_byte (0x41));
  qcompare (utf8_to_cork ("B"), cork_byte (0x42));
  qcompare (utf8_to_cork ("C"), cork_byte (0x43));
  qcompare (utf8_to_cork ("D"), cork_byte (0x44));
  qcompare (utf8_to_cork ("E"), cork_byte (0x45));
  qcompare (utf8_to_cork ("F"), cork_byte (0x46));
  qcompare (utf8_to_cork ("G"), cork_byte (0x47));
  qcompare (utf8_to_cork ("H"), cork_byte (0x48));
  qcompare (utf8_to_cork ("I"), cork_byte (0x49));
  qcompare (utf8_to_cork ("J"), cork_byte (0x4A));
  qcompare (utf8_to_cork ("K"), cork_byte (0x4B));
  qcompare (utf8_to_cork ("L"), cork_byte (0x4C));
  qcompare (utf8_to_cork ("M"), cork_byte (0x4D));
  qcompare (utf8_to_cork ("N"), cork_byte (0x4E));
  qcompare (utf8_to_cork ("O"), cork_byte (0x4F));
}

void
TestConverter::test_utf8_to_cork_5x () {
  qcompare (utf8_to_cork ("P"), cork_byte (0x50));
  qcompare (utf8_to_cork ("Q"), cork_byte (0x51));
  qcompare (utf8_to_cork ("R"), cork_byte (0x52));
  qcompare (utf8_to_cork ("S"), cork_byte (0x53));
  qcompare (utf8_to_cork ("T"), cork_byte (0x54));
  qcompare (utf8_to_cork ("U"), cork_byte (0x55));
  qcompare (utf8_to_cork ("V"), cork_byte (0x56));
  qcompare (utf8_to_cork ("W"), cork_byte (0x57));
  qcompare (utf8_to_cork ("X"), cork_byte (0x58));
  qcompare (utf8_to_cork ("Y"), cork_byte (0x59));
  qcompare (utf8_to_cork ("Z"), cork_byte (0x5A));
  qcompare (utf8_to_cork ("["), cork_byte (0x5B));
  qcompare (utf8_to_cork ("\\"), cork_byte (0x5C));
  qcompare (utf8_to_cork ("]"), cork_byte (0x5D));
  qcompare (utf8_to_cork ("^"), cork_byte (0x5E));
  qcompare (utf8_to_cork ("_"), cork_byte (0x5F));
}

void
TestConverter::test_utf8_to_cork_6x () {
  qcompare (utf8_to_cork ("a"), cork_byte (0x61));
  qcompare (utf8_to_cork ("b"), cork_byte (0x62));
  qcompare (utf8_to_cork ("c"), cork_byte (0x63));
  qcompare (utf8_to_cork ("d"), cork_byte (0x64));
  qcompare (utf8_to_cork ("e"), cork_byte (0x65));
  qcompare (utf8_to_cork ("f"), cork_byte (0x66));
  qcompare (utf8_to_cork ("g"), cork_byte (0x67));
  qcompare (utf8_to_cork ("h"), cork_byte (0x68));
  qcompare (utf8_to_cork ("i"), cork_byte (0x69));
  qcompare (utf8_to_cork ("j"), cork_byte (0x6A));
  qcompare (utf8_to_cork ("k"), cork_byte (0x6B));
  qcompare (utf8_to_cork ("l"), cork_byte (0x6C));
  qcompare (utf8_to_cork ("m"), cork_byte (0x6D));
  qcompare (utf8_to_cork ("n"), cork_byte (0x6E));
  qcompare (utf8_to_cork ("o"), cork_byte (0x6F));
}

void
TestConverter::test_utf8_to_cork_7x () {
  qcompare (utf8_to_cork ("p"), cork_byte (0x70));
  qcompare (utf8_to_cork ("q"), cork_byte (0x71));
  qcompare (utf8_to_cork ("r"), cork_byte (0x72));
  qcompare (utf8_to_cork ("s"), cork_byte (0x73));
  qcompare (utf8_to_cork ("t"), cork_byte (0x74));
  qcompare (utf8_to_cork ("u"), cork_byte (0x75));
  qcompare (utf8_to_cork ("v"), cork_byte (0x76));
  qcompare (utf8_to_cork ("w"), cork_byte (0x77));
  qcompare (utf8_to_cork ("x"), cork_byte (0x78));
  qcompare (utf8_to_cork ("y"), cork_byte (0x79));
  qcompare (utf8_to_cork ("z"), cork_byte (0x7A));
  qcompare (utf8_to_cork ("{"), cork_byte (0x7B));
  qcompare (utf8_to_cork ("|"), cork_byte (0x7C));
  qcompare (utf8_to_cork ("}"), cork_byte (0x7D));
  qcompare (utf8_to_cork ("~"), cork_byte (0x7E));
  // Cork 0x60 is reserved for U+2018 (left single quote); see
  // test_utf8_to_cork_punctuation.
}

void
TestConverter::test_utf8_to_cork_accents () {
  qcompare (utf8_to_cork ("ˆ"),
            cork_byte (0x02));                     // U+02C6 modifier circumflex
  qcompare (utf8_to_cork ("˜"), cork_byte (0x03)); // U+02DC small tilde
  qcompare (utf8_to_cork ("¨"), cork_byte (0x04)); // U+00A8 diaeresis
  qcompare (utf8_to_cork ("˝"), cork_byte (0x05)); // U+02DD double acute
  qcompare (utf8_to_cork ("˚"), cork_byte (0x06)); // U+02DA ring above
  qcompare (utf8_to_cork ("ˇ"), cork_byte (0x07)); // U+02C7 caron
  qcompare (utf8_to_cork ("˘"), cork_byte (0x08)); // U+02D8 breve
  qcompare (utf8_to_cork ("¯"), cork_byte (0x09)); // U+00AF macron
  qcompare (utf8_to_cork ("˙"), cork_byte (0x0A)); // U+02D9 dot above
  qcompare (utf8_to_cork ("¸"), cork_byte (0x0B)); // U+00B8 cedilla
  qcompare (utf8_to_cork ("˛"), cork_byte (0x0C)); // U+02DB ogonek
  qcompare (utf8_to_cork ("´"), cork_byte (0x01)); // U+00B4 acute
}

void
TestConverter::test_utf8_to_cork_punctuation () {
  qcompare (utf8_to_cork ("‚"), cork_byte (0x0D)); // U+201A
  qcompare (utf8_to_cork ("‹"), cork_byte (0x0E)); // U+2039
  qcompare (utf8_to_cork ("›"), cork_byte (0x0F)); // U+203A
  qcompare (utf8_to_cork ("“"), cork_byte (0x10)); // U+201C
  qcompare (utf8_to_cork ("”"), cork_byte (0x11)); // U+201D
  qcompare (utf8_to_cork ("„"), cork_byte (0x12)); // U+201E
  qcompare (utf8_to_cork ("«"), cork_byte (0x13)); // U+00AB
  qcompare (utf8_to_cork ("»"), cork_byte (0x14)); // U+00BB
  qcompare (utf8_to_cork ("–"), cork_byte (0x15)); // U+2013
  qcompare (utf8_to_cork ("—"), cork_byte (0x16)); // U+2014
  qcompare (utf8_to_cork ("⁠"),
            cork_byte (0x17)); // U+2060 WORD JOINER
  qcompare (utf8_to_cork ("‘"),
            cork_byte (0x60)); // U+2018 left single quote
}

void
TestConverter::test_utf8_to_cork_specials () {
  qcompare (utf8_to_cork ("ı"), cork_byte (0x19)); // U+0131 dotless i
  qcompare (utf8_to_cork ("ﬀ"), cork_byte (0x1B)); // U+FB00 ff
  qcompare (utf8_to_cork ("ﬁ"), cork_byte (0x1C)); // U+FB01 fi
  qcompare (utf8_to_cork ("ﬂ"), cork_byte (0x1D)); // U+FB02 fl
  qcompare (utf8_to_cork ("ﬃ"), cork_byte (0x1E)); // U+FB03 ffi
  qcompare (utf8_to_cork ("ﬄ"), cork_byte (0x1F)); // U+FB04 ffl
}

void
TestConverter::test_utf8_to_cork_upper () {
  qcompare (utf8_to_cork ("Ă"), cork_byte (0x80)); // U+0102
  qcompare (utf8_to_cork ("Ą"), cork_byte (0x81)); // U+0104
  qcompare (utf8_to_cork ("Ć"), cork_byte (0x82)); // U+0106
  qcompare (utf8_to_cork ("Č"), cork_byte (0x83)); // U+010C
  qcompare (utf8_to_cork ("Ď"), cork_byte (0x84)); // U+010E
  qcompare (utf8_to_cork ("Ě"), cork_byte (0x85)); // U+011A
  qcompare (utf8_to_cork ("Ę"), cork_byte (0x86)); // U+0118
  qcompare (utf8_to_cork ("Ğ"), cork_byte (0x87)); // U+011E
  qcompare (utf8_to_cork ("Ĺ"), cork_byte (0x88)); // U+0139
  qcompare (utf8_to_cork ("Ľ"), cork_byte (0x89)); // U+013D
  qcompare (utf8_to_cork ("Ł"), cork_byte (0x8A)); // U+0141
  qcompare (utf8_to_cork ("Ń"), cork_byte (0x8B)); // U+0143
  qcompare (utf8_to_cork ("Ň"), cork_byte (0x8C)); // U+0147
  qcompare (utf8_to_cork ("Ŋ"), cork_byte (0x8D)); // U+014A
  qcompare (utf8_to_cork ("Ő"), cork_byte (0x8E)); // U+0150
  qcompare (utf8_to_cork ("Ŕ"), cork_byte (0x8F)); // U+0154
  qcompare (utf8_to_cork ("Ř"), cork_byte (0x90)); // U+0158
  qcompare (utf8_to_cork ("Ś"), cork_byte (0x91)); // U+015A
  qcompare (utf8_to_cork ("Š"), cork_byte (0x92)); // U+0160
  qcompare (utf8_to_cork ("Ş"), cork_byte (0x93)); // U+015E
  qcompare (utf8_to_cork ("Ť"), cork_byte (0x94)); // U+0164
  qcompare (utf8_to_cork ("Ţ"), cork_byte (0x95)); // U+0162
  qcompare (utf8_to_cork ("Ű"), cork_byte (0x96)); // U+0170
  qcompare (utf8_to_cork ("Ů"), cork_byte (0x97)); // U+016E
  qcompare (utf8_to_cork ("Ÿ"), cork_byte (0x98)); // U+0178
  qcompare (utf8_to_cork ("Ź"), cork_byte (0x99)); // U+0179
  qcompare (utf8_to_cork ("Ž"), cork_byte (0x9A)); // U+017D
  qcompare (utf8_to_cork ("Ż"), cork_byte (0x9B)); // U+017B
  qcompare (utf8_to_cork ("Ĳ"), cork_byte (0x9C)); // U+0132
  qcompare (utf8_to_cork ("İ"), cork_byte (0x9D)); // U+0130
  qcompare (utf8_to_cork ("đ"), cork_byte (0x9E)); // U+0111
  qcompare (utf8_to_cork ("§"), cork_byte (0x9F)); // U+00A7 section sign
}

void
TestConverter::test_utf8_to_cork_lower () {
  qcompare (utf8_to_cork ("ă"), cork_byte (0xA0)); // U+0103
  qcompare (utf8_to_cork ("ą"), cork_byte (0xA1)); // U+0105
  qcompare (utf8_to_cork ("ć"), cork_byte (0xA2)); // U+0107
  qcompare (utf8_to_cork ("č"), cork_byte (0xA3)); // U+010D
  qcompare (utf8_to_cork ("ď"), cork_byte (0xA4)); // U+010F
  qcompare (utf8_to_cork ("ě"), cork_byte (0xA5)); // U+011B
  qcompare (utf8_to_cork ("ę"), cork_byte (0xA6)); // U+0119
  qcompare (utf8_to_cork ("ğ"), cork_byte (0xA7)); // U+011F
  qcompare (utf8_to_cork ("ĺ"), cork_byte (0xA8)); // U+013A
  qcompare (utf8_to_cork ("ľ"), cork_byte (0xA9)); // U+013E
  qcompare (utf8_to_cork ("ł"), cork_byte (0xAA)); // U+0142
  qcompare (utf8_to_cork ("ń"), cork_byte (0xAB)); // U+0144
  qcompare (utf8_to_cork ("ň"), cork_byte (0xAC)); // U+0148
  qcompare (utf8_to_cork ("ŋ"), cork_byte (0xAD)); // U+014B
  qcompare (utf8_to_cork ("ő"), cork_byte (0xAE)); // U+0151
  qcompare (utf8_to_cork ("ŕ"), cork_byte (0xAF)); // U+0155
  qcompare (utf8_to_cork ("ř"), cork_byte (0xB0)); // U+0159
  qcompare (utf8_to_cork ("ś"), cork_byte (0xB1)); // U+015B
  qcompare (utf8_to_cork ("š"), cork_byte (0xB2)); // U+0161
  qcompare (utf8_to_cork ("ş"), cork_byte (0xB3)); // U+015F
  qcompare (utf8_to_cork ("ť"), cork_byte (0xB4)); // U+0165
  qcompare (utf8_to_cork ("ţ"), cork_byte (0xB5)); // U+0163
  qcompare (utf8_to_cork ("ű"), cork_byte (0xB6)); // U+0171
  qcompare (utf8_to_cork ("ů"), cork_byte (0xB7)); // U+016F
  qcompare (utf8_to_cork ("ÿ"), cork_byte (0xB8)); // U+00FF
  qcompare (utf8_to_cork ("ź"), cork_byte (0xB9)); // U+017A
  qcompare (utf8_to_cork ("ž"), cork_byte (0xBA)); // U+017E
  qcompare (utf8_to_cork ("ż"), cork_byte (0xBB)); // U+017C
  qcompare (utf8_to_cork ("ĳ"), cork_byte (0xBC)); // U+0133
  qcompare (utf8_to_cork ("¡"), cork_byte (0xBD)); // U+00A1
  qcompare (utf8_to_cork ("¿"), cork_byte (0xBE)); // U+00BF
  qcompare (utf8_to_cork ("£"), cork_byte (0xBF)); // U+00A3 pound sign
}

void
TestConverter::test_utf8_to_cork_latin1 () {
  qcompare (utf8_to_cork ("À"), cork_byte (0xC0)); // U+00C0 À
  qcompare (utf8_to_cork ("Á"), cork_byte (0xC1)); // U+00C1 Á
  qcompare (utf8_to_cork ("Â"), cork_byte (0xC2)); // U+00C2 Â
  qcompare (utf8_to_cork ("Ã"), cork_byte (0xC3)); // U+00C3 Ã
  qcompare (utf8_to_cork ("Ä"), cork_byte (0xC4)); // U+00C4 Ä
  qcompare (utf8_to_cork ("Å"), cork_byte (0xC5)); // U+00C5 Å
  qcompare (utf8_to_cork ("Æ"), cork_byte (0xC6)); // U+00C6 Æ
  qcompare (utf8_to_cork ("Ç"), cork_byte (0xC7)); // U+00C7 Ç
  qcompare (utf8_to_cork ("È"), cork_byte (0xC8)); // U+00C8 È
  qcompare (utf8_to_cork ("É"), cork_byte (0xC9)); // U+00C9 É
  qcompare (utf8_to_cork ("Ê"), cork_byte (0xCA)); // U+00CA Ê
  qcompare (utf8_to_cork ("Ë"), cork_byte (0xCB)); // U+00CB Ë
  qcompare (utf8_to_cork ("Ì"), cork_byte (0xCC)); // U+00CC Ì
  qcompare (utf8_to_cork ("Í"), cork_byte (0xCD)); // U+00CD Í
  qcompare (utf8_to_cork ("Î"), cork_byte (0xCE)); // U+00CE Î
  qcompare (utf8_to_cork ("Ï"), cork_byte (0xCF)); // U+00CF Ï
  qcompare (utf8_to_cork ("Ð"), cork_byte (0xD0)); // U+00D0 Ð
  qcompare (utf8_to_cork ("Ñ"), cork_byte (0xD1)); // U+00D1 Ñ
  qcompare (utf8_to_cork ("Ò"), cork_byte (0xD2)); // U+00D2 Ò
  qcompare (utf8_to_cork ("Ó"), cork_byte (0xD3)); // U+00D3 Ó
  qcompare (utf8_to_cork ("Ô"), cork_byte (0xD4)); // U+00D4 Ô
  qcompare (utf8_to_cork ("Õ"), cork_byte (0xD5)); // U+00D5 Õ
  qcompare (utf8_to_cork ("Ö"), cork_byte (0xD6)); // U+00D6 Ö
  qcompare (utf8_to_cork ("Œ"), cork_byte (0xD7)); // U+0152 Œ
  qcompare (utf8_to_cork ("Ø"), cork_byte (0xD8)); // U+00D8 Ø
  qcompare (utf8_to_cork ("Ù"), cork_byte (0xD9)); // U+00D9 Ù
  qcompare (utf8_to_cork ("Ú"), cork_byte (0xDA)); // U+00DA Ú
  qcompare (utf8_to_cork ("Û"), cork_byte (0xDB)); // U+00DB Û
  qcompare (utf8_to_cork ("Ü"), cork_byte (0xDC)); // U+00DC Ü
  qcompare (utf8_to_cork ("Ý"), cork_byte (0xDD)); // U+00DD Ý
  qcompare (utf8_to_cork ("Þ"), cork_byte (0xDE)); // U+00DE Þ
  // U+00DF (ß) maps to Cork 0xFF (corktounicode 0xFF -> U+00DF, reversible).
  // Cork 0xDF decodes to "SS" (cork-unicode-oneway), so the byte 0xDF is
  // unreachable from utf8_to_cork; see test_cork_to_utf8_Dx and devel/1125.md.
  qcompare (utf8_to_cork ("ß"), cork_byte (0xFF)); // U+00DF ß
  qcompare (utf8_to_cork ("à"), cork_byte (0xE0)); // U+00E0 à
  qcompare (utf8_to_cork ("á"), cork_byte (0xE1)); // U+00E1 á
  qcompare (utf8_to_cork ("â"), cork_byte (0xE2)); // U+00E2 â
  qcompare (utf8_to_cork ("ã"), cork_byte (0xE3)); // U+00E3 ã
  qcompare (utf8_to_cork ("ä"), cork_byte (0xE4)); // U+00E4 ä
  qcompare (utf8_to_cork ("å"), cork_byte (0xE5)); // U+00E5 å
  qcompare (utf8_to_cork ("æ"), cork_byte (0xE6)); // U+00E6 æ
  qcompare (utf8_to_cork ("ç"), cork_byte (0xE7)); // U+00E7 ç
  qcompare (utf8_to_cork ("è"), cork_byte (0xE8)); // U+00E8 è
  qcompare (utf8_to_cork ("é"), cork_byte (0xE9)); // U+00E9 é
  qcompare (utf8_to_cork ("ê"), cork_byte (0xEA)); // U+00EA ê
  qcompare (utf8_to_cork ("ë"), cork_byte (0xEB)); // U+00EB ë
  qcompare (utf8_to_cork ("ì"), cork_byte (0xEC)); // U+00EC ì
  qcompare (utf8_to_cork ("í"), cork_byte (0xED)); // U+00ED í
  qcompare (utf8_to_cork ("î"), cork_byte (0xEE)); // U+00EE î
  qcompare (utf8_to_cork ("ï"), cork_byte (0xEF)); // U+00EF ï
  qcompare (utf8_to_cork ("ð"), cork_byte (0xF0)); // U+00F0 ð
  qcompare (utf8_to_cork ("ñ"), cork_byte (0xF1)); // U+00F1 ñ
  qcompare (utf8_to_cork ("ò"), cork_byte (0xF2)); // U+00F2 ò
  qcompare (utf8_to_cork ("ó"), cork_byte (0xF3)); // U+00F3 ó
  qcompare (utf8_to_cork ("ô"), cork_byte (0xF4)); // U+00F4 ô
  qcompare (utf8_to_cork ("õ"), cork_byte (0xF5)); // U+00F5 õ
  qcompare (utf8_to_cork ("ö"), cork_byte (0xF6)); // U+00F6 ö
  qcompare (utf8_to_cork ("œ"), cork_byte (0xF7)); // U+0153 œ
  qcompare (utf8_to_cork ("ø"), cork_byte (0xF8)); // U+00F8 ø
  qcompare (utf8_to_cork ("ù"), cork_byte (0xF9)); // U+00F9 ù
  qcompare (utf8_to_cork ("ú"), cork_byte (0xFA)); // U+00FA ú
  qcompare (utf8_to_cork ("û"), cork_byte (0xFB)); // U+00FB û
  qcompare (utf8_to_cork ("ü"), cork_byte (0xFC)); // U+00FC ü
  qcompare (utf8_to_cork ("ý"), cork_byte (0xFD)); // U+00FD ý
  qcompare (utf8_to_cork ("þ"), cork_byte (0xFE)); // U+00FE þ
}

void
TestConverter::test_utf8_to_cork_unmapped_high () {
  // Codepoints >= 256 with no Cork mapping are escaped as <#XXXX>.
  qcompare (utf8_to_cork ("中"), "<#4E2D>");  // U+4E2D 中
  qcompare (utf8_to_cork ("😀"), "<#1F600>"); // U+1F600 😀
  qcompare (utf8_to_cork ("​"), "<#200B>"); // U+200B ZWSP
  // U+2019 maps to Cork 0x27 (shares ASCII apostrophe slot), not escaped.
  qcompare (utf8_to_cork ("’"), cork_byte (0x27)); // U+2019 ’
  // U+2010 maps to Cork 0x7F (hyphen), not escaped.
  qcompare (utf8_to_cork ("‐"), cork_byte (0x7F)); // U+2010 ‐
}

void
TestConverter::test_utf8_to_cork_named_unmapped () {
  // U+00A0 maps to the <varspace> named entity (unicode-cork-oneway).
  qcompare (utf8_to_cork (" "), "<varspace>");
  // U+0060 backtick maps to Cork 0x00 (corktounicode has 0x00 -> U+0060,
  // reversible).
  qcompare (utf8_to_cork ("`"), cork_byte (0x00));
}

/******************************************************************************
 * cork_to_utf8
 ******************************************************************************/

void
TestConverter::test_cork_to_utf8_0x () {
  qcompare (cork_to_utf8 (cork_byte (0x00)), "`"); // U+0060
  qcompare (cork_to_utf8 (cork_byte (0x01)), "´"); // U+00B4
  qcompare (cork_to_utf8 (cork_byte (0x02)), "ˆ"); // U+02C6
  qcompare (cork_to_utf8 (cork_byte (0x03)), "˜"); // U+02DC
  qcompare (cork_to_utf8 (cork_byte (0x04)), "¨"); // U+00A8
  qcompare (cork_to_utf8 (cork_byte (0x05)), "˝"); // U+02DD
  qcompare (cork_to_utf8 (cork_byte (0x06)), "˚"); // U+02DA
  qcompare (cork_to_utf8 (cork_byte (0x07)), "ˇ"); // U+02C7
  qcompare (cork_to_utf8 (cork_byte (0x08)), "˘"); // U+02D8
  qcompare (cork_to_utf8 (cork_byte (0x09)), "¯"); // U+00AF
  qcompare (cork_to_utf8 (cork_byte (0x0A)), "˙"); // U+02D9
  qcompare (cork_to_utf8 (cork_byte (0x0B)), "¸"); // U+00B8
  qcompare (cork_to_utf8 (cork_byte (0x0C)), "˛"); // U+02DB
}

void
TestConverter::test_cork_to_utf8_1x () {
  qcompare (cork_to_utf8 (cork_byte (0x0D)), "‚"); // U+201A
  qcompare (cork_to_utf8 (cork_byte (0x0E)), "‹"); // U+2039
  qcompare (cork_to_utf8 (cork_byte (0x0F)), "›"); // U+203A
  qcompare (cork_to_utf8 (cork_byte (0x10)), "“"); // U+201C “
  qcompare (cork_to_utf8 (cork_byte (0x11)), "”"); // U+201D ”
  qcompare (cork_to_utf8 (cork_byte (0x12)), "„"); // U+201E
  qcompare (cork_to_utf8 (cork_byte (0x13)), "«"); // U+00AB «
  qcompare (cork_to_utf8 (cork_byte (0x14)), "»"); // U+00BB »
  qcompare (cork_to_utf8 (cork_byte (0x15)), "–"); // U+2013 –
  qcompare (cork_to_utf8 (cork_byte (0x16)), "—"); // U+2014 —
  qcompare (cork_to_utf8 (cork_byte (0x17)),
            "⁠");                                // U+2060 WORD JOINER
  qcompare (cork_to_utf8 (cork_byte (0x18)), "0"); // perthousand zero (oneway)
  qcompare (cork_to_utf8 (cork_byte (0x19)), "ı"); // U+0131 ı
  qcompare (cork_to_utf8 (cork_byte (0x1A)), "j"); // dotless j (oneway)
  qcompare (cork_to_utf8 (cork_byte (0x1B)), "ﬀ"); // U+FB00 ﬀ
  qcompare (cork_to_utf8 (cork_byte (0x1C)), "ﬁ"); // U+FB01 ﬁ
  qcompare (cork_to_utf8 (cork_byte (0x1D)), "ﬂ"); // U+FB02 ﬂ
  qcompare (cork_to_utf8 (cork_byte (0x1E)), "ﬃ"); // U+FB03 ﬃ
  qcompare (cork_to_utf8 (cork_byte (0x1F)), "ﬄ"); // U+FB04 ﬄ
}

void
TestConverter::test_cork_to_utf8_2x () {
  qcompare (cork_to_utf8 (cork_byte (0x20)), " ");
  qcompare (cork_to_utf8 (cork_byte (0x21)), "!");
  qcompare (cork_to_utf8 (cork_byte (0x22)), "\"");
  qcompare (cork_to_utf8 (cork_byte (0x23)), "#");
  qcompare (cork_to_utf8 (cork_byte (0x24)), "$");
  qcompare (cork_to_utf8 (cork_byte (0x25)), "%");
  qcompare (cork_to_utf8 (cork_byte (0x26)), "&");
  qcompare (cork_to_utf8 (cork_byte (0x27)), "'");
  qcompare (cork_to_utf8 (cork_byte (0x28)), "(");
  qcompare (cork_to_utf8 (cork_byte (0x29)), ")");
  qcompare (cork_to_utf8 (cork_byte (0x2A)), "*");
  qcompare (cork_to_utf8 (cork_byte (0x2B)), "+");
  qcompare (cork_to_utf8 (cork_byte (0x2C)), ",");
  qcompare (cork_to_utf8 (cork_byte (0x2D)), "-");
  qcompare (cork_to_utf8 (cork_byte (0x2E)), ".");
  qcompare (cork_to_utf8 (cork_byte (0x2F)), "/");
}

void
TestConverter::test_cork_to_utf8_3x () {
  qcompare (cork_to_utf8 (cork_byte (0x30)), "0");
  qcompare (cork_to_utf8 (cork_byte (0x31)), "1");
  qcompare (cork_to_utf8 (cork_byte (0x32)), "2");
  qcompare (cork_to_utf8 (cork_byte (0x33)), "3");
  qcompare (cork_to_utf8 (cork_byte (0x34)), "4");
  qcompare (cork_to_utf8 (cork_byte (0x35)), "5");
  qcompare (cork_to_utf8 (cork_byte (0x36)), "6");
  qcompare (cork_to_utf8 (cork_byte (0x37)), "7");
  qcompare (cork_to_utf8 (cork_byte (0x38)), "8");
  qcompare (cork_to_utf8 (cork_byte (0x39)), "9");
  qcompare (cork_to_utf8 (cork_byte (0x3A)), ":");
  qcompare (cork_to_utf8 (cork_byte (0x3B)), ";");
  // 0x3C/0x3E: single byte passes through as literal '<'/'>'. See named-entity
  // tests.
  qcompare (cork_to_utf8 (cork_byte (0x3D)), "=");
  qcompare (cork_to_utf8 (cork_byte (0x3F)), "?");
}

void
TestConverter::test_cork_to_utf8_4x () {
  qcompare (cork_to_utf8 (cork_byte (0x40)), "@");
  qcompare (cork_to_utf8 (cork_byte (0x41)), "A");
  qcompare (cork_to_utf8 (cork_byte (0x42)), "B");
  qcompare (cork_to_utf8 (cork_byte (0x43)), "C");
  qcompare (cork_to_utf8 (cork_byte (0x44)), "D");
  qcompare (cork_to_utf8 (cork_byte (0x45)), "E");
  qcompare (cork_to_utf8 (cork_byte (0x46)), "F");
  qcompare (cork_to_utf8 (cork_byte (0x47)), "G");
  qcompare (cork_to_utf8 (cork_byte (0x48)), "H");
  qcompare (cork_to_utf8 (cork_byte (0x49)), "I");
  qcompare (cork_to_utf8 (cork_byte (0x4A)), "J");
  qcompare (cork_to_utf8 (cork_byte (0x4B)), "K");
  qcompare (cork_to_utf8 (cork_byte (0x4C)), "L");
  qcompare (cork_to_utf8 (cork_byte (0x4D)), "M");
  qcompare (cork_to_utf8 (cork_byte (0x4E)), "N");
  qcompare (cork_to_utf8 (cork_byte (0x4F)), "O");
}

void
TestConverter::test_cork_to_utf8_5x () {
  qcompare (cork_to_utf8 (cork_byte (0x50)), "P");
  qcompare (cork_to_utf8 (cork_byte (0x51)), "Q");
  qcompare (cork_to_utf8 (cork_byte (0x52)), "R");
  qcompare (cork_to_utf8 (cork_byte (0x53)), "S");
  qcompare (cork_to_utf8 (cork_byte (0x54)), "T");
  qcompare (cork_to_utf8 (cork_byte (0x55)), "U");
  qcompare (cork_to_utf8 (cork_byte (0x56)), "V");
  qcompare (cork_to_utf8 (cork_byte (0x57)), "W");
  qcompare (cork_to_utf8 (cork_byte (0x58)), "X");
  qcompare (cork_to_utf8 (cork_byte (0x59)), "Y");
  qcompare (cork_to_utf8 (cork_byte (0x5A)), "Z");
  qcompare (cork_to_utf8 (cork_byte (0x5B)), "[");
  qcompare (cork_to_utf8 (cork_byte (0x5C)), "\\");
  qcompare (cork_to_utf8 (cork_byte (0x5D)), "]");
  qcompare (cork_to_utf8 (cork_byte (0x5E)), "^");
  qcompare (cork_to_utf8 (cork_byte (0x5F)), "_");
}

void
TestConverter::test_cork_to_utf8_6x () {
  qcompare (cork_to_utf8 (cork_byte (0x60)), "‘"); // U+2018 ‘
  qcompare (cork_to_utf8 (cork_byte (0x61)), "a");
  qcompare (cork_to_utf8 (cork_byte (0x62)), "b");
  qcompare (cork_to_utf8 (cork_byte (0x63)), "c");
  qcompare (cork_to_utf8 (cork_byte (0x64)), "d");
  qcompare (cork_to_utf8 (cork_byte (0x65)), "e");
  qcompare (cork_to_utf8 (cork_byte (0x66)), "f");
  qcompare (cork_to_utf8 (cork_byte (0x67)), "g");
  qcompare (cork_to_utf8 (cork_byte (0x68)), "h");
  qcompare (cork_to_utf8 (cork_byte (0x69)), "i");
  qcompare (cork_to_utf8 (cork_byte (0x6A)), "j");
  qcompare (cork_to_utf8 (cork_byte (0x6B)), "k");
  qcompare (cork_to_utf8 (cork_byte (0x6C)), "l");
  qcompare (cork_to_utf8 (cork_byte (0x6D)), "m");
  qcompare (cork_to_utf8 (cork_byte (0x6E)), "n");
  qcompare (cork_to_utf8 (cork_byte (0x6F)), "o");
}

void
TestConverter::test_cork_to_utf8_7x () {
  qcompare (cork_to_utf8 (cork_byte (0x70)), "p");
  qcompare (cork_to_utf8 (cork_byte (0x71)), "q");
  qcompare (cork_to_utf8 (cork_byte (0x72)), "r");
  qcompare (cork_to_utf8 (cork_byte (0x73)), "s");
  qcompare (cork_to_utf8 (cork_byte (0x74)), "t");
  qcompare (cork_to_utf8 (cork_byte (0x75)), "u");
  qcompare (cork_to_utf8 (cork_byte (0x76)), "v");
  qcompare (cork_to_utf8 (cork_byte (0x77)), "w");
  qcompare (cork_to_utf8 (cork_byte (0x78)), "x");
  qcompare (cork_to_utf8 (cork_byte (0x79)), "y");
  qcompare (cork_to_utf8 (cork_byte (0x7A)), "z");
  qcompare (cork_to_utf8 (cork_byte (0x7B)), "{");
  qcompare (cork_to_utf8 (cork_byte (0x7C)), "|");
  qcompare (cork_to_utf8 (cork_byte (0x7D)), "}");
  qcompare (cork_to_utf8 (cork_byte (0x7E)), "~");
  qcompare (cork_to_utf8 (cork_byte (0x7F)), "‐"); // U+2010 hyphen
}

void
TestConverter::test_cork_to_utf8_8x () {
  qcompare (cork_to_utf8 (cork_byte (0x80)), "Ă"); // U+0102
  qcompare (cork_to_utf8 (cork_byte (0x81)), "Ą"); // U+0104
  qcompare (cork_to_utf8 (cork_byte (0x82)), "Ć"); // U+0106
  qcompare (cork_to_utf8 (cork_byte (0x83)), "Č"); // U+010C
  qcompare (cork_to_utf8 (cork_byte (0x84)), "Ď"); // U+010E
  qcompare (cork_to_utf8 (cork_byte (0x85)), "Ě"); // U+011A
  qcompare (cork_to_utf8 (cork_byte (0x86)), "Ę"); // U+0118
  qcompare (cork_to_utf8 (cork_byte (0x87)), "Ğ"); // U+011E
  qcompare (cork_to_utf8 (cork_byte (0x88)), "Ĺ"); // U+0139
  qcompare (cork_to_utf8 (cork_byte (0x89)), "Ľ"); // U+013D
  qcompare (cork_to_utf8 (cork_byte (0x8A)), "Ł"); // U+0141
  qcompare (cork_to_utf8 (cork_byte (0x8B)), "Ń"); // U+0143
  qcompare (cork_to_utf8 (cork_byte (0x8C)), "Ň"); // U+0147
  qcompare (cork_to_utf8 (cork_byte (0x8D)), "Ŋ"); // U+014A
  qcompare (cork_to_utf8 (cork_byte (0x8E)), "Ő"); // U+0150
  qcompare (cork_to_utf8 (cork_byte (0x8F)), "Ŕ"); // U+0154
}

void
TestConverter::test_cork_to_utf8_9x () {
  qcompare (cork_to_utf8 (cork_byte (0x90)), "Ř"); // U+0158
  qcompare (cork_to_utf8 (cork_byte (0x91)), "Ś"); // U+015A
  qcompare (cork_to_utf8 (cork_byte (0x92)), "Š"); // U+0160
  qcompare (cork_to_utf8 (cork_byte (0x93)), "Ş"); // U+015E
  qcompare (cork_to_utf8 (cork_byte (0x94)), "Ť"); // U+0164
  qcompare (cork_to_utf8 (cork_byte (0x95)), "Ţ"); // U+0162
  qcompare (cork_to_utf8 (cork_byte (0x96)), "Ű"); // U+0170
  qcompare (cork_to_utf8 (cork_byte (0x97)), "Ů"); // U+016E
  qcompare (cork_to_utf8 (cork_byte (0x98)), "Ÿ"); // U+0178
  qcompare (cork_to_utf8 (cork_byte (0x99)), "Ź"); // U+0179
  qcompare (cork_to_utf8 (cork_byte (0x9A)), "Ž"); // U+017D
  qcompare (cork_to_utf8 (cork_byte (0x9B)), "Ż"); // U+017B
  qcompare (cork_to_utf8 (cork_byte (0x9C)), "Ĳ"); // U+0132
  qcompare (cork_to_utf8 (cork_byte (0x9D)), "İ"); // U+0130
  qcompare (cork_to_utf8 (cork_byte (0x9E)), "đ"); // U+0111
  qcompare (cork_to_utf8 (cork_byte (0x9F)), "§"); // U+00A7 §
}

void
TestConverter::test_cork_to_utf8_Ax () {
  qcompare (cork_to_utf8 (cork_byte (0xA0)), "ă"); // U+0103
  qcompare (cork_to_utf8 (cork_byte (0xA1)), "ą"); // U+0105
  qcompare (cork_to_utf8 (cork_byte (0xA2)), "ć"); // U+0107
  qcompare (cork_to_utf8 (cork_byte (0xA3)), "č"); // U+010D
  qcompare (cork_to_utf8 (cork_byte (0xA4)), "ď"); // U+010F
  qcompare (cork_to_utf8 (cork_byte (0xA5)), "ě"); // U+011B
  qcompare (cork_to_utf8 (cork_byte (0xA6)), "ę"); // U+0119
  qcompare (cork_to_utf8 (cork_byte (0xA7)), "ğ"); // U+011F
  qcompare (cork_to_utf8 (cork_byte (0xA8)), "ĺ"); // U+013A
  qcompare (cork_to_utf8 (cork_byte (0xA9)), "ľ"); // U+013E
  qcompare (cork_to_utf8 (cork_byte (0xAA)), "ł"); // U+0142
  qcompare (cork_to_utf8 (cork_byte (0xAB)), "ń"); // U+0144
  qcompare (cork_to_utf8 (cork_byte (0xAC)), "ň"); // U+0148
  qcompare (cork_to_utf8 (cork_byte (0xAD)), "ŋ"); // U+014B
  qcompare (cork_to_utf8 (cork_byte (0xAE)), "ő"); // U+0151
  qcompare (cork_to_utf8 (cork_byte (0xAF)), "ŕ"); // U+0155
}

void
TestConverter::test_cork_to_utf8_Bx () {
  qcompare (cork_to_utf8 (cork_byte (0xB0)), "ř"); // U+0159
  qcompare (cork_to_utf8 (cork_byte (0xB1)), "ś"); // U+015B
  qcompare (cork_to_utf8 (cork_byte (0xB2)), "š"); // U+0161
  qcompare (cork_to_utf8 (cork_byte (0xB3)), "ş"); // U+015F
  qcompare (cork_to_utf8 (cork_byte (0xB4)), "ť"); // U+0165
  qcompare (cork_to_utf8 (cork_byte (0xB5)), "ţ"); // U+0163
  qcompare (cork_to_utf8 (cork_byte (0xB6)), "ű"); // U+0171
  qcompare (cork_to_utf8 (cork_byte (0xB7)), "ů"); // U+016F
  qcompare (cork_to_utf8 (cork_byte (0xB8)), "ÿ"); // U+00FF ÿ
  qcompare (cork_to_utf8 (cork_byte (0xB9)), "ź"); // U+017A
  qcompare (cork_to_utf8 (cork_byte (0xBA)), "ž"); // U+017E
  qcompare (cork_to_utf8 (cork_byte (0xBB)), "ż"); // U+017C
  qcompare (cork_to_utf8 (cork_byte (0xBC)), "ĳ"); // U+0133
  qcompare (cork_to_utf8 (cork_byte (0xBD)), "¡"); // U+00A1 ¡
  qcompare (cork_to_utf8 (cork_byte (0xBE)), "¿"); // U+00BF ¿
  qcompare (cork_to_utf8 (cork_byte (0xBF)), "£"); // U+00A3 £
}

void
TestConverter::test_cork_to_utf8_Cx () {
  qcompare (cork_to_utf8 (cork_byte (0xC0)), "À"); // U+00C0 À
  qcompare (cork_to_utf8 (cork_byte (0xC1)), "Á"); // U+00C1 Á
  qcompare (cork_to_utf8 (cork_byte (0xC2)), "Â"); // U+00C2 Â
  qcompare (cork_to_utf8 (cork_byte (0xC3)), "Ã"); // U+00C3 Ã
  qcompare (cork_to_utf8 (cork_byte (0xC4)), "Ä"); // U+00C4 Ä
  qcompare (cork_to_utf8 (cork_byte (0xC5)), "Å"); // U+00C5 Å
  qcompare (cork_to_utf8 (cork_byte (0xC6)), "Æ"); // U+00C6 Æ
  qcompare (cork_to_utf8 (cork_byte (0xC7)), "Ç"); // U+00C7 Ç
  qcompare (cork_to_utf8 (cork_byte (0xC8)), "È"); // U+00C8 È
  qcompare (cork_to_utf8 (cork_byte (0xC9)), "É"); // U+00C9 É
  qcompare (cork_to_utf8 (cork_byte (0xCA)), "Ê"); // U+00CA Ê
  qcompare (cork_to_utf8 (cork_byte (0xCB)), "Ë"); // U+00CB Ë
  qcompare (cork_to_utf8 (cork_byte (0xCC)), "Ì"); // U+00CC Ì
  qcompare (cork_to_utf8 (cork_byte (0xCD)), "Í"); // U+00CD Í
  qcompare (cork_to_utf8 (cork_byte (0xCE)), "Î"); // U+00CE Î
  qcompare (cork_to_utf8 (cork_byte (0xCF)), "Ï"); // U+00CF Ï
}

void
TestConverter::test_cork_to_utf8_Dx () {
  qcompare (cork_to_utf8 (cork_byte (0xD0)), "Ð");  // U+00D0 Ð
  qcompare (cork_to_utf8 (cork_byte (0xD1)), "Ñ");  // U+00D1 Ñ
  qcompare (cork_to_utf8 (cork_byte (0xD2)), "Ò");  // U+00D2 Ò
  qcompare (cork_to_utf8 (cork_byte (0xD3)), "Ó");  // U+00D3 Ó
  qcompare (cork_to_utf8 (cork_byte (0xD4)), "Ô");  // U+00D4 Ô
  qcompare (cork_to_utf8 (cork_byte (0xD5)), "Õ");  // U+00D5 Õ
  qcompare (cork_to_utf8 (cork_byte (0xD6)), "Ö");  // U+00D6 Ö
  qcompare (cork_to_utf8 (cork_byte (0xD7)), "Œ");  // U+0152 Œ
  qcompare (cork_to_utf8 (cork_byte (0xD8)), "Ø");  // U+00D8 Ø
  qcompare (cork_to_utf8 (cork_byte (0xD9)), "Ù");  // U+00D9 Ù
  qcompare (cork_to_utf8 (cork_byte (0xDA)), "Ú");  // U+00DA Ú
  qcompare (cork_to_utf8 (cork_byte (0xDB)), "Û");  // U+00DB Û
  qcompare (cork_to_utf8 (cork_byte (0xDC)), "Ü");  // U+00DC Ü
  qcompare (cork_to_utf8 (cork_byte (0xDD)), "Ý");  // U+00DD Ý
  qcompare (cork_to_utf8 (cork_byte (0xDE)), "Þ");  // U+00DE Þ
  qcompare (cork_to_utf8 (cork_byte (0xDF)), "SS"); // sharp s (oneway)
}

void
TestConverter::test_cork_to_utf8_Ex () {
  qcompare (cork_to_utf8 (cork_byte (0xE0)), "à"); // U+00E0 à
  qcompare (cork_to_utf8 (cork_byte (0xE1)), "á"); // U+00E1 á
  qcompare (cork_to_utf8 (cork_byte (0xE2)), "â"); // U+00E2 â
  qcompare (cork_to_utf8 (cork_byte (0xE3)), "ã"); // U+00E3 ã
  qcompare (cork_to_utf8 (cork_byte (0xE4)), "ä"); // U+00E4 ä
  qcompare (cork_to_utf8 (cork_byte (0xE5)), "å"); // U+00E5 å
  qcompare (cork_to_utf8 (cork_byte (0xE6)), "æ"); // U+00E6 æ
  qcompare (cork_to_utf8 (cork_byte (0xE7)), "ç"); // U+00E7 ç
  qcompare (cork_to_utf8 (cork_byte (0xE8)), "è"); // U+00E8 è
  qcompare (cork_to_utf8 (cork_byte (0xE9)), "é"); // U+00E9 é
  qcompare (cork_to_utf8 (cork_byte (0xEA)), "ê"); // U+00EA ê
  qcompare (cork_to_utf8 (cork_byte (0xEB)), "ë"); // U+00EB ë
  qcompare (cork_to_utf8 (cork_byte (0xEC)), "ì"); // U+00EC ì
  qcompare (cork_to_utf8 (cork_byte (0xED)), "í"); // U+00ED í
  qcompare (cork_to_utf8 (cork_byte (0xEE)), "î"); // U+00EE î
  qcompare (cork_to_utf8 (cork_byte (0xEF)), "ï"); // U+00EF ï
}

void
TestConverter::test_cork_to_utf8_Fx () {
  qcompare (cork_to_utf8 (cork_byte (0xF0)), "ð"); // U+00F0 ð
  qcompare (cork_to_utf8 (cork_byte (0xF1)), "ñ"); // U+00F1 ñ
  qcompare (cork_to_utf8 (cork_byte (0xF2)), "ò"); // U+00F2 ò
  qcompare (cork_to_utf8 (cork_byte (0xF3)), "ó"); // U+00F3 ó
  qcompare (cork_to_utf8 (cork_byte (0xF4)), "ô"); // U+00F4 ô
  qcompare (cork_to_utf8 (cork_byte (0xF5)), "õ"); // U+00F5 õ
  qcompare (cork_to_utf8 (cork_byte (0xF6)), "ö"); // U+00F6 ö
  qcompare (cork_to_utf8 (cork_byte (0xF7)), "œ"); // U+0153 œ
  qcompare (cork_to_utf8 (cork_byte (0xF8)), "ø"); // U+00F8 ø
  qcompare (cork_to_utf8 (cork_byte (0xF9)), "ù"); // U+00F9 ù
  qcompare (cork_to_utf8 (cork_byte (0xFA)), "ú"); // U+00FA ú
  qcompare (cork_to_utf8 (cork_byte (0xFB)), "û"); // U+00FB û
  qcompare (cork_to_utf8 (cork_byte (0xFC)), "ü"); // U+00FC ü
  qcompare (cork_to_utf8 (cork_byte (0xFD)), "ý"); // U+00FD ý
  qcompare (cork_to_utf8 (cork_byte (0xFE)), "þ"); // U+00FE þ
  qcompare (cork_to_utf8 (cork_byte (0xFF)), "ß"); // U+00DF ß
}

void
TestConverter::test_cork_to_utf8_escapes () {
  // <#XXXX> decodes to the UTF-8 encoding of the hex codepoint.
  qcompare (cork_to_utf8 ("<#4E2D>"), "中");  // U+4E2D
  qcompare (cork_to_utf8 ("<#2019>"), "’");   // U+2019
  qcompare (cork_to_utf8 ("<#1F600>"), "😀"); // U+1F600
  // Padded forms are accepted.
  qcompare (cork_to_utf8 ("<#0FF>"), "ÿ");  // U+00FF
  qcompare (cork_to_utf8 ("<#00FF>"), "ÿ"); // U+00FF
  // A <#XXXX> that names a Cork-mapped codepoint overrides the byte mapping.
  qcompare (cork_to_utf8 ("<#201C>"), "“"); // U+201C
}

void
TestConverter::test_cork_to_utf8_named_entities () {
  qcompare (cork_to_utf8 ("<less>"), "<");
  qcompare (cork_to_utf8 ("<gtr>"), ">");
  qcompare (cork_to_utf8 ("<comma>"), ",");
  qcompare (cork_to_utf8 ("<grave>"), "`");
  // A single byte '<' or '>' is passed through as the literal character.
  qcompare (cork_to_utf8 ("<"), "<");
  qcompare (cork_to_utf8 (">"), ">");
}

void
TestConverter::test_utf8_to_cork_named_entities () {
  qcompare (utf8_to_cork ("<"), "<less>");
  qcompare (utf8_to_cork (">"), "<gtr>");
  qcompare (utf8_to_cork (" "), "<varspace>"); // U+00A0
}

void
TestConverter::test_mixed () {
  // Cork byte + <#XXXX> escape + ASCII, decoded.
  qcompare (
      cork_to_utf8 (cork_byte (0x41) * string ("<#2019>") * cork_byte (0x42)),
      "A" * string ("’") * "B");
  // ASCII + CJK + ASCII, encoded.
  qcompare (utf8_to_cork (string ("A") * "中" * "B"),
            cork_byte (0x41) * string ("<#4E2D>") * cork_byte (0x42));
  // Named entity interleaved with bytes.
  qcompare (cork_to_utf8 ("<less>" * cork_byte (0x41) * "<gtr>"), "<A>");
}

void
TestConverter::test_empty () {
  qcompare (cork_to_utf8 (""), "");
  qcompare (utf8_to_cork (""), "");
}

void
TestConverter::test_roundtrip_idempotent () {
  // Every roundtrippable Cork byte satisfies utf8_to_cork(cork_to_utf8(b)) ==
  // b. The 5 non-roundtrip bytes (0x18, 0x1A, 0x3C, 0x3E, 0xDF) are documented
  // in devel/1125.md.
  for (int b= 0; b < 256; b++) {
    if (b == 0x18 || b == 0x1A || b == 0x3C || b == 0x3E || b == 0xDF) continue;
    string cork= cork_byte (b);
    string back= utf8_to_cork (cork_to_utf8 (cork));
    QCOMPARE (back, cork);
  }
}

/******************************************************************************
 * Named entities (<name>)
 *
 * cork_to_utf8 decodes a registered <name> to its UTF-8 codepoint via the
 * tmuniversaltounicode / symbol-unicode / cork-unicode-oneway tables.
 * utf8_to_cork encodes a Unicode codepoint back to its <name> (when no Cork
 * byte exists) via the reverse tables. For most entities the roundtrip is
 * identity, but a handful decode to a codepoint that has a direct Cork byte
 * (e.g. <sterling> -> U+00A3 -> Cork 0xBF) and so do not round-trip to the
 * name; those are covered in test_named_non_identity_roundtrip.
 ******************************************************************************/

void
TestConverter::test_named_text_punctuation () {
  qcompare (cork_to_utf8 ("<less>"), "<");
  qcompare (cork_to_utf8 ("<gtr>"), ">");
  qcompare (cork_to_utf8 ("<comma>"), ",");
  qcompare (cork_to_utf8 ("<grave>"), "`");
  qcompare (cork_to_utf8 ("<varspace>"), " ");  // U+00A0 nbsp
  qcompare (cork_to_utf8 ("<bullet>"), "•");    // U+2022
  qcompare (cork_to_utf8 ("<dag>"), "†");       // U+2020 dagger
  qcompare (cork_to_utf8 ("<ddag>"), "‡");      // U+2021 double dagger
  qcompare (cork_to_utf8 ("<paragraph>"), "¶"); // U+00B6 pilcrow
  qcompare (cork_to_utf8 ("<copyright>"), "©"); // U+00A9
  qcompare (cork_to_utf8 ("<trademark>"), "™"); // U+2122
  qcompare (cork_to_utf8 ("<degree>"), "°");    // U+00B0
  qcompare (cork_to_utf8 ("<hyphen>"), "­");    // U+00AD soft hyphen
  qcompare (cork_to_utf8 ("<nbhyph>"),
            "‑"); // U+2011 non-breaking hyphen
}

void
TestConverter::test_named_currency () {
  qcompare (cork_to_utf8 ("<cent>"), "¢");     // U+00A2
  qcompare (cork_to_utf8 ("<yen>"), "¥");      // U+00A5
  qcompare (cork_to_utf8 ("<currency>"), "¤"); // U+00A4
  // <sterling> decodes to U+00A3 but round-trips to Cork byte 0xBF;
  // see test_named_non_identity_roundtrip.
  qcompare (cork_to_utf8 ("<sterling>"), "£"); // U+00A3
}

void
TestConverter::test_named_greek_lower () {
  qcompare (cork_to_utf8 ("<alpha>"), "α");   // U+03B1
  qcompare (cork_to_utf8 ("<beta>"), "β");    // U+03B2
  qcompare (cork_to_utf8 ("<gamma>"), "γ");   // U+03B3
  qcompare (cork_to_utf8 ("<delta>"), "δ");   // U+03B4
  qcompare (cork_to_utf8 ("<epsilon>"), "ϵ"); // U+03F5 lunate epsilon
  qcompare (cork_to_utf8 ("<zeta>"), "ζ");    // U+03B6
  qcompare (cork_to_utf8 ("<eta>"), "η");     // U+03B7
  qcompare (cork_to_utf8 ("<theta>"), "θ");   // U+03B8
  qcompare (cork_to_utf8 ("<iota>"), "ι");    // U+03B9
  qcompare (cork_to_utf8 ("<kappa>"), "κ");   // U+03BA
  qcompare (cork_to_utf8 ("<lambda>"), "λ");  // U+03BB
  qcompare (cork_to_utf8 ("<mu>"), "μ");      // U+03BC
  qcompare (cork_to_utf8 ("<nu>"), "ν");      // U+03BD
  qcompare (cork_to_utf8 ("<xi>"), "ξ");      // U+03BE
  qcompare (cork_to_utf8 ("<omicron>"), "ο"); // U+03BF
  qcompare (cork_to_utf8 ("<pi>"), "π");      // U+03C0
  qcompare (cork_to_utf8 ("<rho>"), "ρ");     // U+03C1
  qcompare (cork_to_utf8 ("<sigma>"), "σ");   // U+03C3
  qcompare (cork_to_utf8 ("<tau>"), "τ");     // U+03C4
  qcompare (cork_to_utf8 ("<upsilon>"), "υ"); // U+03C5
  qcompare (cork_to_utf8 ("<phi>"), "ϕ");     // U+03D5 phi
  qcompare (cork_to_utf8 ("<chi>"), "χ");     // U+03C7
  qcompare (cork_to_utf8 ("<psi>"), "ψ");     // U+03C8
  qcompare (cork_to_utf8 ("<omega>"), "ω");   // U+03C9
}

void
TestConverter::test_named_greek_upper () {
  qcompare (cork_to_utf8 ("<Alpha>"), "Α");   // U+0391
  qcompare (cork_to_utf8 ("<Beta>"), "Β");    // U+0392
  qcompare (cork_to_utf8 ("<Gamma>"), "Γ");   // U+0393
  qcompare (cork_to_utf8 ("<Delta>"), "Δ");   // U+0394
  qcompare (cork_to_utf8 ("<Epsilon>"), "Ε"); // U+0395
  qcompare (cork_to_utf8 ("<Zeta>"), "Ζ");    // U+0396
  qcompare (cork_to_utf8 ("<Eta>"), "Η");     // U+0397
  qcompare (cork_to_utf8 ("<Theta>"), "Θ");   // U+0398
  qcompare (cork_to_utf8 ("<Iota>"), "Ι");    // U+0399
  qcompare (cork_to_utf8 ("<Kappa>"), "Κ");   // U+039A
  qcompare (cork_to_utf8 ("<Lambda>"), "Λ");  // U+039B
  qcompare (cork_to_utf8 ("<Mu>"), "Μ");      // U+039C
  qcompare (cork_to_utf8 ("<Nu>"), "Ν");      // U+039D
  qcompare (cork_to_utf8 ("<Xi>"), "Ξ");      // U+039E
  qcompare (cork_to_utf8 ("<Omicron>"), "Ο"); // U+039F
  qcompare (cork_to_utf8 ("<Pi>"), "Π");      // U+03A0
  qcompare (cork_to_utf8 ("<Rho>"), "Ρ");     // U+03A1
  qcompare (cork_to_utf8 ("<Sigma>"), "Σ");   // U+03A3
  qcompare (cork_to_utf8 ("<Tau>"), "Τ");     // U+03A4
  qcompare (cork_to_utf8 ("<Upsilon>"), "Υ"); // U+03A5
  qcompare (cork_to_utf8 ("<Phi>"), "Φ");     // U+03A6
  qcompare (cork_to_utf8 ("<Chi>"), "Χ");     // U+03A7
  qcompare (cork_to_utf8 ("<Psi>"), "Ψ");     // U+03A8
  qcompare (cork_to_utf8 ("<Omega>"), "Ω");   // U+03A9
}

void
TestConverter::test_named_greek_variants () {
  // Note: <epsilon> -> U+03F5 (lunate) while <varepsilon> -> U+03B5 (plain),
  // and <phi> -> U+03D5 while <varphi> -> U+03C6; the variant and plain forms
  // are swapped relative to the usual convention.
  qcompare (cork_to_utf8 ("<varepsilon>"), "ε"); // U+03B5
  qcompare (cork_to_utf8 ("<vartheta>"), "ϑ");   // U+03D1
  qcompare (cork_to_utf8 ("<varpi>"), "ϖ");      // U+03D6
  qcompare (cork_to_utf8 ("<varrho>"), "ϱ");     // U+03F1
  qcompare (cork_to_utf8 ("<varsigma>"), "ς");   // U+03C2
  qcompare (cork_to_utf8 ("<varphi>"), "φ");     // U+03C6
}

void
TestConverter::test_named_binary_operators () {
  qcompare (cork_to_utf8 ("<times>"), "×");    // U+00D7
  qcompare (cork_to_utf8 ("<div>"), "÷");      // U+00F7
  qcompare (cork_to_utf8 ("<cdot>"), "⋅");     // U+22C5
  qcompare (cork_to_utf8 ("<ast>"), "∗");      // U+2217
  qcompare (cork_to_utf8 ("<dotplus>"), "∔");  // U+2214
  qcompare (cork_to_utf8 ("<cap>"), "∩");      // U+2229
  qcompare (cork_to_utf8 ("<cup>"), "∪");      // U+222A
  qcompare (cork_to_utf8 ("<sqcap>"), "⊓");    // U+2293
  qcompare (cork_to_utf8 ("<sqcup>"), "⊔");    // U+2294
  qcompare (cork_to_utf8 ("<wedge>"), "∧");    // U+2227
  qcompare (cork_to_utf8 ("<vee>"), "∨");      // U+2228
  qcompare (cork_to_utf8 ("<setminus>"), "∖"); // U+2216
  qcompare (cork_to_utf8 ("<amalg>"), "⨿");    // U+2A3F
  qcompare (cork_to_utf8 ("<wr>"), "≀");       // U+2240
}

void
TestConverter::test_named_relations () {
  qcompare (cork_to_utf8 ("<neq>"), "≠");      // U+2260
  qcompare (cork_to_utf8 ("<leq>"), "≤");      // U+2264
  qcompare (cork_to_utf8 ("<geq>"), "≥");      // U+2265
  qcompare (cork_to_utf8 ("<leqslant>"), "⩽"); // U+2A7D
  qcompare (cork_to_utf8 ("<geqslant>"), "⩾"); // U+2A7E
  qcompare (cork_to_utf8 ("<ll>"), "≪");       // U+226A
  qcompare (cork_to_utf8 ("<gg>"), "≫");       // U+226B
  qcompare (cork_to_utf8 ("<equiv>"), "≡");    // U+2261
  qcompare (cork_to_utf8 ("<sim>"), "∼");      // U+223C
  qcompare (cork_to_utf8 ("<simeq>"), "≃");    // U+2243
  qcompare (cork_to_utf8 ("<cong>"), "≅");     // U+2245
  qcompare (cork_to_utf8 ("<approx>"), "≈");   // U+2248
  qcompare (cork_to_utf8 ("<prec>"), "≺");     // U+227A
  qcompare (cork_to_utf8 ("<succ>"), "≻");     // U+227B
}

void
TestConverter::test_named_arrows () {
  qcompare (cork_to_utf8 ("<rightarrow>"), "→");     // U+2192
  qcompare (cork_to_utf8 ("<leftarrow>"), "←");      // U+2190
  qcompare (cork_to_utf8 ("<leftrightarrow>"), "↔"); // U+2194
  qcompare (cork_to_utf8 ("<Rightarrow>"), "⇒");     // U+21D2
  qcompare (cork_to_utf8 ("<Leftarrow>"), "⇐");      // U+21D0
  qcompare (cork_to_utf8 ("<uparrow>"), "↑");        // U+2191
  qcompare (cork_to_utf8 ("<downarrow>"), "↓");      // U+2193
  qcompare (cork_to_utf8 ("<mapsto>"), "↦");         // U+21A6
  qcompare (cork_to_utf8 ("<leftharpoonup>"), "↼");  // U+21BC
  qcompare (cork_to_utf8 ("<rightharpoonup>"), "⇀"); // U+21C0
}

void
TestConverter::test_named_set_theory () {
  qcompare (cork_to_utf8 ("<in>"), "∈");       // U+2208
  qcompare (cork_to_utf8 ("<notin>"), "∉");    // U+2209
  qcompare (cork_to_utf8 ("<subset>"), "⊂");   // U+2282
  qcompare (cork_to_utf8 ("<supset>"), "⊃");   // U+2283
  qcompare (cork_to_utf8 ("<subseteq>"), "⊆"); // U+2286
  qcompare (cork_to_utf8 ("<supseteq>"), "⊇"); // U+2287
  qcompare (cork_to_utf8 ("<sqsubset>"), "⊏"); // U+228F
  qcompare (cork_to_utf8 ("<sqsupset>"), "⊐"); // U+2290
  qcompare (cork_to_utf8 ("<emptyset>"), "∅"); // U+2205
}

void
TestConverter::test_named_calculus () {
  qcompare (cork_to_utf8 ("<partial>"), "∂"); // U+2202
  qcompare (cork_to_utf8 ("<nabla>"), "∇");   // U+2207
  qcompare (cork_to_utf8 ("<infty>"), "∞");   // U+221E
  qcompare (cork_to_utf8 ("<sqrt>"), "√");    // U+221A
  qcompare (cork_to_utf8 ("<forall>"), "∀");  // U+2200
  qcompare (cork_to_utf8 ("<exists>"), "∃");  // U+2203
  qcompare (cork_to_utf8 ("<angle>"), "∠");   // U+2220
  qcompare (cork_to_utf8 ("<aleph>"), "ℵ");   // U+2135
  // <sum> decodes to U+2211 but round-trips to <big-sum>; see non-identity
  // test.
  qcompare (cork_to_utf8 ("<sum>"), "∑");  // U+2211
  qcompare (cork_to_utf8 ("<int>"), "∫");  // U+222B
  qcompare (cork_to_utf8 ("<prod>"), "∏"); // U+220F
}

void
TestConverter::test_named_dots_dashes () {
  qcompare (cork_to_utf8 ("<cdots>"), "⋯");     // U+22EF
  qcompare (cork_to_utf8 ("<ldots>"), "…");     // U+2026
  qcompare (cork_to_utf8 ("<vdots>"), "⋮");     // U+22EE
  qcompare (cork_to_utf8 ("<ddots>"), "⋱");     // U+22F1
  qcompare (cork_to_utf8 ("<prime>"), "′");     // U+2032
  qcompare (cork_to_utf8 ("<backprime>"), "‵"); // U+2035
}

void
TestConverter::test_named_special_letterforms () {
  qcompare (cork_to_utf8 ("<aleph>"), "ℵ");  // U+2135
  qcompare (cork_to_utf8 ("<beth>"), "ℶ");   // U+2136
  qcompare (cork_to_utf8 ("<gimel>"), "ℷ");  // U+2137
  qcompare (cork_to_utf8 ("<daleth>"), "ℸ"); // U+2138
  qcompare (cork_to_utf8 ("<ell>"), "ℓ");    // U+2113
  // <hbar> decodes to U+210F but round-trips to <hslash>; see non-identity
  // test.
  qcompare (cork_to_utf8 ("<hbar>"), "ℏ");   // U+210F
  qcompare (cork_to_utf8 ("<hslash>"), "ℏ"); // U+210F
  // <Re> / <Im> decode to U+211C / U+2111 but round-trip to <frak-R>/<frak-I>.
  qcompare (cork_to_utf8 ("<Re>"), "ℜ"); // U+211C
  qcompare (cork_to_utf8 ("<Im>"), "ℑ"); // U+2111
}

void
TestConverter::test_named_non_identity_roundtrip () {
  // These entities decode to a codepoint that has a direct Cork byte mapping
  // (preferred over the named entity), so utf8_to_cork does not recover the
  // original name. Documented in devel/1125.md.
  qcompare (utf8_to_cork (cork_to_utf8 ("<sterling>")),
            cork_byte (0xBF)); // £ is Cork 0xBF
  qcompare (utf8_to_cork (cork_to_utf8 ("<guillemotleft>")),
            cork_byte (0x13)); // « is Cork 0x13
  qcompare (utf8_to_cork (cork_to_utf8 ("<guillemotright>")),
            cork_byte (0x14)); // » is Cork 0x14
  // These decode to a codepoint whose canonical reverse name differs.
  qcompare (utf8_to_cork (cork_to_utf8 ("<sum>")), "<big-sum>");
  qcompare (utf8_to_cork (cork_to_utf8 ("<hbar>")), "<hslash>");
  qcompare (utf8_to_cork (cork_to_utf8 ("<Re>")), "<frak-R>");
  qcompare (utf8_to_cork (cork_to_utf8 ("<Im>")), "<frak-I>");
  // Most named entities DO round-trip to themselves; sample a few.
  qcompare (utf8_to_cork (cork_to_utf8 ("<alpha>")), "<alpha>");
  qcompare (utf8_to_cork (cork_to_utf8 ("<infty>")), "<infty>");
  qcompare (utf8_to_cork (cork_to_utf8 ("<partial>")), "<partial>");
  qcompare (utf8_to_cork (cork_to_utf8 ("<rightarrow>")), "<rightarrow>");
  qcompare (utf8_to_cork (cork_to_utf8 ("<emptyset>")), "<emptyset>");
}

// utf8_to_cork direction: codepoints without a direct Cork byte are encoded
// as their canonical named entity. The entity name is not always the obvious
// one (e.g. U+2209 -> <nin>, U+2211 -> <big-sum>, U+211C -> <frak-R>); these
// remaps are sampled in test_utf8_to_cork_named_remap.

void
TestConverter::test_utf8_to_cork_greek () {
  qcompare (utf8_to_cork ("α"), "<alpha>"); // U+03B1
  qcompare (utf8_to_cork ("β"), "<beta>");  // U+03B2
  qcompare (utf8_to_cork ("γ"), "<gamma>"); // U+03B3
  qcompare (utf8_to_cork ("δ"), "<delta>"); // U+03B4
  qcompare (utf8_to_cork ("π"), "<pi>");    // U+03C0
  qcompare (utf8_to_cork ("σ"), "<sigma>"); // U+03C3
  qcompare (utf8_to_cork ("ω"), "<omega>"); // U+03C9
  qcompare (utf8_to_cork ("Α"), "<Alpha>"); // U+0391
  qcompare (utf8_to_cork ("Β"), "<Beta>");  // U+0392
  qcompare (utf8_to_cork ("Γ"), "<Gamma>"); // U+0393
  qcompare (utf8_to_cork ("Δ"), "<Delta>"); // U+0394
  qcompare (utf8_to_cork ("Π"), "<Pi>");    // U+03A0
  qcompare (utf8_to_cork ("Σ"), "<Sigma>"); // U+03A3
  qcompare (utf8_to_cork ("Ω"), "<Omega>"); // U+03A9
}

void
TestConverter::test_utf8_to_cork_math_ops () {
  qcompare (utf8_to_cork ("×"), "<times>");    // U+00D7
  qcompare (utf8_to_cork ("÷"), "<div>");      // U+00F7
  qcompare (utf8_to_cork ("⋅"), "<cdot>");     // U+22C5
  qcompare (utf8_to_cork ("∗"), "<ast>");      // U+2217
  qcompare (utf8_to_cork ("∩"), "<cap>");      // U+2229
  qcompare (utf8_to_cork ("∪"), "<cup>");      // U+222A
  qcompare (utf8_to_cork ("∧"), "<wedge>");    // U+2227
  qcompare (utf8_to_cork ("∨"), "<vee>");      // U+2228
  qcompare (utf8_to_cork ("∖"), "<setminus>"); // U+2216
}

void
TestConverter::test_utf8_to_cork_math_syms () {
  qcompare (utf8_to_cork ("∞"), "<infty>");          // U+221E
  qcompare (utf8_to_cork ("∂"), "<partial>");        // U+2202
  qcompare (utf8_to_cork ("∇"), "<nabla>");          // U+2207
  qcompare (utf8_to_cork ("∀"), "<forall>");         // U+2200
  qcompare (utf8_to_cork ("∃"), "<exists>");         // U+2203
  qcompare (utf8_to_cork ("∠"), "<angle>");          // U+2220
  qcompare (utf8_to_cork ("∅"), "<emptyset>");       // U+2205
  qcompare (utf8_to_cork ("≠"), "<neq>");            // U+2260
  qcompare (utf8_to_cork ("≤"), "<leq>");            // U+2264
  qcompare (utf8_to_cork ("≥"), "<geq>");            // U+2265
  qcompare (utf8_to_cork ("≡"), "<equiv>");          // U+2261
  qcompare (utf8_to_cork ("≈"), "<approx>");         // U+2248
  qcompare (utf8_to_cork ("∈"), "<in>");             // U+2208
  qcompare (utf8_to_cork ("⊂"), "<subset>");         // U+2282
  qcompare (utf8_to_cork ("⊃"), "<supset>");         // U+2283
  qcompare (utf8_to_cork ("→"), "<rightarrow>");     // U+2192
  qcompare (utf8_to_cork ("←"), "<leftarrow>");      // U+2190
  qcompare (utf8_to_cork ("↔"), "<leftrightarrow>"); // U+2194
  qcompare (utf8_to_cork ("⇒"), "<Rightarrow>");     // U+21D2
  qcompare (utf8_to_cork ("⇐"), "<Leftarrow>");      // U+21D0
  qcompare (utf8_to_cork ("↦"), "<mapsto>");         // U+21A6
  qcompare (utf8_to_cork ("ℵ"), "<aleph>");          // U+2135
  qcompare (utf8_to_cork ("ℓ"), "<ell>");            // U+2113
}

void
TestConverter::test_utf8_to_cork_text_syms () {
  qcompare (utf8_to_cork ("•"), "<bullet>");    // U+2022
  qcompare (utf8_to_cork ("†"), "<dagger>");    // U+2020
  qcompare (utf8_to_cork ("‡"), "<ddagger>");   // U+2021
  qcompare (utf8_to_cork ("°"), "<degree>");    // U+00B0
  qcompare (utf8_to_cork ("©"), "<copyright>"); // U+00A9
  qcompare (utf8_to_cork ("®"), "<circledR>");  // U+00AE
  qcompare (utf8_to_cork ("™"), "<trademark>"); // U+2122
  qcompare (utf8_to_cork ("…"), "<ldots>");     // U+2026
}

void
TestConverter::test_utf8_to_cork_named_remap () {
  // Canonical reverse names that differ from the obvious/forward entity name.
  qcompare (utf8_to_cork ("∉"), "<nin>");      // U+2209 (not <notin>)
  qcompare (utf8_to_cork ("∑"), "<big-sum>");  // U+2211 (not <sum>)
  qcompare (utf8_to_cork ("∫"), "<big-int>");  // U+222B (not <int>)
  qcompare (utf8_to_cork ("∏"), "<big-prod>"); // U+220F (not <prod>)
  qcompare (utf8_to_cork ("ℏ"), "<hslash>");   // U+210F (not <hbar>)
  qcompare (utf8_to_cork ("ℜ"), "<frak-R>");   // U+211C (not <Re>)
  qcompare (utf8_to_cork ("ℑ"), "<frak-I>");   // U+2111 (not <Im>)
}

QTEST_MAIN (TestConverter)
#include "converter_test.moc"
