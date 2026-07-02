/** \file list.hpp
 *  \copyright GPLv3
 *  \details linked lists with reference counting
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#ifndef LIST_H
#define LIST_H

#include "basic.hpp"

template <class T> class list_rep;
template <class T> class list;

/**
 * @brief Check if a list is nil (i.e., an empty list).
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list to be checked.
 * @return true if the list is nil, false otherwise.
 */
template <class T> bool is_nil (list<T> l);

/**
 * @brief Check if a list is an atom (i.e., a single item).
 *
 * @param l The list to be checked.
 * @return true if the list is an atom, false otherwise.
 */
template <class T> bool is_atom (list<T> l);

/**
 * @brief Check if two lists are strongly equal (i.e., have the same items in
 * the same order).
 *
 * @param l1 The first list to be compared.
 * @param l2 The second list to be compared.
 * @return true if the lists are strongly equal, false otherwise.
 */
template <class T> bool strong_equal (list<T> l1, list<T> l2);

/**
 * @brief The list class represents a linked list.
 *
 * @tparam T The type of the data stored in the list.
 */
template <class T> class list {
  CONCRETE_NULL_TEMPLATE (list, T);

  /**
   * @brief Construct a new list object with a single item.
   *
   * @param item The item to be stored in the list.
   */
  inline list (T item);

  /**
   * @brief Construct a new list object with an item and a pointer to the next
   * node.
   *
   * @param item The item to be stored in the list.
   * @param next A pointer to the next node in the list.
   */
  inline list (T item, list<T> next);

  /**
   * @brief Construct a new list object with two items and a pointer to the next
   * node.
   *
   * @param item1 The first item to be stored in the list.
   * @param item2 The second item to be stored in the list.
   * @param next A pointer to the next node in the list.
   */
  inline list (T item1, T item2, list<T> next);

  /**
   * @brief Construct a new list object with three items and a pointer to the
   * next node.
   *
   * @param item1 The first item to be stored in the list.
   * @param item2 The second item to be stored in the list.
   * @param item3 The third item to be stored in the list.
   * @param next A pointer to the next node in the list.
   */
  inline list (T item1, T item2, T item3, list<T> next);

  /**
   * @brief Overloaded subscript operator to access the item at a specific index
   * in the list.
   *
   * @param i The index of the item to be accessed.
   * @return T& A reference to the item at the specified index.
   */
  T& operator[] (int i);

  /**
   * @brief A static list object used for initializing new list objects.
   */
  static list<T> init;

  friend bool is_atom      LESSGTR (list<T> l);
  friend bool strong_equal LESSGTR (list<T> l1, list<T> l2);
};

extern int list_count;

/**
 * @brief The list_rep class represents a node in a linked list.
 *
 * @tparam T The type of the data stored in the list.
 */
template <class T> class list_rep : concrete_struct {
public:
  T       item; /**< The data stored in the node. */
  list<T> next; /**< A pointer to the next node in the list. */

  /**
   * @brief Construct a new list_rep object.
   *
   * @param item2 The data to be stored in this node.
   * @param next2 A pointer to the next node in the list.
   */
  inline list_rep<T> (T item2, list<T> next2) : item (item2), next (next2) {
    TM_DEBUG (list_count++);
  }

  /**
   * @brief Destroy the list_rep object.
   */
  inline ~list_rep<T> () { TM_DEBUG (list_count--); }
  friend class list<T>;
};

CONCRETE_NULL_TEMPLATE_CODE (list, class, T);
#define TMPL template <class T>
TMPL inline list<T>::list (T item)
    : rep (tm_new<list_rep<T>> (item, list<T> ())) {}
TMPL inline list<T>::list (T item, list<T> next)
    : rep (tm_new<list_rep<T>> (item, next)) {}
TMPL inline list<T>::list (T item1, T item2, list<T> next)
    : rep (tm_new<list_rep<T>> (item1, list<T> (item2, next))) {}
TMPL inline list<T>::list (T item1, T item2, T item3, list<T> next)
    : rep (tm_new<list_rep<T>> (item1, list<T> (item2, item3, next))) {}
TMPL inline bool
is_atom (list<T> l) {
  return (!is_nil (l)) && is_nil (l->next);
}
TMPL list<T> list<T>::init= list<T> ();

/**
 * @brief Get the number of items in a list.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list whose length is to be calculated.
 * @return int The number of items in the list.
 */
TMPL int N (list<T> l);

/**
 * @brief Create a copy of a list.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list to be copied.
 * @return list<T> A copy of the input list.
 */
TMPL list<T> copy (list<T> l);

/**
 * @brief Create a new list by appending an item to the end of an existing list.
 *
 * @tparam T The type of the data stored in the lists.
 * @param l1 The list to which the item will be appended.
 * @param x The item to be appended.
 * @return list<T> A new list consisting of the input list with the item
 * appended.
 */
TMPL list<T> operator* (list<T> l1, T x);

/**
 * @brief Create a new list by concatenating two existing lists.
 *
 * @tparam T The type of the data stored in the lists.
 * @param l1 The first list to be concatenated.
 * @param l2 The second list to be concatenated.
 * @return list<T> A new list consisting of the items in the input lists in
 * order.
 */
TMPL list<T> operator* (list<T> l1, list<T> l2);

/**
 * @brief Get the first n items of a list.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list whose items are to be retrieved.
 * @param n The number of items to retrieve (default is 1).
 * @return list<T> A new list consisting of the first n items of the input list.
 */
TMPL list<T> head (list<T> l, int n= 1);

/**
 * @brief Get all but the first n items of a list.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list whose items are to be retrieved.
 * @param n The number of items to skip (default is 1).
 * @return list<T> A new list consisting of all but the first n items of the
 * input list.
 */
TMPL list<T> tail (list<T> l, int n= 1);

/**
 * @brief Return the last item of the list.
 * The input list must not be an empty list.
 *
 * @tparam T
 * @param l the list
 * @return T
 */
TMPL T last_item (list<T> l);

/**
 * @brief Get a reference to the last item in a list.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list whose last item is to be accessed.
 * @return T& A reference to the last item in the list.
 */
TMPL T& access_last (list<T>& l);

/**
 * @brief Remove the last item from a list.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list from which the last item is to be removed.
 * @return list<T>& A reference to the modified list.
 */
TMPL list<T>& suppress_last (list<T>& l);

/**
 * @brief Create a new list with the items in reverse order.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list to be reversed.
 * @return list<T> A new list with the items in reverse order.
 */
TMPL list<T> reverse (list<T> l);

/**
 * @brief Create a new list with a specific item removed.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list from which the item is to be removed.
 * @param what The item to be removed.
 * @return list<T> A new list with the specified item removed.
 */
TMPL list<T> remove (list<T> l, T what);

/**
 * @brief Check if a list contains a specific item.
 *
 * @tparam T The type of the data stored in the list.
 * @param l The list to be searched.
 * * @param what The item to search for.
 * @return true if the item is found in the list, false otherwise.
 */
TMPL bool contains (list<T> l, T what);

TMPL tm_ostream& operator<< (tm_ostream& out, list<T> l);
TMPL list<T>& operator<< (list<T>& l, T item);
TMPL list<T>& operator<< (list<T>& l1, list<T> l2);
TMPL list<T>& operator>> (T item, list<T>& l);
TMPL list<T>& operator<< (T& item, list<T>& l);
TMPL bool     operator== (list<T> l1, list<T> l2);
TMPL bool     operator!= (list<T> l1, list<T> l2);
TMPL bool     operator< (list<T> l1, list<T> l2);
TMPL bool     operator<= (list<T> l1, list<T> l2);
#undef TMPL

#include "list.ipp"

#endif // defined LIST_H
