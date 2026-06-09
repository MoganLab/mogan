/*
** Denis RAUX - LIX 2013
** avoid calling external lib in case of static functions
*/
#include <stdlib.h>

#if defined(__TMSTATIC__) && !defined(OS_MINGW) && !defined(OS_WIN)
void*
dlopen (const char* name, int flag) {
  return (NULL);
}
#endif
