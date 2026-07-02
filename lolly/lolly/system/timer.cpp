
/******************************************************************************
 * MODULE     : timer.cpp
 * DESCRIPTION: timer related routines
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "timer.hpp"
#include "array.hpp"
#include "hashmap.hpp"
#include "iterator.hpp"
#include "merge_sort.hpp"
#include "tm_timer.hpp"

namespace lolly {
namespace system {

static hashmap<string, uint32_t> timing_level (0);
static hashmap<string, uint32_t> timing_nr (0);
static hashmap<string, uint32_t> timing_cumul (0);
static hashmap<string, uint32_t> timing_last (0);

/******************************************************************************
 * Routines for benchmarking
 ******************************************************************************/

void
timer_start (string task) {
  // start timer for a given type of task
  if (timing_level[task] == 0) timing_last (task)= (uint32_t) texmacs_time ();
  timing_level (task)++;
}

void
timer_cumul (string task) {
  // end timer for a given type of task, but don't reset timer
  timing_level (task)--;
  if (timing_level[task] == 0) {
    uint32_t ms= ((uint32_t) texmacs_time ()) - timing_last (task);
    timing_nr (task)++;
    timing_cumul (task)+= ms;
    timing_last->reset (task);
  }
}

void
timer_reset (string task) {
  // reset timer for a given type of task
  timing_level->reset (task);
  timing_nr->reset (task);
  timing_cumul->reset (task);
  timing_last->reset (task);
}

void
bench_print (tm_ostream& ostream, string task, uint32_t threshold) {
  uint32_t elapse_time= timing_cumul[task];
  if (elapse_time < threshold) return;

  uint32_t nr= timing_nr[task];
  ostream << "Task '" << task << "' took " << elapse_time << " ms";
  if (nr > 1) ostream << " (" << nr << " invocations)";
  ostream << LF;
}

static array<string>
collect (hashmap<string, uint32_t> h) {
  array<string>    a;
  iterator<string> it= iterate (h);
  while (it->busy ())
    a << it->next ();
  merge_sort (a);
  return a;
}

void
bench_print (tm_ostream& ostream) {
  // print timings for all types of tasks
  array<string> a= collect (timing_cumul);
  int           i, n= N (a);
  for (i= 0; i < n; i++)
    bench_print (ostream, a[i]);
}

} // namespace system
} // namespace lolly
