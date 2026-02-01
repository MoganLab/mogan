
/******************************************************************************
 * MODULE     : unicode_font.hpp
 * DESCRIPTION: True Type fonts (using FreeType II)
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Freetype/free_type.hpp"
#include "Freetype/tt_face.hpp"
#include "Freetype/tt_file.hpp"
#include "Freetype/tt_tools.hpp"
#include "analyze.hpp"
#include "converter.hpp"
#include "cork.hpp"
#include "font.hpp"

/******************************************************************************
 * True Type fonts
 ******************************************************************************/

struct unicode_font_rep : font_rep {
  string      family;
  int         hdpi;
  int         vdpi;
  font_metric fnm;
  font_glyphs fng;
  int         ligs;

  hashmap<string, int> native; // additional native (non unicode) characters

  // only for OpenType fonts
  tt_face      math_face;
  ot_mathtable math_table;
  font         make_rubber_font (font base) override;
  bool get_ot_kerning (string s, SI height, bool top, bool left, SI& kerning);
  bool get_ot_italic_correction (string s, SI& r);
  bool is_ot_integral (string s);
  hashset<unsigned int> ot_integral;

  unicode_font_rep (string name, string family, double size, int hdpi,
                    int vdpi);
  void tex_gyre_operators ();

  unsigned int get_glyphID (string s);
  unsigned int read_unicode_char (string s, int& i);
  unsigned int ligature_replace (unsigned int c, string s, int& i);
  bool         supports (string c);
  void         get_extents (string s, metric& ex);
  void         get_xpositions (string s, SI* xpos, bool ligf);
  void         get_xpositions (string s, SI* xpos);
  void         draw_fixed (renderer ren, string s, SI x, SI y, bool ligf);
  void         draw_fixed (renderer ren, string s, SI x, SI y);
  font         magnify (double zoomx, double zoomy);
  void         advance_glyph (string s, int& pos, bool ligf);
  glyph        get_glyph (string s);
  int          index_glyph (string s, font_metric& fnm, font_glyphs& fng);
  double       get_left_slope (string s);
  double       get_right_slope (string s);
  SI           get_left_correction (string s);
  SI           get_right_correction (string s);
  SI           get_lsub_correction (string s);
  SI           get_lsup_correction (string s);
  SI           get_rsub_correction (string s);
  SI           get_rsup_correction (string s);
  SI           get_wide_correction (string s, int mode);

  double design_unit_to_metric_factor;
  double metric_to_design_unit_factor;
  void   init_design_unit_factor ();
  SI     design_unit_to_metric (int du);
  int    metric_to_design_unit (SI m);
};

font rubber_unicode_font (font base, tt_face math_face);

/**
 * @brief Parses variant information from a cork string.
 *
 * @param s Input string.
 * @param h The head of the string, for example "<big-sum-2>" -> "big".
 * @param r The root of the string, for example "<big-sum-2>" -> "sum".
 * @return Variant number, for example "<big-sum-2>" -> 2.
 */
int parse_variant (string s, string& h, string& r);
