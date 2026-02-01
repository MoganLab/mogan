
/******************************************************************************
 * MODULE     : smart_font.hpp
 * DESCRIPTION: smart merging of several fonts for different unicode ranges
 * COPYRIGHT  : (C) 2013  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <tp://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef SMART_FONT_HPP
#define SMART_FONT_HPP

#include "analyze.hpp"
#include "font.hpp"
#include "resource.hpp"

/******************************************************************************
 * Efficient computation of the appropriate subfont
 ******************************************************************************/
RESOURCE (smart_map);

#define SUBFONT_MAIN 0
#define SUBFONT_ERROR 1

#define REWRITE_NONE 0
#define REWRITE_MATH 1
#define REWRITE_CYRILLIC 2
#define REWRITE_LETTERS 3
#define REWRITE_SPECIAL 4
#define REWRITE_EMULATE 5
#define REWRITE_POOR_BBB 6
#define REWRITE_ITALIC_GREEK 7
#define REWRITE_UPRIGHT_GREEK 8
#define REWRITE_UPRIGHT 9
#define REWRITE_ITALIC 10
#define REWRITE_IGNORE 11

struct smart_map_rep : rep<smart_map> {
  int                  chv[256];
  hashmap<string, int> cht;
  hashmap<tree, int>   fn_nr;
  array<tree>          fn_spec;
  array<int>           fn_rewr;

public:
  smart_map_rep (string name, tree fn)
      : rep<smart_map> (name), cht (-1), fn_nr (-1), fn_spec (2), fn_rewr (2) {
    (void) fn;
    for (int i= 0; i < 256; i++)
      chv[i]= -1;
    fn_nr (tuple ("main")) = SUBFONT_MAIN;
    fn_nr (tuple ("error"))= SUBFONT_ERROR;
    fn_spec[SUBFONT_MAIN]  = tuple ("main");
    fn_spec[SUBFONT_ERROR] = tuple ("error");
    fn_rewr[SUBFONT_MAIN]  = REWRITE_NONE;
    fn_rewr[SUBFONT_ERROR] = REWRITE_NONE;
  }

  int add_font (tree fn, int rewr) {
    if (!fn_nr->contains (fn)) {
      int sz    = N (fn_spec);
      fn_nr (fn)= sz;
      fn_spec << fn;
      fn_rewr << rewr;
      // cout << "Create " << sz << " -> " << fn << "\n";
    }
    return fn_nr[fn];
  }

  int add_char (tree fn, string c) {
    // cout << "Add " << c << " to " << fn << "\n";
    add_font (fn, REWRITE_NONE);
    int nr= fn_nr[fn];
    if (starts (c, "<")) {
      if (!cht->contains (c)) cht (c)= nr;
      else cht (c)= min (nr, cht[c]);
    }
    else {
      int code= (int) (unsigned char) c[0];
      if (chv[code] == -1) chv[code]= nr;
      else chv[code]= min (nr, chv[code]);
    }
    return nr;
  }
};

struct smart_font_rep : font_rep {
  string mfam;
  string family;
  string variant;
  string series;
  string shape;
  string rshape;
  double sz;
  int    hdpi;
  int    dpi;
  int    math_kind;
  int    italic_nr;

  array<font> fn;
  smart_map   sm;

  smart_font_rep (string name, font base_fn, font err_fn, string family,
                  string variant, string series, string shape, double sz,
                  int hdpi, int vdpi);
  font adjust_subfont (font fn);
  font get_math_font (string fam, string var, string ser, string sh);
  font get_cyrillic_font (string fam, string var, string ser, string sh);
  font get_greek_font (string fam, string var, string ser, string sh);
  font get_latin_font (string fam, string var, string ser, string sh);

  void advance (string s, int& pos, string& r, int& nr);
  int  resolve (string c, string fam, int attempt);
  bool is_italic_prime (string c);
  int  resolve_rubber (string c, string fam, int attempt);
  int  resolve (string c);
  void initialize_font (int nr);
  int  adjusted_dpi (string fam, string var, string ser, string sh, int att);

  font make_rubber_font (font base) override;

  bool   supports (string c);
  void   get_extents (string s, metric& ex);
  void   get_xpositions (string s, SI* xpos);
  void   get_xpositions (string s, SI* xpos, SI xk);
  void   draw_fixed (renderer ren, string s, SI x, SI y);
  void   draw_fixed (renderer ren, string s, SI x, SI y, SI xk);
  font   magnify (double zoomx, double zoomy);
  void   advance_glyph (string s, int& pos, bool ligf);
  glyph  get_glyph (string s);
  int    index_glyph (string s, font_metric& fnm, font_glyphs& fng);
  double get_left_slope (string s);

  /**
   * 获取字符串右侧的斜率。
   *
   * 此函数返回给定字符串右侧的倾斜角度，用于光标显示。
   * 针对非斜体模式下的CJK字符和数学符号，若斜率小于0.20则强制垂直。
   * 同时考虑字体粗体(series)属性，对粗体字体进行特殊处理。
   *
   * @param s 输入字符串
   * @return 右侧斜率值，0.0表示垂直
   */
  double get_right_slope (string s);

  SI get_left_correction (string s);
  SI get_right_correction (string s);
  SI get_lsub_correction (string s);
  SI get_lsup_correction (string s);
  SI get_rsub_correction (string s);
  SI get_rsup_correction (string s);
  SI get_wide_correction (string s, int mode);
};

#endif
