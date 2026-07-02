
/** \file basic.hpp
 *  \copyright GPLv3
 *  \details defines basic types and macros used throughout the kernel.
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#ifndef BASIC_H
#define BASIC_H

#include "classdef.hpp"
#include "fast_alloc.hpp"
#include "minmax.hpp"
#include "tm_ostream.hpp"
#include <math.h>

/**
 * @brief Macro used for template specialization of less than and greater than
 * operators.
 */
#define LESSGTR <>

typedef void*        pointer;
typedef unsigned int color;

#define MAX_SI 0x7fffffff

/**
 * @brief Macro defining the minimum value of a signed integer.
 */
#define MIN_SI 0x80000000

/******************************************************************************
 * debugging
 ******************************************************************************/

#if (defined __SUNPRO_CC || defined __clang__ || defined(_WIN32) ||            \
     defined(_WIN64))
#define STACK_NEW_ARRAY(name, T, size) T* name= tm_new_array<T> (size)
#define STACK_DELETE_ARRAY(name) tm_delete_array (name)
#else
#define STACK_NEW_ARRAY(name, T, size) T name[size]
#define STACK_DELETE_ARRAY(name)
#endif

/**
 * @brief Macro used to enable or disable the use of exceptions.
 */
#define USE_EXCEPTIONS

/**
 * @brief Function used to handle a TM failure by displaying an error message
 * and exiting the program.
 *
 * @param msg The error message to display.
 */
void tm_failure (const char* msg);
#ifdef USE_EXCEPTIONS

/**
 * @brief Global variable used to store the current exception message.
 */
extern string the_exception;

/**
 * @brief Function used to throw an exception with a specified error message.
 *
 * @param msg The error message to throw with the exception.
 */
void tm_throw (const char* msg);

/**
 * @brief Function used to handle exceptions by displaying and clearing the
 * exception message.
 * @note if catching an exception throwed by \ref tm_throw, this function must
 * be appended after catch(){...} block.
 */
void handle_exceptions ();

/**
 * @brief Macro used to assert that a condition is true, and throw an exception
 * with an error message if the condition is false.
 */
#define ASSERT(cond, msg)                                                      \
  {                                                                            \
    if (!(cond)) tm_throw (msg);                                               \
  }

/**
 * @brief Macro used to throw an exception with a specified error message.
 */
#define TM_FAILED(msg)                                                         \
  {                                                                            \
    tm_throw (msg);                                                            \
  }
#else
#ifdef DEBUG_ASSERT
#include <assert.h>

/**
 * @brief Macro used to assert that a condition is true, and print an error
 * message and exit the program if the condition is false.
 */
#define ASSERT(cond, msg)                                                      \
  {                                                                            \
    if (!(cond)) {                                                             \
      tm_failure (msg);                                                        \
      assert (cond);                                                           \
    }                                                                          \
  }

/**
 * @brief Macro used to print an error message and exit the program.
 */
#define TM_FAILED(msg)                                                         \
  {                                                                            \
    tm_failure (msg);                                                          \
    assert (false);                                                            \
  }
#else

/**
 * @brief Macro used to assert that a condition is true, and print an error
 * message and exit the program if the condition is false.
 */
#define ASSERT(cond, msg)                                                      \
  {                                                                            \
    if (!(cond)) {                                                             \
      tm_failure (msg);                                                        \
    }                                                                          \
  }

/**
 * @brief Macro used to print an error message and exit the program.
 */
#define TM_FAILED(msg)                                                         \
  {                                                                            \
    tm_failure (msg);                                                          \
  }
#endif
#endif

/******************************************************************************
 * miscellaneous routines
 ******************************************************************************/

/**
 * @brief Hashes an integer.
 *
 * @param i The integer to hash.
 * @return The hashed value of the integer.
 */
inline int
hash (int i) {
  return i;
}

/**
 * @brief Hashes a long integer.
 *
 * @param i The long integer to hash.
 * @return The hashed value of the long integer.
 */
inline int
hash (long int i) {
  return (int) i;
}

/**
 * @brief Hashes a double integer.
 *
 * @param i The double integer to hash.
 * @return The hashed value of the double integer.
 */
inline int
hash (DI i) {
  return (int) i;
}

/**
 * @brief Hashes an unsigned integer.
 *
 * @param i The unsigned integer to hash.
 * @return The hashed value of the unsigned integer.
 */
inline int
hash (unsigned int i) {
  return i;
}

/**
 * @brief Hashes an unsigned long integer.
 *
 * @param i The unsigned long integer to hash.
 * @return The hashed value of the unsigned long integer.
 */
inline int
hash (unsigned long int i) {
  return (int) i;
}

/**
 * @brief Hashes a double number.
 *
 * @param x The double number to hash.
 * @return The hashed value of the double number.
 */
inline int
hash (DN i) {
  return (int) i;
}

/**
 * @brief Hashes a float number.
 *
 * @param x The float numberto hash.
 * @return The hashed value of the float number.
 */
inline int
hash (float x) {
  union {
    int   n;
    float d;
  } u;
  u.d= x;
  return u.n & 0xffffffff;
}

/**
 * @brief Hashes a double number.
 *
 * @param x The double number to hash.
 * @return The hashed value of the double number.
 */
inline int
hash (double x) {
  union {
    DI     n;
    double d;
  } u;
  u.d= x;
  return (int) (u.n ^ (u.n >> 32));
}

/**
 * @brief Returns a copy of an integer.
 *
 * @param x The integer to copy.
 * @return A copy of the integer.
 */
inline int
copy (int x) {
  return x;
}

/**
 * @brief Converts a double to a signed integer, rounding to the nearest
 * integer.
 *
 * @param x The double to convert.
 * @return The converted signed integer.
 */
inline SI
as_int (double x) {
  return (SI) floor (x + 0.5);
}

/**
 * @brief Rounds a double to the nearest integer.
 *
 * @param x The double to round.
 * @return The rounded double.
 */
inline double
tm_round (double x) {
  return floor (x + 0.5);
}

/**
 * @brief Computes a hash value for a pointer.
 *
 * @param ptr The pointer to hash.
 * @return The hash value.
 */
int hash (pointer ptr);

/**
 * @brief Enumeration of display control options.
 */
enum display_control { INDENT, UNINDENT, HRULE, LF };

/**
 * @brief Output operator for display control options.
 *
 * @param out The output stream to write to.
 * @param ctrl The display control option.
 * @return The output stream.
 */
tm_ostream& operator<< (tm_ostream& out, display_control ctrl);

/**
 * @brief Helper struct for type identification and initialization.
 *
 * @tparam T The type to help identify and initialize.
 */
template <typename T> struct type_helper {
  static int      id;   ///< The type identifier.
  static T        init; ///< The initialized value of the type.
  static inline T init_val () {
    return T ();
  } ///< Returns the initialized value of the type.
};

/**
 * @brief Generates a new type identifier.
 *
 * @return The new type identifier.
 */
int new_type_identifier ();

/**
 * @brief Initializes the type identifier and initialized value for a
 * giventemplate type T.
 *
 * @tparam T The type to initialize.
 */
template <typename T> int type_helper<T>::id= new_type_identifier ();
///< The type identifier for type T.
template <typename T> T type_helper<T>::init= T ();
///< The initialized value for type T.

#endif // defined BASIC_H
