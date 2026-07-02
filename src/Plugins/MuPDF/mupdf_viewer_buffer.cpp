/******************************************************************************
 * MODULE     : mupdf_viewer_buffer.cpp
 * DESCRIPTION: RGBA viewer buffer for MuPDF-backed rendering
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "mupdf_viewer_buffer.hpp"

#include <mupdf/fitz.h>

mupdf_viewer_buffer_rep::mupdf_viewer_buffer_rep ()
    : pixels_ (NULL), width_ (0), height_ (0), stride_ (0) {}

mupdf_viewer_buffer_rep::~mupdf_viewer_buffer_rep () {
  if (pixels_ != NULL) delete[] pixels_;
}

void
mupdf_viewer_buffer_rep::resize (int width, int height) {
  if (width < 0) width= 0;
  if (height < 0) height= 0;
  int new_stride= width * 4;
  if (width == width_ && height == height_ && stride_ == new_stride &&
      pixels_ != NULL)
    return;

  if (pixels_ != NULL) {
    delete[] pixels_;
    pixels_= NULL;
  }

  width_ = width;
  height_= height;
  stride_= new_stride;
  if (width_ > 0 && height_ > 0) pixels_= new unsigned char[stride_ * height_];
}

void
mupdf_viewer_buffer_rep::clear (unsigned char r, unsigned char g,
                                unsigned char b, unsigned char a) {
  if (pixels_ == NULL) return;
  for (int y= 0; y < height_; ++y) {
    unsigned char* row= pixels_ + y * stride_;
    for (int x= 0; x < width_; ++x) {
      unsigned char* px= row + x * 4;
      px[0]= r;
      px[1]= g;
      px[2]= b;
      px[3]= a;
    }
  }
}

void
mupdf_viewer_buffer_rep::copy_from_pixmap (fz_pixmap* pix) {
  if (pix == NULL) return;
  resize (pix->w, pix->h);
  if (pixels_ == NULL) return;

  int comps= pix->n;
  unsigned char* src= pix->samples;
  int src_stride= pix->stride;
  for (int y= 0; y < height_; ++y) {
    unsigned char* src_row= src + y * src_stride;
    unsigned char* dst_row= pixels_ + y * stride_;
    for (int x= 0; x < width_; ++x) {
      unsigned char* src_px= src_row + x * comps;
      unsigned char* dst_px= dst_row + x * 4;
      unsigned char  r= 0;
      unsigned char  g= 0;
      unsigned char  b= 0;
      unsigned char  a= 255;

      if (comps == 1) {
        r= g= b= src_px[0];
      }
      else if (comps == 2) {
        r= g= b= src_px[0];
        a        = src_px[1];
      }
      else if (comps >= 3) {
        r= src_px[0];
        g= src_px[1];
        b= src_px[2];
        if (comps >= 4) a= src_px[3];
      }

      if (a != 255 && a != 0) {
        r= (unsigned char) ((r * 255 + (a / 2)) / a);
        g= (unsigned char) ((g * 255 + (a / 2)) / a);
        b= (unsigned char) ((b * 255 + (a / 2)) / a);
      }
      else if (a == 0) {
        r= g= b= 0;
      }

      dst_px[0]= r;
      dst_px[1]= g;
      dst_px[2]= b;
      dst_px[3]= a;
    }
  }
}

unsigned char*
mupdf_viewer_buffer_rep::data () {
  return pixels_;
}

const unsigned char*
mupdf_viewer_buffer_rep::data () const {
  return pixels_;
}

int
mupdf_viewer_buffer_rep::width () const {
  return width_;
}

int
mupdf_viewer_buffer_rep::height () const {
  return height_;
}

int
mupdf_viewer_buffer_rep::stride () const {
  return stride_;
}