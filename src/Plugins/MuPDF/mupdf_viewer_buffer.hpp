/******************************************************************************
 * MODULE     : mupdf_viewer_buffer.hpp
 * DESCRIPTION: RGBA viewer buffer for MuPDF-backed rendering
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef MUPDF_VIEWER_BUFFER_HPP
#define MUPDF_VIEWER_BUFFER_HPP

#include "mupdf_renderer.hpp"

struct fz_pixmap;

class mupdf_viewer_buffer_rep {
  unsigned char* pixels_;
  int            width_;
  int            height_;
  int            stride_;

public:
  mupdf_viewer_buffer_rep ();
  ~mupdf_viewer_buffer_rep ();

  void resize (int width, int height);
  void clear (unsigned char r, unsigned char g, unsigned char b,
              unsigned char a= 255);
  void copy_from_pixmap (fz_pixmap* pix);

  unsigned char*       data ();
  const unsigned char* data () const;
  int                  width () const;
  int                  height () const;
  int                  stride () const;
};

typedef mupdf_viewer_buffer_rep* mupdf_viewer_buffer;

#endif // defined MUPDF_VIEWER_BUFFER_HPP