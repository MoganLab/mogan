
/** \file lolly_lolly_tree.hpp
 *  \copyright GPLv3
 *  \details lolly_tree with data
 *  \author Darcy Shen
 *  \date   2024
 */

#pragma once

#include "array.hpp"
#include "basic.hpp"
#include "list.hpp"
#include "string.hpp"

namespace lolly {
namespace data {

#define CHECK_COMPOUND(t)                                                      \
  TM_DEBUG (ASSERT (t->op != 0, "an compound tree use 0 as operator"))

template <typename T> class lolly_tree;
template <typename T> class lolly_tree_rep;
class blackbox;

template <typename T> class lolly_tree {
  CONCRETE_TEMPLATE (lolly_tree, T);
  // rep field can be atomic or compound or generic

  inline lolly_tree () : rep (tm_new<lolly_tree_rep<T>> (string ())) {}
  inline lolly_tree<T> (string l) : rep (tm_new<lolly_tree_rep<T>> (l)) {}
  inline lolly_tree (const char* l) : rep (tm_new<lolly_tree_rep<T>> (l)) {}

  inline lolly_tree (lolly_tree_rep<T>* rep2) : rep (rep2) { rep->ref_count++; }

  inline lolly_tree (int l, int n= 0)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (n))) {}

  inline lolly_tree (int l, array<lolly_tree<T>> a)
      : rep (tm_new<lolly_tree_rep<T>> (l, a)) {}

  inline lolly_tree (lolly_tree<T> t, int n)
      : rep (tm_new<lolly_tree_rep<T>> (t.rep->op, array<lolly_tree<T>> (n))) {
    CHECK_COMPOUND (t);
  }

  inline lolly_tree (int l, lolly_tree<T> t1)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (1))) {
    rep->a[0]= t1;
  }

  inline lolly_tree (int l, lolly_tree<T> t1, lolly_tree<T> t2)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (2))) {
    rep->a[0]= t1;
    rep->a[1]= t2;
  }

  inline lolly_tree (int l, lolly_tree<T> t1, lolly_tree<T> t2,
                     lolly_tree<T> t3)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (3))) {
    rep->a[0]= t1;
    rep->a[1]= t2;
    rep->a[2]= t3;
  }

  inline lolly_tree (int l, lolly_tree<T> t1, lolly_tree<T> t2,
                     lolly_tree<T> t3, lolly_tree<T> t4)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (4))) {
    rep->a[0]= t1;
    rep->a[1]= t2;
    rep->a[2]= t3;
    rep->a[3]= t4;
  }

  inline lolly_tree (int l, lolly_tree<T> t1, lolly_tree<T> t2,
                     lolly_tree<T> t3, lolly_tree<T> t4, lolly_tree<T> t5)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (5))) {
    rep->a[0]= t1;
    rep->a[1]= t2;
    rep->a[2]= t3;
    rep->a[3]= t4;
    rep->a[4]= t5;
  }

  inline lolly_tree (int l, lolly_tree<T> t1, lolly_tree<T> t2,
                     lolly_tree<T> t3, lolly_tree<T> t4, lolly_tree<T> t5,
                     lolly_tree<T> t6)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (6))) {
    rep->a[0]= t1;
    rep->a[1]= t2;
    rep->a[2]= t3;
    rep->a[3]= t4;
    rep->a[4]= t5;
    rep->a[5]= t6;
  }

  inline lolly_tree (int l, lolly_tree<T> t1, lolly_tree<T> t2,
                     lolly_tree<T> t3, lolly_tree<T> t4, lolly_tree<T> t5,
                     lolly_tree<T> t6, lolly_tree<T> t7)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (7))) {
    rep->a[0]= t1;
    rep->a[1]= t2;
    rep->a[2]= t3;
    rep->a[3]= t4;
    rep->a[4]= t5;
    rep->a[5]= t6;
    rep->a[6]= t7;
  }

  inline lolly_tree (int l, lolly_tree<T> t1, lolly_tree<T> t2,
                     lolly_tree<T> t3, lolly_tree<T> t4, lolly_tree<T> t5,
                     lolly_tree<T> t6, lolly_tree<T> t7, lolly_tree<T> t8)
      : rep (tm_new<lolly_tree_rep<T>> (l, array<lolly_tree<T>> (8))) {
    rep->a[0]= t1;
    rep->a[1]= t2;
    rep->a[2]= t3;
    rep->a[3]= t4;
    rep->a[4]= t5;
    rep->a[5]= t6;
    rep->a[6]= t7;
    rep->a[7]= t8;
  }

  inline lolly_tree<T>& operator[] (int i) {
    CHECK_COMPOUND (*this);
    return rep->a[i];
  }

  inline lolly_tree<T> operator() (int start, int end) {
    lolly_tree<T> r (rep->op, end - start);
    for (int i= start; i < end; i++)
      r[i - start]= rep->a[i];
    return r;
  }
  lolly_tree<T>* begin () { return rep->a.begin (); }
  lolly_tree<T>* end () { return rep->a.end (); }
};
CONCRETE_TEMPLATE_CODE (lolly_tree, typename, T)

template <typename T> class lolly_tree_rep : concrete_struct {
public:
  // the tree_label of op=0 in TeXmacs is STRING
  int op;
  T   data;
  union {
    string               label;
    array<lolly_tree<T>> a;
  };

  inline lolly_tree_rep (string l) : op (0), label (l) {}
  inline lolly_tree_rep (int l, array<lolly_tree<T>> a2) : op (l), a (a2) {}
  inline ~lolly_tree_rep () {
    if (op == 0) {
      label.~string ();
    }
    else if (op > 0) {
      a.~array ();
    };
  }
  friend class lolly_tree<T>;
};

template <typename T>
inline int
N (lolly_tree<T> t) {
  CHECK_COMPOUND (t);
  return N (t->a);
}

template <typename T>
inline int
arity (lolly_tree<T> t) {
  if (t->op == /*STRING*/ 0) return 0;
  else return N (t->a);
}

template <typename T>
inline array<lolly_tree<T>>
A (lolly_tree<T> t) {
  CHECK_COMPOUND (t);
  return t->a;
}

template <typename T>
inline array<lolly_tree<T>>&
AR (lolly_tree<T> t) {
  CHECK_COMPOUND (t);
  return t->a;
}

template <typename T>
inline bool
is_atomic (lolly_tree<T> t) {
  return (t->op == 0);
}

template <typename T>
inline bool
is_compound (lolly_tree<T> t) {
  return ((t->op) > /*STRING*/ 0);
}

template <typename T>
inline bool
is_generic (lolly_tree<T> t) {
  return (t->op) < 0;
}

template <typename T>
inline bool
operator== (lolly_tree<T> t, int lab) {
  return (t->op == lab) && (N (t) == 0);
}

template <typename T>
inline bool
operator!= (lolly_tree<T> t, int lab) {
  return (t->op != lab) || (N (t) != 0);
}

template <typename T>
inline bool
operator== (lolly_tree<T> t, string s) {
  return (t->op == /*STRING*/ 0) && (t->label == s);
}

template <typename T>
inline bool
operator!= (lolly_tree<T> t, string s) {
  return (t->op != /*STRING*/ 0) || (t->label != s);
}

template <typename T>
inline bool
operator== (lolly_tree<T> t, const char* s) {
  return (t->op == /*STRING*/ 0) && (t->label == s);
}

template <typename T>
inline bool
operator!= (lolly_tree<T> t, const char* s) {
  return (t->op != /*STRING*/ 0) || (t->label != s);
}

template <typename T>
inline bool
operator== (lolly_tree<T> t, lolly_tree<T> u) {
  if (strong_equal (t, u)) return true;
  return (t->op == u->op) &&
         (is_atomic (t) ? (t->label == u->label) : (A (t) == A (u)));
}

template <typename T>
inline bool
operator!= (lolly_tree<T> t, lolly_tree<T> u) {
  if (strong_equal (t, u)) return false;
  return (t->op != u->op) ||
         (is_atomic (t) ? (t->label != u->label) : (A (t) != A (u)));
}

template <typename T>
inline bool
strong_equal (lolly_tree<T> t, lolly_tree<T> u) {
  return t.operator->() == u.operator->();
}

template <typename T>
inline bool
is_func (lolly_tree<T> t, int l) {
  return (t->op == l) && (N (t) != 0);
}

template <typename T>
inline bool
is_func (lolly_tree<T> t, int l, int i) {
  return (t->op == l) && (N (t) == i);
}

template <typename T>
inline bool
is_bool (lolly_tree<T> t) {
  return is_atomic (t) && is_bool (t->label);
}

template <typename T>
inline bool
is_int (lolly_tree<T> t) {
  return is_atomic (t) && is_int (t->label);
}

template <typename T>
inline bool
is_double (lolly_tree<T> t) {
  return is_atomic (t) && is_double (t->label);
}

template <typename T>
inline bool
is_string (lolly_tree<T> t) {
  return is_atomic (t);
}

template <typename T>
inline bool
as_bool (lolly_tree<T> t) {
  if (is_atomic (t)) return as_bool (t->label);
  else return false;
}

template <typename T>
inline int
as_int (lolly_tree<T> t) {
  if (is_atomic (t)) return as_int (t->label);
  else return 0;
}

template <typename T>
inline long int
as_long_int (lolly_tree<T> t) {
  if (is_atomic (t)) return as_long_int (t->label);
  else return 0;
}

template <typename T>
inline double
as_double (lolly_tree<T> t) {
  if (is_atomic (t)) return as_double (t->label);
  else return 0.0;
}

template <typename T>
inline string
to_string (lolly_tree<T> t) {
  if (is_atomic (t)) return t->label;
  else return "";
}

template <typename T> lolly_tree<T> copy (lolly_tree<T> t);

template <typename T>
inline lolly_tree<T>
operator* (lolly_tree<T> t1, lolly_tree<T> t2) {
  int i;
  if (is_atomic (t1)) t1= lolly_tree<T> (t2->op, t1);
  if (is_atomic (t2)) t2= lolly_tree<T> (t1->op, t2);
  lolly_tree<T> r (t1, N (t1) + N (t2));
  for (i= 0; i < N (t1); i++)
    r[i]= t1[i];
  for (i= 0; i < N (t2); i++)
    r[i + N (t1)]= t2[i];
  return r;
}

template <typename T>
inline lolly_tree<T>&
operator<< (lolly_tree<T>& t, lolly_tree<T> t2) {
  CHECK_COMPOUND (t);
  t->a << t2;
  return t;
}

template <typename T>
inline lolly_tree<T>&
operator<< (lolly_tree<T>& t, array<lolly_tree<T>> a) {
  CHECK_COMPOUND (t);
  t->a << a;
  return t;
}

template <typename T>
inline tm_ostream&
operator<< (tm_ostream& out, lolly_tree<T> t) {
  if (is_atomic (t)) return out << t->label;
  else if (is_compound (t)) {
    int i, n= N (t);
    out << as_string (t->op);
    if (n == 0) return out << "()";
    out << " (";
    for (i= 0; i < n - 1; i++)
      out << t[i] << ", ";
    out << t[i] << ")";
    return out;
  }
  return out;
}

template <typename T>
inline lolly_tree<T>
copy (lolly_tree<T> t) {
  if (is_atomic (t)) return lolly_tree<T> (copy (t->label));
  else {
    int           i, n= N (t);
    lolly_tree<T> t2 (t, n);
    for (i= 0; i < n; i++)
      t2[i]= copy (t[i]);
    return t2;
  }
}

} // namespace data
} // namespace lolly
