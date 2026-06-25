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

TEST_CASE ("herk_0x") {
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

TEST_CASE ("herk_1x") {
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

TEST_CASE ("herk_2x") {
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

TEST_CASE ("herk_3x") {
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

TEST_CASE ("herk_4x") {
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

TEST_CASE ("herk_5x") {
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

TEST_CASE ("herk_6x") {
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

TEST_CASE ("herk_7x") {
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

TEST_CASE ("herk_8x") {
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

TEST_CASE ("herk_9x") {
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

TEST_CASE ("herk_Ax") {
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

TEST_CASE ("herk_Bx") {
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

TEST_CASE ("herk_Cx") {
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

TEST_CASE ("herk_Dx") {
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

TEST_CASE ("herk_Ex") {
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

TEST_CASE ("herk_Fx") {
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

TEST_CASE ("herk_unmapped") {
  // U+007F is unmapped and stays as a single byte.
  string_eq (utf8_to_herk ("\x7F"), "\x7F");

  // U+00A0 has no Herk mapping.
  string_eq (utf8_to_herk ("\xC2\xA0"), "<#A0>"); // U+00A0
  string_eq (herk_to_utf8 ("<#A0>"), "\xC2\xA0"); // U+00A0

  // U+2019 has no Herk mapping.
  string_eq (utf8_to_herk ("’"), "<#2019>");  // U+2019
  string_eq (utf8_to_herk ("中"), "<#4E2D>"); // U+4E2D
}
