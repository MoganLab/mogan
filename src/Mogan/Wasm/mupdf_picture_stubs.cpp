
/******************************************************************************
 * MODULE     : mupdf_picture_stubs.cpp
 * DESCRIPTION: Stubs for MuPDF picture loading in the Qt-free WASM ImGui viewer
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "mupdf_picture.hpp"

bool
mupdf_supports (string extension) {
  (void) extension;
  return false;
}

bool
mupdf_normal_image_size (url image, int& w, int& h, string* out_wcm_pointer,
                         string* out_hcm_pointer) {
  (void) image;
  w= 0;
  h= 0;
  if (out_wcm_pointer != NULL) *out_wcm_pointer= "";
  if (out_hcm_pointer != NULL) *out_hcm_pointer= "";
  return false;
}

bool
mupdf_pdf_image_size (url image, int& w, int& h, string* out_wcm_pointer,
                      string* out_hcm_pointer) {
  (void) image;
  w= 0;
  h= 0;
  if (out_wcm_pointer != NULL) *out_wcm_pointer= "";
  if (out_hcm_pointer != NULL) *out_hcm_pointer= "";
  return false;
}

string
mupdf_load_and_parse_image (const char* path, int& w, int& h, string extension,
                            string* out_wcm, string* out_hcm) {
  (void) path;
  (void) extension;
  (void) out_wcm;
  (void) out_hcm;
  w= 0;
  h= 0;
  return "";
}

fz_pixmap*
mupdf_load_pixmap (url u, int w, int h, tree eff, SI pixel) {
  (void) u;
  (void) w;
  (void) h;
  (void) eff;
  (void) pixel;
  return NULL;
}

fz_image*
mupdf_load_image (url u) {
  (void) u;
  return NULL;
}
