
/******************************************************************************
 * MODULE     : hashmap.cpp
 * DESCRIPTION: fixed size hashmaps with reference counting
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef HASHMAP_CC
#define HASHMAP_CC

#include "hashmap.hpp"
#define TMPL template <class T, class U>
#define H hashentry<T, U>

/******************************************************************************
 * Hashmap entries
 ******************************************************************************/

TMPL
H::hashentry (int code2, T key2, U im2)
    : code (code2), key (key2), im (im2) {}

TMPL tm_ostream&
operator<< (tm_ostream& out, H h) {
  out << h.key << "->" << h.im;
  return out;
}

TMPL bool
operator== (H h1, H h2) {
  return (h1.code == h2.code) && (h1.key == h2.key) && (h1.im == h2.im);
}

TMPL bool
operator!= (H h1, H h2) {
  return (h1.code != h2.code) || (h1.key != h2.key) || (h1.im != h2.im);
}

/******************************************************************************
 * Routines for hashmaps
 ******************************************************************************/

TMPL void
hashmap_rep<T, U>::resize (int n2) {
  int                    i;
  int                    oldn= n;
  list<hashentry<T, U>>* olda= a;
  n                          = n2;
  a                          = tm_new_array<list<hashentry<T, U>>> (n);
  // 原样重挂旧桶节点到新桶,复用已存的 code 免再哈希
  for (i= 0; i < oldn; i++) {
    list<hashentry<T, U>> l (olda[i]);
    while (!is_nil (l)) {
      list_rep<hashentry<T, U>>* node= l.rep;
      list<hashentry<T, U>>    next (node->next);
      list<hashentry<T, U>>::rehang (
          a[hash_bucket (node->item.code, n)], node);
      l= next;
    }
    olda[i]= list<hashentry<T, U>> ();
  }
  tm_delete_array (olda);
}

TMPL list_rep<hashentry<T, U>>*
hashmap_rep<T, U>::find_node (list<hashentry<T, U>>& bucket, int hv, T x) {
  list_rep<hashentry<T, U>>* p= bucket.rep;
  while (p != NULL) {
    if (p->item.code == hv && p->item.key == x) return p;
    p= p->next.rep;
  }
  return NULL;
}

TMPL bool
hashmap_rep<T, U>::contains (T x) {
  int hv= hash (x);
  return find_node (a[hash_bucket (hv, n)], hv, x) != NULL;
}

TMPL list_rep<hashentry<T, U>>*
hashmap_rep<T, U>::insert_node (int hv, T key, U im) {
  if (size >= n * max) resize (n << 1);
  list_rep<hashentry<T, U>>* node= tm_new<list_rep<hashentry<T, U>>> (
      H (hv, key, im), list<hashentry<T, U>> ());
  list<hashentry<T, U>>& rl= a[hash_bucket (hv, n)];
  list<hashentry<T, U>>::adopt (rl, node);
  size++;
  return node;
}

TMPL bool
hashmap_rep<T, U>::empty () {
  return size == 0;
}

TMPL U&
hashmap_rep<T, U>::bracket_rw (T x) {
  int hv                    = hash (x);
  list_rep<hashentry<T, U>>* p= find_node (a[hash_bucket (hv, n)], hv, x);
  if (p != NULL) return p->item.im;
  return insert_node (hv, x, init)->item.im;
}

TMPL U
hashmap_rep<T, U>::bracket_ro (T x) {
  int hv= hash (x);
  list_rep<hashentry<T, U>>* p=
      find_node (a[hash_bucket (hv, n)], hv, x);
  return p == NULL ? init : p->item.im;
}

TMPL void
hashmap_rep<T, U>::reset (T x) {
  int                    hv= hash (x);
  list<hashentry<T, U>>* l = &(a[hash_bucket (hv, n)]);
  while (!is_nil (*l)) {
    if ((*l)->item.code == hv && (*l)->item.key == x) {
      *l= (*l)->next;
      size--;
      if (size < (n >> 1) * max) resize (n >> 1);
      return;
    }
    l= &((*l)->next);
  }
}

TMPL void
hashmap_rep<T, U>::generate (void (*routine) (T)) {
  int i;
  for (i= 0; i < n; i++) {
    for (auto p= a[i]; !is_nil (p); p= p->next) {
      routine (p->item.key);
    }
  }
}

TMPL tm_ostream&
operator<< (tm_ostream& out, hashmap<T, U> h) {
  int i= 0, j= 0, n= h->n, size= h->size;
  out << "{ ";
  for (; i < n; i++) {
    for (auto p= h->a[i]; !is_nil (p); p= p->next, j++) {
      out << p->item;
      if (j != size - 1) out << ", ";
    }
  }
  out << " }";
  return out;
}

TMPL void
hashmap_rep<T, U>::join (hashmap<T, U> h) {
  int hn= h->n;
  for (int i= 0; i < hn; i++) {
    for (auto p= h->a[i].rep; p != NULL; p= p->next.rep) {
      // 直接复用条目已存的 code,免去对 key 的二次哈希
      int hv                        = p->item.code;
      list_rep<hashentry<T, U>>* q  = find_node (a[hash_bucket (hv, n)], hv,
                                                 p->item.key);
      if (q != NULL) q->item.im    = copy (p->item.im);
      else insert_node (hv, p->item.key, copy (p->item.im));
    }
  }
}

TMPL bool
operator== (hashmap<T, U> h1, hashmap<T, U> h2) {
  if (h1->size != h2->size) return false;
  int hn= h1->n;
  for (int i= 0; i < hn; i++) {
    for (auto p= h1->a[i]; !is_nil (p); p= p->next) {
      // 复用条目已存的 code 在 h2 内查找,免去二次哈希
      int hv                       = p->item.code;
      list_rep<hashentry<T, U>>* q = hashmap_rep<T, U>::find_node (
          h2->a[hash_bucket (hv, h2->n)], hv, p->item.key);
      if (q == NULL || q->item.im != p->item.im) return false;
    }
  }
  return true;
}

TMPL bool
operator!= (hashmap<T, U> h1, hashmap<T, U> h2) {
  return !(h1 == h2);
}

#undef H
#undef TMPL
#endif // defined HASHMAP_CC
