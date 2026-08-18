
/******************************************************************************
 * MODULE     : hashset.cpp
 * DESCRIPTION: fixed size hashsets with reference counting
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef HASHSET_CC
#define HASHSET_CC
#include "hashset.hpp"

template <class T>
void
hashset_rep<T>::resize (int n2) {
  int      i;
  int      oldn= n;
  list<T>* olda= a;
  n            = n2;
  a            = tm_new_array<list<T>> (n);
  // 把旧桶的节点直接搬到新桶:原样重挂节点,避免逐条目重新分配/析构
  for (i= 0; i < oldn; i++) {
    list<T> l (olda[i]);
    while (!is_nil (l)) {
      list_rep<T>* node= l.rep;
      list<T>      next (node->next);
      list<T>&     newl= a[hash_bucket (hash (node->item), n)];
      node->next.rep    = newl.rep; // 桶对旧头部的引用转由 node 持有
      node->ref_count++;            // 所有权转移给新桶
      newl.rep          = node;
      l                 = next;
    }
    olda[i]= list<T> ();
  }
  tm_delete_array (olda);
}

template <class T>
list_rep<T>*
hashset_rep<T>::find_node (list<T>& bucket, T x) {
  list_rep<T>* p= bucket.rep;
  while (p != NULL) {
    if (p->item == x) return p;
    p= p->next.rep;
  }
  return NULL;
}

template <class T>
bool
hashset_rep<T>::contains (T x) {
  return find_node (a[hash_bucket (hash (x), n)], x) != NULL;
}

template <class T>
void
hashset_rep<T>::insert_node (T x) {
  if (size >= n * max) resize (n << 1);
  list_rep<T>* node= tm_new<list_rep<T>> (x, list<T> ());
  list<T>&     rl  = a[hash_bucket (hash (x), n)];
  node->next.rep   = rl.rep; // 桶对旧头部的引用转由 node 持有
  rl.rep           = node;
  size++;
}

template <class T>
void
hashset_rep<T>::insert (T x) {
  list<T>& l= a[hash_bucket (hash (x), n)];
  if (find_node (l, x) != NULL) return;
  insert_node (x);
}

template <class T>
void
hashset_rep<T>::remove (T x) {
  list<T>* lptr= &a[hash_bucket (hash (x), n)];
  while (!is_nil (*lptr)) {
    if ((*lptr)->item == x) {
      *lptr= (*lptr)->next;
      size--;
      return;
    }
    lptr= &((*lptr)->next);
  }
}

template <class T>
hashset<T>
copy (hashset<T> h) {
  int        i, n= h->n;
  hashset<T> h2 (n, h->max);
  h2->size= h->size;
  for (i= 0; i < n; i++)
    h2->a[i]= copy (h->a[i]);
  return h2;
}

template <class T>
bool
operator<= (hashset<T> h1, hashset<T> h2) {
  int i= 0, j= 0, n= h1->n;
  if (N (h1) > N (h2)) return false;
  for (; i < n; i++) {
    list<T> l= h1->a[i];
    for (; !is_nil (l); l= l->next, j++) {
      // 裸指针在 h2 桶内查找,免去 contains 的句柄拷贝
      if (hashset_rep<T>::find_node (
              h2->a[hash_bucket (hash (l->item), h2->n)], l->item) == NULL)
        return false;
    }
  }
  return true;
}

template <class T>
bool
operator< (hashset<T> h1, hashset<T> h2) {
  return (N (h1) < N (h2)) && (h1 <= h2);
}

template <class T>
bool
operator== (hashset<T> h1, hashset<T> h2) {
  return (N (h1) == N (h2)) && (h1 <= h2);
}

template <class T>
tm_ostream&
operator<< (tm_ostream& out, hashset<T> h) {
  int i= 0, j= 0, n= h->n, size= h->size;
  out << "{ ";
  for (; i < n; i++) {
    list<T> l= h->a[i];
    for (; !is_nil (l); l= l->next, j++) {
      out << l->item;
      if (j != size - 1) out << ", ";
    }
  }
  out << " }";
  return out;
}

template <class T>
hashset<T>&
operator<< (hashset<T>& h, T x) {
  h->insert (x);
  return h;
}

#endif // defined HASHSET_CC
