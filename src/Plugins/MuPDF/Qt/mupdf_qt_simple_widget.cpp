/******************************************************************************
 * MODULE     : mupdf_qt_simple_widget.cpp
 * DESCRIPTION: Modified paint process to adapt to MuPDF render
 * COPYRIGHT  : (C) 2022 Massimiliano Gubinelli, Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/
#include "MuPDF/mupdf_picture.hpp"
#include "MuPDF/mupdf_renderer.hpp"
#include "qt_simple_widget.hpp"

void
qt_simple_widget_rep::repaint_invalid_regions () {

  QRegion qrgn;
  // qrgn is to keep track of the area on the screen which needs to be updated
  QPoint origin= canvas ()->origin ();

  coord2 pt_or    = from_qpoint (backing_pos);
  coord2 pt_new_or= from_qpoint (origin);

  QSize _newSize= retina_factor * canvas ()->surface ()->size ();

  bs_ox= -pt_or.x1 * retina_factor;
  bs_oy= -pt_or.x2 * retina_factor;

  SI  new_bs_ox= -pt_new_or.x1 * retina_factor;
  SI  new_bs_oy= -pt_new_or.x2 * retina_factor;
  int new_bs_w = _newSize.width ();
  int new_bs_h = _newSize.height ();
  bs_zoomf     = std_shrinkf;

  // prepare to render to the backing store
  renderer ren= picture_renderer (backing_store, bs_zoomf * retina_factor);

  if ((new_bs_ox != bs_ox) || (new_bs_oy != bs_oy) || (new_bs_w != bs_w) ||
      (new_bs_h != bs_h)) {
    // the viewport changed, reset the backing store

    // create a new backing store with updated viewport and the renderer
    picture new_backing_store= native_picture (
        new_bs_w, new_bs_h, new_bs_ox / PIXEL, new_bs_oy / PIXEL);
    renderer ren2=
        picture_renderer (new_backing_store, bs_zoomf * retina_factor);

    // copy the old backingstore
    SI x1= 0, y1= 0, x2= bs_w, y2= bs_h;
    ren->encode (x1, y1);
    ren->encode (x2, y2);
    ren2->fetch (x1, y2, x2, y1, ren, x1, y2);

    // compute new invalid regions
    // 1. translate
    rectangles invalid;
    int        dx= -(new_bs_ox - bs_ox) / (PIXEL);
    int        dy= (new_bs_oy - bs_oy) / (PIXEL);
    while (!is_nil (invalid_regions)) {
      rectangle r= invalid_regions->item;
      //      rectangle q = rectangle (r->x1+dx,r->y1-dy,r->x2+dx,r->y2-dy);
      rectangle q= rectangle (r->x1 - dx, r->y1 - dy, r->x2 - dx, r->y2 - dy);
      invalid    = rectangles (q, invalid);
      // cout << r << " ---> " << q << LF;
      invalid_regions= invalid_regions->next;
    }
    // 2. add new exposed regions due to translation
    invalid_regions= invalid & rectangles (rectangle (0, 0, bs_w, bs_h));
    if (dy < 0) invalidate_rect (0, 0, bs_w, min (bs_h, -dy));
    else if (dy > 0) invalidate_rect (0, max (0, bs_h - dy), bs_w, bs_h);

    if (dx < 0) invalidate_rect (0, 0, min (-dx, bs_w), bs_h);
    else if (dx > 0) invalidate_rect (max (0, bs_w - dx), 0, bs_w, bs_h);

    // 3. add new exposed regions due to resize
    if (new_bs_w > bs_w) {
      invalidate_rect (bs_w, 0, new_bs_w, new_bs_h);
      //      p.fillRect (QRect (_oldSize.width(), 0,
      //      _newSize.width()-_oldSize.width(), _newSize.height()), Qt::gray);
    }
    if (new_bs_h > bs_h) {
      invalidate_rect (0, bs_h, new_bs_w, new_bs_h);
      //      p.fillRect (QRect (0,_oldSize.height(), _newSize.width(),
      //      _newSize.height()-_oldSize.height()), Qt::gray);
    }

    // syncronize all the picture, just to be safe
    qrgn+= QRect (QPoint (0, 0),
                  QSize (new_bs_w / retina_factor, new_bs_h / retina_factor));

    // update the state
    bs_ox        = new_bs_ox;
    bs_oy        = new_bs_oy;
    bs_w         = new_bs_w;
    bs_h         = new_bs_h;
    backing_pos  = origin;
    backing_store= new_backing_store;
    delete_renderer (ren);
    ren= ren2;
  }

  // repaint invalid rectangles
  {
    rectangles new_regions;
    if (!is_nil (invalid_regions)) {

      rectangle lub= least_upper_bound (invalid_regions);
      if (area (lub) < 1.2 * area (invalid_regions))
        invalid_regions= rectangles (lub);

      // cache since they are muddled by handle_repaint...
      SI ox= ren->ox, oy= ren->oy;

      rectangles rects= invalid_regions;
      invalid_regions = rectangles ();
      while (!is_nil (rects)) {
        rectangle r = copy (rects->item);
        rectangle r0= rects->item;
        QRect     qr= QRect (r0->x1 / retina_factor, r0->y1 / retina_factor,
                             (r0->x2 - r0->x1) / retina_factor,
                             (r0->y2 - r0->y1) / retina_factor);
        // cout << "repainting " << r0 << "\n";
        ren->set_origin (ox, oy);
        ren->encode (r->x1, r->y1);
        ren->encode (r->x2, r->y2);
        ren->set_clipping (r->x1, r->y2, r->x2, r->y1);
        handle_repaint (ren, r->x1, r->y2, r->x2, r->y1);
        if (gui_interrupted ()) {
          // cout << "interrupted repainting of  " << r0 << "\n";
          // ren->set_pencil (green);
          // ren->line (r->x1, r->y1, r->x2, r->y2);
          // ren->line (r->x1, r->y2, r->x2, r->y1);
          invalidate_rect (r0->x1, r0->y1, r0->x2, r0->y2);
        }
        qrgn+= qr;
        rects= rects->next;
      }
    } // !is_nil (invalid_regions)
  }
  delete_renderer (ren);
  // propagate immediately the changes to the screen
  canvas ()->surface ()->repaint (qrgn);
}

QImage
qt_simple_widget_rep::get_backing_store () {
  fz_pixmap* pix= ((mupdf_picture_rep*) backing_store->get_handle ())->pix;
  return get_QImage_from_pixmap (pix);
}
