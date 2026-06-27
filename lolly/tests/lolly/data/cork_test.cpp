/******************************************************************************
 * MODULE     : cork_test.cpp
 * DESCRIPTION: tests on UTF-8 <-> Cork encoding conversions
 * COPYRIGHT  : (C) 2019 Darcy Shen, 2026 Da Shen
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ****************************************************************************/

#include "a_lolly_test.hpp"
#include "lolly/data/cork.hpp"

using lolly::data::cork_to_utf8;
using lolly::data::strict_cork_to_utf8;
using lolly::data::utf8_to_cork;

// Helper: build a single-byte Cork string.
static string
cork_byte (int byte) {
  return string ((char) byte, 1);
}

TEST_CASE ("test_utf8_to_cork_2x") {
  string_eq (utf8_to_cork (" "), cork_byte (0x20));
  string_eq (utf8_to_cork ("!"), cork_byte (0x21));
  string_eq (utf8_to_cork ("\""), cork_byte (0x22));
  string_eq (utf8_to_cork ("#"), cork_byte (0x23));
  string_eq (utf8_to_cork ("$"), cork_byte (0x24));
  string_eq (utf8_to_cork ("%"), cork_byte (0x25));
  string_eq (utf8_to_cork ("&"), cork_byte (0x26));
  string_eq (utf8_to_cork ("'"), cork_byte (0x27));
  string_eq (utf8_to_cork ("("), cork_byte (0x28));
  string_eq (utf8_to_cork (")"), cork_byte (0x29));
  string_eq (utf8_to_cork ("*"), cork_byte (0x2A));
  string_eq (utf8_to_cork ("+"), cork_byte (0x2B));
  string_eq (utf8_to_cork (","), cork_byte (0x2C));
  string_eq (utf8_to_cork ("-"), cork_byte (0x2D));
  string_eq (utf8_to_cork ("."), cork_byte (0x2E));
  string_eq (utf8_to_cork ("/"), cork_byte (0x2F));
}

TEST_CASE ("test_utf8_to_cork_3x") {
  string_eq (utf8_to_cork ("0"), cork_byte (0x30));
  string_eq (utf8_to_cork ("1"), cork_byte (0x31));
  string_eq (utf8_to_cork ("2"), cork_byte (0x32));
  string_eq (utf8_to_cork ("3"), cork_byte (0x33));
  string_eq (utf8_to_cork ("4"), cork_byte (0x34));
  string_eq (utf8_to_cork ("5"), cork_byte (0x35));
  string_eq (utf8_to_cork ("6"), cork_byte (0x36));
  string_eq (utf8_to_cork ("7"), cork_byte (0x37));
  string_eq (utf8_to_cork ("8"), cork_byte (0x38));
  string_eq (utf8_to_cork ("9"), cork_byte (0x39));
  string_eq (utf8_to_cork (":"), cork_byte (0x3A));
  string_eq (utf8_to_cork (";"), cork_byte (0x3B));
  // 0x3C '<' and 0x3E '>' are covered by named-entity tests.
  string_eq (utf8_to_cork ("="), cork_byte (0x3D));
  string_eq (utf8_to_cork ("?"), cork_byte (0x3F));
}

TEST_CASE ("test_utf8_to_cork_4x") {
  string_eq (utf8_to_cork ("@"), cork_byte (0x40));
  string_eq (utf8_to_cork ("A"), cork_byte (0x41));
  string_eq (utf8_to_cork ("B"), cork_byte (0x42));
  string_eq (utf8_to_cork ("C"), cork_byte (0x43));
  string_eq (utf8_to_cork ("D"), cork_byte (0x44));
  string_eq (utf8_to_cork ("E"), cork_byte (0x45));
  string_eq (utf8_to_cork ("F"), cork_byte (0x46));
  string_eq (utf8_to_cork ("G"), cork_byte (0x47));
  string_eq (utf8_to_cork ("H"), cork_byte (0x48));
  string_eq (utf8_to_cork ("I"), cork_byte (0x49));
  string_eq (utf8_to_cork ("J"), cork_byte (0x4A));
  string_eq (utf8_to_cork ("K"), cork_byte (0x4B));
  string_eq (utf8_to_cork ("L"), cork_byte (0x4C));
  string_eq (utf8_to_cork ("M"), cork_byte (0x4D));
  string_eq (utf8_to_cork ("N"), cork_byte (0x4E));
  string_eq (utf8_to_cork ("O"), cork_byte (0x4F));
}

TEST_CASE ("test_utf8_to_cork_5x") {
  string_eq (utf8_to_cork ("P"), cork_byte (0x50));
  string_eq (utf8_to_cork ("Q"), cork_byte (0x51));
  string_eq (utf8_to_cork ("R"), cork_byte (0x52));
  string_eq (utf8_to_cork ("S"), cork_byte (0x53));
  string_eq (utf8_to_cork ("T"), cork_byte (0x54));
  string_eq (utf8_to_cork ("U"), cork_byte (0x55));
  string_eq (utf8_to_cork ("V"), cork_byte (0x56));
  string_eq (utf8_to_cork ("W"), cork_byte (0x57));
  string_eq (utf8_to_cork ("X"), cork_byte (0x58));
  string_eq (utf8_to_cork ("Y"), cork_byte (0x59));
  string_eq (utf8_to_cork ("Z"), cork_byte (0x5A));
  string_eq (utf8_to_cork ("["), cork_byte (0x5B));
  string_eq (utf8_to_cork ("\\"), cork_byte (0x5C));
  string_eq (utf8_to_cork ("]"), cork_byte (0x5D));
  string_eq (utf8_to_cork ("^"), cork_byte (0x5E));
  string_eq (utf8_to_cork ("_"), cork_byte (0x5F));
}

TEST_CASE ("test_utf8_to_cork_6x") {
  string_eq (utf8_to_cork ("a"), cork_byte (0x61));
  string_eq (utf8_to_cork ("b"), cork_byte (0x62));
  string_eq (utf8_to_cork ("c"), cork_byte (0x63));
  string_eq (utf8_to_cork ("d"), cork_byte (0x64));
  string_eq (utf8_to_cork ("e"), cork_byte (0x65));
  string_eq (utf8_to_cork ("f"), cork_byte (0x66));
  string_eq (utf8_to_cork ("g"), cork_byte (0x67));
  string_eq (utf8_to_cork ("h"), cork_byte (0x68));
  string_eq (utf8_to_cork ("i"), cork_byte (0x69));
  string_eq (utf8_to_cork ("j"), cork_byte (0x6A));
  string_eq (utf8_to_cork ("k"), cork_byte (0x6B));
  string_eq (utf8_to_cork ("l"), cork_byte (0x6C));
  string_eq (utf8_to_cork ("m"), cork_byte (0x6D));
  string_eq (utf8_to_cork ("n"), cork_byte (0x6E));
  string_eq (utf8_to_cork ("o"), cork_byte (0x6F));
}

TEST_CASE ("test_utf8_to_cork_7x") {
  string_eq (utf8_to_cork ("p"), cork_byte (0x70));
  string_eq (utf8_to_cork ("q"), cork_byte (0x71));
  string_eq (utf8_to_cork ("r"), cork_byte (0x72));
  string_eq (utf8_to_cork ("s"), cork_byte (0x73));
  string_eq (utf8_to_cork ("t"), cork_byte (0x74));
  string_eq (utf8_to_cork ("u"), cork_byte (0x75));
  string_eq (utf8_to_cork ("v"), cork_byte (0x76));
  string_eq (utf8_to_cork ("w"), cork_byte (0x77));
  string_eq (utf8_to_cork ("x"), cork_byte (0x78));
  string_eq (utf8_to_cork ("y"), cork_byte (0x79));
  string_eq (utf8_to_cork ("z"), cork_byte (0x7A));
  string_eq (utf8_to_cork ("{"), cork_byte (0x7B));
  string_eq (utf8_to_cork ("|"), cork_byte (0x7C));
  string_eq (utf8_to_cork ("}"), cork_byte (0x7D));
  string_eq (utf8_to_cork ("~"), cork_byte (0x7E));
  // Cork 0x60 is reserved for U+2018 (left single quote); see
  // test_utf8_to_cork_punctuation.
}

TEST_CASE ("test_utf8_to_cork_accents") {
  string_eq (utf8_to_cork ("ˆ"),
             cork_byte (0x02)); // U+02C6 modifier circumflex
  string_eq (utf8_to_cork ("˜"), cork_byte (0x03)); // U+02DC small tilde
  string_eq (utf8_to_cork ("¨"), cork_byte (0x04)); // U+00A8 diaeresis
  string_eq (utf8_to_cork ("˝"), cork_byte (0x05)); // U+02DD double acute
  string_eq (utf8_to_cork ("˚"), cork_byte (0x06)); // U+02DA ring above
  string_eq (utf8_to_cork ("ˇ"), cork_byte (0x07)); // U+02C7 caron
  string_eq (utf8_to_cork ("˘"), cork_byte (0x08)); // U+02D8 breve
  string_eq (utf8_to_cork ("¯"), cork_byte (0x09)); // U+00AF macron
  string_eq (utf8_to_cork ("˙"), cork_byte (0x0A)); // U+02D9 dot above
  string_eq (utf8_to_cork ("¸"), cork_byte (0x0B)); // U+00B8 cedilla
  string_eq (utf8_to_cork ("˛"), cork_byte (0x0C)); // U+02DB ogonek
  string_eq (utf8_to_cork ("´"), cork_byte (0x01)); // U+00B4 acute
}

TEST_CASE ("test_utf8_to_cork_punctuation") {
  string_eq (utf8_to_cork ("‚"), cork_byte (0x0D));   // U+201A
  string_eq (utf8_to_cork ("‹"), cork_byte (0x0E));   // U+2039
  string_eq (utf8_to_cork ("›"), cork_byte (0x0F));   // U+203A
  string_eq (utf8_to_cork ("“"), cork_byte (0x10));   // U+201C
  string_eq (utf8_to_cork ("”"), cork_byte (0x11));   // U+201D
  string_eq (utf8_to_cork ("„"), cork_byte (0x12));   // U+201E
  string_eq (utf8_to_cork ("«"), cork_byte (0x13));   // U+00AB
  string_eq (utf8_to_cork ("»"), cork_byte (0x14));   // U+00BB
  string_eq (utf8_to_cork ("–"), cork_byte (0x15));   // U+2013
  string_eq (utf8_to_cork ("—"), cork_byte (0x16));   // U+2014
  string_eq (utf8_to_cork ("⁠"), cork_byte (0x17)); // U+2060 WORD JOINER
  string_eq (utf8_to_cork ("‘"), cork_byte (0x60)); // U+2018 left single quote
}

TEST_CASE ("test_utf8_to_cork_specials") {
  string_eq (utf8_to_cork ("ı"), cork_byte (0x19)); // U+0131 dotless i
  string_eq (utf8_to_cork ("ﬀ"), cork_byte (0x1B)); // U+FB00 ff
  string_eq (utf8_to_cork ("ﬁ"), cork_byte (0x1C)); // U+FB01 fi
  string_eq (utf8_to_cork ("ﬂ"), cork_byte (0x1D)); // U+FB02 fl
  string_eq (utf8_to_cork ("ﬃ"), cork_byte (0x1E)); // U+FB03 ffi
  string_eq (utf8_to_cork ("ﬄ"), cork_byte (0x1F)); // U+FB04 ffl
}

TEST_CASE ("test_utf8_to_cork_upper") {
  string_eq (utf8_to_cork ("Ă"), cork_byte (0x80)); // U+0102
  string_eq (utf8_to_cork ("Ą"), cork_byte (0x81)); // U+0104
  string_eq (utf8_to_cork ("Ć"), cork_byte (0x82)); // U+0106
  string_eq (utf8_to_cork ("Č"), cork_byte (0x83)); // U+010C
  string_eq (utf8_to_cork ("Ď"), cork_byte (0x84)); // U+010E
  string_eq (utf8_to_cork ("Ě"), cork_byte (0x85)); // U+011A
  string_eq (utf8_to_cork ("Ę"), cork_byte (0x86)); // U+0118
  string_eq (utf8_to_cork ("Ğ"), cork_byte (0x87)); // U+011E
  string_eq (utf8_to_cork ("Ĺ"), cork_byte (0x88)); // U+0139
  string_eq (utf8_to_cork ("Ľ"), cork_byte (0x89)); // U+013D
  string_eq (utf8_to_cork ("Ł"), cork_byte (0x8A)); // U+0141
  string_eq (utf8_to_cork ("Ń"), cork_byte (0x8B)); // U+0143
  string_eq (utf8_to_cork ("Ň"), cork_byte (0x8C)); // U+0147
  string_eq (utf8_to_cork ("Ŋ"), cork_byte (0x8D)); // U+014A
  string_eq (utf8_to_cork ("Ő"), cork_byte (0x8E)); // U+0150
  string_eq (utf8_to_cork ("Ŕ"), cork_byte (0x8F)); // U+0154
  string_eq (utf8_to_cork ("Ř"), cork_byte (0x90)); // U+0158
  string_eq (utf8_to_cork ("Ś"), cork_byte (0x91)); // U+015A
  string_eq (utf8_to_cork ("Š"), cork_byte (0x92)); // U+0160
  string_eq (utf8_to_cork ("Ş"), cork_byte (0x93)); // U+015E
  string_eq (utf8_to_cork ("Ť"), cork_byte (0x94)); // U+0164
  string_eq (utf8_to_cork ("Ţ"), cork_byte (0x95)); // U+0162
  string_eq (utf8_to_cork ("Ű"), cork_byte (0x96)); // U+0170
  string_eq (utf8_to_cork ("Ů"), cork_byte (0x97)); // U+016E
  string_eq (utf8_to_cork ("Ÿ"), cork_byte (0x98)); // U+0178
  string_eq (utf8_to_cork ("Ź"), cork_byte (0x99)); // U+0179
  string_eq (utf8_to_cork ("Ž"), cork_byte (0x9A)); // U+017D
  string_eq (utf8_to_cork ("Ż"), cork_byte (0x9B)); // U+017B
  string_eq (utf8_to_cork ("Ĳ"), cork_byte (0x9C)); // U+0132
  string_eq (utf8_to_cork ("İ"), cork_byte (0x9D)); // U+0130
  string_eq (utf8_to_cork ("đ"), cork_byte (0x9E)); // U+0111
  string_eq (utf8_to_cork ("§"), cork_byte (0x9F)); // U+00A7 section sign
}

TEST_CASE ("test_utf8_to_cork_lower") {
  string_eq (utf8_to_cork ("ă"), cork_byte (0xA0)); // U+0103
  string_eq (utf8_to_cork ("ą"), cork_byte (0xA1)); // U+0105
  string_eq (utf8_to_cork ("ć"), cork_byte (0xA2)); // U+0107
  string_eq (utf8_to_cork ("č"), cork_byte (0xA3)); // U+010D
  string_eq (utf8_to_cork ("ď"), cork_byte (0xA4)); // U+010F
  string_eq (utf8_to_cork ("ě"), cork_byte (0xA5)); // U+011B
  string_eq (utf8_to_cork ("ę"), cork_byte (0xA6)); // U+0119
  string_eq (utf8_to_cork ("ğ"), cork_byte (0xA7)); // U+011F
  string_eq (utf8_to_cork ("ĺ"), cork_byte (0xA8)); // U+013A
  string_eq (utf8_to_cork ("ľ"), cork_byte (0xA9)); // U+013E
  string_eq (utf8_to_cork ("ł"), cork_byte (0xAA)); // U+0142
  string_eq (utf8_to_cork ("ń"), cork_byte (0xAB)); // U+0144
  string_eq (utf8_to_cork ("ň"), cork_byte (0xAC)); // U+0148
  string_eq (utf8_to_cork ("ŋ"), cork_byte (0xAD)); // U+014B
  string_eq (utf8_to_cork ("ő"), cork_byte (0xAE)); // U+0151
  string_eq (utf8_to_cork ("ŕ"), cork_byte (0xAF)); // U+0155
  string_eq (utf8_to_cork ("ř"), cork_byte (0xB0)); // U+0159
  string_eq (utf8_to_cork ("ś"), cork_byte (0xB1)); // U+015B
  string_eq (utf8_to_cork ("š"), cork_byte (0xB2)); // U+0161
  string_eq (utf8_to_cork ("ş"), cork_byte (0xB3)); // U+015F
  string_eq (utf8_to_cork ("ť"), cork_byte (0xB4)); // U+0165
  string_eq (utf8_to_cork ("ţ"), cork_byte (0xB5)); // U+0163
  string_eq (utf8_to_cork ("ű"), cork_byte (0xB6)); // U+0171
  string_eq (utf8_to_cork ("ů"), cork_byte (0xB7)); // U+016F
  string_eq (utf8_to_cork ("ÿ"), cork_byte (0xB8)); // U+00FF
  string_eq (utf8_to_cork ("ź"), cork_byte (0xB9)); // U+017A
  string_eq (utf8_to_cork ("ž"), cork_byte (0xBA)); // U+017E
  string_eq (utf8_to_cork ("ż"), cork_byte (0xBB)); // U+017C
  string_eq (utf8_to_cork ("ĳ"), cork_byte (0xBC)); // U+0133
  string_eq (utf8_to_cork ("¡"), cork_byte (0xBD)); // U+00A1
  string_eq (utf8_to_cork ("¿"), cork_byte (0xBE)); // U+00BF
  string_eq (utf8_to_cork ("£"), cork_byte (0xBF)); // U+00A3 pound sign
}

TEST_CASE ("test_utf8_to_cork_latin1") {
  string_eq (utf8_to_cork ("À"), cork_byte (0xC0)); // U+00C0 À
  string_eq (utf8_to_cork ("Á"), cork_byte (0xC1)); // U+00C1 Á
  string_eq (utf8_to_cork ("Â"), cork_byte (0xC2)); // U+00C2 Â
  string_eq (utf8_to_cork ("Ã"), cork_byte (0xC3)); // U+00C3 Ã
  string_eq (utf8_to_cork ("Ä"), cork_byte (0xC4)); // U+00C4 Ä
  string_eq (utf8_to_cork ("Å"), cork_byte (0xC5)); // U+00C5 Å
  string_eq (utf8_to_cork ("Æ"), cork_byte (0xC6)); // U+00C6 Æ
  string_eq (utf8_to_cork ("Ç"), cork_byte (0xC7)); // U+00C7 Ç
  string_eq (utf8_to_cork ("È"), cork_byte (0xC8)); // U+00C8 È
  string_eq (utf8_to_cork ("É"), cork_byte (0xC9)); // U+00C9 É
  string_eq (utf8_to_cork ("Ê"), cork_byte (0xCA)); // U+00CA Ê
  string_eq (utf8_to_cork ("Ë"), cork_byte (0xCB)); // U+00CB Ë
  string_eq (utf8_to_cork ("Ì"), cork_byte (0xCC)); // U+00CC Ì
  string_eq (utf8_to_cork ("Í"), cork_byte (0xCD)); // U+00CD Í
  string_eq (utf8_to_cork ("Î"), cork_byte (0xCE)); // U+00CE Î
  string_eq (utf8_to_cork ("Ï"), cork_byte (0xCF)); // U+00CF Ï
  string_eq (utf8_to_cork ("Ð"), cork_byte (0xD0)); // U+00D0 Ð
  string_eq (utf8_to_cork ("Ñ"), cork_byte (0xD1)); // U+00D1 Ñ
  string_eq (utf8_to_cork ("Ò"), cork_byte (0xD2)); // U+00D2 Ò
  string_eq (utf8_to_cork ("Ó"), cork_byte (0xD3)); // U+00D3 Ó
  string_eq (utf8_to_cork ("Ô"), cork_byte (0xD4)); // U+00D4 Ô
  string_eq (utf8_to_cork ("Õ"), cork_byte (0xD5)); // U+00D5 Õ
  string_eq (utf8_to_cork ("Ö"), cork_byte (0xD6)); // U+00D6 Ö
  string_eq (utf8_to_cork ("Œ"), cork_byte (0xD7)); // U+0152 Œ
  string_eq (utf8_to_cork ("Ø"), cork_byte (0xD8)); // U+00D8 Ø
  string_eq (utf8_to_cork ("Ù"), cork_byte (0xD9)); // U+00D9 Ù
  string_eq (utf8_to_cork ("Ú"), cork_byte (0xDA)); // U+00DA Ú
  string_eq (utf8_to_cork ("Û"), cork_byte (0xDB)); // U+00DB Û
  string_eq (utf8_to_cork ("Ü"), cork_byte (0xDC)); // U+00DC Ü
  string_eq (utf8_to_cork ("Ý"), cork_byte (0xDD)); // U+00DD Ý
  string_eq (utf8_to_cork ("Þ"), cork_byte (0xDE)); // U+00DE Þ
  // U+00DF (ß) maps to Cork 0xFF (corktounicode 0xFF -> U+00DF, reversible).
  // Cork 0xDF decodes to "SS" (cork-unicode-oneway), so the byte 0xDF is
  // unreachable from utf8_to_cork; see test_cork_to_utf8_Dx and devel/1125.md.
  string_eq (utf8_to_cork ("ß"), cork_byte (0xFF)); // U+00DF ß
  string_eq (utf8_to_cork ("à"), cork_byte (0xE0)); // U+00E0 à
  string_eq (utf8_to_cork ("á"), cork_byte (0xE1)); // U+00E1 á
  string_eq (utf8_to_cork ("â"), cork_byte (0xE2)); // U+00E2 â
  string_eq (utf8_to_cork ("ã"), cork_byte (0xE3)); // U+00E3 ã
  string_eq (utf8_to_cork ("ä"), cork_byte (0xE4)); // U+00E4 ä
  string_eq (utf8_to_cork ("å"), cork_byte (0xE5)); // U+00E5 å
  string_eq (utf8_to_cork ("æ"), cork_byte (0xE6)); // U+00E6 æ
  string_eq (utf8_to_cork ("ç"), cork_byte (0xE7)); // U+00E7 ç
  string_eq (utf8_to_cork ("è"), cork_byte (0xE8)); // U+00E8 è
  string_eq (utf8_to_cork ("é"), cork_byte (0xE9)); // U+00E9 é
  string_eq (utf8_to_cork ("ê"), cork_byte (0xEA)); // U+00EA ê
  string_eq (utf8_to_cork ("ë"), cork_byte (0xEB)); // U+00EB ë
  string_eq (utf8_to_cork ("ì"), cork_byte (0xEC)); // U+00EC ì
  string_eq (utf8_to_cork ("í"), cork_byte (0xED)); // U+00ED í
  string_eq (utf8_to_cork ("î"), cork_byte (0xEE)); // U+00EE î
  string_eq (utf8_to_cork ("ï"), cork_byte (0xEF)); // U+00EF ï
  string_eq (utf8_to_cork ("ð"), cork_byte (0xF0)); // U+00F0 ð
  string_eq (utf8_to_cork ("ñ"), cork_byte (0xF1)); // U+00F1 ñ
  string_eq (utf8_to_cork ("ò"), cork_byte (0xF2)); // U+00F2 ò
  string_eq (utf8_to_cork ("ó"), cork_byte (0xF3)); // U+00F3 ó
  string_eq (utf8_to_cork ("ô"), cork_byte (0xF4)); // U+00F4 ô
  string_eq (utf8_to_cork ("õ"), cork_byte (0xF5)); // U+00F5 õ
  string_eq (utf8_to_cork ("ö"), cork_byte (0xF6)); // U+00F6 ö
  string_eq (utf8_to_cork ("œ"), cork_byte (0xF7)); // U+0153 œ
  string_eq (utf8_to_cork ("ø"), cork_byte (0xF8)); // U+00F8 ø
  string_eq (utf8_to_cork ("ù"), cork_byte (0xF9)); // U+00F9 ù
  string_eq (utf8_to_cork ("ú"), cork_byte (0xFA)); // U+00FA ú
  string_eq (utf8_to_cork ("û"), cork_byte (0xFB)); // U+00FB û
  string_eq (utf8_to_cork ("ü"), cork_byte (0xFC)); // U+00FC ü
  string_eq (utf8_to_cork ("ý"), cork_byte (0xFD)); // U+00FD ý
  string_eq (utf8_to_cork ("þ"), cork_byte (0xFE)); // U+00FE þ
}

TEST_CASE ("test_utf8_to_cork_unmapped_high") {
  // Codepoints >= 256 with no Cork mapping are escaped as <#XXXX>.
  string_eq (utf8_to_cork ("中"), "<#4E2D>");  // U+4E2D 中
  string_eq (utf8_to_cork ("😀"), "<#1F600>"); // U+1F600 😀
  string_eq (utf8_to_cork ("​"), "<#200B>"); // U+200B ZWSP
  // U+2019 maps to Cork 0x27 (shares ASCII apostrophe slot), not escaped.
  string_eq (utf8_to_cork ("’"), cork_byte (0x27)); // U+2019 ’
  // U+2010 maps to Cork 0x7F (hyphen), not escaped.
  string_eq (utf8_to_cork ("‐"), cork_byte (0x7F)); // U+2010 ‐
}

TEST_CASE ("test_utf8_to_cork_named_unmapped") {
  // U+00A0 maps to the <varspace> named entity (unicode-cork-oneway).
  string_eq (utf8_to_cork (" "), "<varspace>");
  // U+0060 backtick maps to Cork 0x00 (corktounicode has 0x00 -> U+0060,
  // reversible).
  string_eq (utf8_to_cork ("`"), cork_byte (0x00));
}

/******************************************************************************
 * cork_to_utf8
 ******************************************************************************/

TEST_CASE ("test_cork_to_utf8_0x") {
  string_eq (cork_to_utf8 (cork_byte (0x00)), "`"); // U+0060
  string_eq (cork_to_utf8 (cork_byte (0x01)), "´"); // U+00B4
  string_eq (cork_to_utf8 (cork_byte (0x02)), "ˆ"); // U+02C6
  string_eq (cork_to_utf8 (cork_byte (0x03)), "˜"); // U+02DC
  string_eq (cork_to_utf8 (cork_byte (0x04)), "¨"); // U+00A8
  string_eq (cork_to_utf8 (cork_byte (0x05)), "˝"); // U+02DD
  string_eq (cork_to_utf8 (cork_byte (0x06)), "˚"); // U+02DA
  string_eq (cork_to_utf8 (cork_byte (0x07)), "ˇ"); // U+02C7
  string_eq (cork_to_utf8 (cork_byte (0x08)), "˘"); // U+02D8
  string_eq (cork_to_utf8 (cork_byte (0x09)), "¯"); // U+00AF
  string_eq (cork_to_utf8 (cork_byte (0x0A)), "˙"); // U+02D9
  string_eq (cork_to_utf8 (cork_byte (0x0B)), "¸"); // U+00B8
  string_eq (cork_to_utf8 (cork_byte (0x0C)), "˛"); // U+02DB
}

TEST_CASE ("test_cork_to_utf8_1x") {
  string_eq (cork_to_utf8 (cork_byte (0x0D)), "‚"); // U+201A
  string_eq (cork_to_utf8 (cork_byte (0x0E)), "‹"); // U+2039
  string_eq (cork_to_utf8 (cork_byte (0x0F)), "›"); // U+203A
  string_eq (cork_to_utf8 (cork_byte (0x10)), "“"); // U+201C “
  string_eq (cork_to_utf8 (cork_byte (0x11)), "”"); // U+201D ”
  string_eq (cork_to_utf8 (cork_byte (0x12)), "„"); // U+201E
  string_eq (cork_to_utf8 (cork_byte (0x13)), "«"); // U+00AB «
  string_eq (cork_to_utf8 (cork_byte (0x14)), "»"); // U+00BB »
  string_eq (cork_to_utf8 (cork_byte (0x15)), "–"); // U+2013 –
  string_eq (cork_to_utf8 (cork_byte (0x16)), "—"); // U+2014 —
  string_eq (cork_to_utf8 (cork_byte (0x17)),
             "⁠");                                // U+2060 WORD JOINER
  string_eq (cork_to_utf8 (cork_byte (0x18)), "0"); // perthousand zero (oneway)
  string_eq (cork_to_utf8 (cork_byte (0x19)), "ı"); // U+0131 ı
  string_eq (cork_to_utf8 (cork_byte (0x1A)), "j"); // dotless j (oneway)
  string_eq (cork_to_utf8 (cork_byte (0x1B)), "ﬀ"); // U+FB00 ﬀ
  string_eq (cork_to_utf8 (cork_byte (0x1C)), "ﬁ"); // U+FB01 ﬁ
  string_eq (cork_to_utf8 (cork_byte (0x1D)), "ﬂ"); // U+FB02 ﬂ
  string_eq (cork_to_utf8 (cork_byte (0x1E)), "ﬃ"); // U+FB03 ﬃ
  string_eq (cork_to_utf8 (cork_byte (0x1F)), "ﬄ"); // U+FB04 ﬄ
}

TEST_CASE ("test_cork_to_utf8_2x") {
  string_eq (cork_to_utf8 (cork_byte (0x20)), " ");
  string_eq (cork_to_utf8 (cork_byte (0x21)), "!");
  string_eq (cork_to_utf8 (cork_byte (0x22)), "\"");
  string_eq (cork_to_utf8 (cork_byte (0x23)), "#");
  string_eq (cork_to_utf8 (cork_byte (0x24)), "$");
  string_eq (cork_to_utf8 (cork_byte (0x25)), "%");
  string_eq (cork_to_utf8 (cork_byte (0x26)), "&");
  string_eq (cork_to_utf8 (cork_byte (0x27)), "'");
  string_eq (cork_to_utf8 (cork_byte (0x28)), "(");
  string_eq (cork_to_utf8 (cork_byte (0x29)), ")");
  string_eq (cork_to_utf8 (cork_byte (0x2A)), "*");
  string_eq (cork_to_utf8 (cork_byte (0x2B)), "+");
  string_eq (cork_to_utf8 (cork_byte (0x2C)), ",");
  string_eq (cork_to_utf8 (cork_byte (0x2D)), "-");
  string_eq (cork_to_utf8 (cork_byte (0x2E)), ".");
  string_eq (cork_to_utf8 (cork_byte (0x2F)), "/");
}

TEST_CASE ("test_cork_to_utf8_3x") {
  string_eq (cork_to_utf8 (cork_byte (0x30)), "0");
  string_eq (cork_to_utf8 (cork_byte (0x31)), "1");
  string_eq (cork_to_utf8 (cork_byte (0x32)), "2");
  string_eq (cork_to_utf8 (cork_byte (0x33)), "3");
  string_eq (cork_to_utf8 (cork_byte (0x34)), "4");
  string_eq (cork_to_utf8 (cork_byte (0x35)), "5");
  string_eq (cork_to_utf8 (cork_byte (0x36)), "6");
  string_eq (cork_to_utf8 (cork_byte (0x37)), "7");
  string_eq (cork_to_utf8 (cork_byte (0x38)), "8");
  string_eq (cork_to_utf8 (cork_byte (0x39)), "9");
  string_eq (cork_to_utf8 (cork_byte (0x3A)), ":");
  string_eq (cork_to_utf8 (cork_byte (0x3B)), ";");
  // 0x3C/0x3E: single byte passes through as literal '<'/'>'. See named-entity
  // tests.
  string_eq (cork_to_utf8 (cork_byte (0x3D)), "=");
  string_eq (cork_to_utf8 (cork_byte (0x3F)), "?");
}

TEST_CASE ("test_cork_to_utf8_4x") {
  string_eq (cork_to_utf8 (cork_byte (0x40)), "@");
  string_eq (cork_to_utf8 (cork_byte (0x41)), "A");
  string_eq (cork_to_utf8 (cork_byte (0x42)), "B");
  string_eq (cork_to_utf8 (cork_byte (0x43)), "C");
  string_eq (cork_to_utf8 (cork_byte (0x44)), "D");
  string_eq (cork_to_utf8 (cork_byte (0x45)), "E");
  string_eq (cork_to_utf8 (cork_byte (0x46)), "F");
  string_eq (cork_to_utf8 (cork_byte (0x47)), "G");
  string_eq (cork_to_utf8 (cork_byte (0x48)), "H");
  string_eq (cork_to_utf8 (cork_byte (0x49)), "I");
  string_eq (cork_to_utf8 (cork_byte (0x4A)), "J");
  string_eq (cork_to_utf8 (cork_byte (0x4B)), "K");
  string_eq (cork_to_utf8 (cork_byte (0x4C)), "L");
  string_eq (cork_to_utf8 (cork_byte (0x4D)), "M");
  string_eq (cork_to_utf8 (cork_byte (0x4E)), "N");
  string_eq (cork_to_utf8 (cork_byte (0x4F)), "O");
}

TEST_CASE ("test_cork_to_utf8_5x") {
  string_eq (cork_to_utf8 (cork_byte (0x50)), "P");
  string_eq (cork_to_utf8 (cork_byte (0x51)), "Q");
  string_eq (cork_to_utf8 (cork_byte (0x52)), "R");
  string_eq (cork_to_utf8 (cork_byte (0x53)), "S");
  string_eq (cork_to_utf8 (cork_byte (0x54)), "T");
  string_eq (cork_to_utf8 (cork_byte (0x55)), "U");
  string_eq (cork_to_utf8 (cork_byte (0x56)), "V");
  string_eq (cork_to_utf8 (cork_byte (0x57)), "W");
  string_eq (cork_to_utf8 (cork_byte (0x58)), "X");
  string_eq (cork_to_utf8 (cork_byte (0x59)), "Y");
  string_eq (cork_to_utf8 (cork_byte (0x5A)), "Z");
  string_eq (cork_to_utf8 (cork_byte (0x5B)), "[");
  string_eq (cork_to_utf8 (cork_byte (0x5C)), "\\");
  string_eq (cork_to_utf8 (cork_byte (0x5D)), "]");
  string_eq (cork_to_utf8 (cork_byte (0x5E)), "^");
  string_eq (cork_to_utf8 (cork_byte (0x5F)), "_");
}

TEST_CASE ("test_cork_to_utf8_6x") {
  string_eq (cork_to_utf8 (cork_byte (0x60)), "‘"); // U+2018 ‘
  string_eq (cork_to_utf8 (cork_byte (0x61)), "a");
  string_eq (cork_to_utf8 (cork_byte (0x62)), "b");
  string_eq (cork_to_utf8 (cork_byte (0x63)), "c");
  string_eq (cork_to_utf8 (cork_byte (0x64)), "d");
  string_eq (cork_to_utf8 (cork_byte (0x65)), "e");
  string_eq (cork_to_utf8 (cork_byte (0x66)), "f");
  string_eq (cork_to_utf8 (cork_byte (0x67)), "g");
  string_eq (cork_to_utf8 (cork_byte (0x68)), "h");
  string_eq (cork_to_utf8 (cork_byte (0x69)), "i");
  string_eq (cork_to_utf8 (cork_byte (0x6A)), "j");
  string_eq (cork_to_utf8 (cork_byte (0x6B)), "k");
  string_eq (cork_to_utf8 (cork_byte (0x6C)), "l");
  string_eq (cork_to_utf8 (cork_byte (0x6D)), "m");
  string_eq (cork_to_utf8 (cork_byte (0x6E)), "n");
  string_eq (cork_to_utf8 (cork_byte (0x6F)), "o");
}

TEST_CASE ("test_cork_to_utf8_7x") {
  string_eq (cork_to_utf8 (cork_byte (0x70)), "p");
  string_eq (cork_to_utf8 (cork_byte (0x71)), "q");
  string_eq (cork_to_utf8 (cork_byte (0x72)), "r");
  string_eq (cork_to_utf8 (cork_byte (0x73)), "s");
  string_eq (cork_to_utf8 (cork_byte (0x74)), "t");
  string_eq (cork_to_utf8 (cork_byte (0x75)), "u");
  string_eq (cork_to_utf8 (cork_byte (0x76)), "v");
  string_eq (cork_to_utf8 (cork_byte (0x77)), "w");
  string_eq (cork_to_utf8 (cork_byte (0x78)), "x");
  string_eq (cork_to_utf8 (cork_byte (0x79)), "y");
  string_eq (cork_to_utf8 (cork_byte (0x7A)), "z");
  string_eq (cork_to_utf8 (cork_byte (0x7B)), "{");
  string_eq (cork_to_utf8 (cork_byte (0x7C)), "|");
  string_eq (cork_to_utf8 (cork_byte (0x7D)), "}");
  string_eq (cork_to_utf8 (cork_byte (0x7E)), "~");
  string_eq (cork_to_utf8 (cork_byte (0x7F)), "‐"); // U+2010 hyphen
}

TEST_CASE ("test_cork_to_utf8_8x") {
  string_eq (cork_to_utf8 (cork_byte (0x80)), "Ă"); // U+0102
  string_eq (cork_to_utf8 (cork_byte (0x81)), "Ą"); // U+0104
  string_eq (cork_to_utf8 (cork_byte (0x82)), "Ć"); // U+0106
  string_eq (cork_to_utf8 (cork_byte (0x83)), "Č"); // U+010C
  string_eq (cork_to_utf8 (cork_byte (0x84)), "Ď"); // U+010E
  string_eq (cork_to_utf8 (cork_byte (0x85)), "Ě"); // U+011A
  string_eq (cork_to_utf8 (cork_byte (0x86)), "Ę"); // U+0118
  string_eq (cork_to_utf8 (cork_byte (0x87)), "Ğ"); // U+011E
  string_eq (cork_to_utf8 (cork_byte (0x88)), "Ĺ"); // U+0139
  string_eq (cork_to_utf8 (cork_byte (0x89)), "Ľ"); // U+013D
  string_eq (cork_to_utf8 (cork_byte (0x8A)), "Ł"); // U+0141
  string_eq (cork_to_utf8 (cork_byte (0x8B)), "Ń"); // U+0143
  string_eq (cork_to_utf8 (cork_byte (0x8C)), "Ň"); // U+0147
  string_eq (cork_to_utf8 (cork_byte (0x8D)), "Ŋ"); // U+014A
  string_eq (cork_to_utf8 (cork_byte (0x8E)), "Ő"); // U+0150
  string_eq (cork_to_utf8 (cork_byte (0x8F)), "Ŕ"); // U+0154
}

TEST_CASE ("test_cork_to_utf8_9x") {
  string_eq (cork_to_utf8 (cork_byte (0x90)), "Ř"); // U+0158
  string_eq (cork_to_utf8 (cork_byte (0x91)), "Ś"); // U+015A
  string_eq (cork_to_utf8 (cork_byte (0x92)), "Š"); // U+0160
  string_eq (cork_to_utf8 (cork_byte (0x93)), "Ş"); // U+015E
  string_eq (cork_to_utf8 (cork_byte (0x94)), "Ť"); // U+0164
  string_eq (cork_to_utf8 (cork_byte (0x95)), "Ţ"); // U+0162
  string_eq (cork_to_utf8 (cork_byte (0x96)), "Ű"); // U+0170
  string_eq (cork_to_utf8 (cork_byte (0x97)), "Ů"); // U+016E
  string_eq (cork_to_utf8 (cork_byte (0x98)), "Ÿ"); // U+0178
  string_eq (cork_to_utf8 (cork_byte (0x99)), "Ź"); // U+0179
  string_eq (cork_to_utf8 (cork_byte (0x9A)), "Ž"); // U+017D
  string_eq (cork_to_utf8 (cork_byte (0x9B)), "Ż"); // U+017B
  string_eq (cork_to_utf8 (cork_byte (0x9C)), "Ĳ"); // U+0132
  string_eq (cork_to_utf8 (cork_byte (0x9D)), "İ"); // U+0130
  string_eq (cork_to_utf8 (cork_byte (0x9E)), "đ"); // U+0111
  string_eq (cork_to_utf8 (cork_byte (0x9F)), "§"); // U+00A7 §
}

TEST_CASE ("test_cork_to_utf8_Ax") {
  string_eq (cork_to_utf8 (cork_byte (0xA0)), "ă"); // U+0103
  string_eq (cork_to_utf8 (cork_byte (0xA1)), "ą"); // U+0105
  string_eq (cork_to_utf8 (cork_byte (0xA2)), "ć"); // U+0107
  string_eq (cork_to_utf8 (cork_byte (0xA3)), "č"); // U+010D
  string_eq (cork_to_utf8 (cork_byte (0xA4)), "ď"); // U+010F
  string_eq (cork_to_utf8 (cork_byte (0xA5)), "ě"); // U+011B
  string_eq (cork_to_utf8 (cork_byte (0xA6)), "ę"); // U+0119
  string_eq (cork_to_utf8 (cork_byte (0xA7)), "ğ"); // U+011F
  string_eq (cork_to_utf8 (cork_byte (0xA8)), "ĺ"); // U+013A
  string_eq (cork_to_utf8 (cork_byte (0xA9)), "ľ"); // U+013E
  string_eq (cork_to_utf8 (cork_byte (0xAA)), "ł"); // U+0142
  string_eq (cork_to_utf8 (cork_byte (0xAB)), "ń"); // U+0144
  string_eq (cork_to_utf8 (cork_byte (0xAC)), "ň"); // U+0148
  string_eq (cork_to_utf8 (cork_byte (0xAD)), "ŋ"); // U+014B
  string_eq (cork_to_utf8 (cork_byte (0xAE)), "ő"); // U+0151
  string_eq (cork_to_utf8 (cork_byte (0xAF)), "ŕ"); // U+0155
}

TEST_CASE ("test_cork_to_utf8_Bx") {
  string_eq (cork_to_utf8 (cork_byte (0xB0)), "ř"); // U+0159
  string_eq (cork_to_utf8 (cork_byte (0xB1)), "ś"); // U+015B
  string_eq (cork_to_utf8 (cork_byte (0xB2)), "š"); // U+0161
  string_eq (cork_to_utf8 (cork_byte (0xB3)), "ş"); // U+015F
  string_eq (cork_to_utf8 (cork_byte (0xB4)), "ť"); // U+0165
  string_eq (cork_to_utf8 (cork_byte (0xB5)), "ţ"); // U+0163
  string_eq (cork_to_utf8 (cork_byte (0xB6)), "ű"); // U+0171
  string_eq (cork_to_utf8 (cork_byte (0xB7)), "ů"); // U+016F
  string_eq (cork_to_utf8 (cork_byte (0xB8)), "ÿ"); // U+00FF ÿ
  string_eq (cork_to_utf8 (cork_byte (0xB9)), "ź"); // U+017A
  string_eq (cork_to_utf8 (cork_byte (0xBA)), "ž"); // U+017E
  string_eq (cork_to_utf8 (cork_byte (0xBB)), "ż"); // U+017C
  string_eq (cork_to_utf8 (cork_byte (0xBC)), "ĳ"); // U+0133
  string_eq (cork_to_utf8 (cork_byte (0xBD)), "¡"); // U+00A1 ¡
  string_eq (cork_to_utf8 (cork_byte (0xBE)), "¿"); // U+00BF ¿
  string_eq (cork_to_utf8 (cork_byte (0xBF)), "£"); // U+00A3 £
}

TEST_CASE ("test_cork_to_utf8_Cx") {
  string_eq (cork_to_utf8 (cork_byte (0xC0)), "À"); // U+00C0 À
  string_eq (cork_to_utf8 (cork_byte (0xC1)), "Á"); // U+00C1 Á
  string_eq (cork_to_utf8 (cork_byte (0xC2)), "Â"); // U+00C2 Â
  string_eq (cork_to_utf8 (cork_byte (0xC3)), "Ã"); // U+00C3 Ã
  string_eq (cork_to_utf8 (cork_byte (0xC4)), "Ä"); // U+00C4 Ä
  string_eq (cork_to_utf8 (cork_byte (0xC5)), "Å"); // U+00C5 Å
  string_eq (cork_to_utf8 (cork_byte (0xC6)), "Æ"); // U+00C6 Æ
  string_eq (cork_to_utf8 (cork_byte (0xC7)), "Ç"); // U+00C7 Ç
  string_eq (cork_to_utf8 (cork_byte (0xC8)), "È"); // U+00C8 È
  string_eq (cork_to_utf8 (cork_byte (0xC9)), "É"); // U+00C9 É
  string_eq (cork_to_utf8 (cork_byte (0xCA)), "Ê"); // U+00CA Ê
  string_eq (cork_to_utf8 (cork_byte (0xCB)), "Ë"); // U+00CB Ë
  string_eq (cork_to_utf8 (cork_byte (0xCC)), "Ì"); // U+00CC Ì
  string_eq (cork_to_utf8 (cork_byte (0xCD)), "Í"); // U+00CD Í
  string_eq (cork_to_utf8 (cork_byte (0xCE)), "Î"); // U+00CE Î
  string_eq (cork_to_utf8 (cork_byte (0xCF)), "Ï"); // U+00CF Ï
}

TEST_CASE ("test_cork_to_utf8_Dx") {
  string_eq (cork_to_utf8 (cork_byte (0xD0)), "Ð");  // U+00D0 Ð
  string_eq (cork_to_utf8 (cork_byte (0xD1)), "Ñ");  // U+00D1 Ñ
  string_eq (cork_to_utf8 (cork_byte (0xD2)), "Ò");  // U+00D2 Ò
  string_eq (cork_to_utf8 (cork_byte (0xD3)), "Ó");  // U+00D3 Ó
  string_eq (cork_to_utf8 (cork_byte (0xD4)), "Ô");  // U+00D4 Ô
  string_eq (cork_to_utf8 (cork_byte (0xD5)), "Õ");  // U+00D5 Õ
  string_eq (cork_to_utf8 (cork_byte (0xD6)), "Ö");  // U+00D6 Ö
  string_eq (cork_to_utf8 (cork_byte (0xD7)), "Œ");  // U+0152 Œ
  string_eq (cork_to_utf8 (cork_byte (0xD8)), "Ø");  // U+00D8 Ø
  string_eq (cork_to_utf8 (cork_byte (0xD9)), "Ù");  // U+00D9 Ù
  string_eq (cork_to_utf8 (cork_byte (0xDA)), "Ú");  // U+00DA Ú
  string_eq (cork_to_utf8 (cork_byte (0xDB)), "Û");  // U+00DB Û
  string_eq (cork_to_utf8 (cork_byte (0xDC)), "Ü");  // U+00DC Ü
  string_eq (cork_to_utf8 (cork_byte (0xDD)), "Ý");  // U+00DD Ý
  string_eq (cork_to_utf8 (cork_byte (0xDE)), "Þ");  // U+00DE Þ
  string_eq (cork_to_utf8 (cork_byte (0xDF)), "SS"); // sharp s (oneway)
}

TEST_CASE ("test_cork_to_utf8_Ex") {
  string_eq (cork_to_utf8 (cork_byte (0xE0)), "à"); // U+00E0 à
  string_eq (cork_to_utf8 (cork_byte (0xE1)), "á"); // U+00E1 á
  string_eq (cork_to_utf8 (cork_byte (0xE2)), "â"); // U+00E2 â
  string_eq (cork_to_utf8 (cork_byte (0xE3)), "ã"); // U+00E3 ã
  string_eq (cork_to_utf8 (cork_byte (0xE4)), "ä"); // U+00E4 ä
  string_eq (cork_to_utf8 (cork_byte (0xE5)), "å"); // U+00E5 å
  string_eq (cork_to_utf8 (cork_byte (0xE6)), "æ"); // U+00E6 æ
  string_eq (cork_to_utf8 (cork_byte (0xE7)), "ç"); // U+00E7 ç
  string_eq (cork_to_utf8 (cork_byte (0xE8)), "è"); // U+00E8 è
  string_eq (cork_to_utf8 (cork_byte (0xE9)), "é"); // U+00E9 é
  string_eq (cork_to_utf8 (cork_byte (0xEA)), "ê"); // U+00EA ê
  string_eq (cork_to_utf8 (cork_byte (0xEB)), "ë"); // U+00EB ë
  string_eq (cork_to_utf8 (cork_byte (0xEC)), "ì"); // U+00EC ì
  string_eq (cork_to_utf8 (cork_byte (0xED)), "í"); // U+00ED í
  string_eq (cork_to_utf8 (cork_byte (0xEE)), "î"); // U+00EE î
  string_eq (cork_to_utf8 (cork_byte (0xEF)), "ï"); // U+00EF ï
}

TEST_CASE ("test_cork_to_utf8_Fx") {
  string_eq (cork_to_utf8 (cork_byte (0xF0)), "ð"); // U+00F0 ð
  string_eq (cork_to_utf8 (cork_byte (0xF1)), "ñ"); // U+00F1 ñ
  string_eq (cork_to_utf8 (cork_byte (0xF2)), "ò"); // U+00F2 ò
  string_eq (cork_to_utf8 (cork_byte (0xF3)), "ó"); // U+00F3 ó
  string_eq (cork_to_utf8 (cork_byte (0xF4)), "ô"); // U+00F4 ô
  string_eq (cork_to_utf8 (cork_byte (0xF5)), "õ"); // U+00F5 õ
  string_eq (cork_to_utf8 (cork_byte (0xF6)), "ö"); // U+00F6 ö
  string_eq (cork_to_utf8 (cork_byte (0xF7)), "œ"); // U+0153 œ
  string_eq (cork_to_utf8 (cork_byte (0xF8)), "ø"); // U+00F8 ø
  string_eq (cork_to_utf8 (cork_byte (0xF9)), "ù"); // U+00F9 ù
  string_eq (cork_to_utf8 (cork_byte (0xFA)), "ú"); // U+00FA ú
  string_eq (cork_to_utf8 (cork_byte (0xFB)), "û"); // U+00FB û
  string_eq (cork_to_utf8 (cork_byte (0xFC)), "ü"); // U+00FC ü
  string_eq (cork_to_utf8 (cork_byte (0xFD)), "ý"); // U+00FD ý
  string_eq (cork_to_utf8 (cork_byte (0xFE)), "þ"); // U+00FE þ
  string_eq (cork_to_utf8 (cork_byte (0xFF)), "ß"); // U+00DF ß
}

TEST_CASE ("test_cork_to_utf8_escapes") {
  // <#XXXX> decodes to the UTF-8 encoding of the hex codepoint.
  string_eq (cork_to_utf8 ("<#4E2D>"), "中");  // U+4E2D
  string_eq (cork_to_utf8 ("<#2019>"), "’");   // U+2019
  string_eq (cork_to_utf8 ("<#1F600>"), "😀"); // U+1F600
  // Padded forms are accepted.
  string_eq (cork_to_utf8 ("<#0FF>"), "ÿ");  // U+00FF
  string_eq (cork_to_utf8 ("<#00FF>"), "ÿ"); // U+00FF
  // A <#XXXX> that names a Cork-mapped codepoint overrides the byte mapping.
  string_eq (cork_to_utf8 ("<#201C>"), "“"); // U+201C
}

TEST_CASE ("test_cork_to_utf8_named_entities") {
  string_eq (cork_to_utf8 ("<less>"), "<");
  string_eq (cork_to_utf8 ("<gtr>"), ">");
  string_eq (cork_to_utf8 ("<comma>"), ",");
  string_eq (cork_to_utf8 ("<grave>"), "`");
  // A single byte '<' or '>' is passed through as the literal character.
  string_eq (cork_to_utf8 ("<"), "<");
  string_eq (cork_to_utf8 (">"), ">");
}

TEST_CASE ("test_utf8_to_cork_named_entities") {
  string_eq (utf8_to_cork ("<"), "<less>");
  string_eq (utf8_to_cork (">"), "<gtr>");
  string_eq (utf8_to_cork (" "), "<varspace>"); // U+00A0
}

TEST_CASE ("test_mixed") {
  // Cork byte + <#XXXX> escape + ASCII, decoded.
  string_eq (
      cork_to_utf8 (cork_byte (0x41) * string ("<#2019>") * cork_byte (0x42)),
      "A" * string ("’") * "B");
  // ASCII + CJK + ASCII, encoded.
  string_eq (utf8_to_cork (string ("A") * "中" * "B"),
             cork_byte (0x41) * string ("<#4E2D>") * cork_byte (0x42));
  // Named entity interleaved with bytes.
  string_eq (cork_to_utf8 ("<less>" * cork_byte (0x41) * "<gtr>"), "<A>");
}

TEST_CASE ("test_empty") {
  string_eq (cork_to_utf8 (""), "");
  string_eq (utf8_to_cork (""), "");
}

TEST_CASE ("test_roundtrip_idempotent") {
  // Every roundtrippable Cork byte satisfies utf8_to_cork(cork_to_utf8(b)) ==
  // b. The 5 non-roundtrip bytes (0x18, 0x1A, 0x3C, 0x3E, 0xDF) are documented
  // in devel/1125.md.
  for (int b= 0; b < 256; b++) {
    if (b == 0x18 || b == 0x1A || b == 0x3C || b == 0x3E || b == 0xDF) continue;
    string cork= cork_byte (b);
    string back= utf8_to_cork (cork_to_utf8 (cork));
    string_eq (back, cork);
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

TEST_CASE ("test_named_text_punctuation") {
  string_eq (cork_to_utf8 ("<less>"), "<");
  string_eq (cork_to_utf8 ("<gtr>"), ">");
  string_eq (cork_to_utf8 ("<comma>"), ",");
  string_eq (cork_to_utf8 ("<grave>"), "`");
  string_eq (cork_to_utf8 ("<varspace>"), " ");  // U+00A0 nbsp
  string_eq (cork_to_utf8 ("<bullet>"), "•");    // U+2022
  string_eq (cork_to_utf8 ("<dag>"), "†");       // U+2020 dagger
  string_eq (cork_to_utf8 ("<ddag>"), "‡");      // U+2021 double dagger
  string_eq (cork_to_utf8 ("<paragraph>"), "¶"); // U+00B6 pilcrow
  string_eq (cork_to_utf8 ("<copyright>"), "©"); // U+00A9
  string_eq (cork_to_utf8 ("<trademark>"), "™"); // U+2122
  string_eq (cork_to_utf8 ("<degree>"), "°");    // U+00B0
  string_eq (cork_to_utf8 ("<hyphen>"), "­");    // U+00AD soft hyphen
  string_eq (cork_to_utf8 ("<nbhyph>"), "‑");    // U+2011 non-breaking hyphen
}

TEST_CASE ("test_named_currency") {
  string_eq (cork_to_utf8 ("<cent>"), "¢");     // U+00A2
  string_eq (cork_to_utf8 ("<yen>"), "¥");      // U+00A5
  string_eq (cork_to_utf8 ("<currency>"), "¤"); // U+00A4
  // <sterling> decodes to U+00A3 but round-trips to Cork byte 0xBF;
  // see test_named_non_identity_roundtrip.
  string_eq (cork_to_utf8 ("<sterling>"), "£"); // U+00A3
}

TEST_CASE ("test_named_greek_lower") {
  string_eq (cork_to_utf8 ("<alpha>"), "α");   // U+03B1
  string_eq (cork_to_utf8 ("<beta>"), "β");    // U+03B2
  string_eq (cork_to_utf8 ("<gamma>"), "γ");   // U+03B3
  string_eq (cork_to_utf8 ("<delta>"), "δ");   // U+03B4
  string_eq (cork_to_utf8 ("<epsilon>"), "ϵ"); // U+03F5 lunate epsilon
  string_eq (cork_to_utf8 ("<zeta>"), "ζ");    // U+03B6
  string_eq (cork_to_utf8 ("<eta>"), "η");     // U+03B7
  string_eq (cork_to_utf8 ("<theta>"), "θ");   // U+03B8
  string_eq (cork_to_utf8 ("<iota>"), "ι");    // U+03B9
  string_eq (cork_to_utf8 ("<kappa>"), "κ");   // U+03BA
  string_eq (cork_to_utf8 ("<lambda>"), "λ");  // U+03BB
  string_eq (cork_to_utf8 ("<mu>"), "μ");      // U+03BC
  string_eq (cork_to_utf8 ("<nu>"), "ν");      // U+03BD
  string_eq (cork_to_utf8 ("<xi>"), "ξ");      // U+03BE
  string_eq (cork_to_utf8 ("<omicron>"), "ο"); // U+03BF
  string_eq (cork_to_utf8 ("<pi>"), "π");      // U+03C0
  string_eq (cork_to_utf8 ("<rho>"), "ρ");     // U+03C1
  string_eq (cork_to_utf8 ("<sigma>"), "σ");   // U+03C3
  string_eq (cork_to_utf8 ("<tau>"), "τ");     // U+03C4
  string_eq (cork_to_utf8 ("<upsilon>"), "υ"); // U+03C5
  string_eq (cork_to_utf8 ("<phi>"), "ϕ");     // U+03D5 phi
  string_eq (cork_to_utf8 ("<chi>"), "χ");     // U+03C7
  string_eq (cork_to_utf8 ("<psi>"), "ψ");     // U+03C8
  string_eq (cork_to_utf8 ("<omega>"), "ω");   // U+03C9
}

TEST_CASE ("test_named_greek_upper") {
  string_eq (cork_to_utf8 ("<Alpha>"), "Α");   // U+0391
  string_eq (cork_to_utf8 ("<Beta>"), "Β");    // U+0392
  string_eq (cork_to_utf8 ("<Gamma>"), "Γ");   // U+0393
  string_eq (cork_to_utf8 ("<Delta>"), "Δ");   // U+0394
  string_eq (cork_to_utf8 ("<Epsilon>"), "Ε"); // U+0395
  string_eq (cork_to_utf8 ("<Zeta>"), "Ζ");    // U+0396
  string_eq (cork_to_utf8 ("<Eta>"), "Η");     // U+0397
  string_eq (cork_to_utf8 ("<Theta>"), "Θ");   // U+0398
  string_eq (cork_to_utf8 ("<Iota>"), "Ι");    // U+0399
  string_eq (cork_to_utf8 ("<Kappa>"), "Κ");   // U+039A
  string_eq (cork_to_utf8 ("<Lambda>"), "Λ");  // U+039B
  string_eq (cork_to_utf8 ("<Mu>"), "Μ");      // U+039C
  string_eq (cork_to_utf8 ("<Nu>"), "Ν");      // U+039D
  string_eq (cork_to_utf8 ("<Xi>"), "Ξ");      // U+039E
  string_eq (cork_to_utf8 ("<Omicron>"), "Ο"); // U+039F
  string_eq (cork_to_utf8 ("<Pi>"), "Π");      // U+03A0
  string_eq (cork_to_utf8 ("<Rho>"), "Ρ");     // U+03A1
  string_eq (cork_to_utf8 ("<Sigma>"), "Σ");   // U+03A3
  string_eq (cork_to_utf8 ("<Tau>"), "Τ");     // U+03A4
  string_eq (cork_to_utf8 ("<Upsilon>"), "Υ"); // U+03A5
  string_eq (cork_to_utf8 ("<Phi>"), "Φ");     // U+03A6
  string_eq (cork_to_utf8 ("<Chi>"), "Χ");     // U+03A7
  string_eq (cork_to_utf8 ("<Psi>"), "Ψ");     // U+03A8
  string_eq (cork_to_utf8 ("<Omega>"), "Ω");   // U+03A9
}

TEST_CASE ("test_named_greek_variants") {
  // Note: <epsilon> -> U+03F5 (lunate) while <varepsilon> -> U+03B5 (plain),
  // and <phi> -> U+03D5 while <varphi> -> U+03C6; the variant and plain forms
  // are swapped relative to the usual convention.
  string_eq (cork_to_utf8 ("<varepsilon>"), "ε"); // U+03B5
  string_eq (cork_to_utf8 ("<vartheta>"), "ϑ");   // U+03D1
  string_eq (cork_to_utf8 ("<varpi>"), "ϖ");      // U+03D6
  string_eq (cork_to_utf8 ("<varrho>"), "ϱ");     // U+03F1
  string_eq (cork_to_utf8 ("<varsigma>"), "ς");   // U+03C2
  string_eq (cork_to_utf8 ("<varphi>"), "φ");     // U+03C6
}

TEST_CASE ("test_named_binary_operators") {
  string_eq (cork_to_utf8 ("<times>"), "×");    // U+00D7
  string_eq (cork_to_utf8 ("<div>"), "÷");      // U+00F7
  string_eq (cork_to_utf8 ("<cdot>"), "⋅");     // U+22C5
  string_eq (cork_to_utf8 ("<ast>"), "∗");      // U+2217
  string_eq (cork_to_utf8 ("<dotplus>"), "∔");  // U+2214
  string_eq (cork_to_utf8 ("<cap>"), "∩");      // U+2229
  string_eq (cork_to_utf8 ("<cup>"), "∪");      // U+222A
  string_eq (cork_to_utf8 ("<sqcap>"), "⊓");    // U+2293
  string_eq (cork_to_utf8 ("<sqcup>"), "⊔");    // U+2294
  string_eq (cork_to_utf8 ("<wedge>"), "∧");    // U+2227
  string_eq (cork_to_utf8 ("<vee>"), "∨");      // U+2228
  string_eq (cork_to_utf8 ("<setminus>"), "∖"); // U+2216
  string_eq (cork_to_utf8 ("<amalg>"), "⨿");    // U+2A3F
  string_eq (cork_to_utf8 ("<wr>"), "≀");       // U+2240
}

TEST_CASE ("test_named_relations") {
  string_eq (cork_to_utf8 ("<neq>"), "≠");      // U+2260
  string_eq (cork_to_utf8 ("<leq>"), "≤");      // U+2264
  string_eq (cork_to_utf8 ("<geq>"), "≥");      // U+2265
  string_eq (cork_to_utf8 ("<leqslant>"), "⩽"); // U+2A7D
  string_eq (cork_to_utf8 ("<geqslant>"), "⩾"); // U+2A7E
  string_eq (cork_to_utf8 ("<ll>"), "≪");       // U+226A
  string_eq (cork_to_utf8 ("<gg>"), "≫");       // U+226B
  string_eq (cork_to_utf8 ("<equiv>"), "≡");    // U+2261
  string_eq (cork_to_utf8 ("<sim>"), "∼");      // U+223C
  string_eq (cork_to_utf8 ("<simeq>"), "≃");    // U+2243
  string_eq (cork_to_utf8 ("<cong>"), "≅");     // U+2245
  string_eq (cork_to_utf8 ("<approx>"), "≈");   // U+2248
  string_eq (cork_to_utf8 ("<prec>"), "≺");     // U+227A
  string_eq (cork_to_utf8 ("<succ>"), "≻");     // U+227B
}

TEST_CASE ("test_named_arrows") {
  string_eq (cork_to_utf8 ("<rightarrow>"), "→");     // U+2192
  string_eq (cork_to_utf8 ("<leftarrow>"), "←");      // U+2190
  string_eq (cork_to_utf8 ("<leftrightarrow>"), "↔"); // U+2194
  string_eq (cork_to_utf8 ("<Rightarrow>"), "⇒");     // U+21D2
  string_eq (cork_to_utf8 ("<Leftarrow>"), "⇐");      // U+21D0
  string_eq (cork_to_utf8 ("<uparrow>"), "↑");        // U+2191
  string_eq (cork_to_utf8 ("<downarrow>"), "↓");      // U+2193
  string_eq (cork_to_utf8 ("<mapsto>"), "↦");         // U+21A6
  string_eq (cork_to_utf8 ("<leftharpoonup>"), "↼");  // U+21BC
  string_eq (cork_to_utf8 ("<rightharpoonup>"), "⇀"); // U+21C0
}

TEST_CASE ("test_named_set_theory") {
  string_eq (cork_to_utf8 ("<in>"), "∈");       // U+2208
  string_eq (cork_to_utf8 ("<notin>"), "∉");    // U+2209
  string_eq (cork_to_utf8 ("<subset>"), "⊂");   // U+2282
  string_eq (cork_to_utf8 ("<supset>"), "⊃");   // U+2283
  string_eq (cork_to_utf8 ("<subseteq>"), "⊆"); // U+2286
  string_eq (cork_to_utf8 ("<supseteq>"), "⊇"); // U+2287
  string_eq (cork_to_utf8 ("<sqsubset>"), "⊏"); // U+228F
  string_eq (cork_to_utf8 ("<sqsupset>"), "⊐"); // U+2290
  string_eq (cork_to_utf8 ("<emptyset>"), "∅"); // U+2205
}

TEST_CASE ("test_named_calculus") {
  string_eq (cork_to_utf8 ("<partial>"), "∂"); // U+2202
  string_eq (cork_to_utf8 ("<nabla>"), "∇");   // U+2207
  string_eq (cork_to_utf8 ("<infty>"), "∞");   // U+221E
  string_eq (cork_to_utf8 ("<sqrt>"), "√");    // U+221A
  string_eq (cork_to_utf8 ("<forall>"), "∀");  // U+2200
  string_eq (cork_to_utf8 ("<exists>"), "∃");  // U+2203
  string_eq (cork_to_utf8 ("<angle>"), "∠");   // U+2220
  string_eq (cork_to_utf8 ("<aleph>"), "ℵ");   // U+2135
  // <sum> decodes to U+2211 but round-trips to <big-sum>; see non-identity
  // test.
  string_eq (cork_to_utf8 ("<sum>"), "∑");  // U+2211
  string_eq (cork_to_utf8 ("<int>"), "∫");  // U+222B
  string_eq (cork_to_utf8 ("<prod>"), "∏"); // U+220F
}

TEST_CASE ("test_named_dots_dashes") {
  string_eq (cork_to_utf8 ("<cdots>"), "⋯");     // U+22EF
  string_eq (cork_to_utf8 ("<ldots>"), "…");     // U+2026
  string_eq (cork_to_utf8 ("<vdots>"), "⋮");     // U+22EE
  string_eq (cork_to_utf8 ("<ddots>"), "⋱");     // U+22F1
  string_eq (cork_to_utf8 ("<prime>"), "′");     // U+2032
  string_eq (cork_to_utf8 ("<backprime>"), "‵"); // U+2035
}

TEST_CASE ("test_named_special_letterforms") {
  string_eq (cork_to_utf8 ("<aleph>"), "ℵ");  // U+2135
  string_eq (cork_to_utf8 ("<beth>"), "ℶ");   // U+2136
  string_eq (cork_to_utf8 ("<gimel>"), "ℷ");  // U+2137
  string_eq (cork_to_utf8 ("<daleth>"), "ℸ"); // U+2138
  string_eq (cork_to_utf8 ("<ell>"), "ℓ");    // U+2113
  // <hbar> decodes to U+210F but round-trips to <hslash>; see non-identity
  // test.
  string_eq (cork_to_utf8 ("<hbar>"), "ℏ");   // U+210F
  string_eq (cork_to_utf8 ("<hslash>"), "ℏ"); // U+210F
  // <Re> / <Im> decode to U+211C / U+2111 but round-trip to <frak-R>/<frak-I>.
  string_eq (cork_to_utf8 ("<Re>"), "ℜ"); // U+211C
  string_eq (cork_to_utf8 ("<Im>"), "ℑ"); // U+2111
}

TEST_CASE ("test_named_non_identity_roundtrip") {
  // These entities decode to a codepoint that has a direct Cork byte mapping
  // (preferred over the named entity), so utf8_to_cork does not recover the
  // original name. Documented in devel/1125.md.
  string_eq (utf8_to_cork (cork_to_utf8 ("<sterling>")),
             cork_byte (0xBF)); // £ is Cork 0xBF
  string_eq (utf8_to_cork (cork_to_utf8 ("<guillemotleft>")),
             cork_byte (0x13)); // « is Cork 0x13
  string_eq (utf8_to_cork (cork_to_utf8 ("<guillemotright>")),
             cork_byte (0x14)); // » is Cork 0x14
  // These decode to a codepoint whose canonical reverse name differs.
  string_eq (utf8_to_cork (cork_to_utf8 ("<sum>")), "<big-sum>");
  string_eq (utf8_to_cork (cork_to_utf8 ("<hbar>")), "<hslash>");
  string_eq (utf8_to_cork (cork_to_utf8 ("<Re>")), "<frak-R>");
  string_eq (utf8_to_cork (cork_to_utf8 ("<Im>")), "<frak-I>");
  // Most named entities DO round-trip to themselves; sample a few.
  string_eq (utf8_to_cork (cork_to_utf8 ("<alpha>")), "<alpha>");
  string_eq (utf8_to_cork (cork_to_utf8 ("<infty>")), "<infty>");
  string_eq (utf8_to_cork (cork_to_utf8 ("<partial>")), "<partial>");
  string_eq (utf8_to_cork (cork_to_utf8 ("<rightarrow>")), "<rightarrow>");
  string_eq (utf8_to_cork (cork_to_utf8 ("<emptyset>")), "<emptyset>");
}

// utf8_to_cork direction: codepoints without a direct Cork byte are encoded
// as their canonical named entity. The entity name is not always the obvious
// one (e.g. U+2209 -> <nin>, U+2211 -> <big-sum>, U+211C -> <frak-R>); these
// remaps are sampled in test_utf8_to_cork_named_remap.

TEST_CASE ("test_utf8_to_cork_greek") {
  string_eq (utf8_to_cork ("α"), "<alpha>"); // U+03B1
  string_eq (utf8_to_cork ("β"), "<beta>");  // U+03B2
  string_eq (utf8_to_cork ("γ"), "<gamma>"); // U+03B3
  string_eq (utf8_to_cork ("δ"), "<delta>"); // U+03B4
  string_eq (utf8_to_cork ("π"), "<pi>");    // U+03C0
  string_eq (utf8_to_cork ("σ"), "<sigma>"); // U+03C3
  string_eq (utf8_to_cork ("ω"), "<omega>"); // U+03C9
  string_eq (utf8_to_cork ("Α"), "<Alpha>"); // U+0391
  string_eq (utf8_to_cork ("Β"), "<Beta>");  // U+0392
  string_eq (utf8_to_cork ("Γ"), "<Gamma>"); // U+0393
  string_eq (utf8_to_cork ("Δ"), "<Delta>"); // U+0394
  string_eq (utf8_to_cork ("Π"), "<Pi>");    // U+03A0
  string_eq (utf8_to_cork ("Σ"), "<Sigma>"); // U+03A3
  string_eq (utf8_to_cork ("Ω"), "<Omega>"); // U+03A9
}

TEST_CASE ("test_utf8_to_cork_math_ops") {
  string_eq (utf8_to_cork ("×"), "<times>");    // U+00D7
  string_eq (utf8_to_cork ("÷"), "<div>");      // U+00F7
  string_eq (utf8_to_cork ("⋅"), "<cdot>");     // U+22C5
  string_eq (utf8_to_cork ("∗"), "<ast>");      // U+2217
  string_eq (utf8_to_cork ("∩"), "<cap>");      // U+2229
  string_eq (utf8_to_cork ("∪"), "<cup>");      // U+222A
  string_eq (utf8_to_cork ("∧"), "<wedge>");    // U+2227
  string_eq (utf8_to_cork ("∨"), "<vee>");      // U+2228
  string_eq (utf8_to_cork ("∖"), "<setminus>"); // U+2216
}

TEST_CASE ("test_utf8_to_cork_math_syms") {
  string_eq (utf8_to_cork ("∞"), "<infty>");          // U+221E
  string_eq (utf8_to_cork ("∂"), "<partial>");        // U+2202
  string_eq (utf8_to_cork ("∇"), "<nabla>");          // U+2207
  string_eq (utf8_to_cork ("∀"), "<forall>");         // U+2200
  string_eq (utf8_to_cork ("∃"), "<exists>");         // U+2203
  string_eq (utf8_to_cork ("∠"), "<angle>");          // U+2220
  string_eq (utf8_to_cork ("∅"), "<emptyset>");       // U+2205
  string_eq (utf8_to_cork ("≠"), "<neq>");            // U+2260
  string_eq (utf8_to_cork ("≤"), "<leq>");            // U+2264
  string_eq (utf8_to_cork ("≥"), "<geq>");            // U+2265
  string_eq (utf8_to_cork ("≡"), "<equiv>");          // U+2261
  string_eq (utf8_to_cork ("≈"), "<approx>");         // U+2248
  string_eq (utf8_to_cork ("∈"), "<in>");             // U+2208
  string_eq (utf8_to_cork ("⊂"), "<subset>");         // U+2282
  string_eq (utf8_to_cork ("⊃"), "<supset>");         // U+2283
  string_eq (utf8_to_cork ("→"), "<rightarrow>");     // U+2192
  string_eq (utf8_to_cork ("←"), "<leftarrow>");      // U+2190
  string_eq (utf8_to_cork ("↔"), "<leftrightarrow>"); // U+2194
  string_eq (utf8_to_cork ("⇒"), "<Rightarrow>");     // U+21D2
  string_eq (utf8_to_cork ("⇐"), "<Leftarrow>");      // U+21D0
  string_eq (utf8_to_cork ("↦"), "<mapsto>");         // U+21A6
  string_eq (utf8_to_cork ("ℵ"), "<aleph>");          // U+2135
  string_eq (utf8_to_cork ("ℓ"), "<ell>");            // U+2113
}

TEST_CASE ("test_utf8_to_cork_text_syms") {
  string_eq (utf8_to_cork ("•"), "<bullet>");    // U+2022
  string_eq (utf8_to_cork ("†"), "<dagger>");    // U+2020
  string_eq (utf8_to_cork ("‡"), "<ddagger>");   // U+2021
  string_eq (utf8_to_cork ("°"), "<degree>");    // U+00B0
  string_eq (utf8_to_cork ("©"), "<copyright>"); // U+00A9
  string_eq (utf8_to_cork ("®"), "<circledR>");  // U+00AE
  string_eq (utf8_to_cork ("™"), "<trademark>"); // U+2122
  string_eq (utf8_to_cork ("…"), "<ldots>");     // U+2026
}

TEST_CASE ("test_utf8_to_cork_named_remap") {
  // Canonical reverse names that differ from the obvious/forward entity name.
  string_eq (utf8_to_cork ("∉"), "<nin>");      // U+2209 (not <notin>)
  string_eq (utf8_to_cork ("∑"), "<big-sum>");  // U+2211 (not <sum>)
  string_eq (utf8_to_cork ("∫"), "<big-int>");  // U+222B (not <int>)
  string_eq (utf8_to_cork ("∏"), "<big-prod>"); // U+220F (not <prod>)
  string_eq (utf8_to_cork ("ℏ"), "<hslash>");   // U+210F (not <hbar>)
  string_eq (utf8_to_cork ("ℜ"), "<frak-R>");   // U+211C (not <Re>)
  string_eq (utf8_to_cork ("ℑ"), "<frak-I>");   // U+2111 (not <Im>)
}

TEST_CASE ("test_cork_to_utf8_escape_edge_cases") {
  // Hex digits are case-insensitive.
  string_eq (cork_to_utf8 ("<#4e2d>"), "中");
  // Multiple escapes in one string decode independently.
  string_eq (cork_to_utf8 ("<#4E2D><#2019>"), "中’");
  // Escapes interleave with literal bytes.
  string_eq (cork_to_utf8 ("X<#4E2D>Y"), "X中Y");
  string_eq (cork_to_utf8 ("<#4E2D><alpha>"), "中α");
  // A dangling escape with no closing '>' consumes to end of string.
  string_eq (cork_to_utf8 ("a<#4E2D"), "a中");
  // Empty hex, lone "<#", and non-hex digits all decode to the empty string
  // (from_hexadecimal("") == 0, encode_as_utf8(0) == "").
  string_eq (cork_to_utf8 ("<#>"), "");
  string_eq (cork_to_utf8 ("<#"), "");
  string_eq (cork_to_utf8 ("<#GHIJ>"), "");
}

TEST_CASE ("test_cork_to_utf8_unknown_entities") {
  // An unknown <name> is passed through verbatim.
  string_eq (cork_to_utf8 ("<foobar>"), "<foobar>");
  // An incomplete entity prefix (no closing '>') is passed through verbatim.
  string_eq (cork_to_utf8 ("<les"), "<les");
  // A single '<' not followed by '#' is passed through as a literal.
  string_eq (cork_to_utf8 ("a<b"), "a<b");
}

TEST_CASE ("test_utf8_to_cork_invalid_utf8") {
  // A lone continuation byte (0x80) has no UTF-8 decode; it is copied through
  // as the Cork byte 0x80 (copy_unmatched).
  string_eq (utf8_to_cork ("\x80"), cork_byte (0x80));
  // A lone lead byte (0xC3 with no continuation) likewise copies through.
  string_eq (utf8_to_cork ("\xC3"), cork_byte (0xC3));
}

TEST_CASE ("test_named_mathops_extras") {
  string_eq (cork_to_utf8 ("<asymp>"), "≍");       // U+224D
  string_eq (cork_to_utf8 ("<backsim>"), "∽");     // U+223D
  string_eq (cork_to_utf8 ("<between>"), "≬");     // U+226C
  string_eq (cork_to_utf8 ("<doteq>"), "≐");       // U+2250
  string_eq (cork_to_utf8 ("<lessdot>"), "⋖");     // U+22D6
  string_eq (cork_to_utf8 ("<gtrdot>"), "⋗");      // U+22D7
  string_eq (cork_to_utf8 ("<lesssim>"), "≲");     // U+2272
  string_eq (cork_to_utf8 ("<gtrsim>"), "≳");      // U+2273
  string_eq (cork_to_utf8 ("<lessgtr>"), "≶");     // U+2276
  string_eq (cork_to_utf8 ("<gtrless>"), "≷");     // U+2277
  string_eq (cork_to_utf8 ("<curlyeqprec>"), "⋞"); // U+22DE
  string_eq (cork_to_utf8 ("<curlyeqsucc>"), "⋟"); // U+22DF
  string_eq (cork_to_utf8 ("<Bumpeq>"), "≎");      // U+224E
  string_eq (cork_to_utf8 ("<Cap>"), "⋒");         // U+22D2
  string_eq (cork_to_utf8 ("<Cup>"), "⋓");         // U+22D3
  string_eq (cork_to_utf8 ("<Subset>"), "⋐");      // U+22D0
  string_eq (cork_to_utf8 ("<Supset>"), "⋑");      // U+22D1
}

TEST_CASE ("test_named_arrows_extras") {
  string_eq (cork_to_utf8 ("<Downarrow>"), "⇓");       // U+21D3
  string_eq (cork_to_utf8 ("<Leftrightarrow>"), "⇔");  // U+21D4
  string_eq (cork_to_utf8 ("<Updownarrow>"), "⇕");     // U+21D5
  string_eq (cork_to_utf8 ("<Lleftarrow>"), "⇚");      // U+21DA
  string_eq (cork_to_utf8 ("<Rrightarrow>"), "⇛");     // U+21DB
  string_eq (cork_to_utf8 ("<hookleftarrow>"), "↩");   // U+21A9
  string_eq (cork_to_utf8 ("<hookrightarrow>"), "↪");  // U+21AA
  string_eq (cork_to_utf8 ("<nwarrow>"), "↖");         // U+2196
  string_eq (cork_to_utf8 ("<nearrow>"), "↗");         // U+2197
  string_eq (cork_to_utf8 ("<searrow>"), "↘");         // U+2198
  string_eq (cork_to_utf8 ("<swarrow>"), "↙");         // U+2199
  string_eq (cork_to_utf8 ("<mapsfrom>"), "↤");        // U+21A4
  string_eq (cork_to_utf8 ("<nleftarrow>"), "↚");      // U+219A
  string_eq (cork_to_utf8 ("<nrightarrow>"), "↛");     // U+219B
  string_eq (cork_to_utf8 ("<nLeftarrow>"), "⇍");      // U+21CD
  string_eq (cork_to_utf8 ("<nRightarrow>"), "⇏");     // U+21CF
  string_eq (cork_to_utf8 ("<nleftrightarrow>"), "↮"); // U+21AE
}

TEST_CASE ("test_named_misc_symbols") {
  // Zodiac
  string_eq (cork_to_utf8 ("<aries>"), "♈");       // U+2648
  string_eq (cork_to_utf8 ("<taurus>"), "♉");      // U+2649
  string_eq (cork_to_utf8 ("<gemini>"), "♊");      // U+264A
  string_eq (cork_to_utf8 ("<cancer>"), "♋");      // U+264B
  string_eq (cork_to_utf8 ("<leo>"), "♌");         // U+264C
  string_eq (cork_to_utf8 ("<virgo>"), "♍");       // U+264D
  string_eq (cork_to_utf8 ("<libra>"), "♎");       // U+264E
  string_eq (cork_to_utf8 ("<scorpio>"), "♏");     // U+264F
  string_eq (cork_to_utf8 ("<sagittarius>"), "♐"); // U+2650
  string_eq (cork_to_utf8 ("<capricornus>"), "♑"); // U+2651
  string_eq (cork_to_utf8 ("<aquarius>"), "♒");    // U+2652
  string_eq (cork_to_utf8 ("<pisces>"), "♓");      // U+2653
  // Planets / astrological
  string_eq (cork_to_utf8 ("<astrosun>"), "☉"); // U+2609
  string_eq (cork_to_utf8 ("<mercury>"), "☿");  // U+263F
  string_eq (cork_to_utf8 ("<female>"), "♀");   // U+2640
  string_eq (cork_to_utf8 ("<male>"), "♂");     // U+2642
  string_eq (cork_to_utf8 ("<earth>"), "♁");    // U+2641
  // Music
  string_eq (cork_to_utf8 ("<flat>"), "♭");       // U+266D
  string_eq (cork_to_utf8 ("<natural>"), "♮");    // U+266E
  string_eq (cork_to_utf8 ("<sharp>"), "♯");      // U+266F
  string_eq (cork_to_utf8 ("<eighthnote>"), "♪"); // U+266A
  // Card suits
  string_eq (cork_to_utf8 ("<spadesuit>"), "♠");   // U+2660
  string_eq (cork_to_utf8 ("<heartsuit>"), "♥");   // U+2665
  string_eq (cork_to_utf8 ("<diamondsuit>"), "♦"); // U+2666
  string_eq (cork_to_utf8 ("<clubsuit>"), "♣");    // U+2663
  // Other
  string_eq (cork_to_utf8 ("<checkmark>"), "✓"); // U+2713
  string_eq (cork_to_utf8 ("<maltese>"), "✠");   // U+2720
}

TEST_CASE ("test_named_letterlike_extras") {
  string_eq (cork_to_utf8 ("<Mho>"), "℧");         // U+2127
  string_eq (cork_to_utf8 ("<complement>"), "∁");  // U+2201
  string_eq (cork_to_utf8 ("<nexists>"), "∄");     // U+2204
  string_eq (cork_to_utf8 ("<circledS>"), "Ⓢ");    // U+24C8
  string_eq (cork_to_utf8 ("<backepsilon>"), "϶"); // U+03F6
  string_eq (cork_to_utf8 ("<digamma>"), "ϝ");     // U+03DD
  string_eq (cork_to_utf8 ("<varkappa>"), "ϰ");    // U+03F0
  // Blackboard bold constants
  string_eq (cork_to_utf8 ("<bbb-C>"), "ℂ"); // U+2102
  string_eq (cork_to_utf8 ("<bbb-H>"), "ℍ"); // U+210D
  string_eq (cork_to_utf8 ("<bbb-N>"), "ℕ"); // U+2115
  string_eq (cork_to_utf8 ("<bbb-P>"), "ℙ"); // U+2119
  string_eq (cork_to_utf8 ("<bbb-Q>"), "ℚ"); // U+211A
  string_eq (cork_to_utf8 ("<bbb-R>"), "ℝ"); // U+211D
  string_eq (cork_to_utf8 ("<bbb-Z>"), "ℤ"); // U+2124
}

TEST_CASE ("test_named_geometric") {
  string_eq (cork_to_utf8 ("<Circle>"), "○");            // U+25CB
  string_eq (cork_to_utf8 ("<CIRCLE>"), "●");            // U+25CF
  string_eq (cork_to_utf8 ("<Square>"), "□");            // U+25A1
  string_eq (cork_to_utf8 ("<blacksquare>"), "■");       // U+25A0
  string_eq (cork_to_utf8 ("<star>"), "⋆");              // U+22C6
  string_eq (cork_to_utf8 ("<bigstar>"), "★");           // U+2605
  string_eq (cork_to_utf8 ("<vartriangle>"), "▵");       // U+25B5
  string_eq (cork_to_utf8 ("<blacktriangle>"), "▴");     // U+25B4
  string_eq (cork_to_utf8 ("<blacktriangledown>"), "▾"); // U+25BE
  string_eq (cork_to_utf8 ("<diamond>"), "⋄");           // U+22C4
  string_eq (cork_to_utf8 ("<lozenge>"), "◊");           // U+25CA
  string_eq (cork_to_utf8 ("<blacklozenge>"), "⧫");      // U+29EB
}

TEST_CASE ("test_named_apl_technical") {
  string_eq (cork_to_utf8 ("<APLbox>"), "⎕");           // U+2395
  string_eq (cork_to_utf8 ("<APLinput>"), "⍞");         // U+235E
  string_eq (cork_to_utf8 ("<APLleftarrowbox>"), "⍇");  // U+2347
  string_eq (cork_to_utf8 ("<APLrightarrowbox>"), "⍈"); // U+2348
  string_eq (cork_to_utf8 ("<APLuparrowbox>"), "⍐");    // U+2350
  string_eq (cork_to_utf8 ("<APLdownarrowbox>"), "⍗");  // U+2357
}

TEST_CASE ("test_named_big_operators") {
  string_eq (cork_to_utf8 ("<big-vee>"), "⋁");     // U+22C1
  string_eq (cork_to_utf8 ("<big-wedge>"), "⋀");   // U+22C0
  string_eq (cork_to_utf8 ("<big-cap>"), "⋂");     // U+22C2
  string_eq (cork_to_utf8 ("<big-cup>"), "⋃");     // U+22C3
  string_eq (cork_to_utf8 ("<big-oplus>"), "⨁");   // U+2A01
  string_eq (cork_to_utf8 ("<big-otimes>"), "⨂");  // U+2A02
  string_eq (cork_to_utf8 ("<big-odot>"), "⨀");    // U+2A00
  string_eq (cork_to_utf8 ("<big-sqcup>"), "⨆");   // U+2A06
  string_eq (cork_to_utf8 ("<big-pluscup>"), "⨄"); // U+2A04
}

TEST_CASE ("test_named_brackets") {
  string_eq (cork_to_utf8 ("<langle>"), "⟨");    // U+27E8
  string_eq (cork_to_utf8 ("<rangle>"), "⟩");    // U+27E9
  string_eq (cork_to_utf8 ("<llangle>"), "⟪");   // U+27EA
  string_eq (cork_to_utf8 ("<rrangle>"), "⟫");   // U+27EB
  string_eq (cork_to_utf8 ("<llbracket>"), "⟦"); // U+27E6
  string_eq (cork_to_utf8 ("<rrbracket>"), "⟧"); // U+27E7
  string_eq (cork_to_utf8 ("<lceil>"), "⌈");     // U+2308
  string_eq (cork_to_utf8 ("<rceil>"), "⌉");     // U+2309
  string_eq (cork_to_utf8 ("<lfloor>"), "⌊");    // U+230A
  string_eq (cork_to_utf8 ("<rfloor>"), "⌋");    // U+230B
  string_eq (cork_to_utf8 ("<ulcorner>"), "⌜");  // U+231C
  string_eq (cork_to_utf8 ("<urcorner>"), "⌝");  // U+231D
  string_eq (cork_to_utf8 ("<llcorner>"), "⌞");  // U+231E
  string_eq (cork_to_utf8 ("<lrcorner>"), "⌟");  // U+231F
}

TEST_CASE ("test_named_math_alphanumeric") {
  // Math Alphanumeric Symbols block (U+1D400+). Only four prefixes are
  // registered: <b-*> (bold/italic), <bbb-*> (blackboard bold),
  // <cal-*> (script), <frak-*> (fraktur). Other variants like <i->, <sf->,
  // <tt-> are not entities and pass through verbatim.
  string_eq (cork_to_utf8 ("<b-0>"), "𝟎");    // U+1D7CE bold 0
  string_eq (cork_to_utf8 ("<b-A>"), "𝑨");    // U+1D468 bold italic A
  string_eq (cork_to_utf8 ("<b-z>"), "𝒛");    // U+1D49B bold italic z
  string_eq (cork_to_utf8 ("<bbb-A>"), "𝔸");  // U+1D538
  string_eq (cork_to_utf8 ("<bbb-z>"), "𝕫");  // U+1D56B
  string_eq (cork_to_utf8 ("<cal-A>"), "𝒜");  // U+1D49C
  string_eq (cork_to_utf8 ("<cal-z>"), "𝓏");  // U+1D4CF
  string_eq (cork_to_utf8 ("<frak-A>"), "𝔄"); // U+1D504
  string_eq (cork_to_utf8 ("<frak-z>"), "𝔷"); // U+1D577
  // Unregistered prefixes pass through verbatim (not entities).
  string_eq (cork_to_utf8 ("<i-A>"), "<i-A>");
  string_eq (cork_to_utf8 ("<sf-A>"), "<sf-A>");
  string_eq (cork_to_utf8 ("<tt-A>"), "<tt-A>");
}

// strict_cork_to_utf8 differs from cork_to_utf8 only in that it does not load
// the symbol-unicode-fallback table (8 long-arrow fallbacks). Everything else
// (Cork bytes, <#XXXX> escapes, other named entities, unknown entities) is
// identical.

TEST_CASE ("test_strict_cork_to_utf8_same_as_cork") {
  // Cork bytes decode identically.
  string_eq (strict_cork_to_utf8 (cork_byte (0x00)), "`");
  string_eq (strict_cork_to_utf8 (cork_byte (0x10)), "“");
  string_eq (strict_cork_to_utf8 (cork_byte (0xC0)), "À");
  // <#XXXX> escapes work the same.
  string_eq (strict_cork_to_utf8 ("<#4E2D>"), "中");
  string_eq (strict_cork_to_utf8 ("<#201C>"), "“");
  // Named entities (non-fallback) decode identically.
  string_eq (strict_cork_to_utf8 ("<less>"), "<");
  string_eq (strict_cork_to_utf8 ("<alpha>"), "α");
  string_eq (strict_cork_to_utf8 ("<infty>"), "∞");
  string_eq (strict_cork_to_utf8 ("<rightarrow>"), "→");
  // Unknown entities pass through verbatim, same as cork_to_utf8.
  string_eq (strict_cork_to_utf8 ("<foobar>"), "<foobar>");
  // Empty input.
  string_eq (strict_cork_to_utf8 (""), "");
}

TEST_CASE ("test_strict_cork_to_utf8_fallback_excluded") {
  // These 8 long-arrow fallbacks are decoded by cork_to_utf8 (which loads
  // symbol-unicode-fallback) but NOT by strict_cork_to_utf8, which passes
  // them through verbatim.
  string_eq (cork_to_utf8 ("<longuparrow>"), "↑");           // U+2191
  string_eq (cork_to_utf8 ("<longdownarrow>"), "↓");         // U+2193
  string_eq (cork_to_utf8 ("<longupdownarrow>"), "↕");       // U+2195
  string_eq (cork_to_utf8 ("<Longuparrow>"), "⇑");           // U+21D1
  string_eq (cork_to_utf8 ("<Longdownarrow>"), "⇓");         // U+21D3
  string_eq (cork_to_utf8 ("<Longupdownarrow>"), "⇕");       // U+21D5
  string_eq (cork_to_utf8 ("<longtwoheadleftarrow>"), "↞");  // U+219E
  string_eq (cork_to_utf8 ("<longtwoheadrightarrow>"), "↠"); // U+21A0

  string_eq (strict_cork_to_utf8 ("<longuparrow>"), "<longuparrow>");
  string_eq (strict_cork_to_utf8 ("<longdownarrow>"), "<longdownarrow>");
  string_eq (strict_cork_to_utf8 ("<longupdownarrow>"), "<longupdownarrow>");
  string_eq (strict_cork_to_utf8 ("<Longuparrow>"), "<Longuparrow>");
  string_eq (strict_cork_to_utf8 ("<Longdownarrow>"), "<Longdownarrow>");
  string_eq (strict_cork_to_utf8 ("<Longupdownarrow>"), "<Longupdownarrow>");
  string_eq (strict_cork_to_utf8 ("<longtwoheadleftarrow>"),
             "<longtwoheadleftarrow>");
  string_eq (strict_cork_to_utf8 ("<longtwoheadrightarrow>"),
             "<longtwoheadrightarrow>");
}
