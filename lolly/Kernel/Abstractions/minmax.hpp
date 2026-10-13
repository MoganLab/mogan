/** \file minmax.hpp
 *  \copyright GPLv3
 *  \details defines basic type aliases and min/max function for these types.
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#pragma once

#include <cstdint>
#include <type_traits>

typedef char           QI;
typedef unsigned char  QN;
typedef short          HI;
typedef unsigned short HN;
typedef int            SI;
typedef unsigned int   SN;
typedef int64_t        DI;
typedef uint64_t       DN;

template <typename T, typename U>
inline auto
min (T i, U j) -> typename std::common_type<T, U>::type {
  return (i < j) ? i : j;
}

template <typename T, typename U>
inline auto
max (T i, U j) -> typename std::common_type<T, U>::type {
  return (i > j) ? i : j;
}
