/******************************************************************************
 * MODULE     : box_parameters.hpp
 * DESCRIPTION: value types describing ornament / art-box layout parameters
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef BOX_PARAMETERS_H
#define BOX_PARAMETERS_H
#include "array.hpp"
#include "basic.hpp"
#include "brush.hpp"
#include "tree.hpp"

class ornament_parameters_rep : concrete_struct {
public:
  tree         shape, tst;
  SI           lw, bw, rw, tw;
  double       lext, bext, rext, text;
  SI           lpad, bpad, rpad, tpad;
  SI           lcor, bcor, rcor, tcor;
  brush        bg, xc;
  array<brush> border;

  inline ornament_parameters_rep (tree shape2, tree tst2, SI lw2, SI bw2,
                                  SI rw2, SI tw2, double lx, double bx,
                                  double rx, double tx, SI lpad2, SI bpad2,
                                  SI rpad2, SI tpad2, SI lcor2, SI bcor2,
                                  SI rcor2, SI tcor2, brush bg2, brush xc2,
                                  array<brush> border2)
      : shape (shape2), tst (tst2), lw (lw2), bw (bw2), rw (rw2), tw (tw2),
        lext (lx), bext (bx), rext (rx), text (tx), lpad (lpad2), bpad (bpad2),
        rpad (rpad2), tpad (tpad2), lcor (lcor2), bcor (bcor2), rcor (rcor2),
        tcor (tcor2), bg (bg2), xc (xc2), border (border2) {}
  friend class ornament_parameters;
};

class ornament_parameters {
  CONCRETE (ornament_parameters);
  inline ornament_parameters (tree shape2, tree tst2, SI lw2, SI bw2, SI rw2,
                              SI tw2, double lx, double bx, double rx,
                              double tx, SI lpad2, SI bpad2, SI rpad2, SI tpad2,
                              SI lcor2, SI bcor2, SI rcor2, SI tcor2, brush bg2,
                              brush xc2, array<brush> border2)
      : rep (tm_new<ornament_parameters_rep> (
            shape2, tst2, lw2, bw2, rw2, tw2, lx, bx, rx, tx, lpad2, bpad2,
            rpad2, tpad2, lcor2, bcor2, rcor2, tcor2, bg2, xc2, border2)) {}
};
CONCRETE_CODE (ornament_parameters);

inline ornament_parameters
copy (ornament_parameters ps) {
  return ornament_parameters (
      ps->shape, ps->tst, ps->lw, ps->bw, ps->rw, ps->tw, ps->lext, ps->bext,
      ps->rext, ps->text, ps->lpad, ps->bpad, ps->rpad, ps->tpad, ps->lcor,
      ps->bcor, ps->rcor, ps->tcor, ps->bg, ps->xc, ps->border);
}

class art_box_parameters_rep : concrete_struct {
public:
  tree data;
  SI   lpad, bpad, rpad, tpad;

  inline art_box_parameters_rep (tree data2, SI lpad2, SI bpad2, SI rpad2,
                                 SI tpad2)
      : data (data2), lpad (lpad2), bpad (bpad2), rpad (rpad2), tpad (tpad2) {}
  friend class art_box_parameters;
};

class art_box_parameters {
  CONCRETE (art_box_parameters);
  inline art_box_parameters (tree data2, SI lpad2, SI bpad2, SI rpad2, SI tpad2)
      : rep (tm_new<art_box_parameters_rep> (data2, lpad2, bpad2, rpad2,
                                             tpad2)) {}
};
CONCRETE_CODE (art_box_parameters);

inline art_box_parameters
copy (art_box_parameters ps) {
  return art_box_parameters (ps->data, ps->lpad, ps->bpad, ps->rpad, ps->tpad);
}

#endif // defined BOX_PARAMETERS_H
