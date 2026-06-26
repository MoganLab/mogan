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
  qcompare (utf8_to_cork ("\xCB\x86"),
            cork_byte (0x02)); // U+02C6 modifier circumflex
  qcompare (utf8_to_cork ("\xCB\x9C"), cork_byte (0x03)); // U+02DC small tilde
  qcompare (utf8_to_cork ("\xC2\xA8"), cork_byte (0x04)); // U+00A8 diaeresis
  qcompare (utf8_to_cork ("\xCB\x9D"), cork_byte (0x05)); // U+02DD double acute
  qcompare (utf8_to_cork ("\xCB\x9A"), cork_byte (0x06)); // U+02DA ring above
  qcompare (utf8_to_cork ("\xCB\x87"), cork_byte (0x07)); // U+02C7 caron
  qcompare (utf8_to_cork ("\xCB\x98"), cork_byte (0x08)); // U+02D8 breve
  qcompare (utf8_to_cork ("\xC2\xAF"), cork_byte (0x09)); // U+00AF macron
  qcompare (utf8_to_cork ("\xCB\x99"), cork_byte (0x0A)); // U+02D9 dot above
  qcompare (utf8_to_cork ("\xC2\xB8"), cork_byte (0x0B)); // U+00B8 cedilla
  qcompare (utf8_to_cork ("\xCB\x9B"), cork_byte (0x0C)); // U+02DB ogonek
  qcompare (utf8_to_cork ("\xC2\xB4"), cork_byte (0x01)); // U+00B4 acute
}

void
TestConverter::test_utf8_to_cork_punctuation () {
  qcompare (utf8_to_cork ("\xE2\x80\x9A"), cork_byte (0x0D)); // U+201A
  qcompare (utf8_to_cork ("\xE2\x80\xB9"), cork_byte (0x0E)); // U+2039
  qcompare (utf8_to_cork ("\xE2\x80\xBA"), cork_byte (0x0F)); // U+203A
  qcompare (utf8_to_cork ("\xE2\x80\x9C"), cork_byte (0x10)); // U+201C
  qcompare (utf8_to_cork ("\xE2\x80\x9D"), cork_byte (0x11)); // U+201D
  qcompare (utf8_to_cork ("\xE2\x80\x9E"), cork_byte (0x12)); // U+201E
  qcompare (utf8_to_cork ("\xC2\xAB"), cork_byte (0x13));     // U+00AB
  qcompare (utf8_to_cork ("\xC2\xBB"), cork_byte (0x14));     // U+00BB
  qcompare (utf8_to_cork ("\xE2\x80\x93"), cork_byte (0x15)); // U+2013
  qcompare (utf8_to_cork ("\xE2\x80\x94"), cork_byte (0x16)); // U+2014
  qcompare (utf8_to_cork ("\xE2\x81\xA0"),
            cork_byte (0x17)); // U+2060 WORD JOINER
  qcompare (utf8_to_cork ("\xE2\x80\x98"),
            cork_byte (0x60)); // U+2018 left single quote
}

void
TestConverter::test_utf8_to_cork_specials () {
  qcompare (utf8_to_cork ("\xC4\xB1"), cork_byte (0x19)); // U+0131 dotless i
  qcompare (utf8_to_cork ("\xEF\xAC\x80"), cork_byte (0x1B)); // U+FB00 ff
  qcompare (utf8_to_cork ("\xEF\xAC\x81"), cork_byte (0x1C)); // U+FB01 fi
  qcompare (utf8_to_cork ("\xEF\xAC\x82"), cork_byte (0x1D)); // U+FB02 fl
  qcompare (utf8_to_cork ("\xEF\xAC\x83"), cork_byte (0x1E)); // U+FB03 ffi
  qcompare (utf8_to_cork ("\xEF\xAC\x84"), cork_byte (0x1F)); // U+FB04 ffl
}

void
TestConverter::test_utf8_to_cork_upper () {
  qcompare (utf8_to_cork ("\xC4\x82"), cork_byte (0x80)); // U+0102
  qcompare (utf8_to_cork ("\xC4\x84"), cork_byte (0x81)); // U+0104
  qcompare (utf8_to_cork ("\xC4\x86"), cork_byte (0x82)); // U+0106
  qcompare (utf8_to_cork ("\xC4\x8C"), cork_byte (0x83)); // U+010C
  qcompare (utf8_to_cork ("\xC4\x8E"), cork_byte (0x84)); // U+010E
  qcompare (utf8_to_cork ("\xC4\x9A"), cork_byte (0x85)); // U+011A
  qcompare (utf8_to_cork ("\xC4\x98"), cork_byte (0x86)); // U+0118
  qcompare (utf8_to_cork ("\xC4\x9E"), cork_byte (0x87)); // U+011E
  qcompare (utf8_to_cork ("\xC4\xB9"), cork_byte (0x88)); // U+0139
  qcompare (utf8_to_cork ("\xC4\xBD"), cork_byte (0x89)); // U+013D
  qcompare (utf8_to_cork ("\xC5\x81"), cork_byte (0x8A)); // U+0141
  qcompare (utf8_to_cork ("\xC5\x83"), cork_byte (0x8B)); // U+0143
  qcompare (utf8_to_cork ("\xC5\x87"), cork_byte (0x8C)); // U+0147
  qcompare (utf8_to_cork ("\xC5\x8A"), cork_byte (0x8D)); // U+014A
  qcompare (utf8_to_cork ("\xC5\x90"), cork_byte (0x8E)); // U+0150
  qcompare (utf8_to_cork ("\xC5\x94"), cork_byte (0x8F)); // U+0154
  qcompare (utf8_to_cork ("\xC5\x98"), cork_byte (0x90)); // U+0158
  qcompare (utf8_to_cork ("\xC5\x9A"), cork_byte (0x91)); // U+015A
  qcompare (utf8_to_cork ("\xC5\xA0"), cork_byte (0x92)); // U+0160
  qcompare (utf8_to_cork ("\xC5\x9E"), cork_byte (0x93)); // U+015E
  qcompare (utf8_to_cork ("\xC5\xA4"), cork_byte (0x94)); // U+0164
  qcompare (utf8_to_cork ("\xC5\xA2"), cork_byte (0x95)); // U+0162
  qcompare (utf8_to_cork ("\xC5\xB0"), cork_byte (0x96)); // U+0170
  qcompare (utf8_to_cork ("\xC5\xAE"), cork_byte (0x97)); // U+016E
  qcompare (utf8_to_cork ("\xC5\xB8"), cork_byte (0x98)); // U+0178
  qcompare (utf8_to_cork ("\xC5\xB9"), cork_byte (0x99)); // U+0179
  qcompare (utf8_to_cork ("\xC5\xBD"), cork_byte (0x9A)); // U+017D
  qcompare (utf8_to_cork ("\xC5\xBB"), cork_byte (0x9B)); // U+017B
  qcompare (utf8_to_cork ("\xC4\xB2"), cork_byte (0x9C)); // U+0132
  qcompare (utf8_to_cork ("\xC4\xB0"), cork_byte (0x9D)); // U+0130
  qcompare (utf8_to_cork ("\xC4\x91"), cork_byte (0x9E)); // U+0111
  qcompare (utf8_to_cork ("\xC2\xA7"), cork_byte (0x9F)); // U+00A7 section sign
}

void
TestConverter::test_utf8_to_cork_lower () {
  qcompare (utf8_to_cork ("\xC4\x83"), cork_byte (0xA0)); // U+0103
  qcompare (utf8_to_cork ("\xC4\x85"), cork_byte (0xA1)); // U+0105
  qcompare (utf8_to_cork ("\xC4\x87"), cork_byte (0xA2)); // U+0107
  qcompare (utf8_to_cork ("\xC4\x8D"), cork_byte (0xA3)); // U+010D
  qcompare (utf8_to_cork ("\xC4\x8F"), cork_byte (0xA4)); // U+010F
  qcompare (utf8_to_cork ("\xC4\x9B"), cork_byte (0xA5)); // U+011B
  qcompare (utf8_to_cork ("\xC4\x99"), cork_byte (0xA6)); // U+0119
  qcompare (utf8_to_cork ("\xC4\x9F"), cork_byte (0xA7)); // U+011F
  qcompare (utf8_to_cork ("\xC4\xBA"), cork_byte (0xA8)); // U+013A
  qcompare (utf8_to_cork ("\xC4\xBE"), cork_byte (0xA9)); // U+013E
  qcompare (utf8_to_cork ("\xC5\x82"), cork_byte (0xAA)); // U+0142
  qcompare (utf8_to_cork ("\xC5\x84"), cork_byte (0xAB)); // U+0144
  qcompare (utf8_to_cork ("\xC5\x88"), cork_byte (0xAC)); // U+0148
  qcompare (utf8_to_cork ("\xC5\x8B"), cork_byte (0xAD)); // U+014B
  qcompare (utf8_to_cork ("\xC5\x91"), cork_byte (0xAE)); // U+0151
  qcompare (utf8_to_cork ("\xC5\x95"), cork_byte (0xAF)); // U+0155
  qcompare (utf8_to_cork ("\xC5\x99"), cork_byte (0xB0)); // U+0159
  qcompare (utf8_to_cork ("\xC5\x9B"), cork_byte (0xB1)); // U+015B
  qcompare (utf8_to_cork ("\xC5\xA1"), cork_byte (0xB2)); // U+0161
  qcompare (utf8_to_cork ("\xC5\x9F"), cork_byte (0xB3)); // U+015F
  qcompare (utf8_to_cork ("\xC5\xA5"), cork_byte (0xB4)); // U+0165
  qcompare (utf8_to_cork ("\xC5\xA3"), cork_byte (0xB5)); // U+0163
  qcompare (utf8_to_cork ("\xC5\xB1"), cork_byte (0xB6)); // U+0171
  qcompare (utf8_to_cork ("\xC5\xAF"), cork_byte (0xB7)); // U+016F
  qcompare (utf8_to_cork ("\xC3\xBF"), cork_byte (0xB8)); // U+00FF
  qcompare (utf8_to_cork ("\xC5\xBA"), cork_byte (0xB9)); // U+017A
  qcompare (utf8_to_cork ("\xC5\xBE"), cork_byte (0xBA)); // U+017E
  qcompare (utf8_to_cork ("\xC5\xBC"), cork_byte (0xBB)); // U+017C
  qcompare (utf8_to_cork ("\xC4\xB3"), cork_byte (0xBC)); // U+0133
  qcompare (utf8_to_cork ("\xC2\xA1"), cork_byte (0xBD)); // U+00A1
  qcompare (utf8_to_cork ("\xC2\xBF"), cork_byte (0xBE)); // U+00BF
  qcompare (utf8_to_cork ("\xC2\xA3"), cork_byte (0xBF)); // U+00A3 pound sign
}

void
TestConverter::test_utf8_to_cork_latin1 () {
  qcompare (utf8_to_cork ("\xC3\x80"), cork_byte (0xC0)); // U+00C0 À
  qcompare (utf8_to_cork ("\xC3\x81"), cork_byte (0xC1)); // U+00C1 Á
  qcompare (utf8_to_cork ("\xC3\x82"), cork_byte (0xC2)); // U+00C2 Â
  qcompare (utf8_to_cork ("\xC3\x83"), cork_byte (0xC3)); // U+00C3 Ã
  qcompare (utf8_to_cork ("\xC3\x84"), cork_byte (0xC4)); // U+00C4 Ä
  qcompare (utf8_to_cork ("\xC3\x85"), cork_byte (0xC5)); // U+00C5 Å
  qcompare (utf8_to_cork ("\xC3\x86"), cork_byte (0xC6)); // U+00C6 Æ
  qcompare (utf8_to_cork ("\xC3\x87"), cork_byte (0xC7)); // U+00C7 Ç
  qcompare (utf8_to_cork ("\xC3\x88"), cork_byte (0xC8)); // U+00C8 È
  qcompare (utf8_to_cork ("\xC3\x89"), cork_byte (0xC9)); // U+00C9 É
  qcompare (utf8_to_cork ("\xC3\x8A"), cork_byte (0xCA)); // U+00CA Ê
  qcompare (utf8_to_cork ("\xC3\x8B"), cork_byte (0xCB)); // U+00CB Ë
  qcompare (utf8_to_cork ("\xC3\x8C"), cork_byte (0xCC)); // U+00CC Ì
  qcompare (utf8_to_cork ("\xC3\x8D"), cork_byte (0xCD)); // U+00CD Í
  qcompare (utf8_to_cork ("\xC3\x8E"), cork_byte (0xCE)); // U+00CE Î
  qcompare (utf8_to_cork ("\xC3\x8F"), cork_byte (0xCF)); // U+00CF Ï
  qcompare (utf8_to_cork ("\xC3\x90"), cork_byte (0xD0)); // U+00D0 Ð
  qcompare (utf8_to_cork ("\xC3\x91"), cork_byte (0xD1)); // U+00D1 Ñ
  qcompare (utf8_to_cork ("\xC3\x92"), cork_byte (0xD2)); // U+00D2 Ò
  qcompare (utf8_to_cork ("\xC3\x93"), cork_byte (0xD3)); // U+00D3 Ó
  qcompare (utf8_to_cork ("\xC3\x94"), cork_byte (0xD4)); // U+00D4 Ô
  qcompare (utf8_to_cork ("\xC3\x95"), cork_byte (0xD5)); // U+00D5 Õ
  qcompare (utf8_to_cork ("\xC3\x96"), cork_byte (0xD6)); // U+00D6 Ö
  qcompare (utf8_to_cork ("\xC5\x92"), cork_byte (0xD7)); // U+0152 Œ
  qcompare (utf8_to_cork ("\xC3\x98"), cork_byte (0xD8)); // U+00D8 Ø
  qcompare (utf8_to_cork ("\xC3\x99"), cork_byte (0xD9)); // U+00D9 Ù
  qcompare (utf8_to_cork ("\xC3\x9A"), cork_byte (0xDA)); // U+00DA Ú
  qcompare (utf8_to_cork ("\xC3\x9B"), cork_byte (0xDB)); // U+00DB Û
  qcompare (utf8_to_cork ("\xC3\x9C"), cork_byte (0xDC)); // U+00DC Ü
  qcompare (utf8_to_cork ("\xC3\x9D"), cork_byte (0xDD)); // U+00DD Ý
  qcompare (utf8_to_cork ("\xC3\x9E"), cork_byte (0xDE)); // U+00DE Þ
  // U+00DF (ß) maps to Cork 0xFF (corktounicode 0xFF -> U+00DF, reversible).
  // Cork 0xDF decodes to "SS" (cork-unicode-oneway), so the byte 0xDF is
  // unreachable from utf8_to_cork; see test_cork_to_utf8_Dx and devel/1125.md.
  qcompare (utf8_to_cork ("\xC3\x9F"), cork_byte (0xFF)); // U+00DF ß
  qcompare (utf8_to_cork ("\xC3\xA0"), cork_byte (0xE0)); // U+00E0 à
  qcompare (utf8_to_cork ("\xC3\xA1"), cork_byte (0xE1)); // U+00E1 á
  qcompare (utf8_to_cork ("\xC3\xA2"), cork_byte (0xE2)); // U+00E2 â
  qcompare (utf8_to_cork ("\xC3\xA3"), cork_byte (0xE3)); // U+00E3 ã
  qcompare (utf8_to_cork ("\xC3\xA4"), cork_byte (0xE4)); // U+00E4 ä
  qcompare (utf8_to_cork ("\xC3\xA5"), cork_byte (0xE5)); // U+00E5 å
  qcompare (utf8_to_cork ("\xC3\xA6"), cork_byte (0xE6)); // U+00E6 æ
  qcompare (utf8_to_cork ("\xC3\xA7"), cork_byte (0xE7)); // U+00E7 ç
  qcompare (utf8_to_cork ("\xC3\xA8"), cork_byte (0xE8)); // U+00E8 è
  qcompare (utf8_to_cork ("\xC3\xA9"), cork_byte (0xE9)); // U+00E9 é
  qcompare (utf8_to_cork ("\xC3\xAA"), cork_byte (0xEA)); // U+00EA ê
  qcompare (utf8_to_cork ("\xC3\xAB"), cork_byte (0xEB)); // U+00EB ë
  qcompare (utf8_to_cork ("\xC3\xAC"), cork_byte (0xEC)); // U+00EC ì
  qcompare (utf8_to_cork ("\xC3\xAD"), cork_byte (0xED)); // U+00ED í
  qcompare (utf8_to_cork ("\xC3\xAE"), cork_byte (0xEE)); // U+00EE î
  qcompare (utf8_to_cork ("\xC3\xAF"), cork_byte (0xEF)); // U+00EF ï
  qcompare (utf8_to_cork ("\xC3\xB0"), cork_byte (0xF0)); // U+00F0 ð
  qcompare (utf8_to_cork ("\xC3\xB1"), cork_byte (0xF1)); // U+00F1 ñ
  qcompare (utf8_to_cork ("\xC3\xB2"), cork_byte (0xF2)); // U+00F2 ò
  qcompare (utf8_to_cork ("\xC3\xB3"), cork_byte (0xF3)); // U+00F3 ó
  qcompare (utf8_to_cork ("\xC3\xB4"), cork_byte (0xF4)); // U+00F4 ô
  qcompare (utf8_to_cork ("\xC3\xB5"), cork_byte (0xF5)); // U+00F5 õ
  qcompare (utf8_to_cork ("\xC3\xB6"), cork_byte (0xF6)); // U+00F6 ö
  qcompare (utf8_to_cork ("\xC5\x93"), cork_byte (0xF7)); // U+0153 œ
  qcompare (utf8_to_cork ("\xC3\xB8"), cork_byte (0xF8)); // U+00F8 ø
  qcompare (utf8_to_cork ("\xC3\xB9"), cork_byte (0xF9)); // U+00F9 ù
  qcompare (utf8_to_cork ("\xC3\xBA"), cork_byte (0xFA)); // U+00FA ú
  qcompare (utf8_to_cork ("\xC3\xBB"), cork_byte (0xFB)); // U+00FB û
  qcompare (utf8_to_cork ("\xC3\xBC"), cork_byte (0xFC)); // U+00FC ü
  qcompare (utf8_to_cork ("\xC3\xBD"), cork_byte (0xFD)); // U+00FD ý
  qcompare (utf8_to_cork ("\xC3\xBE"), cork_byte (0xFE)); // U+00FE þ
}

void
TestConverter::test_utf8_to_cork_unmapped_high () {
  // Codepoints >= 256 with no Cork mapping are escaped as <#XXXX>.
  qcompare (utf8_to_cork ("\xE4\xB8\xAD"), "<#4E2D>");      // U+4E2D 中
  qcompare (utf8_to_cork ("\xF0\x9F\x98\x80"), "<#1F600>"); // U+1F600 😀
  qcompare (utf8_to_cork ("\xE2\x80\x8B"), "<#200B>");      // U+200B ZWSP
  // U+2019 maps to Cork 0x27 (shares ASCII apostrophe slot), not escaped.
  qcompare (utf8_to_cork ("\xE2\x80\x99"), cork_byte (0x27)); // U+2019 ’
  // U+2010 maps to Cork 0x7F (hyphen), not escaped.
  qcompare (utf8_to_cork ("\xE2\x80\x90"), cork_byte (0x7F)); // U+2010 ‐
}

void
TestConverter::test_utf8_to_cork_named_unmapped () {
  // U+00A0 maps to the <varspace> named entity (unicode-cork-oneway).
  qcompare (utf8_to_cork ("\xC2\xA0"), "<varspace>");
  // U+0060 backtick maps to Cork 0x00 (corktounicode has 0x00 -> U+0060,
  // reversible).
  qcompare (utf8_to_cork ("`"), cork_byte (0x00));
}

/******************************************************************************
 * cork_to_utf8
 ******************************************************************************/

void
TestConverter::test_cork_to_utf8_0x () {
  qcompare (cork_to_utf8 (cork_byte (0x00)), "`");        // U+0060
  qcompare (cork_to_utf8 (cork_byte (0x01)), "\xC2\xB4"); // U+00B4
  qcompare (cork_to_utf8 (cork_byte (0x02)), "\xCB\x86"); // U+02C6
  qcompare (cork_to_utf8 (cork_byte (0x03)), "\xCB\x9C"); // U+02DC
  qcompare (cork_to_utf8 (cork_byte (0x04)), "\xC2\xA8"); // U+00A8
  qcompare (cork_to_utf8 (cork_byte (0x05)), "\xCB\x9D"); // U+02DD
  qcompare (cork_to_utf8 (cork_byte (0x06)), "\xCB\x9A"); // U+02DA
  qcompare (cork_to_utf8 (cork_byte (0x07)), "\xCB\x87"); // U+02C7
  qcompare (cork_to_utf8 (cork_byte (0x08)), "\xCB\x98"); // U+02D8
  qcompare (cork_to_utf8 (cork_byte (0x09)), "\xC2\xAF"); // U+00AF
  qcompare (cork_to_utf8 (cork_byte (0x0A)), "\xCB\x99"); // U+02D9
  qcompare (cork_to_utf8 (cork_byte (0x0B)), "\xC2\xB8"); // U+00B8
  qcompare (cork_to_utf8 (cork_byte (0x0C)), "\xCB\x9B"); // U+02DB
}

void
TestConverter::test_cork_to_utf8_1x () {
  qcompare (cork_to_utf8 (cork_byte (0x0D)), "\xE2\x80\x9A"); // U+201A
  qcompare (cork_to_utf8 (cork_byte (0x0E)), "\xE2\x80\xB9"); // U+2039
  qcompare (cork_to_utf8 (cork_byte (0x0F)), "\xE2\x80\xBA"); // U+203A
  qcompare (cork_to_utf8 (cork_byte (0x10)), "\xE2\x80\x9C"); // U+201C “
  qcompare (cork_to_utf8 (cork_byte (0x11)), "\xE2\x80\x9D"); // U+201D ”
  qcompare (cork_to_utf8 (cork_byte (0x12)), "\xE2\x80\x9E"); // U+201E
  qcompare (cork_to_utf8 (cork_byte (0x13)), "\xC2\xAB");     // U+00AB «
  qcompare (cork_to_utf8 (cork_byte (0x14)), "\xC2\xBB");     // U+00BB »
  qcompare (cork_to_utf8 (cork_byte (0x15)), "\xE2\x80\x93"); // U+2013 –
  qcompare (cork_to_utf8 (cork_byte (0x16)), "\xE2\x80\x94"); // U+2014 —
  qcompare (cork_to_utf8 (cork_byte (0x17)),
            "\xE2\x81\xA0");                       // U+2060 WORD JOINER
  qcompare (cork_to_utf8 (cork_byte (0x18)), "0"); // perthousand zero (oneway)
  qcompare (cork_to_utf8 (cork_byte (0x19)), "\xC4\xB1"); // U+0131 ı
  qcompare (cork_to_utf8 (cork_byte (0x1A)), "j");        // dotless j (oneway)
  qcompare (cork_to_utf8 (cork_byte (0x1B)), "\xEF\xAC\x80"); // U+FB00 ﬀ
  qcompare (cork_to_utf8 (cork_byte (0x1C)), "\xEF\xAC\x81"); // U+FB01 ﬁ
  qcompare (cork_to_utf8 (cork_byte (0x1D)), "\xEF\xAC\x82"); // U+FB02 ﬂ
  qcompare (cork_to_utf8 (cork_byte (0x1E)), "\xEF\xAC\x83"); // U+FB03 ﬃ
  qcompare (cork_to_utf8 (cork_byte (0x1F)), "\xEF\xAC\x84"); // U+FB04 ﬄ
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
  qcompare (cork_to_utf8 (cork_byte (0x60)), "\xE2\x80\x98"); // U+2018 ‘
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
  qcompare (cork_to_utf8 (cork_byte (0x7F)), "\xE2\x80\x90"); // U+2010 hyphen
}

void
TestConverter::test_cork_to_utf8_8x () {
  qcompare (cork_to_utf8 (cork_byte (0x80)), "\xC4\x82"); // U+0102
  qcompare (cork_to_utf8 (cork_byte (0x81)), "\xC4\x84"); // U+0104
  qcompare (cork_to_utf8 (cork_byte (0x82)), "\xC4\x86"); // U+0106
  qcompare (cork_to_utf8 (cork_byte (0x83)), "\xC4\x8C"); // U+010C
  qcompare (cork_to_utf8 (cork_byte (0x84)), "\xC4\x8E"); // U+010E
  qcompare (cork_to_utf8 (cork_byte (0x85)), "\xC4\x9A"); // U+011A
  qcompare (cork_to_utf8 (cork_byte (0x86)), "\xC4\x98"); // U+0118
  qcompare (cork_to_utf8 (cork_byte (0x87)), "\xC4\x9E"); // U+011E
  qcompare (cork_to_utf8 (cork_byte (0x88)), "\xC4\xB9"); // U+0139
  qcompare (cork_to_utf8 (cork_byte (0x89)), "\xC4\xBD"); // U+013D
  qcompare (cork_to_utf8 (cork_byte (0x8A)), "\xC5\x81"); // U+0141
  qcompare (cork_to_utf8 (cork_byte (0x8B)), "\xC5\x83"); // U+0143
  qcompare (cork_to_utf8 (cork_byte (0x8C)), "\xC5\x87"); // U+0147
  qcompare (cork_to_utf8 (cork_byte (0x8D)), "\xC5\x8A"); // U+014A
  qcompare (cork_to_utf8 (cork_byte (0x8E)), "\xC5\x90"); // U+0150
  qcompare (cork_to_utf8 (cork_byte (0x8F)), "\xC5\x94"); // U+0154
}

void
TestConverter::test_cork_to_utf8_9x () {
  qcompare (cork_to_utf8 (cork_byte (0x90)), "\xC5\x98"); // U+0158
  qcompare (cork_to_utf8 (cork_byte (0x91)), "\xC5\x9A"); // U+015A
  qcompare (cork_to_utf8 (cork_byte (0x92)), "\xC5\xA0"); // U+0160
  qcompare (cork_to_utf8 (cork_byte (0x93)), "\xC5\x9E"); // U+015E
  qcompare (cork_to_utf8 (cork_byte (0x94)), "\xC5\xA4"); // U+0164
  qcompare (cork_to_utf8 (cork_byte (0x95)), "\xC5\xA2"); // U+0162
  qcompare (cork_to_utf8 (cork_byte (0x96)), "\xC5\xB0"); // U+0170
  qcompare (cork_to_utf8 (cork_byte (0x97)), "\xC5\xAE"); // U+016E
  qcompare (cork_to_utf8 (cork_byte (0x98)), "\xC5\xB8"); // U+0178
  qcompare (cork_to_utf8 (cork_byte (0x99)), "\xC5\xB9"); // U+0179
  qcompare (cork_to_utf8 (cork_byte (0x9A)), "\xC5\xBD"); // U+017D
  qcompare (cork_to_utf8 (cork_byte (0x9B)), "\xC5\xBB"); // U+017B
  qcompare (cork_to_utf8 (cork_byte (0x9C)), "\xC4\xB2"); // U+0132
  qcompare (cork_to_utf8 (cork_byte (0x9D)), "\xC4\xB0"); // U+0130
  qcompare (cork_to_utf8 (cork_byte (0x9E)), "\xC4\x91"); // U+0111
  qcompare (cork_to_utf8 (cork_byte (0x9F)), "\xC2\xA7"); // U+00A7 §
}

void
TestConverter::test_cork_to_utf8_Ax () {
  qcompare (cork_to_utf8 (cork_byte (0xA0)), "\xC4\x83"); // U+0103
  qcompare (cork_to_utf8 (cork_byte (0xA1)), "\xC4\x85"); // U+0105
  qcompare (cork_to_utf8 (cork_byte (0xA2)), "\xC4\x87"); // U+0107
  qcompare (cork_to_utf8 (cork_byte (0xA3)), "\xC4\x8D"); // U+010D
  qcompare (cork_to_utf8 (cork_byte (0xA4)), "\xC4\x8F"); // U+010F
  qcompare (cork_to_utf8 (cork_byte (0xA5)), "\xC4\x9B"); // U+011B
  qcompare (cork_to_utf8 (cork_byte (0xA6)), "\xC4\x99"); // U+0119
  qcompare (cork_to_utf8 (cork_byte (0xA7)), "\xC4\x9F"); // U+011F
  qcompare (cork_to_utf8 (cork_byte (0xA8)), "\xC4\xBA"); // U+013A
  qcompare (cork_to_utf8 (cork_byte (0xA9)), "\xC4\xBE"); // U+013E
  qcompare (cork_to_utf8 (cork_byte (0xAA)), "\xC5\x82"); // U+0142
  qcompare (cork_to_utf8 (cork_byte (0xAB)), "\xC5\x84"); // U+0144
  qcompare (cork_to_utf8 (cork_byte (0xAC)), "\xC5\x88"); // U+0148
  qcompare (cork_to_utf8 (cork_byte (0xAD)), "\xC5\x8B"); // U+014B
  qcompare (cork_to_utf8 (cork_byte (0xAE)), "\xC5\x91"); // U+0151
  qcompare (cork_to_utf8 (cork_byte (0xAF)), "\xC5\x95"); // U+0155
}

void
TestConverter::test_cork_to_utf8_Bx () {
  qcompare (cork_to_utf8 (cork_byte (0xB0)), "\xC5\x99"); // U+0159
  qcompare (cork_to_utf8 (cork_byte (0xB1)), "\xC5\x9B"); // U+015B
  qcompare (cork_to_utf8 (cork_byte (0xB2)), "\xC5\xA1"); // U+0161
  qcompare (cork_to_utf8 (cork_byte (0xB3)), "\xC5\x9F"); // U+015F
  qcompare (cork_to_utf8 (cork_byte (0xB4)), "\xC5\xA5"); // U+0165
  qcompare (cork_to_utf8 (cork_byte (0xB5)), "\xC5\xA3"); // U+0163
  qcompare (cork_to_utf8 (cork_byte (0xB6)), "\xC5\xB1"); // U+0171
  qcompare (cork_to_utf8 (cork_byte (0xB7)), "\xC5\xAF"); // U+016F
  qcompare (cork_to_utf8 (cork_byte (0xB8)), "\xC3\xBF"); // U+00FF ÿ
  qcompare (cork_to_utf8 (cork_byte (0xB9)), "\xC5\xBA"); // U+017A
  qcompare (cork_to_utf8 (cork_byte (0xBA)), "\xC5\xBE"); // U+017E
  qcompare (cork_to_utf8 (cork_byte (0xBB)), "\xC5\xBC"); // U+017C
  qcompare (cork_to_utf8 (cork_byte (0xBC)), "\xC4\xB3"); // U+0133
  qcompare (cork_to_utf8 (cork_byte (0xBD)), "\xC2\xA1"); // U+00A1 ¡
  qcompare (cork_to_utf8 (cork_byte (0xBE)), "\xC2\xBF"); // U+00BF ¿
  qcompare (cork_to_utf8 (cork_byte (0xBF)), "\xC2\xA3"); // U+00A3 £
}

void
TestConverter::test_cork_to_utf8_Cx () {
  qcompare (cork_to_utf8 (cork_byte (0xC0)), "\xC3\x80"); // U+00C0 À
  qcompare (cork_to_utf8 (cork_byte (0xC1)), "\xC3\x81"); // U+00C1 Á
  qcompare (cork_to_utf8 (cork_byte (0xC2)), "\xC3\x82"); // U+00C2 Â
  qcompare (cork_to_utf8 (cork_byte (0xC3)), "\xC3\x83"); // U+00C3 Ã
  qcompare (cork_to_utf8 (cork_byte (0xC4)), "\xC3\x84"); // U+00C4 Ä
  qcompare (cork_to_utf8 (cork_byte (0xC5)), "\xC3\x85"); // U+00C5 Å
  qcompare (cork_to_utf8 (cork_byte (0xC6)), "\xC3\x86"); // U+00C6 Æ
  qcompare (cork_to_utf8 (cork_byte (0xC7)), "\xC3\x87"); // U+00C7 Ç
  qcompare (cork_to_utf8 (cork_byte (0xC8)), "\xC3\x88"); // U+00C8 È
  qcompare (cork_to_utf8 (cork_byte (0xC9)), "\xC3\x89"); // U+00C9 É
  qcompare (cork_to_utf8 (cork_byte (0xCA)), "\xC3\x8A"); // U+00CA Ê
  qcompare (cork_to_utf8 (cork_byte (0xCB)), "\xC3\x8B"); // U+00CB Ë
  qcompare (cork_to_utf8 (cork_byte (0xCC)), "\xC3\x8C"); // U+00CC Ì
  qcompare (cork_to_utf8 (cork_byte (0xCD)), "\xC3\x8D"); // U+00CD Í
  qcompare (cork_to_utf8 (cork_byte (0xCE)), "\xC3\x8E"); // U+00CE Î
  qcompare (cork_to_utf8 (cork_byte (0xCF)), "\xC3\x8F"); // U+00CF Ï
}

void
TestConverter::test_cork_to_utf8_Dx () {
  qcompare (cork_to_utf8 (cork_byte (0xD0)), "\xC3\x90"); // U+00D0 Ð
  qcompare (cork_to_utf8 (cork_byte (0xD1)), "\xC3\x91"); // U+00D1 Ñ
  qcompare (cork_to_utf8 (cork_byte (0xD2)), "\xC3\x92"); // U+00D2 Ò
  qcompare (cork_to_utf8 (cork_byte (0xD3)), "\xC3\x93"); // U+00D3 Ó
  qcompare (cork_to_utf8 (cork_byte (0xD4)), "\xC3\x94"); // U+00D4 Ô
  qcompare (cork_to_utf8 (cork_byte (0xD5)), "\xC3\x95"); // U+00D5 Õ
  qcompare (cork_to_utf8 (cork_byte (0xD6)), "\xC3\x96"); // U+00D6 Ö
  qcompare (cork_to_utf8 (cork_byte (0xD7)), "\xC5\x92"); // U+0152 Œ
  qcompare (cork_to_utf8 (cork_byte (0xD8)), "\xC3\x98"); // U+00D8 Ø
  qcompare (cork_to_utf8 (cork_byte (0xD9)), "\xC3\x99"); // U+00D9 Ù
  qcompare (cork_to_utf8 (cork_byte (0xDA)), "\xC3\x9A"); // U+00DA Ú
  qcompare (cork_to_utf8 (cork_byte (0xDB)), "\xC3\x9B"); // U+00DB Û
  qcompare (cork_to_utf8 (cork_byte (0xDC)), "\xC3\x9C"); // U+00DC Ü
  qcompare (cork_to_utf8 (cork_byte (0xDD)), "\xC3\x9D"); // U+00DD Ý
  qcompare (cork_to_utf8 (cork_byte (0xDE)), "\xC3\x9E"); // U+00DE Þ
  qcompare (cork_to_utf8 (cork_byte (0xDF)), "SS");       // sharp s (oneway)
}

void
TestConverter::test_cork_to_utf8_Ex () {
  qcompare (cork_to_utf8 (cork_byte (0xE0)), "\xC3\xA0"); // U+00E0 à
  qcompare (cork_to_utf8 (cork_byte (0xE1)), "\xC3\xA1"); // U+00E1 á
  qcompare (cork_to_utf8 (cork_byte (0xE2)), "\xC3\xA2"); // U+00E2 â
  qcompare (cork_to_utf8 (cork_byte (0xE3)), "\xC3\xA3"); // U+00E3 ã
  qcompare (cork_to_utf8 (cork_byte (0xE4)), "\xC3\xA4"); // U+00E4 ä
  qcompare (cork_to_utf8 (cork_byte (0xE5)), "\xC3\xA5"); // U+00E5 å
  qcompare (cork_to_utf8 (cork_byte (0xE6)), "\xC3\xA6"); // U+00E6 æ
  qcompare (cork_to_utf8 (cork_byte (0xE7)), "\xC3\xA7"); // U+00E7 ç
  qcompare (cork_to_utf8 (cork_byte (0xE8)), "\xC3\xA8"); // U+00E8 è
  qcompare (cork_to_utf8 (cork_byte (0xE9)), "\xC3\xA9"); // U+00E9 é
  qcompare (cork_to_utf8 (cork_byte (0xEA)), "\xC3\xAA"); // U+00EA ê
  qcompare (cork_to_utf8 (cork_byte (0xEB)), "\xC3\xAB"); // U+00EB ë
  qcompare (cork_to_utf8 (cork_byte (0xEC)), "\xC3\xAC"); // U+00EC ì
  qcompare (cork_to_utf8 (cork_byte (0xED)), "\xC3\xAD"); // U+00ED í
  qcompare (cork_to_utf8 (cork_byte (0xEE)), "\xC3\xAE"); // U+00EE î
  qcompare (cork_to_utf8 (cork_byte (0xEF)), "\xC3\xAF"); // U+00EF ï
}

void
TestConverter::test_cork_to_utf8_Fx () {
  qcompare (cork_to_utf8 (cork_byte (0xF0)), "\xC3\xB0"); // U+00F0 ð
  qcompare (cork_to_utf8 (cork_byte (0xF1)), "\xC3\xB1"); // U+00F1 ñ
  qcompare (cork_to_utf8 (cork_byte (0xF2)), "\xC3\xB2"); // U+00F2 ò
  qcompare (cork_to_utf8 (cork_byte (0xF3)), "\xC3\xB3"); // U+00F3 ó
  qcompare (cork_to_utf8 (cork_byte (0xF4)), "\xC3\xB4"); // U+00F4 ô
  qcompare (cork_to_utf8 (cork_byte (0xF5)), "\xC3\xB5"); // U+00F5 õ
  qcompare (cork_to_utf8 (cork_byte (0xF6)), "\xC3\xB6"); // U+00F6 ö
  qcompare (cork_to_utf8 (cork_byte (0xF7)), "\xC5\x93"); // U+0153 œ
  qcompare (cork_to_utf8 (cork_byte (0xF8)), "\xC3\xB8"); // U+00F8 ø
  qcompare (cork_to_utf8 (cork_byte (0xF9)), "\xC3\xB9"); // U+00F9 ù
  qcompare (cork_to_utf8 (cork_byte (0xFA)), "\xC3\xBA"); // U+00FA ú
  qcompare (cork_to_utf8 (cork_byte (0xFB)), "\xC3\xBB"); // U+00FB û
  qcompare (cork_to_utf8 (cork_byte (0xFC)), "\xC3\xBC"); // U+00FC ü
  qcompare (cork_to_utf8 (cork_byte (0xFD)), "\xC3\xBD"); // U+00FD ý
  qcompare (cork_to_utf8 (cork_byte (0xFE)), "\xC3\xBE"); // U+00FE þ
  qcompare (cork_to_utf8 (cork_byte (0xFF)), "\xC3\x9F"); // U+00DF ß
}

void
TestConverter::test_cork_to_utf8_escapes () {
  // <#XXXX> decodes to the UTF-8 encoding of the hex codepoint.
  qcompare (cork_to_utf8 ("<#4E2D>"), "中");                // U+4E2D
  qcompare (cork_to_utf8 ("<#2019>"), "\xE2\x80\x99");      // U+2019
  qcompare (cork_to_utf8 ("<#1F600>"), "\xF0\x9F\x98\x80"); // U+1F600
  // Padded forms are accepted.
  qcompare (cork_to_utf8 ("<#0FF>"), "\xC3\xBF");  // U+00FF
  qcompare (cork_to_utf8 ("<#00FF>"), "\xC3\xBF"); // U+00FF
  // A <#XXXX> that names a Cork-mapped codepoint overrides the byte mapping.
  qcompare (cork_to_utf8 ("<#201C>"), "\xE2\x80\x9C"); // U+201C
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
  qcompare (utf8_to_cork ("\xC2\xA0"), "<varspace>"); // U+00A0
}

void
TestConverter::test_mixed () {
  // Cork byte + <#XXXX> escape + ASCII, decoded.
  qcompare (
      cork_to_utf8 (cork_byte (0x41) * string ("<#2019>") * cork_byte (0x42)),
      "A" * string ("\xE2\x80\x99") * "B");
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

QTEST_MAIN (TestConverter)
#include "converter_test.moc"
