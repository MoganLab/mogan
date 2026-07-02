
/******************************************************************************
 * MODULE     : mupdf_picture_core.cpp
 * DESCRIPTION: Core picture objects and image renderer for MuPDF (Qt-free)
 * COPYRIGHT  : (C) 2022 Massimiliano Gubinelli, Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "mupdf_picture.hpp"
#include "tm_debug.hpp"

static const double MUPDF_PDF_SCALE= 4.0;

/******************************************************************************
 * Abstract mupdf pictures
 ******************************************************************************/

mupdf_picture_rep::mupdf_picture_rep (fz_pixmap* _pix, int ox2, int oy2)
    : pix (_pix), im (NULL), w (fz_pixmap_width (mupdf_context (), pix)),
      h (fz_pixmap_height (mupdf_context (), pix)), ox (ox2), oy (oy2) {
  fz_keep_pixmap (mupdf_context (), pix);
}

mupdf_picture_rep::~mupdf_picture_rep () {
  fz_drop_pixmap (mupdf_context (), pix);
  fz_drop_image (mupdf_context (), im);
}

picture_kind
mupdf_picture_rep::get_type () {
  return picture_native;
}
void*
mupdf_picture_rep::get_handle () {
  return (void*) this;
}

int
mupdf_picture_rep::get_width () {
  return w;
}
int
mupdf_picture_rep::get_height () {
  return h;
}
int
mupdf_picture_rep::get_origin_x () {
  return ox;
}
int
mupdf_picture_rep::get_origin_y () {
  return oy;
}
void
mupdf_picture_rep::set_origin (int ox2, int oy2) {
  ox= ox2;
  oy= oy2;
}

color
mupdf_picture_rep::internal_get_pixel (int x, int y) {
  if (pix == NULL) return 0;
  if (x < 0 || y < 0 || x >= w || y >= h) return 0;

  fz_context*    ctx    = mupdf_context ();
  unsigned char* samples= fz_pixmap_samples (ctx, pix);
  int            n      = pix->n;
  bool           alpha  = pix->alpha;
  int            stride = pix->stride;
  if (samples == NULL || stride <= 0) return 0;
  if (n <= 0) {
    static bool warned_n_le0_get= false;
    if (!warned_n_le0_get) {
      std_error << "mupdf_picture_rep::internal_get_pixel: invalid channel "
                << "count n=" << n << LF;
      warned_n_le0_get= true;
    }
    return 0;
  }
  if (n > 4) {
    static bool warned_n_gt4_get= false;
    if (!warned_n_gt4_get) {
      std_error << "mupdf_picture_rep::internal_get_pixel: unsupported "
                << "channel count n=" << n << LF;
      warned_n_gt4_get= true;
    }
    return 0;
  }
  int            row= h - 1 - y;
  unsigned char* p  = samples + row * stride + x * n;

  int r= 0, g= 0, b= 0, a= 255;
  if (n == 1) {
    r= g= b= p[0];
  }
  else if (n == 2) {
    r= g= b= p[0];
    a      = p[1];
  }
  else {
    r= p[0];
    g= p[1];
    b= p[2];
    if (alpha) a= p[n - 1];
  }

  if (a == 255 || !alpha) return rgb_color (r, g, b, 255);
  return rgbap_to_argb ((a << 24) + (b << 16) + (g << 8) + r);
}

void
mupdf_picture_rep::internal_set_pixel (int x, int y, color c) {
  if (pix == NULL) return;
  if (x < 0 || y < 0 || x >= w || y >= h) return;

  fz_context*    ctx    = mupdf_context ();
  unsigned char* samples= fz_pixmap_samples (ctx, pix);
  int            n      = pix->n;
  bool           alpha  = pix->alpha;
  int            stride = pix->stride;
  if (samples == NULL || stride <= 0) return;
  if (n <= 0) {
    static bool warned_n_le0_set= false;
    if (!warned_n_le0_set) {
      std_error << "mupdf_picture_rep::internal_set_pixel: invalid channel "
                << "count n=" << n << LF;
      warned_n_le0_set= true;
    }
    return;
  }
  if (n > 4) {
    static bool warned_n_gt4_set= false;
    if (!warned_n_gt4_set) {
      std_error << "mupdf_picture_rep::internal_set_pixel: unsupported "
                << "channel count n=" << n << LF;
      warned_n_gt4_set= true;
    }
    return;
  }
  int            row= h - 1 - y;
  unsigned char* p  = samples + row * stride + x * n;

  int r, g, b, a;
  get_rgb_color (c, r, g, b, a);

  if (alpha) {
    r= (r * a) / 255;
    g= (g * a) / 255;
    b= (b * a) / 255;
  }

  if (n == 1) {
    p[0]= (unsigned char) ((77 * r + 150 * g + 29 * b) >> 8);
    return;
  }

  if (n == 2) {
    p[0]= (unsigned char) ((77 * r + 150 * g + 29 * b) >> 8);
    p[1]= (unsigned char) a;
    return;
  }

  p[0]= (unsigned char) r;
  p[1]= (unsigned char) g;
  p[2]= (unsigned char) b;
  if (alpha) p[n - 1]= (unsigned char) a;
}

picture
mupdf_picture (fz_pixmap* _pix, int ox, int oy) {
  return (picture) tm_new<mupdf_picture_rep, fz_pixmap*, int, int> (_pix, ox,
                                                                    oy);
}

picture
as_mupdf_picture (picture pic) {
  if (pic->get_type () == picture_native) return pic;
  fz_pixmap* pix=
      fz_new_pixmap (mupdf_context (), fz_device_rgb (mupdf_context ()),
                     pic->get_width (), pic->get_height (), NULL, 1);
  picture ret= mupdf_picture (pix, pic->get_origin_x (), pic->get_origin_y ());
  fz_drop_pixmap (mupdf_context (), pix);
  ret->copy_from (pic); // FIXME: is this inefficient???
  return ret;
}

#ifdef USE_MUPDF_RENDERER
picture
as_native_picture (picture pict) {
  return as_mupdf_picture (pict);
}

picture
native_picture (int w, int h, int ox, int oy) {
  fz_pixmap* pix= fz_new_pixmap (
      mupdf_context (), fz_device_rgb (mupdf_context ()), w, h, NULL, 1);
  fz_clear_pixmap_with_value (mupdf_context (), pix, 255); // white background
  picture p= mupdf_picture (pix, ox, oy);
  fz_drop_pixmap (mupdf_context (), pix);
  return p;
}
#endif

/******************************************************************************
 * Rendering on images
 ******************************************************************************/

class mupdf_image_renderer_rep : public mupdf_renderer_rep {
public:
  picture pict;

public:
  mupdf_image_renderer_rep (picture pict, double zoom);
  void* get_data_handle ();
};

mupdf_image_renderer_rep::mupdf_image_renderer_rep (picture p, double zoom)
    : mupdf_renderer_rep (), pict (p) {
  zoomf  = zoom;
  shrinkf= (int) tm_round (std_shrinkf / zoomf);
  pixel  = (SI) tm_round ((std_shrinkf * PIXEL) / zoomf);
  thicken= (shrinkf >> 1) * PIXEL;

  int pw = p->get_width ();
  int ph = p->get_height ();
  int pox= p->get_origin_x ();
  int poy= p->get_origin_y ();

  ox= pox * pixel;
  oy= poy * pixel;

  cx1= 0;
  cy1= -ph * pixel;
  cx2= pw * pixel;
  cy2= 0;

  mupdf_picture_rep* handle= (mupdf_picture_rep*) pict->get_handle ();
  begin (handle->pix);
}

void*
mupdf_image_renderer_rep::get_data_handle () {
  return (void*) this;
}

#ifdef USE_MUPDF_RENDERER
renderer
picture_renderer (picture p, double zoomf) {
  return (renderer) tm_new<mupdf_image_renderer_rep> (p, zoomf);
}
#endif
