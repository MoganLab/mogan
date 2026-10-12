
/******************************************************************************
 * MODULE     : space.hpp
 * DESCRIPTION: spacing
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef SPACE_H
#define SPACE_H
#include "tree.hpp"

class space_rep : concrete_struct {
public:
  DI min;
  DI def;
  DI max;

  space_rep (DI def);
  space_rep (DI min, DI def, DI max);

  friend class space;
};

class space {
  CONCRETE (space);
  space (DI def= 0);
  space (DI min, DI def, DI max);
  operator tree ();
  inline void operator+= (space spc) {
    rep->min+= spc->min;
    rep->def+= spc->def;
    rep->max+= spc->max;
  }
};
CONCRETE_CODE (space);

bool        operator== (space spc1, space spc2);
bool        operator!= (space spc1, space spc2);
tm_ostream& operator<< (tm_ostream& out, space spc);
space       copy (space spc);
space       operator+ (space spc1, space spc2);
space       operator- (space spc1, space spc2);
space       operator* (int i, space spc);
space       operator* (double x, space spc);
space       operator/ (space spc, int i);
space       operator/ (space spc, double x);
space       max (space spc1, space spc2);

#endif // defined SPACE_H
