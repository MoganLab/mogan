
/******************************************************************************
 * MODULE     : learn_handwriting.cpp
 * DESCRIPTION: Facilities for learning handwriting
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "handwriting.hpp"

/******************************************************************************
 * Learning glyphs
 ******************************************************************************/

array<glyph_record> learned_glyphs;

void
clear_learned_glyphs () {
  learned_glyphs= array<glyph_record> ();
}

void
register_glyph (string name, contours gl) {
  glyph_record r;
  r.name= name;
  r.gl  = gl;
  invariants (gl, 1, r.disc1, r.cont1);
  invariants (gl, 2, r.disc2, r.cont2);
  r.hash1= hash (r.disc1);
  r.hash2= hash (r.disc2);
  learned_glyphs << r;
  // cout << "Added " << name << ", " << r.disc1 << "\n";
}
