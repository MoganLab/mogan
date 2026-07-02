
/******************************************************************************
 * MODULE     : colors.hpp
 * DESCRIPTION: Multi format RGB based color management
 * COPYRIGHT  : (C) 2014  François Poulain
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#pragma once
#include "analyze.hpp"
#include "basic.hpp"
#include "hashmap.hpp"
#include "string.hpp"

namespace moebius {
namespace data {

extern int  pastel;
extern bool true_colors;
extern bool reverse_colors;

extern color black, white, red, green, blue;
extern color yellow, magenta, orange, brown, pink;
extern color light_grey, grey, dark_grey;

extern color tm_background;

void set_true_colors (bool b);
bool get_reverse_colors ();
void set_reverse_colors (bool b);
void set_color_attrs (int cs, int cf, int gr);
void get_color_attrs (int& cs, int& cf, int& gr, int& ct);

void initialize_colors ();

color rgb_color (int r, int g, int b, int a= 255);
// get a color by its RGB and alpha components
// ranging from 0 to 255 included

void get_rgb_color (color col, int& r, int& g, int& b, int& a);
// get the RGB components of a color

color xpm_color (string s);
// get a color by its xpm description

void get_xpm_color (color col, string& s);
// get the XPM description of a color

color cmyk_color (int c, int m, int y, int k, int a= 255);
// get a color by its CMYK components

void get_cmyk_color (color col, int& r, int& g, int& b, int& a);
// get the CMYK components of a color

bool       is_color_name (string s);
color      named_color (string s, int a= 255);
string     named_rgb_color (array<int> rgba);
array<int> get_named_rgb_color (string col);
// get a color by its name

string get_hex_color (string s);
string get_hex_color (color c);
// get an RGB hexadecimal description of the color

color blend_colors (color fg, color bg);
// blend two colors

string named_color_to_xcolormap (string s);
// get xcolor colorscheme

void reverse (int& r, int& g, int& b);
// reverse RGB components in place

color reverse (color col);
// get reverse color

/******************************************************************************
 * Color vectors
 ******************************************************************************/

typedef struct {
  const char* name;
  int         c, m, y, k;
} cmyk_record;

typedef struct {
  const char* name;
  int         r, g, b;
} rgb_record;

/******************************************************************************
 * Color dictionnaries
 ******************************************************************************/

typedef hashmap<string, color> colorhash;
inline colorhash               tm_ch, dvips_ch, x11_ch, xc_ch, svg_ch;
inline color
dvips_color (string s) {
  return dvips_ch[locase_all (s)];
};
inline color
x11_color (string s) {
  return x11_ch[locase_all (s)];
};
inline color
svg_color (string s) {
  return svg_ch[locase_all (s)];
};
inline color
html_color (string s) {
  return svg_ch[locase_all (s)];
};
inline color
tm_color (string s) {
  return tm_ch[locase_all (s)];
};
inline color
xc_color (string s) {
  return xc_ch[locase_all (s)];
};

} // namespace data
} // namespace moebius
