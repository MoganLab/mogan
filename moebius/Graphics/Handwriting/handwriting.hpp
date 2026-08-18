
/******************************************************************************
 * MODULE     : handwriting.hpp
 * DESCRIPTION: Facilities for handwriting
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "poly_line.hpp"

extern array<contours>      learned_glyphs;
extern array<string>        learned_names;
extern array<array<tree>>   learned_disc1;
extern array<array<double>> learned_cont1;
extern array<array<tree>>   learned_disc2;
extern array<array<double>> learned_cont2;
extern array<int>           learned_hash1;
extern array<int>           learned_hash2;

void   clear_learned_glyphs ();
void   register_glyph (string name, contours gl);
void   recognize_glyph_one (contours gl, int& level, string& best,
                            double& best_rec);
string recognize_glyph (contours gl);

array<point> simplify (array<point> a, double eps, double thr);
