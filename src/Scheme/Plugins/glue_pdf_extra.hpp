/******************************************************************************
 * MODULE     : glue_pdf_extra.hpp
 * DESCRIPTION: helper functions used by glue_pdf (generated, standalone)
 *              and by init_glue_plugins.cpp's own registrations.
 *              Extracted so glue_pdf.cpp can be compiled as an
 *              independent translation unit.
 ******************************************************************************/

#ifndef GLUE_PDF_EXTRA_HPP
#define GLUE_PDF_EXTRA_HPP

#include "Pdf/pdf_image.hpp"
#include "url.hpp"

inline bool
supports_native_pdf () {
#ifdef USE_PLUGIN_PDF
  return true;
#else
  return false;
#endif
}

inline string
pdfhummus_version () {
  return string (PDFHUMMUS_VERSION);
}

inline array<int>
pdfhummus_image_size (url pdf_image) {
  int w= 0, h= 0;
  hummus_pdf_image_size (pdf_image, w, h);
  return array (w, h);
}

#endif
