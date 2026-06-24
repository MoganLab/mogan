
/** \file blackbox.hpp
 *  \copyright GPLv3
 *  \details defines a templated blackbox class representing an opaque pointer
 *  \author Joris van der Hoeven
 *  \date   2005
 */

#ifndef BLACKBOX_H
#define BLACKBOX_H
#include "basic.hpp"

/**
 * @brief A template class representing an opaque pointer.
 */
class blackbox_rep : public abstract_struct {
public:
  inline blackbox_rep () {}
  inline virtual ~blackbox_rep () {}
  virtual int         get_type ()              = 0;
  virtual bool        equal (blackbox_rep* ptr)= 0;
  virtual tm_ostream& display (tm_ostream& out)= 0;
};

class blackbox {
public:
  ABSTRACT_NULL (blackbox);
};
ABSTRACT_NULL_CODE (blackbox);

template <class T> class whitebox_rep : public blackbox_rep {
public:
  T data; /**< The data stored in the whitebox. */

public:
  /**
   * @brief Constructor.
   * @param data2 The data to be stored in the whitebox.
   */
  inline whitebox_rep (const T& data2) : data (data2) {}
  inline ~whitebox_rep () {}

  /**
   * @brief Get the type of the whitebox representation.
   * @return The type of the whitebox representation.
   */
  inline int get_type () { return type_helper<T>::id; }

  /**
   * @brief Check if the whitebox representation is equal to another
   * blackbox_rep pointer.
   * @param ptr A pointer to another blackbox_rep.
   * @return True if the whitebox representation is equal to the other
   * blackbox_rep, false otherwise.
   */
  inline bool equal (blackbox_rep* ptr) {
    return ptr != NULL && ptr->get_type () == type_helper<T>::id &&
           ((whitebox_rep<T>*) ptr)->data == data;
  }

  /**
   * @brief Display the whitebox representation.
   * @param out The output stream to display the whitebox representation.
   * @return The output stream after displaying the whitebox representation.
   */
  inline tm_ostream& display (tm_ostream& out) { return out << data; }
};

/**
 * @brief Equality operator for blackbox instances.
 * @param bb1 The first blackbox instance.
 * @param bb2 The second blackbox instance.
 * @return True if the blackbox instances are equal, false otherwise.
 */
inline bool
operator== (blackbox bb1, blackbox bb2) {
  if (is_nil (bb1)) return is_nil (bb2);
  else return bb1->equal (bb2.rep);
}

/**
 * @brief Inequality operator for blackbox instances.
 * @param bb1 The first blackbox instance.
 * @param bb2 The second blackbox instance.
 * @return True if the blackbox instances are not equal, false otherwise.
 */
inline bool
operator!= (blackbox bb1, blackbox bb2) {
  if (is_nil (bb1)) return !is_nil (bb2);
  else return !bb1->equal (bb2.rep);
}

/**
 * @brief Output stream operator for blackbox instances.
 * @param out The output stream.
 * @param bb The blackbox instance.
 * @return The output stream after displaying the blackbox instance.
 */
inline tm_ostream&
operator<< (tm_ostream& out, blackbox bb) {
  if (is_nil (bb)) return out << "nil";
  else return bb->display (out);
}

/**
 * @brief Get the type of the blackbox instance.
 * @param bb The blackbox instance.
 * @return The type of the blackbox instance.
 */
inline int
type_box (blackbox bb) {
  return is_nil (bb) ? 0 : bb->get_type ();
}

/**
 * @brief Create a blackbox instance with the given data.
 * @tparam T The type of data to be stored in the whitebox.
 * @param data The data to be stored in the whitebox.
 * @return The blackbox instance.
 */
template <class T>
blackbox
close_box (const T& data) {
  return tm_new<whitebox_rep<T>> (data);
}

/**
 * @brief Open the blackbox instance and retrieve the stored data.
 * @tparam T The type of data stored in the whitebox.
 * @param bb The blackbox instance.
 * @return The stored data.
 */
template <class T>
T
open_box (blackbox bb) {
  ASSERT (type_box (bb) == type_helper<T>::id, "type mismatch");
  return ((whitebox_rep<T>*) bb.rep)->data;
}

#endif // BLACKBOX_H
