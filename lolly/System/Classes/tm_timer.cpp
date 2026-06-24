
/******************************************************************************
 * MODULE     : timer.cpp
 * DESCRIPTION: timers
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tm_timer.hpp"
#include "basic.hpp"
#include "iterator.hpp"
#include "merge_sort.hpp"
#include "tm_ostream.hpp"

#include "tbox/tbox.h"

/******************************************************************************
 * Getting the time
 ******************************************************************************/
time_t
get_sec_time () {
  tb_timeval_t tp= {0};
  tb_gettimeofday (&tp, tb_null);
  return (time_t) tp.tv_sec;
}

time_t
get_usec_time () {
  tb_timeval_t tp= {0};
  tb_gettimeofday (&tp, tb_null);
  return (time_t) tp.tv_usec;
}

time_t
raw_time () {
  tb_timeval_t tp= {0};
  tb_gettimeofday (&tp, tb_null);
  return (time_t) ((tp.tv_sec * 1000) + (tp.tv_usec / 1000));
}

static time_t start_time= raw_time ();

time_t
texmacs_time () {
  tb_timeval_t tp= {0};
  tb_gettimeofday (&tp, tb_null);
  return ((time_t) ((tp.tv_sec * 1000) + (tp.tv_usec / 1000))) - start_time;
}
