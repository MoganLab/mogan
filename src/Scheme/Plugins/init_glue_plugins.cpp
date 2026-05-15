
/******************************************************************************
 * MODULE     : init_glue_plugins.cpp
 * DESCRIPTION: Plugins Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "init_glue_plugins.hpp"
#include "object.hpp"
#include "object_l1.hpp"
#include "object_l2.hpp"
#include "s7_tm.hpp"

#include "Xml/xml.hpp"
#include "glue_xml.cpp"

#ifdef USE_PLUGIN_HTML
#include "Html/html.hpp"
#include "glue_html.cpp"
#endif

bool
use_plugin_updater () {
#ifdef USE_PLUGIN_SPARKLE
  return true;
#else
  return false;
#endif
}

#ifdef USE_PLUGIN_SPARKLE
#include "Updater/tm_updater.hpp"
#include "glue_updater.cpp"
#endif

bool
use_plugin_tex () {
#ifdef USE_PLUGIN_TEX
  return true;
#else
  return false;
#endif
}

#ifdef USE_PLUGIN_TEX
#include "Tex/tex.hpp"
#include "glue_tex.cpp"
#endif

bool
use_plugin_bibtex () {
#ifdef USE_PLUGIN_BIBTEX
  return true;
#else
  return false;
#endif
}

#ifdef USE_PLUGIN_BIBTEX
#include "Bibtex/bibtex.hpp"
#include "Bibtex/bibtex_functions.hpp"
#include "glue_bibtex.cpp"
#endif

bool
supports_native_pdf () {
#ifdef USE_PLUGIN_PDF
  return true;
#else
  return false;
#endif
}

string
pdfhummus_version () {
  return string (PDFHUMMUS_VERSION);
}

#ifdef USE_PLUGIN_PDF
#include "Pdf/pdf_hummus_extract_attachment.hpp"
#include "Pdf/pdf_hummus_make_attachment.hpp"
#include "Pdf/pdf_image.hpp"

array<int>
pdfhummus_image_size (url pdf_image) {
  int w= 0, h= 0;
  hummus_pdf_image_size (pdf_image, w, h);
  return array (w, h);
}

#include "glue_pdf.cpp"
#endif

bool
supports_ghostscript () {
#ifdef USE_PLUGIN_GS
  return true;
#else
  return false;
#endif
}

#ifdef USE_PLUGIN_GS
#include "glue_ghostscript.cpp"
#endif

#include "glue_plugin.cpp"

void
initialize_glue_plugins () {
  initialize_glue_plugin ();
  initialize_glue_xml ();

#ifdef USE_PLUGIN_HTML
  initialize_glue_html ();
#endif

#ifdef USE_PLUGIN_SPARKLE
  initialize_glue_updater ();
#endif

#ifdef USE_PLUGIN_BIBTEX
  initialize_glue_bibtex ();
#endif

#ifdef USE_PLUGIN_TEX
  initialize_glue_tex ();
#endif

#ifdef USE_PLUGIN_GS
  initialize_glue_ghostscript ();
#endif

#ifdef USE_PLUGIN_PDF
  initialize_glue_pdf ();
#endif
}
