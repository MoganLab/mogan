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

TEST_CASE ("herk_to_utf8_0x") {
  string_eq (herk_to_utf8 (herk_byte (0x00)), "`"); // U+0060
  string_eq (utf8_to_herk ("`"), herk_byte (0x00)); // U+0060
  string_eq (herk_to_utf8 (herk_byte (0x01)), "´"); // U+00B4
  string_eq (herk_to_utf8 (herk_byte (0x02)), "ˆ"); // U+02C6
  string_eq (herk_to_utf8 (herk_byte (0x03)), "˜"); // U+02DC
  string_eq (herk_to_utf8 (herk_byte (0x04)), "¨"); // U+00A8
  string_eq (herk_to_utf8 (herk_byte (0x05)), "˝"); // U+02DD
  string_eq (herk_to_utf8 (herk_byte (0x06)), "˚"); // U+02DA
  string_eq (herk_to_utf8 (herk_byte (0x07)), "ˇ"); // U+02C7
  string_eq (herk_to_utf8 (herk_byte (0x08)), "˘"); // U+02D8
  string_eq (herk_to_utf8 (herk_byte (0x09)), "¯"); // U+00AF
  string_eq (herk_to_utf8 (herk_byte (0x0A)), "˙"); // U+02D9
  string_eq (herk_to_utf8 (herk_byte (0x0B)), "¸"); // U+00B8
  string_eq (herk_to_utf8 (herk_byte (0x0C)), "˛"); // U+02DB
  string_eq (herk_to_utf8 (herk_byte (0x0D)), "‚"); // U+201A
  string_eq (herk_to_utf8 (herk_byte (0x0E)), "‹"); // U+2039
  string_eq (herk_to_utf8 (herk_byte (0x0F)), "›"); // U+203A
}

TEST_CASE ("herk_to_utf8_1x") {
  string_eq (herk_to_utf8 (herk_byte (0x10)), "“");            // U+201C
  string_eq (herk_to_utf8 (herk_byte (0x11)), "”");            // U+201D
  string_eq (herk_to_utf8 (herk_byte (0x12)), "„");            // U+201E
  string_eq (herk_to_utf8 (herk_byte (0x13)), "«");            // U+00AB
  string_eq (herk_to_utf8 (herk_byte (0x14)), "»");            // U+00BB
  string_eq (herk_to_utf8 (herk_byte (0x15)), "–");            // U+2013
  string_eq (herk_to_utf8 (herk_byte (0x16)), "—");            // U+2014
  string_eq (herk_to_utf8 (herk_byte (0x17)), "\xE2\x80\x8B"); // U+200B
  string_eq (herk_to_utf8 (herk_byte (0x18)), "₀");            // U+2080
  string_eq (herk_to_utf8 (herk_byte (0x19)), "ı");            // U+0131
  string_eq (herk_to_utf8 (herk_byte (0x1A)), "ȷ");            // U+0237
  string_eq (herk_to_utf8 (herk_byte (0x1B)), "ﬀ");            // U+FB00
  string_eq (herk_to_utf8 (herk_byte (0x1C)), "ﬁ");            // U+FB01
  string_eq (herk_to_utf8 (herk_byte (0x1D)), "ﬂ");            // U+FB02
  string_eq (herk_to_utf8 (herk_byte (0x1E)), "ﬃ");            // U+FB03
  string_eq (herk_to_utf8 (herk_byte (0x1F)), "ﬄ");            // U+FB04
}

TEST_CASE ("herk_to_utf8_2x") {
  string_eq (herk_to_utf8 (herk_byte (0x20)), " ");  // U+0020
  string_eq (herk_to_utf8 (herk_byte (0x21)), "!");  // U+0021
  string_eq (herk_to_utf8 (herk_byte (0x22)), "\""); // U+0022
  string_eq (herk_to_utf8 (herk_byte (0x23)), "#");  // U+0023
  string_eq (herk_to_utf8 (herk_byte (0x24)), "$");  // U+0024
  string_eq (herk_to_utf8 (herk_byte (0x25)), "%");  // U+0025
  string_eq (herk_to_utf8 (herk_byte (0x26)), "&");  // U+0026
  string_eq (herk_to_utf8 (herk_byte (0x27)), "'");  // U+0027
  string_eq (herk_to_utf8 (herk_byte (0x28)), "(");  // U+0028
  string_eq (herk_to_utf8 (herk_byte (0x29)), ")");  // U+0029
  string_eq (herk_to_utf8 (herk_byte (0x2A)), "*");  // U+002A
  string_eq (herk_to_utf8 (herk_byte (0x2B)), "+");  // U+002B
  string_eq (herk_to_utf8 (herk_byte (0x2C)), ",");  // U+002C
  string_eq (herk_to_utf8 (herk_byte (0x2D)), "-");  // U+002D
  string_eq (herk_to_utf8 (herk_byte (0x2E)), ".");  // U+002E
  string_eq (herk_to_utf8 (herk_byte (0x2F)), "/");  // U+002F
}

TEST_CASE ("herk_to_utf8_3x") {
  string_eq (herk_to_utf8 (herk_byte (0x30)), "0"); // U+0030
  string_eq (herk_to_utf8 (herk_byte (0x31)), "1"); // U+0031
  string_eq (herk_to_utf8 (herk_byte (0x32)), "2"); // U+0032
  string_eq (herk_to_utf8 (herk_byte (0x33)), "3"); // U+0033
  string_eq (herk_to_utf8 (herk_byte (0x34)), "4"); // U+0034
  string_eq (herk_to_utf8 (herk_byte (0x35)), "5"); // U+0035
  string_eq (herk_to_utf8 (herk_byte (0x36)), "6"); // U+0036
  string_eq (herk_to_utf8 (herk_byte (0x37)), "7"); // U+0037
  string_eq (herk_to_utf8 (herk_byte (0x38)), "8"); // U+0038
  string_eq (herk_to_utf8 (herk_byte (0x39)), "9"); // U+0039
  string_eq (herk_to_utf8 (herk_byte (0x3A)), ":"); // U+003A
  string_eq (herk_to_utf8 (herk_byte (0x3B)), ";"); // U+003B
  string_eq (herk_to_utf8 (herk_byte (0x3C)), "<"); // U+003C
  string_eq (herk_to_utf8 (herk_byte (0x3D)), "="); // U+003D
  string_eq (herk_to_utf8 (herk_byte (0x3E)), ">"); // U+003E
  string_eq (herk_to_utf8 (herk_byte (0x3F)), "?"); // U+003F
}

TEST_CASE ("herk_to_utf8_4x") {
  string_eq (herk_to_utf8 (herk_byte (0x40)), "@"); // U+0040
  string_eq (herk_to_utf8 (herk_byte (0x41)), "A"); // U+0041
  string_eq (herk_to_utf8 (herk_byte (0x42)), "B"); // U+0042
  string_eq (herk_to_utf8 (herk_byte (0x43)), "C"); // U+0043
  string_eq (herk_to_utf8 (herk_byte (0x44)), "D"); // U+0044
  string_eq (herk_to_utf8 (herk_byte (0x45)), "E"); // U+0045
  string_eq (herk_to_utf8 (herk_byte (0x46)), "F"); // U+0046
  string_eq (herk_to_utf8 (herk_byte (0x47)), "G"); // U+0047
  string_eq (herk_to_utf8 (herk_byte (0x48)), "H"); // U+0048
  string_eq (herk_to_utf8 (herk_byte (0x49)), "I"); // U+0049
  string_eq (herk_to_utf8 (herk_byte (0x4A)), "J"); // U+004A
  string_eq (herk_to_utf8 (herk_byte (0x4B)), "K"); // U+004B
  string_eq (herk_to_utf8 (herk_byte (0x4C)), "L"); // U+004C
  string_eq (herk_to_utf8 (herk_byte (0x4D)), "M"); // U+004D
  string_eq (herk_to_utf8 (herk_byte (0x4E)), "N"); // U+004E
  string_eq (herk_to_utf8 (herk_byte (0x4F)), "O"); // U+004F
}

TEST_CASE ("herk_to_utf8_5x") {
  string_eq (herk_to_utf8 (herk_byte (0x50)), "P");  // U+0050
  string_eq (herk_to_utf8 (herk_byte (0x51)), "Q");  // U+0051
  string_eq (herk_to_utf8 (herk_byte (0x52)), "R");  // U+0052
  string_eq (herk_to_utf8 (herk_byte (0x53)), "S");  // U+0053
  string_eq (herk_to_utf8 (herk_byte (0x54)), "T");  // U+0054
  string_eq (herk_to_utf8 (herk_byte (0x55)), "U");  // U+0055
  string_eq (herk_to_utf8 (herk_byte (0x56)), "V");  // U+0056
  string_eq (herk_to_utf8 (herk_byte (0x57)), "W");  // U+0057
  string_eq (herk_to_utf8 (herk_byte (0x58)), "X");  // U+0058
  string_eq (herk_to_utf8 (herk_byte (0x59)), "Y");  // U+0059
  string_eq (herk_to_utf8 (herk_byte (0x5A)), "Z");  // U+005A
  string_eq (herk_to_utf8 (herk_byte (0x5B)), "[");  // U+005B
  string_eq (herk_to_utf8 (herk_byte (0x5C)), "\\"); // U+005C
  string_eq (herk_to_utf8 (herk_byte (0x5D)), "]");  // U+005D
  string_eq (herk_to_utf8 (herk_byte (0x5E)), "^");  // U+005E
  string_eq (herk_to_utf8 (herk_byte (0x5F)), "_");  // U+005F
}

TEST_CASE ("herk_to_utf8_6x") {
  string_eq (herk_to_utf8 (herk_byte (0x60)), "‘"); // U+2018
  string_eq (herk_to_utf8 (herk_byte (0x61)), "a"); // U+0061
  string_eq (herk_to_utf8 (herk_byte (0x62)), "b"); // U+0062
  string_eq (herk_to_utf8 (herk_byte (0x63)), "c"); // U+0063
  string_eq (herk_to_utf8 (herk_byte (0x64)), "d"); // U+0064
  string_eq (herk_to_utf8 (herk_byte (0x65)), "e"); // U+0065
  string_eq (herk_to_utf8 (herk_byte (0x66)), "f"); // U+0066
  string_eq (herk_to_utf8 (herk_byte (0x67)), "g"); // U+0067
  string_eq (herk_to_utf8 (herk_byte (0x68)), "h"); // U+0068
  string_eq (herk_to_utf8 (herk_byte (0x69)), "i"); // U+0069
  string_eq (herk_to_utf8 (herk_byte (0x6A)), "j"); // U+006A
  string_eq (herk_to_utf8 (herk_byte (0x6B)), "k"); // U+006B
  string_eq (herk_to_utf8 (herk_byte (0x6C)), "l"); // U+006C
  string_eq (herk_to_utf8 (herk_byte (0x6D)), "m"); // U+006D
  string_eq (herk_to_utf8 (herk_byte (0x6E)), "n"); // U+006E
  string_eq (herk_to_utf8 (herk_byte (0x6F)), "o"); // U+006F
}

TEST_CASE ("herk_to_utf8_7x") {
  string_eq (herk_to_utf8 (herk_byte (0x70)), "p");        // U+0070
  string_eq (herk_to_utf8 (herk_byte (0x71)), "q");        // U+0071
  string_eq (herk_to_utf8 (herk_byte (0x72)), "r");        // U+0072
  string_eq (herk_to_utf8 (herk_byte (0x73)), "s");        // U+0073
  string_eq (herk_to_utf8 (herk_byte (0x74)), "t");        // U+0074
  string_eq (herk_to_utf8 (herk_byte (0x75)), "u");        // U+0075
  string_eq (herk_to_utf8 (herk_byte (0x76)), "v");        // U+0076
  string_eq (herk_to_utf8 (herk_byte (0x77)), "w");        // U+0077
  string_eq (herk_to_utf8 (herk_byte (0x78)), "x");        // U+0078
  string_eq (herk_to_utf8 (herk_byte (0x79)), "y");        // U+0079
  string_eq (herk_to_utf8 (herk_byte (0x7A)), "z");        // U+007A
  string_eq (herk_to_utf8 (herk_byte (0x7B)), "{");        // U+007B
  string_eq (herk_to_utf8 (herk_byte (0x7C)), "|");        // U+007C
  string_eq (herk_to_utf8 (herk_byte (0x7D)), "}");        // U+007D
  string_eq (herk_to_utf8 (herk_byte (0x7E)), "~");        // U+007E
  string_eq (herk_to_utf8 (herk_byte (0x7F)), "\xC2\xAD"); // U+00AD
}

TEST_CASE ("herk_to_utf8_8x") {
  string_eq (herk_to_utf8 (herk_byte (0x80)), "Ă"); // U+0102
  string_eq (herk_to_utf8 (herk_byte (0x81)), "Ą"); // U+0104
  string_eq (herk_to_utf8 (herk_byte (0x82)), "Ć"); // U+0106
  string_eq (herk_to_utf8 (herk_byte (0x83)), "Č"); // U+010C
  string_eq (herk_to_utf8 (herk_byte (0x84)), "Ď"); // U+010E
  string_eq (herk_to_utf8 (herk_byte (0x85)), "Ě"); // U+011A
  string_eq (herk_to_utf8 (herk_byte (0x86)), "Ę"); // U+0118
  string_eq (herk_to_utf8 (herk_byte (0x87)), "Ğ"); // U+011E
  string_eq (herk_to_utf8 (herk_byte (0x88)), "Ĺ"); // U+0139
  string_eq (herk_to_utf8 (herk_byte (0x89)), "Ľ"); // U+013D
  string_eq (herk_to_utf8 (herk_byte (0x8A)), "Ł"); // U+0141
  string_eq (herk_to_utf8 (herk_byte (0x8B)), "Ń"); // U+0143
  string_eq (herk_to_utf8 (herk_byte (0x8C)), "Ň"); // U+0147
  string_eq (herk_to_utf8 (herk_byte (0x8D)), "Ŋ"); // U+014A
  string_eq (herk_to_utf8 (herk_byte (0x8E)), "Ő"); // U+0150
  string_eq (herk_to_utf8 (herk_byte (0x8F)), "Ŕ"); // U+0154
}

TEST_CASE ("herk_to_utf8_9x") {
  string_eq (herk_to_utf8 (herk_byte (0x90)), "Ř"); // U+0158
  string_eq (herk_to_utf8 (herk_byte (0x91)), "Ś"); // U+015A
  string_eq (herk_to_utf8 (herk_byte (0x92)), "Š"); // U+0160
  string_eq (herk_to_utf8 (herk_byte (0x93)), "Ş"); // U+015E
  string_eq (herk_to_utf8 (herk_byte (0x94)), "Ť"); // U+0164
  string_eq (herk_to_utf8 (herk_byte (0x95)), "Ţ"); // U+0162
  string_eq (herk_to_utf8 (herk_byte (0x96)), "Ű"); // U+0170
  string_eq (herk_to_utf8 (herk_byte (0x97)), "Ů"); // U+016E
  string_eq (herk_to_utf8 (herk_byte (0x98)), "Ÿ"); // U+0178
  string_eq (herk_to_utf8 (herk_byte (0x99)), "Ź"); // U+0179
  string_eq (herk_to_utf8 (herk_byte (0x9A)), "Ž"); // U+017D
  string_eq (herk_to_utf8 (herk_byte (0x9B)), "Ż"); // U+017B
  string_eq (herk_to_utf8 (herk_byte (0x9C)), "Ĳ"); // U+0132
  string_eq (herk_to_utf8 (herk_byte (0x9D)), "İ"); // U+0130
  string_eq (herk_to_utf8 (herk_byte (0x9E)), "đ"); // U+0111
  string_eq (herk_to_utf8 (herk_byte (0x9F)), "§"); // U+00A7
}

TEST_CASE ("herk_to_utf8_Ax") {
  string_eq (herk_to_utf8 (herk_byte (0xA0)), "ă"); // U+0103
  string_eq (herk_to_utf8 (herk_byte (0xA1)), "ą"); // U+0105
  string_eq (herk_to_utf8 (herk_byte (0xA2)), "ć"); // U+0107
  string_eq (herk_to_utf8 (herk_byte (0xA3)), "č"); // U+010D
  string_eq (herk_to_utf8 (herk_byte (0xA4)), "ď"); // U+010F
  string_eq (herk_to_utf8 (herk_byte (0xA5)), "ě"); // U+011B
  string_eq (herk_to_utf8 (herk_byte (0xA6)), "ę"); // U+0119
  string_eq (herk_to_utf8 (herk_byte (0xA7)), "ğ"); // U+011F
  string_eq (herk_to_utf8 (herk_byte (0xA8)), "ĺ"); // U+013A
  string_eq (herk_to_utf8 (herk_byte (0xA9)), "ľ"); // U+013E
  string_eq (herk_to_utf8 (herk_byte (0xAA)), "ł"); // U+0142
  string_eq (herk_to_utf8 (herk_byte (0xAB)), "ń"); // U+0144
  string_eq (herk_to_utf8 (herk_byte (0xAC)), "ň"); // U+0148
  string_eq (herk_to_utf8 (herk_byte (0xAD)), "ŋ"); // U+014B
  string_eq (herk_to_utf8 (herk_byte (0xAE)), "ő"); // U+0151
  string_eq (herk_to_utf8 (herk_byte (0xAF)), "ŕ"); // U+0155
}

TEST_CASE ("herk_to_utf8_Bx") {
  string_eq (herk_to_utf8 (herk_byte (0xB0)), "ř"); // U+0159
  string_eq (herk_to_utf8 (herk_byte (0xB1)), "ś"); // U+015B
  string_eq (herk_to_utf8 (herk_byte (0xB2)), "š"); // U+0161
  string_eq (herk_to_utf8 (herk_byte (0xB3)), "ş"); // U+015F
  string_eq (herk_to_utf8 (herk_byte (0xB4)), "ť"); // U+0165
  string_eq (herk_to_utf8 (herk_byte (0xB5)), "ţ"); // U+0163
  string_eq (herk_to_utf8 (herk_byte (0xB6)), "ű"); // U+0171
  string_eq (herk_to_utf8 (herk_byte (0xB7)), "ů"); // U+016F
  string_eq (herk_to_utf8 (herk_byte (0xB8)), "ÿ"); // U+00FF
  string_eq (herk_to_utf8 (herk_byte (0xB9)), "ź"); // U+017A
  string_eq (herk_to_utf8 (herk_byte (0xBA)), "ž"); // U+017E
  string_eq (herk_to_utf8 (herk_byte (0xBB)), "ż"); // U+017C
  string_eq (herk_to_utf8 (herk_byte (0xBC)), "ĳ"); // U+0133
  string_eq (herk_to_utf8 (herk_byte (0xBD)), "¡"); // U+00A1
  string_eq (herk_to_utf8 (herk_byte (0xBE)), "¿"); // U+00BF
  string_eq (herk_to_utf8 (herk_byte (0xBF)), "£"); // U+00A3
}

TEST_CASE ("herk_to_utf8_Cx") {
  string_eq (herk_to_utf8 (herk_byte (0xC0)), "À"); // U+00C0
  string_eq (herk_to_utf8 (herk_byte (0xC1)), "Á"); // U+00C1
  string_eq (herk_to_utf8 (herk_byte (0xC2)), "Â"); // U+00C2
  string_eq (herk_to_utf8 (herk_byte (0xC3)), "Ã"); // U+00C3
  string_eq (herk_to_utf8 (herk_byte (0xC4)), "Ä"); // U+00C4
  string_eq (herk_to_utf8 (herk_byte (0xC5)), "Å"); // U+00C5
  string_eq (herk_to_utf8 (herk_byte (0xC6)), "Æ"); // U+00C6
  string_eq (herk_to_utf8 (herk_byte (0xC7)), "Ç"); // U+00C7
  string_eq (herk_to_utf8 (herk_byte (0xC8)), "È"); // U+00C8
  string_eq (herk_to_utf8 (herk_byte (0xC9)), "É"); // U+00C9
  string_eq (herk_to_utf8 (herk_byte (0xCA)), "Ê"); // U+00CA
  string_eq (herk_to_utf8 (herk_byte (0xCB)), "Ë"); // U+00CB
  string_eq (herk_to_utf8 (herk_byte (0xCC)), "Ì"); // U+00CC
  string_eq (herk_to_utf8 (herk_byte (0xCD)), "Í"); // U+00CD
  string_eq (herk_to_utf8 (herk_byte (0xCE)), "Î"); // U+00CE
  string_eq (herk_to_utf8 (herk_byte (0xCF)), "Ï"); // U+00CF
}

TEST_CASE ("herk_to_utf8_Dx") {
  string_eq (herk_to_utf8 (herk_byte (0xD0)), "Ð"); // U+00D0
  string_eq (herk_to_utf8 (herk_byte (0xD1)), "Ñ"); // U+00D1
  string_eq (herk_to_utf8 (herk_byte (0xD2)), "Ò"); // U+00D2
  string_eq (herk_to_utf8 (herk_byte (0xD3)), "Ó"); // U+00D3
  string_eq (herk_to_utf8 (herk_byte (0xD4)), "Ô"); // U+00D4
  string_eq (herk_to_utf8 (herk_byte (0xD5)), "Õ"); // U+00D5
  string_eq (herk_to_utf8 (herk_byte (0xD6)), "Ö"); // U+00D6
  string_eq (herk_to_utf8 (herk_byte (0xD7)), "Œ"); // U+0152
  string_eq (herk_to_utf8 (herk_byte (0xD8)), "Ø"); // U+00D8
  string_eq (herk_to_utf8 (herk_byte (0xD9)), "Ù"); // U+00D9
  string_eq (herk_to_utf8 (herk_byte (0xDA)), "Ú"); // U+00DA
  string_eq (herk_to_utf8 (herk_byte (0xDB)), "Û"); // U+00DB
  string_eq (herk_to_utf8 (herk_byte (0xDC)), "Ü"); // U+00DC
  string_eq (herk_to_utf8 (herk_byte (0xDD)), "Ý"); // U+00DD
  string_eq (herk_to_utf8 (herk_byte (0xDE)), "Þ"); // U+00DE
  string_eq (herk_to_utf8 (herk_byte (0xDF)), "ẞ"); // U+1E9E
}

TEST_CASE ("herk_to_utf8_Ex") {
  string_eq (herk_to_utf8 (herk_byte (0xE0)), "à"); // U+00E0
  string_eq (herk_to_utf8 (herk_byte (0xE1)), "á"); // U+00E1
  string_eq (herk_to_utf8 (herk_byte (0xE2)), "â"); // U+00E2
  string_eq (herk_to_utf8 (herk_byte (0xE3)), "ã"); // U+00E3
  string_eq (herk_to_utf8 (herk_byte (0xE4)), "ä"); // U+00E4
  string_eq (herk_to_utf8 (herk_byte (0xE5)), "å"); // U+00E5
  string_eq (herk_to_utf8 (herk_byte (0xE6)), "æ"); // U+00E6
  string_eq (herk_to_utf8 (herk_byte (0xE7)), "ç"); // U+00E7
  string_eq (herk_to_utf8 (herk_byte (0xE8)), "è"); // U+00E8
  string_eq (herk_to_utf8 (herk_byte (0xE9)), "é"); // U+00E9
  string_eq (herk_to_utf8 (herk_byte (0xEA)), "ê"); // U+00EA
  string_eq (herk_to_utf8 (herk_byte (0xEB)), "ë"); // U+00EB
  string_eq (herk_to_utf8 (herk_byte (0xEC)), "ì"); // U+00EC
  string_eq (herk_to_utf8 (herk_byte (0xED)), "í"); // U+00ED
  string_eq (herk_to_utf8 (herk_byte (0xEE)), "î"); // U+00EE
  string_eq (herk_to_utf8 (herk_byte (0xEF)), "ï"); // U+00EF
}

TEST_CASE ("herk_to_utf8_Fx") {
  string_eq (herk_to_utf8 (herk_byte (0xF0)), "ð"); // U+00F0
  string_eq (herk_to_utf8 (herk_byte (0xF1)), "ñ"); // U+00F1
  string_eq (herk_to_utf8 (herk_byte (0xF2)), "ò"); // U+00F2
  string_eq (herk_to_utf8 (herk_byte (0xF3)), "ó"); // U+00F3
  string_eq (herk_to_utf8 (herk_byte (0xF4)), "ô"); // U+00F4
  string_eq (herk_to_utf8 (herk_byte (0xF5)), "õ"); // U+00F5
  string_eq (herk_to_utf8 (herk_byte (0xF6)), "ö"); // U+00F6
  string_eq (herk_to_utf8 (herk_byte (0xF7)), "œ"); // U+0153
  string_eq (herk_to_utf8 (herk_byte (0xF8)), "ø"); // U+00F8
  string_eq (herk_to_utf8 (herk_byte (0xF9)), "ù"); // U+00F9
  string_eq (herk_to_utf8 (herk_byte (0xFA)), "ú"); // U+00FA
  string_eq (herk_to_utf8 (herk_byte (0xFB)), "û"); // U+00FB
  string_eq (herk_to_utf8 (herk_byte (0xFC)), "ü"); // U+00FC
  string_eq (herk_to_utf8 (herk_byte (0xFD)), "ý"); // U+00FD
  string_eq (herk_to_utf8 (herk_byte (0xFE)), "þ"); // U+00FE
  string_eq (herk_to_utf8 (herk_byte (0xFF)), "ß"); // U+00DF
}

TEST_CASE ("utf8_to_herk_0x") {
  string_eq (utf8_to_herk (herk_byte (0x00)), "<#00>"); // U+0000
  string_eq (utf8_to_herk (""), "<#01>");              // U+0001
  string_eq (utf8_to_herk (""), "<#02>");              // U+0002
  string_eq (utf8_to_herk (""), "<#03>");              // U+0003
  string_eq (utf8_to_herk (""), "<#04>");              // U+0004
  string_eq (utf8_to_herk (""), "<#05>");              // U+0005
  string_eq (utf8_to_herk (""), "<#06>");              // U+0006
  string_eq (utf8_to_herk (""), "<#07>");              // U+0007
  string_eq (utf8_to_herk (""), "<#08>");              // U+0008
  string_eq (utf8_to_herk ("	"), "<#09>");           // U+0009
  string_eq (utf8_to_herk ("\x0A"), "<#0A>");           // U+000A
  string_eq (utf8_to_herk (""), "<#0B>");              // U+000B
  string_eq (utf8_to_herk (""), "<#0C>");              // U+000C
  string_eq (utf8_to_herk ("\x0D"), "<#0D>");           // U+000D
  string_eq (utf8_to_herk (""), "<#0E>");              // U+000E
  string_eq (utf8_to_herk (""), "<#0F>");              // U+000F
}

TEST_CASE ("utf8_to_herk_1x") {
  string_eq (utf8_to_herk (""), "<#10>"); // U+0010
  string_eq (utf8_to_herk (""), "<#11>"); // U+0011
  string_eq (utf8_to_herk (""), "<#12>"); // U+0012
  string_eq (utf8_to_herk (""), "<#13>"); // U+0013
  string_eq (utf8_to_herk (""), "<#14>"); // U+0014
  string_eq (utf8_to_herk (""), "<#15>"); // U+0015
  string_eq (utf8_to_herk (""), "<#16>"); // U+0016
  string_eq (utf8_to_herk (""), "<#17>"); // U+0017
  string_eq (utf8_to_herk (""), "<#18>"); // U+0018
  string_eq (utf8_to_herk (""), "<#19>"); // U+0019
  string_eq (utf8_to_herk ("\x1A"), "<#1A>"); // U+001A
  string_eq (utf8_to_herk (""), "<#1B>"); // U+001B
  string_eq (utf8_to_herk (""), "<#1C>"); // U+001C
  string_eq (utf8_to_herk (""), "<#1D>"); // U+001D
  string_eq (utf8_to_herk (""), "<#1E>"); // U+001E
  string_eq (utf8_to_herk (""), "<#1F>"); // U+001F
}

TEST_CASE ("utf8_to_herk_2x") {
  string_eq (utf8_to_herk (" "), herk_byte (0x20));  // U+0020
  string_eq (utf8_to_herk ("!"), herk_byte (0x21));  // U+0021
  string_eq (utf8_to_herk ("\""), herk_byte (0x22)); // U+0022
  string_eq (utf8_to_herk ("#"), herk_byte (0x23));  // U+0023
  string_eq (utf8_to_herk ("$"), herk_byte (0x24));  // U+0024
  string_eq (utf8_to_herk ("%"), herk_byte (0x25));  // U+0025
  string_eq (utf8_to_herk ("&"), herk_byte (0x26));  // U+0026
  string_eq (utf8_to_herk ("'"), herk_byte (0x27));  // U+0027
  string_eq (utf8_to_herk ("("), herk_byte (0x28));  // U+0028
  string_eq (utf8_to_herk (")"), herk_byte (0x29));  // U+0029
  string_eq (utf8_to_herk ("*"), herk_byte (0x2A));  // U+002A
  string_eq (utf8_to_herk ("+"), herk_byte (0x2B));  // U+002B
  string_eq (utf8_to_herk (","), herk_byte (0x2C));  // U+002C
  string_eq (utf8_to_herk ("-"), herk_byte (0x2D));  // U+002D
  string_eq (utf8_to_herk ("."), herk_byte (0x2E));  // U+002E
  string_eq (utf8_to_herk ("/"), herk_byte (0x2F));  // U+002F
}

TEST_CASE ("utf8_to_herk_3x") {
  string_eq (utf8_to_herk ("0"), herk_byte (0x30)); // U+0030
  string_eq (utf8_to_herk ("1"), herk_byte (0x31)); // U+0031
  string_eq (utf8_to_herk ("2"), herk_byte (0x32)); // U+0032
  string_eq (utf8_to_herk ("3"), herk_byte (0x33)); // U+0033
  string_eq (utf8_to_herk ("4"), herk_byte (0x34)); // U+0034
  string_eq (utf8_to_herk ("5"), herk_byte (0x35)); // U+0035
  string_eq (utf8_to_herk ("6"), herk_byte (0x36)); // U+0036
  string_eq (utf8_to_herk ("7"), herk_byte (0x37)); // U+0037
  string_eq (utf8_to_herk ("8"), herk_byte (0x38)); // U+0038
  string_eq (utf8_to_herk ("9"), herk_byte (0x39)); // U+0039
  string_eq (utf8_to_herk (":"), herk_byte (0x3A)); // U+003A
  string_eq (utf8_to_herk (";"), herk_byte (0x3B)); // U+003B
  string_eq (utf8_to_herk ("<"), herk_byte (0x3C)); // U+003C
  string_eq (utf8_to_herk ("="), herk_byte (0x3D)); // U+003D
  string_eq (utf8_to_herk (">"), herk_byte (0x3E)); // U+003E
  string_eq (utf8_to_herk ("?"), herk_byte (0x3F)); // U+003F
}

TEST_CASE ("utf8_to_herk_4x") {
  string_eq (utf8_to_herk ("@"), herk_byte (0x40)); // U+0040
  string_eq (utf8_to_herk ("A"), herk_byte (0x41)); // U+0041
  string_eq (utf8_to_herk ("B"), herk_byte (0x42)); // U+0042
  string_eq (utf8_to_herk ("C"), herk_byte (0x43)); // U+0043
  string_eq (utf8_to_herk ("D"), herk_byte (0x44)); // U+0044
  string_eq (utf8_to_herk ("E"), herk_byte (0x45)); // U+0045
  string_eq (utf8_to_herk ("F"), herk_byte (0x46)); // U+0046
  string_eq (utf8_to_herk ("G"), herk_byte (0x47)); // U+0047
  string_eq (utf8_to_herk ("H"), herk_byte (0x48)); // U+0048
  string_eq (utf8_to_herk ("I"), herk_byte (0x49)); // U+0049
  string_eq (utf8_to_herk ("J"), herk_byte (0x4A)); // U+004A
  string_eq (utf8_to_herk ("K"), herk_byte (0x4B)); // U+004B
  string_eq (utf8_to_herk ("L"), herk_byte (0x4C)); // U+004C
  string_eq (utf8_to_herk ("M"), herk_byte (0x4D)); // U+004D
  string_eq (utf8_to_herk ("N"), herk_byte (0x4E)); // U+004E
  string_eq (utf8_to_herk ("O"), herk_byte (0x4F)); // U+004F
}

TEST_CASE ("utf8_to_herk_5x") {
  string_eq (utf8_to_herk ("P"), herk_byte (0x50));  // U+0050
  string_eq (utf8_to_herk ("Q"), herk_byte (0x51));  // U+0051
  string_eq (utf8_to_herk ("R"), herk_byte (0x52));  // U+0052
  string_eq (utf8_to_herk ("S"), herk_byte (0x53));  // U+0053
  string_eq (utf8_to_herk ("T"), herk_byte (0x54));  // U+0054
  string_eq (utf8_to_herk ("U"), herk_byte (0x55));  // U+0055
  string_eq (utf8_to_herk ("V"), herk_byte (0x56));  // U+0056
  string_eq (utf8_to_herk ("W"), herk_byte (0x57));  // U+0057
  string_eq (utf8_to_herk ("X"), herk_byte (0x58));  // U+0058
  string_eq (utf8_to_herk ("Y"), herk_byte (0x59));  // U+0059
  string_eq (utf8_to_herk ("Z"), herk_byte (0x5A));  // U+005A
  string_eq (utf8_to_herk ("["), herk_byte (0x5B));  // U+005B
  string_eq (utf8_to_herk ("\\"), herk_byte (0x5C)); // U+005C
  string_eq (utf8_to_herk ("]"), herk_byte (0x5D));  // U+005D
  string_eq (utf8_to_herk ("^"), herk_byte (0x5E));  // U+005E
  string_eq (utf8_to_herk ("_"), herk_byte (0x5F));  // U+005F
}

TEST_CASE ("utf8_to_herk_6x") {
  string_eq (utf8_to_herk ("`"), herk_byte (0x00)); // U+0060
  string_eq (utf8_to_herk ("a"), herk_byte (0x61)); // U+0061
  string_eq (utf8_to_herk ("b"), herk_byte (0x62)); // U+0062
  string_eq (utf8_to_herk ("c"), herk_byte (0x63)); // U+0063
  string_eq (utf8_to_herk ("d"), herk_byte (0x64)); // U+0064
  string_eq (utf8_to_herk ("e"), herk_byte (0x65)); // U+0065
  string_eq (utf8_to_herk ("f"), herk_byte (0x66)); // U+0066
  string_eq (utf8_to_herk ("g"), herk_byte (0x67)); // U+0067
  string_eq (utf8_to_herk ("h"), herk_byte (0x68)); // U+0068
  string_eq (utf8_to_herk ("i"), herk_byte (0x69)); // U+0069
  string_eq (utf8_to_herk ("j"), herk_byte (0x6A)); // U+006A
  string_eq (utf8_to_herk ("k"), herk_byte (0x6B)); // U+006B
  string_eq (utf8_to_herk ("l"), herk_byte (0x6C)); // U+006C
  string_eq (utf8_to_herk ("m"), herk_byte (0x6D)); // U+006D
  string_eq (utf8_to_herk ("n"), herk_byte (0x6E)); // U+006E
  string_eq (utf8_to_herk ("o"), herk_byte (0x6F)); // U+006F
}

TEST_CASE ("utf8_to_herk_7x") {
  string_eq (utf8_to_herk ("p"), herk_byte (0x70)); // U+0070
  string_eq (utf8_to_herk ("q"), herk_byte (0x71)); // U+0071
  string_eq (utf8_to_herk ("r"), herk_byte (0x72)); // U+0072
  string_eq (utf8_to_herk ("s"), herk_byte (0x73)); // U+0073
  string_eq (utf8_to_herk ("t"), herk_byte (0x74)); // U+0074
  string_eq (utf8_to_herk ("u"), herk_byte (0x75)); // U+0075
  string_eq (utf8_to_herk ("v"), herk_byte (0x76)); // U+0076
  string_eq (utf8_to_herk ("w"), herk_byte (0x77)); // U+0077
  string_eq (utf8_to_herk ("x"), herk_byte (0x78)); // U+0078
  string_eq (utf8_to_herk ("y"), herk_byte (0x79)); // U+0079
  string_eq (utf8_to_herk ("z"), herk_byte (0x7A)); // U+007A
  string_eq (utf8_to_herk ("{"), herk_byte (0x7B)); // U+007B
  string_eq (utf8_to_herk ("|"), herk_byte (0x7C)); // U+007C
  string_eq (utf8_to_herk ("}"), herk_byte (0x7D)); // U+007D
  string_eq (utf8_to_herk ("~"), herk_byte (0x7E)); // U+007E
  string_eq (utf8_to_herk ("\x7F"), "\x7F");        // U+007F unmapped
}

TEST_CASE ("utf8_to_herk_8x") {
  string_eq (utf8_to_herk ("\xC2\x80"), "<#80>"); // U+0080 unmapped
  string_eq (utf8_to_herk ("\xC2\x81"), "<#81>"); // U+0081 unmapped
  string_eq (utf8_to_herk ("\xC2\x82"), "<#82>"); // U+0082 unmapped
  string_eq (utf8_to_herk ("\xC2\x83"), "<#83>"); // U+0083 unmapped
  string_eq (utf8_to_herk ("\xC2\x84"), "<#84>"); // U+0084 unmapped
  string_eq (utf8_to_herk ("\xC2\x85"), "<#85>"); // U+0085 unmapped
  string_eq (utf8_to_herk ("\xC2\x86"), "<#86>"); // U+0086 unmapped
  string_eq (utf8_to_herk ("\xC2\x87"), "<#87>"); // U+0087 unmapped
  string_eq (utf8_to_herk ("\xC2\x88"), "<#88>"); // U+0088 unmapped
  string_eq (utf8_to_herk ("\xC2\x89"), "<#89>"); // U+0089 unmapped
  string_eq (utf8_to_herk ("\xC2\x8A"), "<#8A>"); // U+008A unmapped
  string_eq (utf8_to_herk ("\xC2\x8B"), "<#8B>"); // U+008B unmapped
  string_eq (utf8_to_herk ("\xC2\x8C"), "<#8C>"); // U+008C unmapped
  string_eq (utf8_to_herk ("\xC2\x8D"), "<#8D>"); // U+008D unmapped
  string_eq (utf8_to_herk ("\xC2\x8E"), "<#8E>"); // U+008E unmapped
  string_eq (utf8_to_herk ("\xC2\x8F"), "<#8F>"); // U+008F unmapped
}

TEST_CASE ("utf8_to_herk_9x") {
  string_eq (utf8_to_herk ("\xC2\x90"), "<#90>"); // U+0090 unmapped
  string_eq (utf8_to_herk ("\xC2\x91"), "<#91>"); // U+0091 unmapped
  string_eq (utf8_to_herk ("\xC2\x92"), "<#92>"); // U+0092 unmapped
  string_eq (utf8_to_herk ("\xC2\x93"), "<#93>"); // U+0093 unmapped
  string_eq (utf8_to_herk ("\xC2\x94"), "<#94>"); // U+0094 unmapped
  string_eq (utf8_to_herk ("\xC2\x95"), "<#95>"); // U+0095 unmapped
  string_eq (utf8_to_herk ("\xC2\x96"), "<#96>"); // U+0096 unmapped
  string_eq (utf8_to_herk ("\xC2\x97"), "<#97>"); // U+0097 unmapped
  string_eq (utf8_to_herk ("\xC2\x98"), "<#98>"); // U+0098 unmapped
  string_eq (utf8_to_herk ("\xC2\x99"), "<#99>"); // U+0099 unmapped
  string_eq (utf8_to_herk ("\xC2\x9A"), "<#9A>"); // U+009A unmapped
  string_eq (utf8_to_herk ("\xC2\x9B"), "<#9B>"); // U+009B unmapped
  string_eq (utf8_to_herk ("\xC2\x9C"), "<#9C>"); // U+009C unmapped
  string_eq (utf8_to_herk ("\xC2\x9D"), "<#9D>"); // U+009D unmapped
  string_eq (utf8_to_herk ("\xC2\x9E"), "<#9E>"); // U+009E unmapped
  string_eq (utf8_to_herk ("\xC2\x9F"), "<#9F>"); // U+009F unmapped
}

TEST_CASE ("utf8_to_herk_Ax") {
  string_eq (utf8_to_herk ("\xC2\xA0"), "<#A0>");          // U+00A0 unmapped
  string_eq (utf8_to_herk ("\xC2\xA1"), herk_byte (0xBD)); // U+00A1
  string_eq (utf8_to_herk ("\xC2\xA2"), "<#A2>");          // U+00A2 unmapped
  string_eq (utf8_to_herk ("\xC2\xA3"), herk_byte (0xBF)); // U+00A3
  string_eq (utf8_to_herk ("\xC2\xA4"), "<#A4>");          // U+00A4 unmapped
  string_eq (utf8_to_herk ("\xC2\xA5"), "<#A5>");          // U+00A5 unmapped
  string_eq (utf8_to_herk ("\xC2\xA6"), "<#A6>");          // U+00A6 unmapped
  string_eq (utf8_to_herk ("\xC2\xA7"), herk_byte (0x9F)); // U+00A7
  string_eq (utf8_to_herk ("\xC2\xA8"), herk_byte (0x04)); // U+00A8
  string_eq (utf8_to_herk ("\xC2\xA9"), "<#A9>");          // U+00A9 unmapped
  string_eq (utf8_to_herk ("\xC2\xAA"), "<#AA>");          // U+00AA unmapped
  string_eq (utf8_to_herk ("\xC2\xAB"), herk_byte (0x13)); // U+00AB
  string_eq (utf8_to_herk ("\xC2\xAC"), "<#AC>");          // U+00AC unmapped
  string_eq (utf8_to_herk ("\xC2\xAD"), herk_byte (0x7F)); // U+00AD
  string_eq (utf8_to_herk ("\xC2\xAE"), "<#AE>");          // U+00AE unmapped
  string_eq (utf8_to_herk ("\xC2\xAF"), herk_byte (0x09)); // U+00AF
}

TEST_CASE ("utf8_to_herk_Bx") {
  string_eq (utf8_to_herk ("\xC2\xB0"), "<#B0>");          // U+00B0 unmapped
  string_eq (utf8_to_herk ("\xC2\xB1"), "<#B1>");          // U+00B1 unmapped
  string_eq (utf8_to_herk ("\xC2\xB2"), "<#B2>");          // U+00B2 unmapped
  string_eq (utf8_to_herk ("\xC2\xB3"), "<#B3>");          // U+00B3 unmapped
  string_eq (utf8_to_herk ("\xC2\xB4"), herk_byte (0x01)); // U+00B4
  string_eq (utf8_to_herk ("\xC2\xB5"), "<#B5>");          // U+00B5 unmapped
  string_eq (utf8_to_herk ("\xC2\xB6"), "<#B6>");          // U+00B6 unmapped
  string_eq (utf8_to_herk ("\xC2\xB7"), "<#B7>");          // U+00B7 unmapped
  string_eq (utf8_to_herk ("\xC2\xB8"), herk_byte (0x0B)); // U+00B8
  string_eq (utf8_to_herk ("\xC2\xB9"), "<#B9>");          // U+00B9 unmapped
  string_eq (utf8_to_herk ("\xC2\xBA"), "<#BA>");          // U+00BA unmapped
  string_eq (utf8_to_herk ("\xC2\xBB"), herk_byte (0x14)); // U+00BB
  string_eq (utf8_to_herk ("\xC2\xBC"), "<#BC>");          // U+00BC unmapped
  string_eq (utf8_to_herk ("\xC2\xBD"), "<#BD>");          // U+00BD unmapped
  string_eq (utf8_to_herk ("\xC2\xBE"), "<#BE>");          // U+00BE unmapped
  string_eq (utf8_to_herk ("\xC2\xBF"), herk_byte (0xBE)); // U+00BF
}

TEST_CASE ("utf8_to_herk_Cx") {
  string_eq (utf8_to_herk ("\xC3\x80"), herk_byte (0xC0)); // U+00C0
  string_eq (utf8_to_herk ("\xC3\x81"), herk_byte (0xC1)); // U+00C1
  string_eq (utf8_to_herk ("\xC3\x82"), herk_byte (0xC2)); // U+00C2
  string_eq (utf8_to_herk ("\xC3\x83"), herk_byte (0xC3)); // U+00C3
  string_eq (utf8_to_herk ("\xC3\x84"), herk_byte (0xC4)); // U+00C4
  string_eq (utf8_to_herk ("\xC3\x85"), herk_byte (0xC5)); // U+00C5
  string_eq (utf8_to_herk ("\xC3\x86"), herk_byte (0xC6)); // U+00C6
  string_eq (utf8_to_herk ("\xC3\x87"), herk_byte (0xC7)); // U+00C7
  string_eq (utf8_to_herk ("\xC3\x88"), herk_byte (0xC8)); // U+00C8
  string_eq (utf8_to_herk ("\xC3\x89"), herk_byte (0xC9)); // U+00C9
  string_eq (utf8_to_herk ("\xC3\x8A"), herk_byte (0xCA)); // U+00CA
  string_eq (utf8_to_herk ("\xC3\x8B"), herk_byte (0xCB)); // U+00CB
  string_eq (utf8_to_herk ("\xC3\x8C"), herk_byte (0xCC)); // U+00CC
  string_eq (utf8_to_herk ("\xC3\x8D"), herk_byte (0xCD)); // U+00CD
  string_eq (utf8_to_herk ("\xC3\x8E"), herk_byte (0xCE)); // U+00CE
  string_eq (utf8_to_herk ("\xC3\x8F"), herk_byte (0xCF)); // U+00CF
}

TEST_CASE ("utf8_to_herk_Dx") {
  string_eq (utf8_to_herk ("\xC3\x90"), herk_byte (0xD0)); // U+00D0
  string_eq (utf8_to_herk ("\xC3\x91"), herk_byte (0xD1)); // U+00D1
  string_eq (utf8_to_herk ("\xC3\x92"), herk_byte (0xD2)); // U+00D2
  string_eq (utf8_to_herk ("\xC3\x93"), herk_byte (0xD3)); // U+00D3
  string_eq (utf8_to_herk ("\xC3\x94"), herk_byte (0xD4)); // U+00D4
  string_eq (utf8_to_herk ("\xC3\x95"), herk_byte (0xD5)); // U+00D5
  string_eq (utf8_to_herk ("\xC3\x96"), herk_byte (0xD6)); // U+00D6
  string_eq (utf8_to_herk ("\xC3\x97"), "<#D7>");          // U+00D7 unmapped
  string_eq (utf8_to_herk ("\xC3\x98"), herk_byte (0xD8)); // U+00D8
  string_eq (utf8_to_herk ("\xC3\x99"), herk_byte (0xD9)); // U+00D9
  string_eq (utf8_to_herk ("\xC3\x9A"), herk_byte (0xDA)); // U+00DA
  string_eq (utf8_to_herk ("\xC3\x9B"), herk_byte (0xDB)); // U+00DB
  string_eq (utf8_to_herk ("\xC3\x9C"), herk_byte (0xDC)); // U+00DC
  string_eq (utf8_to_herk ("\xC3\x9D"), herk_byte (0xDD)); // U+00DD
  string_eq (utf8_to_herk ("\xC3\x9E"), herk_byte (0xDE)); // U+00DE
  string_eq (utf8_to_herk ("\xC3\x9F"), herk_byte (0xFF)); // U+00DF
}

TEST_CASE ("utf8_to_herk_Ex") {
  string_eq (utf8_to_herk ("\xC3\xA0"), herk_byte (0xE0)); // U+00E0
  string_eq (utf8_to_herk ("\xC3\xA1"), herk_byte (0xE1)); // U+00E1
  string_eq (utf8_to_herk ("\xC3\xA2"), herk_byte (0xE2)); // U+00E2
  string_eq (utf8_to_herk ("\xC3\xA3"), herk_byte (0xE3)); // U+00E3
  string_eq (utf8_to_herk ("\xC3\xA4"), herk_byte (0xE4)); // U+00E4
  string_eq (utf8_to_herk ("\xC3\xA5"), herk_byte (0xE5)); // U+00E5
  string_eq (utf8_to_herk ("\xC3\xA6"), herk_byte (0xE6)); // U+00E6
  string_eq (utf8_to_herk ("\xC3\xA7"), herk_byte (0xE7)); // U+00E7
  string_eq (utf8_to_herk ("\xC3\xA8"), herk_byte (0xE8)); // U+00E8
  string_eq (utf8_to_herk ("\xC3\xA9"), herk_byte (0xE9)); // U+00E9
  string_eq (utf8_to_herk ("\xC3\xAA"), herk_byte (0xEA)); // U+00EA
  string_eq (utf8_to_herk ("\xC3\xAB"), herk_byte (0xEB)); // U+00EB
  string_eq (utf8_to_herk ("\xC3\xAC"), herk_byte (0xEC)); // U+00EC
  string_eq (utf8_to_herk ("\xC3\xAD"), herk_byte (0xED)); // U+00ED
  string_eq (utf8_to_herk ("\xC3\xAE"), herk_byte (0xEE)); // U+00EE
  string_eq (utf8_to_herk ("\xC3\xAF"), herk_byte (0xEF)); // U+00EF
}

TEST_CASE ("utf8_to_herk_Fx") {
  string_eq (utf8_to_herk ("\xC3\xB0"), herk_byte (0xF0)); // U+00F0
  string_eq (utf8_to_herk ("\xC3\xB1"), herk_byte (0xF1)); // U+00F1
  string_eq (utf8_to_herk ("\xC3\xB2"), herk_byte (0xF2)); // U+00F2
  string_eq (utf8_to_herk ("\xC3\xB3"), herk_byte (0xF3)); // U+00F3
  string_eq (utf8_to_herk ("\xC3\xB4"), herk_byte (0xF4)); // U+00F4
  string_eq (utf8_to_herk ("\xC3\xB5"), herk_byte (0xF5)); // U+00F5
  string_eq (utf8_to_herk ("\xC3\xB6"), herk_byte (0xF6)); // U+00F6
  string_eq (utf8_to_herk ("\xC3\xB7"), "<#F7>");          // U+00F7 unmapped
  string_eq (utf8_to_herk ("\xC3\xB8"), herk_byte (0xF8)); // U+00F8
  string_eq (utf8_to_herk ("\xC3\xB9"), herk_byte (0xF9)); // U+00F9
  string_eq (utf8_to_herk ("\xC3\xBA"), herk_byte (0xFA)); // U+00FA
  string_eq (utf8_to_herk ("\xC3\xBB"), herk_byte (0xFB)); // U+00FB
  string_eq (utf8_to_herk ("\xC3\xBC"), herk_byte (0xFC)); // U+00FC
  string_eq (utf8_to_herk ("\xC3\xBD"), herk_byte (0xFD)); // U+00FD
  string_eq (utf8_to_herk ("\xC3\xBE"), herk_byte (0xFE)); // U+00FE
  string_eq (utf8_to_herk ("\xC3\xBF"), herk_byte (0xB8)); // U+00FF
}

TEST_CASE ("utf8_to_herk_high_mapped") {
  string_eq (utf8_to_herk ("\xC4\x82"), herk_byte (0x80));     // U+0102
  string_eq (utf8_to_herk ("\xC4\x83"), herk_byte (0xA0));     // U+0103
  string_eq (utf8_to_herk ("\xC4\x84"), herk_byte (0x81));     // U+0104
  string_eq (utf8_to_herk ("\xC4\x85"), herk_byte (0xA1));     // U+0105
  string_eq (utf8_to_herk ("\xC4\x86"), herk_byte (0x82));     // U+0106
  string_eq (utf8_to_herk ("\xC4\x87"), herk_byte (0xA2));     // U+0107
  string_eq (utf8_to_herk ("\xC4\x8C"), herk_byte (0x83));     // U+010C
  string_eq (utf8_to_herk ("\xC4\x8D"), herk_byte (0xA3));     // U+010D
  string_eq (utf8_to_herk ("\xC4\x8E"), herk_byte (0x84));     // U+010E
  string_eq (utf8_to_herk ("\xC4\x8F"), herk_byte (0xA4));     // U+010F
  string_eq (utf8_to_herk ("\xC4\x91"), herk_byte (0x9E));     // U+0111
  string_eq (utf8_to_herk ("\xC4\x98"), herk_byte (0x86));     // U+0118
  string_eq (utf8_to_herk ("\xC4\x99"), herk_byte (0xA6));     // U+0119
  string_eq (utf8_to_herk ("\xC4\x9A"), herk_byte (0x85));     // U+011A
  string_eq (utf8_to_herk ("\xC4\x9B"), herk_byte (0xA5));     // U+011B
  string_eq (utf8_to_herk ("\xC4\x9E"), herk_byte (0x87));     // U+011E
  string_eq (utf8_to_herk ("\xC4\x9F"), herk_byte (0xA7));     // U+011F
  string_eq (utf8_to_herk ("\xC4\xB0"), herk_byte (0x9D));     // U+0130
  string_eq (utf8_to_herk ("\xC4\xB1"), herk_byte (0x19));     // U+0131
  string_eq (utf8_to_herk ("\xC4\xB2"), herk_byte (0x9C));     // U+0132
  string_eq (utf8_to_herk ("\xC4\xB3"), herk_byte (0xBC));     // U+0133
  string_eq (utf8_to_herk ("\xC4\xB9"), herk_byte (0x88));     // U+0139
  string_eq (utf8_to_herk ("\xC4\xBA"), herk_byte (0xA8));     // U+013A
  string_eq (utf8_to_herk ("\xC4\xBD"), herk_byte (0x89));     // U+013D
  string_eq (utf8_to_herk ("\xC4\xBE"), herk_byte (0xA9));     // U+013E
  string_eq (utf8_to_herk ("\xC5\x81"), herk_byte (0x8A));     // U+0141
  string_eq (utf8_to_herk ("\xC5\x82"), herk_byte (0xAA));     // U+0142
  string_eq (utf8_to_herk ("\xC5\x83"), herk_byte (0x8B));     // U+0143
  string_eq (utf8_to_herk ("\xC5\x84"), herk_byte (0xAB));     // U+0144
  string_eq (utf8_to_herk ("\xC5\x87"), herk_byte (0x8C));     // U+0147
  string_eq (utf8_to_herk ("\xC5\x88"), herk_byte (0xAC));     // U+0148
  string_eq (utf8_to_herk ("\xC5\x8A"), herk_byte (0x8D));     // U+014A
  string_eq (utf8_to_herk ("\xC5\x8B"), herk_byte (0xAD));     // U+014B
  string_eq (utf8_to_herk ("\xC5\x90"), herk_byte (0x8E));     // U+0150
  string_eq (utf8_to_herk ("\xC5\x91"), herk_byte (0xAE));     // U+0151
  string_eq (utf8_to_herk ("\xC5\x92"), herk_byte (0xD7));     // U+0152
  string_eq (utf8_to_herk ("\xC5\x93"), herk_byte (0xF7));     // U+0153
  string_eq (utf8_to_herk ("\xC5\x94"), herk_byte (0x8F));     // U+0154
  string_eq (utf8_to_herk ("\xC5\x95"), herk_byte (0xAF));     // U+0155
  string_eq (utf8_to_herk ("\xC5\x98"), herk_byte (0x90));     // U+0158
  string_eq (utf8_to_herk ("\xC5\x99"), herk_byte (0xB0));     // U+0159
  string_eq (utf8_to_herk ("\xC5\x9A"), herk_byte (0x91));     // U+015A
  string_eq (utf8_to_herk ("\xC5\x9B"), herk_byte (0xB1));     // U+015B
  string_eq (utf8_to_herk ("\xC5\x9E"), herk_byte (0x93));     // U+015E
  string_eq (utf8_to_herk ("\xC5\x9F"), herk_byte (0xB3));     // U+015F
  string_eq (utf8_to_herk ("\xC5\xA0"), herk_byte (0x92));     // U+0160
  string_eq (utf8_to_herk ("\xC5\xA1"), herk_byte (0xB2));     // U+0161
  string_eq (utf8_to_herk ("\xC5\xA2"), herk_byte (0x95));     // U+0162
  string_eq (utf8_to_herk ("\xC5\xA3"), herk_byte (0xB5));     // U+0163
  string_eq (utf8_to_herk ("\xC5\xA4"), herk_byte (0x94));     // U+0164
  string_eq (utf8_to_herk ("\xC5\xA5"), herk_byte (0xB4));     // U+0165
  string_eq (utf8_to_herk ("\xC5\xAE"), herk_byte (0x97));     // U+016E
  string_eq (utf8_to_herk ("\xC5\xAF"), herk_byte (0xB7));     // U+016F
  string_eq (utf8_to_herk ("\xC5\xB0"), herk_byte (0x96));     // U+0170
  string_eq (utf8_to_herk ("\xC5\xB1"), herk_byte (0xB6));     // U+0171
  string_eq (utf8_to_herk ("\xC5\xB8"), herk_byte (0x98));     // U+0178
  string_eq (utf8_to_herk ("\xC5\xB9"), herk_byte (0x99));     // U+0179
  string_eq (utf8_to_herk ("\xC5\xBA"), herk_byte (0xB9));     // U+017A
  string_eq (utf8_to_herk ("\xC5\xBB"), herk_byte (0x9B));     // U+017B
  string_eq (utf8_to_herk ("\xC5\xBC"), herk_byte (0xBB));     // U+017C
  string_eq (utf8_to_herk ("\xC5\xBD"), herk_byte (0x9A));     // U+017D
  string_eq (utf8_to_herk ("\xC5\xBE"), herk_byte (0xBA));     // U+017E
  string_eq (utf8_to_herk ("\xC8\xB7"), herk_byte (0x1A));     // U+0237
  string_eq (utf8_to_herk ("\xCB\x86"), herk_byte (0x02));     // U+02C6
  string_eq (utf8_to_herk ("\xCB\x87"), herk_byte (0x07));     // U+02C7
  string_eq (utf8_to_herk ("\xCB\x98"), herk_byte (0x08));     // U+02D8
  string_eq (utf8_to_herk ("\xCB\x99"), herk_byte (0x0A));     // U+02D9
  string_eq (utf8_to_herk ("\xCB\x9A"), herk_byte (0x06));     // U+02DA
  string_eq (utf8_to_herk ("\xCB\x9B"), herk_byte (0x0C));     // U+02DB
  string_eq (utf8_to_herk ("\xCB\x9C"), herk_byte (0x03));     // U+02DC
  string_eq (utf8_to_herk ("\xCB\x9D"), herk_byte (0x05));     // U+02DD
  string_eq (utf8_to_herk ("\xE1\xBA\x9E"), herk_byte (0xDF)); // U+1E9E
  string_eq (utf8_to_herk ("\xE2\x80\x8B"), herk_byte (0x17)); // U+200B
  string_eq (utf8_to_herk ("\xE2\x80\x93"), herk_byte (0x15)); // U+2013
  string_eq (utf8_to_herk ("\xE2\x80\x94"), herk_byte (0x16)); // U+2014
  string_eq (utf8_to_herk ("\xE2\x80\x98"), herk_byte (0x60)); // U+2018
  string_eq (utf8_to_herk ("\xE2\x80\x9A"), herk_byte (0x0D)); // U+201A
  string_eq (utf8_to_herk ("\xE2\x80\x9C"), herk_byte (0x10)); // U+201C
  string_eq (utf8_to_herk ("\xE2\x80\x9D"), herk_byte (0x11)); // U+201D
  string_eq (utf8_to_herk ("\xE2\x80\x9E"), herk_byte (0x12)); // U+201E
  string_eq (utf8_to_herk ("\xE2\x80\xB9"), herk_byte (0x0E)); // U+2039
  string_eq (utf8_to_herk ("\xE2\x80\xBA"), herk_byte (0x0F)); // U+203A
  string_eq (utf8_to_herk ("\xE2\x82\x80"), herk_byte (0x18)); // U+2080
  string_eq (utf8_to_herk ("\xEF\xAC\x80"), herk_byte (0x1B)); // U+FB00
  string_eq (utf8_to_herk ("\xEF\xAC\x81"), herk_byte (0x1C)); // U+FB01
  string_eq (utf8_to_herk ("\xEF\xAC\x82"), herk_byte (0x1D)); // U+FB02
  string_eq (utf8_to_herk ("\xEF\xAC\x83"), herk_byte (0x1E)); // U+FB03
  string_eq (utf8_to_herk ("\xEF\xAC\x84"), herk_byte (0x1F)); // U+FB04
}

TEST_CASE ("utf8_to_herk_high_unmapped") {
  string_eq (utf8_to_herk ("\xC4\x80"), "<#100>");      // U+0100
  string_eq (utf8_to_herk ("\xC4\x90"), "<#110>");      // U+0110
  string_eq (utf8_to_herk ("\xC4\xA0"), "<#120>");      // U+0120
  string_eq (utf8_to_herk ("\xC8\x80"), "<#200>");      // U+0200
  string_eq (utf8_to_herk ("\xE2\x80\x99"), "<#2019>"); // U+2019
  string_eq (utf8_to_herk ("\xE3\x80\x82"), "<#3002>"); // U+3002
  string_eq (utf8_to_herk ("\xE4\xB8\xAD"), "<#4E2D>"); // U+4E2D
}

TEST_CASE ("herk_escapes") {
  string_eq (herk_to_utf8 ("<#00>"), herk_byte (0x00));
  string_eq (herk_to_utf8 ("<#0F>"), herk_byte (0x0F));
  string_eq (herk_to_utf8 ("<#10>"), herk_byte (0x10));
  string_eq (herk_to_utf8 ("<#1F>"), herk_byte (0x1F));
  string_eq (herk_to_utf8 ("<#FF>"), "ÿ");    // U+00FF
  string_eq (herk_to_utf8 ("<#0FF>"), "ÿ");   // U+00FF
  string_eq (herk_to_utf8 ("<#00FF>"), "ÿ");  // U+00FF
  string_eq (herk_to_utf8 ("<#4E2D>"), "中"); // U+4E2D

  string_eq (utf8_to_herk ("<#00>"), "<#00>");
  string_eq (utf8_to_herk ("<#FF>"), "<#FF>");
  string_eq (utf8_to_herk ("<#0FF>"), "<#0FF>");
  string_eq (utf8_to_herk ("<#00FF>"), "<#00FF>");
}

TEST_CASE ("herk_named_escapes") {
  string_eq (utf8_to_herk ("<less>"), "<less>");
  string_eq (herk_to_utf8 ("<less>"), "<less>");
  string_eq (utf8_to_herk ("<gtr>"), "<gtr>");
  string_eq (herk_to_utf8 ("<gtr>"), "<gtr>");
}

TEST_CASE ("herk_mixed") {
  string_eq (
      herk_to_utf8 (herk_byte (0x41) * string ("<#2019>") * herk_byte (0x42)),
      "A" * string ("’") * "B");
  string_eq (utf8_to_herk ("A"
                           "中"
                           "B"),
             herk_byte (0x41) * string ("<#4E2D>") * herk_byte (0x42));
}
