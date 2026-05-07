
/******************************************************************************
 * MODULE     : mupdf_renderer.hpp
 * DESCRIPTION: Raster device with MuPDF
 * COPYRIGHT  : (C) 2022 Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef MUPDF_RENDERER_HPP
#define MUPDF_RENDERER_HPP

#include "QTMPixmapOrImage.hpp"
#include "basic_renderer.hpp"

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

fz_context* mupdf_context ();

/******************************************************************************
 * Graphic renderer
 ******************************************************************************/

class mupdf_renderer_rep : public basic_renderer_rep {
protected:
  // we draw on a pixmap via a device and a pdf graphic processor
  fz_pixmap*     pixmap;
  fz_device*     dev;
  pdf_processor* proc;

  // some state
  color fg, bg;
  SI    lw;

  //  pencil    pen;
  //  brush     bgb, fgb;

  double prev_text_x, prev_text_y;
  bool   in_text;
  string cfn;
  float  fsize;

  static bool           clip_active;
  static pdf_processor* clip_proc;

  bool transform_active;

  // geometry

  SI to_x (SI x) {
    x+= ox;
    if (x >= 0) x= x / pixel;
    else x= (x - pixel + 1) / pixel;
    return x;
  };

  SI to_y (SI y) {
    y+= oy;
    if (y >= 0) y= y / pixel;
    else y= (y - pixel + 1) / pixel;
    return y;
  };

  double to_x_double (SI x) {
    double rx= ((double) (x + ox)) / pixel - 0.5;
    return rx;
  };

  double to_y_double (SI y) {
    double ry= ((double) (y + oy)) / pixel - 0.5;
    return ry;
  };

  void begin_text ();
  void end_text ();

  void select_line_width (SI w);
  void select_stroke_color (color c);
  void select_fill_color (color c);
  void select_alpha (int a);
  void select_stroke_pattern (brush br);
  void select_fill_pattern (brush br);
  void register_pattern (brush br, SI pixel);

public:
  mupdf_renderer_rep (int w= 0, int h= 0);
  ~mupdf_renderer_rep ();
  void* get_handle ();

  void set_zoom_factor (double zoom);

  void begin (void* handle);
  void end ();

  // void set_extent (int _w, int _h) { w = _w; h = _h; }
  void get_extents (int& w, int& h);

  void set_transformation (frame fr);
  void reset_transformation ();

  void set_clipping (SI x1, SI y1, SI x2, SI y2, bool restore= false);
  void set_pencil (pencil p);
  void set_brush (brush b);
  void set_background (brush b2);

  void draw_bis (int char_code, font_glyphs fn, SI x, SI y);
  void draw (int glyph_index, font_glyphs fn, SI x, SI y, int codepoint= -1);
  void draw_scalable (scalable im, SI x, SI y, int alpha);

  void line (SI x1, SI y1, SI x2, SI y2);
  void lines (array<SI> x, array<SI> y);
  void clear (SI x1, SI y1, SI x2, SI y2);
  void fill (SI x1, SI y1, SI x2, SI y2);
  void arc (SI x1, SI y1, SI x2, SI y2, int alpha, int delta);
  void fill_arc (SI x1, SI y1, SI x2, SI y2, int alpha, int delta);
  void polygon (array<SI> x, array<SI> y, bool convex= true);
  //  void  draw_triangle (SI x1, SI y1, SI x2, SI y2, SI x3, SI y3);

  void bezier_arc (SI x1, SI y1, SI x2, SI y2, int alpha, int delta,
                   bool filled);

  void new_shadow (renderer& ren);
  void delete_shadow (renderer& ren);
  void get_shadow (renderer ren, SI x1, SI y1, SI x2, SI y2);
  void put_shadow (renderer ren, SI x1, SI y1, SI x2, SI y2);

  void apply_shadow (SI x1, SI y1, SI x2, SI y2);
  void fetch (SI x1, SI y1, SI x2, SI y2, renderer ren, SI x, SI y);

  void draw_picture (picture pict, SI x, SI y, int alpha);

  friend class mupdf_proxy_renderer_rep;
};

mupdf_renderer_rep* the_mupdf_renderer ();

// Read data from url
fz_buffer* mupdf_read_from_url (url u);

// Convert fz_pixmap to QImage, QTMPixmapOrImage
QImage           get_QImage_from_pixmap (fz_pixmap* pix);
QTMPixmapOrImage get_QTMPixmapOrImage_from_pixmap (fz_pixmap* pix);

#endif // defined MUPDF_RENDERER_HPP
