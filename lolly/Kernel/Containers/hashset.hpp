
/**
 * \file      hashset.h
 * \details   A simple implementation of a hash set.
 * \copyright GPLv3
 * \author    Joris van der Hoeven
 * \date      1999
 */

#ifndef HASHSET_H
#define HASHSET_H
#include "list.hpp"

template <class T> class hashset;
template <class T> class hashset_iterator_rep;
template <class T> int         N (hashset<T> h);
template <class T> tm_ostream& operator<< (tm_ostream& out, hashset<T> h);
template <class T> bool        operator<= (hashset<T> h1, hashset<T> h2);
template <class T> hashset<T>  copy (hashset<T> h);

/**
 * @brief The hashset_rep class represents an entry in a hash set.
 *
 * @tparam T The type of the data stored in the hash set.
 */
template <class T> class hashset_rep : concrete_struct {
  int      size; /**< The size of the hash set (number of entries). */
  int      n;    /**< The number of keys (a power of two). */
  int      max;  /**< The mean number of entries per key. */
  list<T>* a;    /**< The array of entries. */

public:
  /**
   * @brief Construct a new hashset_rep object with default values.
   *
   */
  inline hashset_rep ()
      : size (0), n (1), max (1), a (tm_new_array<list<T>> (1)) {}

  /**
   * @brief Construct a new hashset_rep object with specified values.
   *
   * @param n2 The number of keys (a power of two).
   * @param max2 The mean number of entries per key.
   */
  inline hashset_rep (int n2, int max2= 1)
      : size (0), n (n2), max (max2), a (tm_new_array<list<T>> (n)) {}

  /**
   * @brief Destroy the hashset_rep object.
   */
  inline ~hashset_rep () { tm_delete_array (a); }

  bool contains (T x);
  void resize (int n);
  void insert (T x);
  void remove (T x);

  friend class hashset<T>;
  friend int N LESSGTR (hashset<T> h);
  friend tm_ostream& operator<< LESSGTR (tm_ostream & out, hashset<T> h);
  friend bool            operator<= LESSGTR (hashset<T> h1, hashset<T> h2);
  friend hashset<T> copy LESSGTR (hashset<T> h);
  friend class hashset_iterator_rep<T>;
};

/**
 * @brief The hashset class represents a hash set.
 *
 * @tparam T The type of the data stored in the hash set.
 */
template <class T> class hashset {
  CONCRETE_TEMPLATE (hashset, T);

  /**
   * @brief Construct a new hashset object with specified values.
   *
   * @param n The number of keys (a power of two).
   * @param max The mean number of entries per key.
   */
  inline hashset (int n= 1, int max= 1)
      : rep (tm_new<hashset_rep<T>> (n, max)) {}
};
CONCRETE_TEMPLATE_CODE (hashset, class, T);

/**
 * @brief Get the number of entries in a hash set.
 *
 * @tparam T The type of the data stored in the hash set.
 * @param h The hash set to query.
 * @return int The number of entries in the hash set.
 */
template <class T>
inline int
N (hashset<T> h) {
  return h->size;
}

/**
 * @brief Equality comparison operator for hash sets.
 *
 * @tparam T The type of the data stored in the hash sets.
 * @param h1 The first hash set to be compared.
 * @param h2 The second hash set to be compared.
 * @return true if the hash sets are equal, false otherwise.
 */
template <class T> bool operator== (hashset<T> h1, hashset<T> h2);

/**
 * @brief Less-than-or-equal-to comparison operator for hash sets.
 *
 * @tparam T The type of the data stored in the hash sets.
 * @param h1 The first hash set to be compared.
 * @param h2 The second hash set to be compared.
 * @return true if the first hash set is less than or equal to the second hash
 * set, false otherwise.
 */
template <class T> bool operator<= (hashset<T> h1, hashset<T> h2);

/**
 * @brief Less-than comparison operator for hash sets.
 *
 * @tparam T The type of the data stored in the hash sets.
 * @param h1 The first hash set to be compared.
 * @param h2 The second hash set to be compared.
 * @return true if the first hash set is less than the second hash set, false
 * otherwise.
 */
template <class T> bool operator< (hashset<T> h1, hashset<T> h2);

/**
 * @brief Insert an item into a hash set.
 *
 * @tparam T The type of the data stored in the hash set * @param h The hash set
 * to insert into.
 * @param x The item to insert.
 * @return hashset<T>& A reference to the modified hash set.
 */
template <class T> hashset<T>& operator<< (hashset<T>& h, T x);

#include "hashset.ipp"

#endif // defined HASHSET_H
