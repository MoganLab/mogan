
/******************************************************************************
 * MODULE     : printer.hpp
 * DESCRIPTION: Windows under X
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef PRINTER_H
#define PRINTER_H
#include "hashmap.hpp"
#include "renderer.hpp"
#include "url.hpp"

class printer_rep : public renderer_rep {
  url    ps_file_name;
  int    dpi;
  int    nr_pages;
  string page_type;
  bool   landscape;
  double paper_w;
  double paper_h;
  int    psw;
  int    psh;
  bool   use_alpha;
  string prologue;
  string body;
  int    cur_page;
  int    linelen;

  color  fg, bg;
  int    opacity;
  pencil pen;
  brush  bgb;
  int    ncols;
  SI     lw;
  int    nwidths;
  string cfn;
  int    nfonts;
  DI     xpos, ypos;
  bool   tex_flag;
  tree   toc;

  hashmap<string, string>     defs;
  hashmap<string, string>     tex_chars;
  hashmap<string, string>     tex_width;
  hashmap<string, string>     tex_fonts;
  hashmap<string, array<int>> tex_font_chars;
  hashmap<string, string>     metadata;

public:
  printer_rep (url ps_file_name, int dpi, int nr_pages, string ptype,
               bool landsc, double paper_w, double paper_h);
  ~printer_rep ();
  bool is_printer ();
  void next_page ();

  /*********************** subroutines for printing **************************/

  void   define (string comm, string defn);
  void   sep ();
  void   cr ();
  void   print (string s);
  void   print (DI x, DI y);
  void   move_to (DI x, DI y);
  string define_alpha (int a);
  void   select_color (color c);
  void   select_line_width (SI w);

  /********************* subroutines for drawing text ************************/

  void make_tex_char (string name, QN c, glyph gl);
  void select_tex_font (string name);
  void generate_tex_fonts ();

  /************************ subroutines hyperlinks ***************************/

  void anchor (string label, DI x1, DI y1, DI x2, DI y2);
  void href (string label, DI x1, DI y1, DI x2, DI y2);
  void toc_entry (string kind, string title, string raw_title, DI x, DI y);
  void generate_toc_item (tree t);
  void generate_toc ();
  void set_metadata (string kind, string val);
  void generate_metadata ();

  /********************** routines from renderer.hpp *************************/

  void set_transformation (frame fr);
  void reset_transformation ();

  void   set_clipping (DI x1, DI y1, DI x2, DI y2, bool restore= false);
  pencil get_pencil ();
  brush  get_background ();
  void   set_pencil (pencil p);
  void   set_background (brush b);
  void   draw (int glyph_index, font_glyphs fn, DI x, DI y, int codepoint= -1);
  void   line (DI x1, DI y1, DI x2, DI y2);
  void   lines (array<SI> x, array<SI> y);
  void   clear (DI x1, DI y1, DI x2, DI y2);
  void   fill (DI x1, DI y1, DI x2, DI y2);
  void   arc (DI x1, DI y1, DI x2, DI y2, int alpha, int delta);
  void   fill_arc (DI x1, DI y1, DI x2, DI y2, int alpha, int delta);
  void   polygon (array<SI> x, array<SI> y, bool convex= true);

  void fetch (DI x1, DI y1, DI x2, DI y2, renderer ren, DI x, DI y);
  void new_shadow (renderer& ren);
  void delete_shadow (renderer& ren);
  void get_shadow (renderer ren, DI x1, DI y1, DI x2, DI y2);
  void put_shadow (renderer ren, DI x1, DI y1, DI x2, DI y2);
  void apply_shadow (DI x1, DI y1, DI x2, DI y2);

  renderer shadow (picture& pic, DI x1, DI y1, DI x2, DI y2);
  void     draw_picture (picture p, DI x, DI y, int alpha);
  void     draw_scalable (scalable im, DI x, DI y, int alpha);
  void image (string name, string eps, DI x1, DI y1, DI x2, DI y2, DI w, DI h,
              DI x, DI y, int alpha);
};

renderer printer (url ps_file_name, int dpi, int nr_pages= 1,
                  string page_type= "a4", bool landscape= false,
                  double paper_w= 21.0, double paper_h= 29.7);

#endif // defined PRINTER_H
