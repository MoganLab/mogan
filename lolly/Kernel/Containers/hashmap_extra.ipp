
/******************************************************************************
 * MODULE     : hashmap_extra.cpp
 * DESCRIPTION: extra routines for hashmap<string,tree>
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef HASHMAP_EXTRA_CC
#define HASHMAP_EXTRA_CC

#include "hashmap.hpp"
#define TMPL template <class T, class U>
#define H hashentry<T, U>

TMPL void
hashmap_rep<T, U>::write_back (T x, hashmap<T, U> base) {
  int                   hv= hash (x);
  list<hashentry<T, U>> l (a[hv & (n - 1)]);
  while (!is_nil (l)) {
    if (l->item.code == hv && l->item.key == x) return;
    l= l->next;
  }
  if (size >= n * max) resize (n << 1);
  list<hashentry<T, U>>& rl= a[hv & (n - 1)];
  rl                       = list<hashentry<T, U>> (H (hv, x, init), rl);
  size++;

  list<hashentry<T, U>> bl (base->a[hv & (base->n - 1)]);
  while (!is_nil (bl)) {
    if (bl->item.code == hv && bl->item.key == x) {
      rl->item.im= bl->item.im;
      return;
    }
    bl= bl->next;
  }
  rl->item.im= base->init;
}

TMPL void
hashmap_rep<T, U>::pre_patch (hashmap<T, U> patch, hashmap<T, U> base) {
  int i= 0, n= patch->n;
  for (; i < n; i++) {
    list<hashentry<T, U>> l= patch->a[i];
    for (; !is_nil (l); l= l->next) {
      T x= l->item.key;
      U y= contains (x) ? bracket_ro (x) : l->item.im;
      if (base[x] == y) reset (x);
      else bracket_rw (x)= y;
    }
  }
}

TMPL void
hashmap_rep<T, U>::post_patch (hashmap<T, U> patch, hashmap<T, U> base) {
  int i= 0, n= patch->n;
  for (; i < n; i++) {
    list<hashentry<T, U>> l= patch->a[i];
    for (; !is_nil (l); l= l->next) {
      T x= l->item.key;
      U y= l->item.im;
      if (base[x] == y) reset (x);
      else bracket_rw (x)= y;
    }
  }
}

TMPL list<hashentry<T, U>>
     copy_list (list<hashentry<T, U>> l) {
  if (is_nil (l)) return l;

  hashentry<T, U>       e0 (l->item.code, l->item.key, l->item.im);
  list<hashentry<T, U>> head (e0);
  auto                  tail= head;

  for (auto cur= l->next; !is_nil (cur); cur= cur->next) {
    hashentry<T, U> en (cur->item.code, cur->item.key, cur->item.im);
    tail->next= list<hashentry<T, U>> (en);
    tail      = tail->next;
  }
  return head;
}

TMPL hashmap<T, U>
     copy (hashmap<T, U> h) {
  int           i, n= h->n;
  hashmap<T, U> h2 (h->init, n, h->max);
  h2->size= h->size;
  for (i= 0; i < n; i++)
    h2->a[i]= copy_list (h->a[i]);
  return h2;
}

TMPL hashmap<T, U>
     changes (hashmap<T, U> patch, hashmap<T, U> base) {
  int           i;
  hashmap<T, U> h (base->init);
  for (i= 0; i < patch->n; i++) {
    list<hashentry<T, U>> l (patch->a[i]);
    while (!is_nil (l)) {
      if (l->item.im != base[l->item.key]) h (l->item.key)= l->item.im;
      l= l->next;
    }
  }
  return h;
}

TMPL hashmap<T, U>
     invert (hashmap<T, U> patch, hashmap<T, U> base) {
  int           i;
  hashmap<T, U> h (base->init);
  for (i= 0; i < patch->n; i++) {
    list<hashentry<T, U>> l (patch->a[i]);
    while (!is_nil (l)) {
      if (l->item.im != base[l->item.key]) h (l->item.key)= base[l->item.key];
      l= l->next;
    }
  }
  return h;
}

#undef H
#undef TMPL
#endif // defined HASHMAP_EXTRA_CC
