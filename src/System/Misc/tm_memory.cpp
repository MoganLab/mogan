
/******************************************************************************
 * MODULE     : tm_memory.cpp
 * DESCRIPTION: Runtime memory monitoring utilities (Linux only)
 * COPYRIGHT  : (C) 2026  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tm_memory.hpp"

#include "analyze.hpp"
#include "tm_sys_utils.hpp"

#include <cstdio>

int64_t
get_rss () {
#ifdef OS_GNU_LINUX
  FILE* f= fopen ("/proc/self/status", "r");
  if (!f) return 0;

  int64_t rss= 0;
  char    line[256];
  while (fgets (line, sizeof (line), f)) {
    if (line[0] == 'V' && line[1] == 'm' && line[2] == 'R' && line[3] == 'S' &&
        line[4] == 'S' && line[5] == ':') {
      const char* p= line + 6;
      while (*p == ' ' || *p == '\t')
        p++;
      rss= atoll (p);
      break;
    }
  }
  fclose (f);
  return rss;
#else
  return 0;
#endif
}

string
memory_info_string () {
  string r;
#ifdef OS_GNU_LINUX
  FILE* f= fopen ("/proc/self/status", "r");
  if (!f) {
    r << "RSS: " << as_string (get_rss ()) << " kB";
    return r;
  }

  char line[256];
  while (fgets (line, sizeof (line), f)) {
    string sl (line);
    if (starts (sl, "VmRSS:") || starts (sl, "VmSize:") ||
        starts (sl, "VmPeak:") || starts (sl, "VmSwap:")) {
      if (N (sl) > 0 && sl[N (sl) - 1] == '\n') sl= sl (0, N (sl) - 1);
      if (starts (sl, "Vm")) sl= sl (2, N (sl));
      r << sl;
      if (N (r) > 0 && r[N (r) - 1] != '\n') r << "\n";
    }
  }
  fclose (f);
#else
  r << "RSS: " << as_string (get_rss ()) << " kB (Linux only)";
#endif
  return r;
}
