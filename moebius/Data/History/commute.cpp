
/******************************************************************************
 * MODULE     : commute.cpp
 * DESCRIPTION: Commutation of modifications
 * COPYRIGHT  : (C) 2009  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "patch.hpp"
#include "tree_helper.hpp"

/******************************************************************************
 * Subroutines
 ******************************************************************************/

int
insert_length (tree t) {
  if (is_atomic (t)) return N (t->label);
  else return N (t);
}

static tree
insert_range (tree t, int i, int len) {
  if (is_atomic (t)) return t->label (i, i + len);
  else return t (i, i + len);
}

static modification
dup (modification m) {
  return modification (m->k, copy (m->p), m->t);
}

/******************************************************************************
 * Inversion of modifications
 ******************************************************************************/

modification
invert (modification m, tree t) {
  ASSERT (is_applicable (t, m), "modification not applicable");
  path rp= root (m);
  switch (m->k) {
  case MOD_ASSIGN:
    return mod_assign (rp, copy (subtree (t, rp)));
  case MOD_INSERT:
    return mod_remove (rp, index (m), insert_length (m->t));
  case MOD_REMOVE: {
    int i= index (m);
    int n= argument (m);
    return mod_insert (rp, i, copy (insert_range (subtree (t, rp), i, n)));
  }
  case MOD_SPLIT:
    return mod_join (rp, index (m));
  case MOD_JOIN: {
    int i= index (m);
    return mod_split (rp, i, insert_length (subtree (t, rp * i)));
  }
  case MOD_ASSIGN_NODE:
    return mod_assign_node (rp, L (subtree (t, rp)));
  case MOD_INSERT_NODE:
    return mod_remove_node (rp, argument (m));
  case MOD_REMOVE_NODE: {
    tree u= subtree (t, rp);
    int  i= index (m);
    return mod_insert_node (rp, i, copy (u (0, i) * u (i + 1, N (u))));
  }
  case MOD_SET_CURSOR:
    return m;
  default:
    TM_FAILED ("unexpected situation");
  }
  return m;
}

/******************************************************************************
 * Commutation of modifications
 ******************************************************************************/

static bool
swap1 (modification& m1, modification& m2, int i, int d) {
  modification r1= dup (m2);
  modification r2= dup (m1);
  if (m2->p->item >= i)
    if (m2->p->item != i || !is_nil (root (m2)) || m2->k != MOD_INSERT)
      r1->p->item-= d;
  if (is_nil (root (m2))) switch (m2->k) {
    case MOD_INSERT: {
      int b2= m2->p->item;
      int e2= b2 + insert_length (m2->t);
      if (b2 <= i) r2->p->item+= (e2 - b2);
      break;
    }
    case MOD_REMOVE: {
      int b2= m2->p->item;
      int e2= b2 + m2->p->next->item;
      if (b2 <= i && i < e2)
        if (b2 != i || m1->k != MOD_REMOVE) return false;
      if (b2 < i) r2->p->item-= (e2 - b2);
      break;
    }
    case MOD_SPLIT:
      if (m2->p->item < i) r2->p->item++;
      break;
    case MOD_JOIN:
      if (m2->p->item == i - 1) return false;
      if (m2->p->item < i) r2->p->item--;
      break;
    case MOD_INSERT_NODE: {
      modification aux= m2;
      m2              = modification (m1->k, path (m2->p->item, m1->p), m1->t);
      m1              = aux;
      return true;
    }
    case MOD_REMOVE_NODE:
      return false;
    case MOD_SET_CURSOR:
      return false;
    }
  m1= r1;
  m2= r2;
  return true;
}

static bool
swap2 (modification& m1, modification& m2, int i, int d) {
  modification r1= dup (m2);
  modification r2= dup (m1);
  if (m1->p->item >= i) r2->p->item+= d;
  m1= r1;
  m2= r2;
  return true;
}

bool
swap_basic (modification& m1, modification& m2) {
  modification aux= m1;
  m1              = m2;
  m2              = aux;
  return true;
}

bool
swap (modification& m1, modification& m2) {
  // Assuming that m1;m2 (the patch m1 followed by m2) is well-defined,
  // determine modifications m1* and m2* such that m2*;m1* is equivalent
  // to m1;m2.  If such modifications exist, then set m1 := m2* and
  // m2 := m1* and return true.  Otherwise, return false
  path rp1= root (m1);
  path rp2= root (m2);
  if (is_nil (rp1)) switch (m1->k) {
    case MOD_ASSIGN: {
      if (m1 == m2) return true;
      if (!is_nil (rp2) || m2->k != MOD_INSERT_NODE) return false;
      modification aux= m2;
      m2              = modification (m1->k, path (m2->p->item, m1->p), m1->t);
      m1              = aux;
      return true;
    }
    case MOD_INSERT: {
      if (is_nil (m2->p)) return false;
      int b= m1->p->item;
      int e= b + insert_length (m1->t);
      if (m2->p->item >= b && m2->p->item < e)
        if (!is_nil (root (m2)) || m2->p->item != b || m2->k != MOD_INSERT)
          if (!is_nil (root (m2)) || m2->k != MOD_INSERT_NODE) return false;
      return swap1 (m1, m2, b, e - b);
    }
    case MOD_REMOVE: {
      if (is_nil (m2->p)) return false;
      int i= m1->p->item;
      int d= m1->p->next->item;
      return swap1 (m1, m2, i, -d);
    }
    case MOD_SPLIT: {
      if (is_nil (m2->p)) return false;
      int i= m1->p->item;
      if (m2->p->item == i || m2->p->item == i + 1)
        if (!is_nil (root (m2)) || m2->p->item != i || m2->k != MOD_INSERT)
          if (!is_nil (root (m2)) || m2->k != MOD_INSERT_NODE) return false;
      return swap1 (m1, m2, i, 1);
    }
    case MOD_JOIN: {
      if (is_nil (m2->p)) return false;
      int i= m1->p->item;
      if (m2->p->item == i)
        if (!is_nil (root (m2)) || m2->k != MOD_INSERT) return false;
      return swap1 (m1, m2, i, -1);
    }
    case MOD_ASSIGN_NODE: {
      if (!is_nil (root (m2))) return swap_basic (m1, m2);
      if (m1 == m2) return true;
      if (!is_nil (rp2) || m2->k != MOD_INSERT_NODE) return false;
      modification aux= m2;
      m2              = modification (m1->k, path (m2->p->item, m1->p), m1->t);
      m1              = aux;
      return true;
    }
    case MOD_INSERT_NODE: {
      if (is_nil (root (m2))) return false;
      if (m2->p->item != m1->p->item) return false;
      modification aux= m1;
      m1              = modification (m2->k, m2->p->next, m2->t);
      if (m2->k != MOD_INSERT_NODE || !is_nil (root (m1))) m2= aux;
      else m2= modification (aux->k, path (m1->p->item, aux->p), aux->t);
      return true;
    }
    case MOD_REMOVE_NODE: {
      modification aux= m1;
      m1              = modification (m2->k, path (m1->p->item, m2->p), m2->t);
      m2              = aux;
      return true;
    }
    case MOD_SET_CURSOR: {
      if (!is_nil (rp2) || m2->k == MOD_JOIN || m2->k == MOD_SPLIT ||
          m2->k == MOD_ASSIGN_NODE)
        return swap_basic (m1, m2);
      return false;
    }
    }
  else if (is_nil (rp2)) switch (m2->k) {
    case MOD_ASSIGN:
      return false;
    case MOD_INSERT: {
      int b= m2->p->item;
      int e= b + insert_length (m2->t);
      return swap2 (m1, m2, b, e - b);
    }
    case MOD_REMOVE: {
      int b= m2->p->item;
      int e= b + m2->p->next->item;
      if (m1->p->item >= b && m1->p->item < e) return false;
      return swap2 (m1, m2, b, b - e);
    }
    case MOD_SPLIT: {
      int i= m2->p->item;
      if (m1->p->item == i) return false;
      return swap2 (m1, m2, i, 1);
    }
    case MOD_JOIN: {
      int i= m2->p->item;
      if (m1->p->item == i || m1->p->item == i + 1) return false;
      return swap2 (m1, m2, i, -1);
    }
    case MOD_ASSIGN_NODE:
      return swap_basic (m1, m2);
    case MOD_INSERT_NODE: {
      modification aux= m2;
      m2              = modification (m1->k, path (m2->p->item, m1->p), m1->t);
      m1              = aux;
      return true;
    }
    case MOD_REMOVE_NODE: {
      if (m1->p->item != m2->p->item) return false;
      modification aux= m2;
      m2              = modification (m1->k, m1->p->next, m1->t);
      m1              = aux;
      return true;
    }
    case MOD_SET_CURSOR: {
      if (!is_nil (rp1) || m1->k == MOD_JOIN || m1->k == MOD_SPLIT ||
          m1->k == MOD_ASSIGN_NODE)
        return swap_basic (m1, m2);
      return false;
    }
    }
  else if (rp1->item == rp2->item) {
    path         h (rp1->item);
    modification s1= m1 / h;
    modification s2= m2 / h;
    bool         r = swap (s1, s2);
    m1             = h * s1;
    m2             = h * s2;
    return r;
  }
  else return swap_basic (m1, m2);
  TM_FAILED ("unexpected situation");
  return false;
}

bool
commute (modification m1, modification m2) {
  // Assuming that m1;m2 (the patch m1 followed by m2) is well-defined,
  // determine whether there exist modifications m1* and m2* such that
  // m2*;m1* is equivalent to m1;m2
  modification s1= m1;
  modification s2= m2;
  return swap (s1, s2);
}

bool
can_pull (modification m1, modification m2) {
  return commute (m2, m1);
}

modification
pull (modification m1, modification m2) {
  // Assuming that m2;m1 (the patch m2 followed by m1) is well-defined,
  // return the modification m1* such that m1;m2 is equivalent to m2*;m1*
  // for a suitable modification m1* (assuming that m1* and m2* exist).
  modification s1= m2;
  modification s2= m1;
  ASSERT (swap (s1, s2), "modification cannot be pulled");
  return s1;
}

modification
co_pull (modification m1, modification m2) {
  // Same as pull, but return m2* instead of m1*
  modification s1= m2;
  modification s2= m1;
  ASSERT (swap (s1, s2), "modification cannot be pulled");
  return s2;
}

/******************************************************************************
 * Joining simple modifications when possible
 ******************************************************************************/

bool
join (modification& m1, modification m2, tree t) {
  if (m1->k == MOD_INSERT && m2->k == MOD_INSERT && is_atomic (m1->t) &&
      root (m1) == root (m2) &&
      (index (m2) == index (m1) ||
       index (m2) == index (m1) + N (m1->t->label))) {
    string s= m1->t->label * m2->t->label;
    if (index (m2) == index (m1)) s= m2->t->label * m1->t->label;
    m1= mod_insert (root (m1), index (m1), tree (s));
    return true;
  }
  if (m1->k == MOD_REMOVE && m2->k == MOD_REMOVE &&
      is_atomic (subtree (t, root (m1))) && root (m1) == root (m2) &&
      (index (m1) == index (m2) || index (m1) == index (m2) + argument (m2))) {
    m1= mod_remove (root (m2), index (m2), argument (m1) + argument (m2));
    return true;
  }
  return false;
}
