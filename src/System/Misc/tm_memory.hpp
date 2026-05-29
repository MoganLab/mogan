
/******************************************************************************
 * MODULE     : tm_memory.hpp
 * DESCRIPTION: Runtime memory monitoring utilities
 * COPYRIGHT  : (C) 2026  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TM_MEMORY_H
#define TM_MEMORY_H

#include "string.hpp"

/**
 * @brief Returns the current Resident Set Size (RSS) in kilobytes.
 *
 * On Linux, reads VmRSS from /proc/self/status.
 * Returns 0 if the value cannot be determined.
 */
int64_t get_rss ();

/**
 * @brief Returns a human-readable string with current memory usage info.
 *
 * Includes RSS andVmSize where available.
 */
string memory_info_string ();

#endif // defined TM_MEMORY_H
