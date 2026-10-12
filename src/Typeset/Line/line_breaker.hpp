/******************************************************************************
 * MODULE     : line_breaker.hpp
 * DESCRIPTION: Line breaking facility for paragraphs
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven, 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LINE_BREAKER_HPP
#define LINE_BREAKER_HPP

#include "Format/line_item.hpp"
#include "path.hpp"

array<path> line_breaks (array<line_item> a, int start, int end, SI line_width,
                         SI large_width, SI first_spc, SI last_spc,
                         bool ragged= false);

#endif // LINE_BREAKER_HPP
