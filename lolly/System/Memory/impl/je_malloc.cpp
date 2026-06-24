
/******************************************************************************
 * MODULE     : Fast memory allocation using jemalloc
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2023-2024  jingkaimori
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "assert.h"
#include "fast_alloc.hpp"
#include "tm_ostream.hpp"
#include <jemalloc/jemalloc.h>

int mem_used ();

/*****************************************************************************/
// General purpose fast allocation routines
/*****************************************************************************/

void*
safe_malloc (size_t sz) {
  void* ptr= malloc (sz);

  if (ptr == NULL) {
    cerr << "Fatal error: out of memory\n";
    abort ();
  }
  return ptr;
}

void*
fast_alloc (size_t sz) {
  return safe_malloc (sz);
}

void
fast_free (void* ptr, size_t sz) {
  free (ptr);
}

void*
fast_realloc (void* ptr, size_t, size_t new_size) {
  void* new_ptr= realloc (ptr, new_size);

  if (new_ptr == NULL) {
    cerr << "Fatal error: out of memory\n";
    abort ();
  }
  return new_ptr;
}

void*
fast_new (size_t s) {
  return safe_malloc (s);
}

void
fast_delete (void* ptr) {
  free (ptr);
}

void
mem_init () {}

/******************************************************************************
 * Statistics
 ******************************************************************************/

int
mem_used () {
  cerr << "memory statistics is NOT IMPLEMENTED\n";
  return 0;
}

void
mem_info () {
  cout << "\n------- (NOT IMPLEMENTED) memory statistics -------\n";
}
