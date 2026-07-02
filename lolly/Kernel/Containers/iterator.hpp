
/** \file iterator.hpp
 *  \copyright GPLv3
 *  \details dynamic iterators
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#ifndef ITERATOR_H
#define ITERATOR_H
#include "hashmap.hpp"
#include "hashset.hpp"

extern int iterator_count;

/**
 * @class iterator_rep
 * @brief An abstract base class for iterator implementation.
 * @tparam T The type for the iterator's return value.
 */
template <class T> class iterator_rep : public abstract_struct {
public:
  /**
   * @brief Constructor. Increments the iterator count.
   */
  inline iterator_rep<T> () { TM_DEBUG (iterator_count++); }

  /**
   * @brief Destructor. Decrements the iterator count.
   */
  inline virtual ~iterator_rep<T> () { TM_DEBUG (iterator_count--); }

  /**
   * @brief Returns whether the iterator has another element.
   * @return True if the iterator has another element, false otherwise.
   */
  virtual bool busy ()= 0;

  /**
   * @brief Returns the current element of the iterator, and move the iterator
   * forward
   * @return The current element of the iterator.
   */
  virtual T next ()= 0;

  /**
   * @brief Returns the current element of the iterator.
   * @return The current element of the iterator.
   */
  virtual T current ()= 0;

  /**
   * @brief Increase the iterator
   */
  virtual void increase ()= 0;

  /**
   * @brief Returns the number of elements remaining in the iterator.
   * @return The number of elements remaining in the iterator.
   */
  virtual int remains ();
};

/**
 * @class iterator
 * @brief A template class for iterators.
 * @tparam T The type for the iterator's return value.
 */
template <class T> struct iterator {
  ABSTRACT_TEMPLATE (iterator, T);
  iterator<T>    begin () { return *this; };
  std::nullptr_t end () { return nullptr; };
  void           operator++ () { this->rep->increase (); };
  /**
   * @brief Returns the current element of the iterator, then move the iterator
   * to the next element.
   */
  T    operator* () { return this->rep->current (); };
  bool operator!= (std::nullptr_t) { return this->rep->busy (); }
};
ABSTRACT_TEMPLATE_CODE (iterator, class, T);

/**
 * @brief Outputs the iterator object to an output stream.
 * @tparam T The type for the iterator's return value.
 * @param out The output stream.
 * @param it The iterator object.
 * @return The output stream.
 */
template <class T> tm_ostream& operator<< (tm_ostream& out, iterator<T> it);

/**
 * @brief Generates an iterator for a container of type hashmap<T, U>.
 * @tparam T The type of the elements in the container.
 * @tparam U The type of the values in the hashmap.
 * @param h The hashmap container.
 * @return An iterator object for the hashmap container.
 */
template <class T, class U> iterator<T> iterate (hashmap<T, U> h);

/**
 * @brief Generates an iterator for a container of type hashset<T>.
 * @tparam T The type of the elements in the container.
 * @param h The hashset container.
 * @return An iterator object for the hashset container.
 */
template <class T> iterator<T> iterate (hashset<T> h);

#include "iterator.ipp"

#endif // defined ITERATOR_H
