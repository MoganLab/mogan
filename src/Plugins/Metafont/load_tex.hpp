
/******************************************************************************
 * MODULE     : load_tex.hpp
 * DESCRIPTION: Loading TeX font metrics and glyphs
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LOAD_TEX_H
#define LOAD_TEX_H
#include "load_pk.hpp"
#include "load_tfm.hpp"

void setup_tex ();
void init_tex ();
void load_tex (string family, double size, int dpi, int dsize,
               tex_font_metric& tfm, font_glyphs& pk);

double to_tex_font_size (double sz);

#endif // defined LOAD_TEX_H
