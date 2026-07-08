/******************************************************************************
 * MODULE     : mupdf_qt_ui_element.cpp
 * DESCRIPTION: Modified QT glue widget render process to adapt to MuPDF render
 * COPYRIGHT  : (C) 2025 The Mogan Stem Authors
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "MuPDF/mupdf_renderer.hpp"
#include "qt_ui_element.hpp"

QTMPixmapOrImage
qt_glue_widget_rep::render () {
  if (w > 128 || h > 128) {
    // Reduce pixmap width and height to improve performance
    SI factor= max (w, h) / 128;
    w/= factor;
    h/= factor;
  }
  fz_pixmap* pix= fz_new_pixmap (
      mupdf_context (), fz_device_rgb (mupdf_context ()), w, h, NULL, 1);
  mupdf_renderer_rep* ren= the_mupdf_renderer ();
  ren->begin (pix);
  rectangle r= rectangle (0, 0, w, h);
  ren->set_origin (0, 0);
  ren->encode (r->x1, r->y1);
  ren->encode (r->x2, r->y2);
  ren->set_clipping (r->x1, r->y2, r->x2, r->y1);
  if (col == "") {
    // do nothing
  }
  else {
    if (is_atomic (col)) {
      color c= named_color (col->label);
      ren->set_background (c);
      ren->set_pencil (c);
      ren->fill (r->x1, r->y2, r->x2, r->y1);
    }
    else {
      ren->set_shrinking_factor (std_shrinkf);
      brush old_b= ren->get_background ();
      ren->set_background (col);
      ren->clear_pattern (5 * r->x1, 5 * r->y2, 5 * r->x2, 5 * r->y1);
      ren->set_background (old_b);
      ren->set_shrinking_factor (1);
    }
  }
  ren->end ();

  return get_QTMPixmapOrImage_from_pixmap (pix);
}
