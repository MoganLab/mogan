
/** \file array.hpp
 *  \copyright GPLv3
 *  \details defines a templated array class representing a fixed-size array of
             elements.
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#ifndef ARRAY_H
#define ARRAY_H
#include "basic.hpp"

/**
 * @brief A template class representing an array.
 *
 * @tparam T Type of elements in the array.
 */
template <class T> class array;

/**
 * @brief Get the length of the array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The array.
 * @return   The length of the array.
 */
template <class T> int N (array<T> a);

/**
 * @brief Get a pointer to the first element of the array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The array.
 * @return   A pointer to the first element of the array.
 */
template <class T> T* A (array<T> a);

/**
 * @brief Make a copy of an array.
 *
 * @tparam T Type of elements in the array.
 * @param  x The array to be copied.
 * @return   A copy of the array.
 */
template <class T> array<T> copy (array<T> x);

/**
 * @brief A concrete struct representing the implementation of an array.
 *
 * @tparam T Type of elements in the array.
 */
template <class T> class array_rep : concrete_struct {
  int n;
  T*  a;

public:
  /**
   * @brief Construct a new array representation object with length 0.
   */
  inline array_rep () : n (0), a (NULL) {}

  /**
   * @brief Construct a new array representation object with specified length.
   *
   * @param n The length of the array.
   */
  array_rep (int n);

  /**
   * @brief Destroy the array representation object.
   */
  inline ~array_rep () {
    if (n != 0) tm_delete_array (a);
  }

  /**
   * @brief Resize the array length to n
   *
   * @param n
   */
  void resize (int n);

  friend class array<T>;
  friend int N         LESSGTR (array<T> a);
  friend T* A          LESSGTR (array<T> a);
  friend array<T> copy LESSGTR (array<T> a);
};

/**
 * @brief A template class representing an array.
 *
 * @tparam T Type of elements in the array.
 */
template <class T> class array {
  CONCRETE_TEMPLATE (array, T);

  /**
   * @brief Construct a new array object with length 0.
   */
  inline array (int n= 0) : rep (tm_new<array_rep<T>> (n)) {}

  /**
   * @brief Construct a new array object from a C array with specified length.
   *
   * @tparam T Type of elements in the array.
   * @param  a The C array.
   * @param  n The length of the C array.
   */
  array (T* a, int n);

  /**
   * @brief Construct a new array object with two elements.
   *
   * @tparam T Type of elements in the array.
   * @param  x1 The first element.
   * @param  x2 The second element.
   */
  array (T x1, T x2);

  /**
   * @brief Construct a new array object with three elements.
   *
   * @tparam T Type of elements in the array.
   * @param  x1 The first element.
   * @param  x2 The second element.
   * @param  x3 The third element.
   */
  array (T x1, T x2, T x3);

  /**
   * @brief Construct a new array object with four elements.
   *
   * @tparam T Type of elements in the array.
   * @param  x1 The first element.
   * @param  x2 The second element.
   * @param  x3 The third element.
   * @param  x4 The fourth element.
   */
  array (T x1, T x2, T x3, T x4);

  /**
   * @brief Construct a new array object with five elements.
   *
   * @tparam T Type of elements in the array.
   * @param  x1 The first element.
   * @param  x2 The second element.
   * @param  x3 The third element.
   * @param  x4 The fourth element.
   * @param  x5 The fifth element.
   */
  array (T x1, T x2, T x3, T x4, T x5);

  /**
   * @brief Get a reference to the element atthe specified index.
   *
   * @tparam T Type of elements in the array.
   * @param  i The index of the element.
   * @return   A reference to the element at the specified index.
   */
  inline T& operator[] (int i) { return rep->a[i]; }

  T* begin () { return rep->a; }
  T* end () { return rep->a + rep->n; }
};
CONCRETE_TEMPLATE_CODE (array, class, T);

#define TMPL template <class T>
TMPL inline int
N (array<T> a) {
  return a->n;
}
TMPL inline T*
A (array<T> a) {
  return a->a;
}
TMPL inline array<T>
copy (array<T> a) {
  return array<T> (a->a, a->n);
}

/**
 * @brief Output the array to an output stream.
 *
 * @tparam T Type of elements in the array.
 * @param  out The output stream.
 * @param  a   The array to be output.
 * @return     The output stream.
 */
TMPL tm_ostream& operator<< (tm_ostream& out, array<T> a);

/**
 * @brief Append an element to the end of the array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The array.
 * @param  x The element to be appended.
 * @return   The updated array.
 */
TMPL array<T>& operator<< (array<T>& a, T x);

/**
 * @brief Append an array to the end of another array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The first array.
 * @param  b The second array.
 * @return   The updated array.
 */
TMPL array<T>& operator<< (array<T>& a, array<T> b);

/**
 * @brief Check if an array contains a specified element.
 *
 * @tparam T Type of elements in the array.
 * @param  a The array.
 * @param  b The element to be checked.
 * @return   True if the array contains the element, false otherwise.
 */
TMPL bool contains (T a, array<T> b);

/**
 * @brief Append an element to the beginning of an array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The array.
 * @param  b The element to be appended.
 * @return   The updated array.
 */
TMPL array<T> append (T a, array<T> b);

/**
 * @brief Append an array to the end of another array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The first array.
 * @param  b The second array.
 * @return   The updated array.
 */
TMPL array<T> append (array<T> a, array<T> b);

/**
 * @brief Get a subarray of an array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The array.
 * @param  i The start index of the subarray.
 * @param  j The end index of the subarray.
 * @return   The subarray.
 */
TMPL array<T> range (array<T> a, int i, int j);

/**
 * @brief Reverse an array.
 *
 * @tparam T Type of elements in the array.
 * @param  a The array.
 * @return   The reversed array.
 */
TMPL array<T> reverse (array<T> a);

/**
 * @brief Check if two arrays are equal.
 *
 * @tparam T Type of elements in the arrays.
 * @param  a The first array.
 * @param  b The second array.
 * @return   True if the two arrays are equal, false otherwise.
 */
TMPL bool operator== (array<T> a, array<T> b);

/**
 * @brief Check if two arrays are not equal.
 *
 * @tparam T Type of elements in the arrays.
 * @param  a The first array.
 * @param b The second array.
 * @return True if the two arrays are not equal, false otherwise.
 */
TMPL bool operator!= (array<T> a, array<T> b);

/**
 * @brief Multiply an array object by a scalar value of type T.
 *
 * @param a The target array.
 * @param c The scalar value.
 * @return  Return a new array object whose elements are the elements of a
 * multiplied by c.
 */
TMPL array<T> operator* (array<T> a, T c);

/**
 * @brief Divide an array object by a scalar value of type T.
 *
 * @param a The target array.
 * @param c The scalar value.
 * @return Return a new array object whose elements are the elements of a
 * divided by c.
 */
TMPL array<T> operator/ (array<T> a, T c);

/**
 * @brief This function computes the hash of an array object a.
 *
 * @param a The array object.
 * @return Return an integer hash value.
 */
TMPL int hash (array<T> a);
#undef TMPL

#ifdef STRING_H
// Function parameters participating in template signature resolution are never
// cast implicitely.
inline array<string>&
operator<< (array<string>& a, char* x) {
  return a << string (x);
}
#endif

#include "array.ipp"

#endif // defined ARRAY_H
