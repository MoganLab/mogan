
/******************************************************************************
 * MODULE     : tt_file.hpp
 * DESCRIPTION: Finding a True Type font
 * COPYRIGHT  : (C) 2003  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TT_FILE_H
#define TT_FILE_H

#include "bitmap_font.hpp"
#include "url.hpp"

url        tt_font_path ();
array<url> tt_font_paths ();
url        tt_add_to_font_path (url u);
bool       tt_font_exists (string font_basename);
url        tt_font_find (string font_basename);
string     tt_find_name (string name, int size);

#ifdef USE_FREETYPE
font_glyphs tt_font_glyphs (string family, double size, int hdpi, int vdpi);
#endif // USE_FREETYPE

#endif // TT_FILE_H
