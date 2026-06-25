/** \file colors_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for color management
 *  \author Da Shen
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include <moebius/data/colors.hpp>

using namespace moebius;
using namespace moebius::data;

static void
get_rgb (color c, int& r, int& g, int& b) {
  int a;
  get_rgb_color (c, r, g, b, a);
}

TEST_CASE ("test initialize_colors sets basic colors") {
  initialize_colors ();
  int r, g, b;
  get_rgb (black, r, g, b);
  CHECK_EQ (r == 0 && g == 0 && b == 0, true);
  get_rgb (white, r, g, b);
  CHECK_EQ (r == 255 && g == 255 && b == 255, true);
  get_rgb (red, r, g, b);
  CHECK_EQ (r == 255 && g == 0 && b == 0, true);
  get_rgb (green, r, g, b);
  CHECK_EQ (r == 0 && g == 255 && b == 0, true);
  get_rgb (blue, r, g, b);
  CHECK_EQ (r == 0 && g == 0 && b == 255, true);
}

TEST_CASE ("test rgb_color round-trip") {
  initialize_colors ();
  color c= rgb_color (10, 20, 30, 255);
  int   r, g, b, a;
  get_rgb_color (c, r, g, b, a);
  CHECK_EQ (r == 10, true);
  CHECK_EQ (g == 20, true);
  CHECK_EQ (b == 30, true);
  CHECK_EQ (a == 255, true);
}

TEST_CASE ("test rgb_color with alpha") {
  initialize_colors ();
  color c= rgb_color (100, 150, 200, 128);
  int   r, g, b, a;
  get_rgb_color (c, r, g, b, a);
  CHECK_EQ (r == 100, true);
  CHECK_EQ (g == 150, true);
  CHECK_EQ (b == 200, true);
  CHECK_EQ (a == 128, true);
}

TEST_CASE ("test named_color parses basic names") {
  initialize_colors ();
  int r, g, b;
  get_rgb (named_color ("red"), r, g, b);
  CHECK_EQ (r == 255 && g == 0 && b == 0, true);
  get_rgb (named_color ("blue"), r, g, b);
  CHECK_EQ (r == 0 && g == 0 && b == 255, true);
  get_rgb (named_color ("white"), r, g, b);
  CHECK_EQ (r == 255 && g == 255 && b == 255, true);
}

TEST_CASE ("test named_color is case-insensitive") {
  initialize_colors ();
  int r, g, b;
  get_rgb (named_color ("Red"), r, g, b);
  CHECK_EQ (r == 255, true);
  get_rgb (named_color ("BLUE"), r, g, b);
  CHECK_EQ (b == 255, true);
}

TEST_CASE ("test named_color parses hex") {
  initialize_colors ();
  int r, g, b;
  color c= named_color ("#aabbcc");
  get_rgb (c, r, g, b);
  CHECK_EQ (r == 0xaa, true);
  CHECK_EQ (g == 0xbb, true);
  CHECK_EQ (b == 0xcc, true);
}

TEST_CASE ("test named_color parses short hex") {
  initialize_colors ();
  int r, g, b;
  color c= named_color ("#abc");
  get_rgb (c, r, g, b);
  CHECK_EQ (r == 0xaa, true);
  CHECK_EQ (g == 0xbb, true);
  CHECK_EQ (b == 0xcc, true);
}

TEST_CASE ("test named_color unknown falls back to black") {
  initialize_colors ();
  int r, g, b;
  get_rgb (named_color ("nosuchcolor123"), r, g, b);
  CHECK_EQ (r == 0 && g == 0 && b == 0, true);
}

TEST_CASE ("test cmyk_color extremes") {
  initialize_colors ();
  int r, g, b;
  // cmyk(0,0,0,0) -> white
  get_rgb (cmyk_color (0, 0, 0, 0), r, g, b);
  CHECK_EQ (r == 255 && g == 255 && b == 255, true);
  // cmyk(0,0,0,255) -> black
  get_rgb (cmyk_color (0, 0, 0, 255), r, g, b);
  CHECK_EQ (r == 0 && g == 0 && b == 0, true);
}

TEST_CASE ("test xpm_color hex forms") {
  initialize_colors ();
  int r, g, b, a;
  get_rgb_color (xpm_color ("#abc"), r, g, b, a);
  CHECK_EQ (r == 0xaa && g == 0xbb && b == 0xcc, true);
  get_rgb_color (xpm_color ("#aabbcc"), r, g, b, a);
  CHECK_EQ (r == 0xaa && g == 0xbb && b == 0xcc, true);
  // "none" is transparent placeholder
  get_rgb_color (xpm_color ("none"), r, g, b, a);
  CHECK_EQ (a == 0, true);
}

TEST_CASE ("test get_hex_color round-trip") {
  initialize_colors ();
  // as_hexadecimal uses uppercase hex digits
  string s= get_hex_color (rgb_color (0xaa, 0xbb, 0xcc, 255));
  CHECK_EQ (s == "#AABBCC", true);
  string s2= get_hex_color (rgb_color (0, 0, 0, 255));
  CHECK_EQ (s2 == "#000000", true);
}

TEST_CASE ("test get_hex_color with alpha") {
  initialize_colors ();
  string s= get_hex_color (rgb_color (0xaa, 0xbb, 0xcc, 0x80));
  CHECK_EQ (s == "#AABBCC80", true);
}

TEST_CASE ("test get_hex_color via name") {
  initialize_colors ();
  string s= get_hex_color ("red");
  CHECK_EQ (s == "#FF0000", true);
  string s2= get_hex_color ("#aabbcc");
  CHECK_EQ (s2 == "#AABBCC", true);
}

TEST_CASE ("test is_color_name") {
  initialize_colors ();
  CHECK_EQ (is_color_name ("red"), true);
  CHECK_EQ (is_color_name ("black"), true);
  CHECK_EQ (is_color_name ("#000000"), true);
  CHECK_EQ (is_color_name ("nosuchcolor123"), false);
}

TEST_CASE ("test named color tables are populated") {
  initialize_colors ();
  // Trigger lazy population via named_color first
  named_color ("red");
  int r, g, b;
  // x11 has 'Red' -> lowercased 'red' on lookup
  get_rgb (x11_color ("red"), r, g, b);
  CHECK_EQ (r == 255, true);
  // svg has 'red'
  get_rgb (svg_color ("red"), r, g, b);
  CHECK_EQ (r == 255, true);
  // dvips has 'Red' -> lowercased 'red', CMYK(0,255,255,0) -> (255,0,0)
  get_rgb (dvips_color ("red"), r, g, b);
  CHECK_EQ (r == 255, true);
  // tm has 'red'
  get_rgb (tm_color ("red"), r, g, b);
  CHECK_EQ (r == 255, true);
}

TEST_CASE ("test blend_colors opaque returns fg") {
  initialize_colors ();
  color fg= rgb_color (10, 20, 30, 255);
  color bg= rgb_color (200, 200, 200, 255);
  CHECK_EQ (blend_colors (fg, bg) == fg, true);
}

TEST_CASE ("test blend_colors semi-transparent") {
  initialize_colors ();
  color fg= rgb_color (0, 0, 0, 128);
  color bg= rgb_color (255, 255, 255, 255);
  color r = blend_colors (fg, bg);
  int  rr, rg, rb, ra;
  get_rgb_color (r, rr, rg, rb, ra);
  // (255*(255-128) + 0*128)/255 = 127
  CHECK_EQ (rr == 127, true);
  CHECK_EQ (rg == 127, true);
  CHECK_EQ (rb == 127, true);
}

TEST_CASE ("test reverse produces different color") {
  initialize_colors ();
  color rev= reverse (red);
  CHECK_EQ (rev != red, true);
}

TEST_CASE ("test named_color_to_xcolormap") {
  initialize_colors ();
  named_color ("red");
  CHECK_EQ (named_color_to_xcolormap ("red") != "none", true);
  CHECK_EQ (named_color_to_xcolormap ("nosuchcolor123") == "none", true);
}
