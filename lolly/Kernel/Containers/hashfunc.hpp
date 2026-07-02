/** \file hashfunc.hpp
 *  \copyright GPLv3
 *  \details This hashfunc is a generic hash function class that utilizes a
 *           concrete implementation of a hash function for a given type.
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#ifndef HASHFUNC_H
#define HASHFUNC_H
#include "hashmap.hpp"

/**
 * @brief A concrete implementation of a hash function for a given type.
 * @tparam T The input type of the hash function.
 * @tparam U The output type of the hash function.
 */
template <class T, class U> class hashfunc_rep : public concrete_struct {
  U (*func) (T);          /**< A pointer to the hash function. */
  hashmap<T, U> remember; /**< A hash map to store remembered values. */
public:
  /**
   * @brief Constructor for hashfunc_rep.
   * @param func2 A pointer to the hash function to be used.
   * @param init The initial value for the hash map.
   */
  inline hashfunc_rep (U (*func2) (T), U init)
      : func (func2), remember (init) {}

  /**
   * @brief Applies the hash function to the given input.
   * @param x The input value to be hashed.
   * @return The output of the hash function for the given input.
   */
  U apply (T x);
};

/**
 * @brief A generic hash function class that uses a concrete implementation of a
 * hash function.
 * @tparam T The input type of the hash function.
 * @tparam U The output type of the hash function.
 */
template <class T, class U> class hashfunc {
  CONCRETE_TEMPLATE_2 (hashfunc, T, U);

  /**
   * @brief Constructor for hashfunc.
   * @param func A pointer to the hash function to be used.
   * @param init The initial value for the hash map.
   */
  inline hashfunc (U (*func) (T), U init)
      : rep (tm_new<hashfunc_rep<T, U>> (func, init)) {}

  /**
   * @brief Applies the hash function to the given input.
   * @param x The input value to be hashed.
   * @return The output of the hash function for the given input.
   */
  inline U operator[] (T x) { return rep->apply (x); }
};
CONCRETE_TEMPLATE_2_CODE (hashfunc, class, T, class, U);

#include "hashfunc.ipp"

#endif // defined HASHFUNC_H
