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
template <class T, class U> class hashmap_rep;
template <class T> class hashset_rep;

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

  // resize 时重挂桶内节点,需直接访问 rep 做所有权转移
  template <class T2, class U2> friend class hashmap_rep;
  template <class T2> friend class hashset_rep;

  /**
   * @brief 把 node 重挂到 dst 链头(所有权转移,node 仍被原持有者引用)。
   * @note 句柄赋值会释放 node 对原后继的引用;ref_count++ 补偿 dst.rep
   * 裸赋值新增的引用。hashmap/hashset resize 搬桶时使用。
   */
  static void rehang (list<T>& dst, list_rep<T>* node) {
    node->next= dst;
    node->ref_count++;
    dst.rep= node;
  }

  /**
   * @brief 把新建节点(ref_count==1)挂到 dst 链头,所有权让渡给 dst。
   * @note dst 对旧头部的引用原样转由 node 持有,不经引用计数。
   * hashmap/hashset 插入新节点时使用。
   */
  static void adopt (list<T>& dst, list_rep<T>* node) {
    node->next.rep= dst.rep;
    dst.rep       = node;
  }

  /**
   * @brief 新建一个仅含 item 的节点(ref_count==1, next 为 nil)。
   * @note 配合 adopt_tail 做迭代式尾插构建，供跳过深层递归的
   * 列表整体映射(如 rectangles 的 translate/thicken)使用。
   */
  static list_rep<T>* fresh_cell (T item);

  /**
   * @brief 把 fresh_cell 新建的节点挂到 tail 槽位,并把 tail 推进到新链尾。
   * @param tail 当前链尾的 next 槽位(构建期间始终为 nil)
   * @param cell fresh_cell 新建的节点
   * @note 槽位为 NULL 且节点 ref_count==1,裸赋值即所有权让渡,
   * 不经引用计数;正确使用时无需 INC/DEC。
   */
  static void adopt_tail (list<T>*& tail, list_rep<T>* cell) {
    tail->rep= cell;
    tail     = &cell->next;
  }

  /**
   * @brief 新建仅含 item 的节点并尾挂到 tail(等价 fresh_cell + adopt_tail)。
   * @param tail 当前链尾的 next 槽位(构建期间始终为 nil)
   * @note 融合封装,使「adopt_tail 只接受 fresh_cell 节点」的约定无法被绕过。
   */
  static void append (list<T>*& tail, T item) {
    adopt_tail (tail, fresh_cell (item));
  }

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
  template <class T2, class U2> friend class hashmap_rep;
  template <class T2> friend class hashset_rep;
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
TMPL inline list_rep<T>*
list<T>::fresh_cell (T item) {
  return tm_new<list_rep<T>> (item, list<T> ());
}
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
/**
 * @brief Append an item to the end of a list in place.
 *
 * @note Each append walks from the head to the tail, i.e. O(n) per call;
 * appending n items one by one costs O(n^2). To build a long list, prepend
 * with `list<T> (item, l)` and finish with `reverse (l)`, or keep a local
 * handle to the last node as `copy (list<T>)` does.
 * @note The append mutates the (possibly shared) list in place.
 */
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
