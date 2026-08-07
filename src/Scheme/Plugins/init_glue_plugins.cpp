
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

#ifdef USE_PLUGIN_HTML
#include "Html/html.hpp"
#endif

#ifdef USE_PLUGIN_TEX
#include "Tex/tex.hpp"
#endif

#ifdef USE_PLUGIN_BIBTEX
#include "Bibtex/bibtex.hpp"
#include "Bibtex/bibtex_functions.hpp"
#endif

#ifdef USE_PLUGIN_PDF
#include "Pdf/pdf_hummus_extract_attachment.hpp"
#include "Pdf/pdf_hummus_make_attachment.hpp"
#include "Pdf/pdf_image.hpp"
#endif

void
initialize_glue_plugins () {
  initialize_glue_plugin ();
  initialize_glue_xml ();

#ifdef USE_PLUGIN_HTML
  initialize_glue_html ();
#endif

#if defined(USE_PLUGIN_VELOPACK)
  initialize_glue_updater ();
#endif

#ifdef USE_PLUGIN_BIBTEX
  initialize_glue_bibtex ();
#endif

#ifdef USE_PLUGIN_TEX
  initialize_glue_tex ();
#endif

#ifdef LORO_ENABLED
  initialize_glue_collab ();
#endif

#ifdef USE_PLUGIN_GS
  initialize_glue_ghostscript ();
#endif

#ifdef USE_PLUGIN_PDF
  initialize_glue_pdf ();
#endif
}
