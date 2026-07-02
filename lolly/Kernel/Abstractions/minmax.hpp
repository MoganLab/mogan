/** \file minmax.hpp
 *  \copyright GPLv3
 *  \details defines basic type aliases and min/max function for these types.
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#pragma once

typedef int                    SI;
typedef unsigned int           SN;
typedef short                  HI;
typedef unsigned short         HN;
typedef char                   QI;
typedef unsigned char          QN;
typedef long long int          DI;
typedef unsigned long long int DN;

/**
 * @brief Returns the minimum of two signed integers.
 *
 * @param i The first integer.
 * @param j The second integer.
 * @return The smaller of the two integers.
 */
inline SI
min (SI i, SI j) {
  if (i < j) return i;
  else return j;
}

/**
 * @brief Returns the maximum of two signed integers.
 *
 * @param i The first integer.
 * @param j The second integer.
 * @return The larger of the two integers.
 */
inline SI
max (SI i, SI j) {
  if (i > j) return i;
  else return j;
}

/**
 * @brief Returns the minimum of two signed integers.
 *
 * @param i The first integer.
 * @param j The second integer.
 * @return The smaller of the two integers.
 */
inline DI
min (DI i, DI j) {
  if (i < j) return i;
  else return j;
}

/**
 * @brief Returns the maximum of two signed integers.
 *
 * @param i The first integer.
 * @param j The second integer.
 * @return The larger of the two integers.
 */
inline DI
max (DI i, DI j) {
  if (i > j) return i;
  else return j;
}

/**
 * @brief Returns the minimum of two doubles.
 *
 * @param i The first double.
 * @param j The second double.
 * @return The smaller of the two doubles.
 */
inline double
min (double i, double j) {
  if (i < j) return i;
  else return j;
}

/**
 * @brief Returns the maximum of two doubles.
 *
 * @param i The first double.
 * @param j The second double.
 * @return The larger of the two doubles.
 */
inline double
max (double i, double j) {
  if (i > j) return i;
  else return j;
}