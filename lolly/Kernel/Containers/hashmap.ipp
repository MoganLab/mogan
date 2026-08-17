
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
  // 把旧桶的节点直接搬到新桶:复用已存的 code 免再哈希,
  // 并原样重挂节点,避免逐条目重新分配/析构
  for (i= 0; i < oldn; i++) {
    list<hashentry<T, U>> l (olda[i]);
    while (!is_nil (l)) {
      list_rep<hashentry<T, U>>* node= l.rep;
      list<hashentry<T, U>>    next (node->next);
      list<hashentry<T, U>>&   newl  = a[hash_bucket (node->item.code, n)];
      node->next                     = newl;
      node->ref_count++; // 所有权转移给新桶
      newl.rep                       = node;
      l                              = next;
    }
    olda[i]= list<hashentry<T, U>> ();
  }
  tm_delete_array (olda);
}

TMPL bool
hashmap_rep<T, U>::contains (T x) {
  int hv= hash (x);
  for (auto l= a[hash_bucket (hv, n)]; !is_nil (l); l= l->next) {
    if (l->item.code == hv && l->item.key == x) return true;
  }
  return false;
}

TMPL bool
hashmap_rep<T, U>::empty () {
  return size == 0;
}

TMPL U&
hashmap_rep<T, U>::bracket_rw (T x) {
  int hv= hash (x);
  for (auto p= a[hash_bucket (hv, n)]; !is_nil (p); p= p->next) {
    if (p->item.code == hv && p->item.key == x) return p->item.im;
  }
  if (size >= n * max) resize (n << 1);
  list<hashentry<T, U>>& rl= a[hash_bucket (hv, n)];
  rl                       = list<hashentry<T, U>> (H (hv, x, init), rl);
  size++;
  return rl->item.im;
}

TMPL U
hashmap_rep<T, U>::bracket_ro (T x) {
  int                   hv= hash (x);
  list<hashentry<T, U>> l (a[hash_bucket (hv, n)]);
  while (!is_nil (l)) {
    if (l->item.code == hv && l->item.key == x) return l->item.im;
    l= l->next;
  }
  return init;
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
  int i= 0, n= h->n;
  for (; i < n; i++) {
    for (auto p= h->a[i]; !is_nil (p); p= p->next) {
      // 直接复用条目已存的 code,免去对 key 的二次哈希
      int hv                    = p->item.code;
      list<hashentry<T, U>>* rl = &(a[hash_bucket (hv, this->n)]);
      bool found                = false;
      for (auto q= *rl; !is_nil (q); q= q->next) {
        if (q->item.code == hv && q->item.key == p->item.key) {
          q->item.im= copy (p->item.im);
          found    = true;
          break;
        }
      }
      if (!found) {
        if (size >= this->n * max) resize (this->n << 1);
        rl           = &(a[hash_bucket (hv, this->n)]);
        *rl          = list<hashentry<T, U>> (
            H (hv, p->item.key, copy (p->item.im)), *rl);
        size++;
      }
    }
  }
}

TMPL bool
operator== (hashmap<T, U> h1, hashmap<T, U> h2) {
  if (h1->size != h2->size) return false;
  int i= 0, n= h1->n;
  for (; i < n; i++) {
    for (auto p= h1->a[i]; !is_nil (p); p= p->next)
      if (h2[p->item.key] != p->item.im) return false;
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
