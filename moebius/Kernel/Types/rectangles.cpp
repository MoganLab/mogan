
/******************************************************************************
 * MODULE     : rectangles.cpp
 * DESCRIPTION: Rectangles and lists of rectangles with reference counting.
 *              Used in graphical programs.
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "rectangles.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include <moebius/tree_label.hpp>

/******************************************************************************
 * Routines for rectangles
 ******************************************************************************/

rectangle_rep::rectangle_rep (SI x1b, SI y1b, SI x2b, SI y2b)
    : x1 (x1b), y1 (y1b), x2 (x2b), y2 (y2b) {}

rectangle::rectangle (SI x1b, SI y1b, SI x2b, SI y2b)
    : rep (tm_new<rectangle_rep> (x1b, y1b, x2b, y2b)) {}

rectangle::operator tree () {
  return tuple (as_string (rep->x1), as_string (rep->y1), as_string (rep->x2),
                as_string (rep->y2));
}

tm_ostream&
operator<< (tm_ostream& out, rectangle r) {
  out << "rectangle (" << r->x1 << ", " << r->y1 << ", " << r->x2 << ", "
      << r->y2 << ")";
  return out;
}

rectangle
copy (rectangle r) {
  return rectangle (r->x1, r->y1, r->x2, r->y2);
}

bool
operator== (rectangle r1, rectangle r2) {
  return (r1->x1 == r2->x1) && (r1->y1 == r2->y1) && (r1->x2 == r2->x2) &&
         (r1->y2 == r2->y2);
}

bool
operator!= (rectangle r1, rectangle r2) {
  return (r1->x1 != r2->x1) || (r1->y1 != r2->y1) || (r1->x2 != r2->x2) ||
         (r1->y2 != r2->y2);
}

bool
operator<= (rectangle r1, rectangle r2) {
  return (r1->x1 >= r2->x1) && (r1->x2 <= r2->x2) && (r1->y1 >= r2->y1) &&
         (r1->y2 <= r2->y2);
}

bool
intersect (rectangle r1, rectangle r2) {
  return (r1->x1 < r2->x2) && (r1->x2 > r2->x1) && (r1->y1 < r2->y2) &&
         (r1->y2 > r2->y1);
}

rectangle
translate (const rectangle& r, SI x, SI y) {
  return rectangle (r->x1 + x, r->y1 + y, r->x2 + x, r->y2 + y);
}

double
area (rectangle r) {
  double w= max (r->x2 - r->x1, 0);
  double h= max (r->y2 - r->y1, 0);
  return w * h;
}

bool
is_zero (rectangle r) {
  return (r->x1 == 0) && (r->x2 == 0) && (r->y1 == 0) && (r->y2 == 0);
}

/******************************************************************************
 * Miscellaneous subroutines
 ******************************************************************************/

// FIXME: Why do we need this? Compiler bug?
#define min(x, y) ((x) <= (y) ? (x) : (y))
#define max(x, y) ((x) <= (y) ? (y) : (x))

void
complement (rectangle r1, rectangle r2, rectangles& l) {
  if (!intersect (r1, r2)) {
    r1 >> l;
    return;
  }
  if (r1->x1 < r2->x1) rectangle (r1->x1, r1->y1, r2->x1, r1->y2) >> l;
  if (r1->x2 > r2->x2) rectangle (r2->x2, r1->y1, r1->x2, r1->y2) >> l;
  if (r1->y1 < r2->y1)
    rectangle (max (r1->x1, r2->x1), r1->y1, min (r1->x2, r2->x2), r2->y1) >> l;
  if (r1->y2 > r2->y2)
    rectangle (max (r1->x1, r2->x1), r2->y2, min (r1->x2, r2->x2), r1->y2) >> l;
}

void
intersection (rectangle r1, rectangle r2, rectangles& l) {
  if (!intersect (r1, r2)) return;
  rectangle (max (r1->x1, r2->x1), max (r1->y1, r2->y1), min (r1->x2, r2->x2),
             min (r1->y2, r2->y2)) >>
      l;
}

rectangle
least_upper_bound (rectangle r1, rectangle r2) {
  return rectangle (min (r1->x1, r2->x1), min (r1->y1, r2->y1),
                    max (r1->x2, r2->x2), max (r1->y2, r2->y2));
}

rectangle
operator* (rectangle r, int d) {
  return rectangle (r->x1 * d, r->y1 * d, r->x2 * d, r->y2 * d);
}

rectangle
operator* (rectangle r, double x) {
  return rectangle ((SI) floor (r->x1 * x), (SI) floor (r->y1 * x),
                    (SI) ceil (r->x2 * x), (SI) ceil (r->y2 * x));
}

rectangle
operator/ (rectangle r, int d) {
  return rectangle (r->x1 / d, r->y1 / d, r->x2 / d, r->y2 / d);
}

rectangle
operator/ (rectangle r, double x) {
  return rectangle ((SI) floor (r->x1 / x), (SI) floor (r->y1 / x),
                    (SI) ceil (r->x2 / x), (SI) ceil (r->y2 / x));
}

rectangle
thicken (rectangle r, SI width, SI height) {
  return rectangle (r->x1 - width, r->y1 - height, r->x2 + width,
                    r->y2 + height);
}

/******************************************************************************
 * Exported routines for rectangles
 ******************************************************************************/

// 单个矩形对 l2 求差,结果尾挂到 tail:
// 与 l2 全不交时原矩形一次直挂,零差分开销
static void
append_difference (rectangle r, rectangles l2, rectangles*& tail) {
  rectangles cur;
  bool       cut= false;
  for (rectangles q= l2; !is_nil (q); q= q->next) {
    if (!intersect (r, q->item)) continue;
    if (!cut) {
      complement (r, q->item, cur);
      cut= true;
      continue;
    }
    rectangles next;
    for (rectangles p= cur; !is_nil (p); p= p->next)
      complement (p->item, q->item, next);
    cur= next;
  }
  if (!cut) rectangles::append (tail, r);
  else
    for (rectangles p= cur; !is_nil (p); p= p->next)
      rectangles::append (tail, p->item);
}

rectangles
operator- (rectangles l1, rectangles l2) {
  // 逐矩形独立求差:未与任何被减矩形相交的元素一次直挂,
  // 避免旧实现对 l2 每个元素整表重建的 O(n1*n2) 次全表克隆
  rectangles  out;
  rectangles* tail= &out;
  for (; !is_nil (l1); l1= l1->next)
    append_difference (l1->item, l2, tail);
  return out;
}

rectangles
operator& (rectangles l1, rectangles l2) {
  rectangles l, lc1, lc2;
  for (lc1= l1; !is_nil (lc1); lc1= lc1->next)
    for (lc2= l2; !is_nil (lc2); lc2= lc2->next)
      intersection (lc1->item, lc2->item, l);
  return l;
}

bool
adjacent (rectangle r1, rectangle r2) {
  return (((r1->x2 == r2->x1) || (r1->x1 == r2->x2)) &&
          ((r1->y1 == r2->y1) && (r1->y2 == r2->y2))) ||
         (((r1->y2 == r2->y1) || (r1->y1 == r2->y2)) &&
          ((r1->x1 == r2->x1) && (r1->x2 == r2->x2)));
}

static inline rectangles
reverse_append (rectangles rev_prefix, rectangles tail) {
  rectangles acc= tail;
  for (rectangles p= rev_prefix; !is_nil (p); p= p->next)
    acc= rectangles (p->item, acc);
  return acc;
}

void
disjoint_union_inplace (rectangles& out, rectangles l, rectangle r,
                        rectangles& scratch_rev) {
  scratch_rev= rectangles ();

  rectangles cur= l;
  while (!is_nil (cur) && !adjacent (cur->item, r)) {
    scratch_rev= rectangles (cur->item, scratch_rev);
    cur        = cur->next;
  }

  if (is_nil (cur)) {
    scratch_rev= rectangles (r, scratch_rev);
    out        = reverse_append (scratch_rev, rectangles ());
    return;
  }

  rectangle merged= least_upper_bound (cur->item, r);
  cur             = cur->next;
  while (!is_nil (cur) && adjacent (cur->item, merged)) {
    merged= least_upper_bound (cur->item, merged);
    cur   = cur->next;
  }
  scratch_rev= rectangles (merged, scratch_rev);
  out        = reverse_append (scratch_rev, cur);
}

rectangles
disjoint_union (rectangles l, rectangle r) {
  rectangles out, scratch_rev;
  disjoint_union_inplace (out, l, r, scratch_rev);
  return out;
}

rectangles
operator| (rectangles l1, rectangles l2) {
  // operator- 的结果全是新鲜节点,可原地改链:
  // 每个 l2 元素只做摘链/改写,不再克隆整个前缀
  rectangles l= l1 - l2;
  for (; !is_nil (l2); l2= l2->next) {
    rectangle   r   = l2->item;
    rectangles* link= &l;
    while (!is_nil (*link)) {
      if (!adjacent ((*link)->item, r)) {
        link= &(*link)->next;
        continue;
      }
      // 相邻则并入 r 并把被吸收的节点摘链,继续向后扫
      r    = least_upper_bound ((*link)->item, r);
      *link= (*link)->next;
    }
    rectangles::append (link, r);
  }
  return l;
}

rectangles
translate (const rectangles& l, SI x, SI y) {
  // 迭代 + 尾槽位直挂：避免深度等于列表长度的递归，也避免 reverse 的二次分配
  rectangles  out;
  rectangles* tail= &out;
  for (rectangles p= l; !is_nil (p); p= p->next) {
    rectangle& r= p->item;
    rectangles::append (tail,
                        rectangle (r->x1 + x, r->y1 + y, r->x2 + x, r->y2 + y));
  }
  return out;
}

rectangles
thicken (const rectangles& l, SI width, SI height) {
  // 迭代 + 尾槽位直挂：避免深度等于列表长度的递归，也避免 reverse 的二次分配
  rectangles  out;
  rectangles* tail= &out;
  for (rectangles p= l; !is_nil (p); p= p->next) {
    rectangle& r= p->item;
    rectangles::append (tail, rectangle (r->x1 - width, r->y1 - height,
                                         r->x2 + width, r->y2 + height));
  }
  return out;
}

rectangles
outlines (rectangles rs, SI pixel) {
  return simplify (
      correct (thicken (rs, pixel, 3 * pixel) - thicken (rs, 0, 2 * pixel)));
}

rectangles
operator* (rectangles l, int d) {
  if (is_nil (l)) return l;
  return rectangles (l->item * d, l->next * d);
}

rectangles
operator/ (rectangles l, int d) {
  if (is_nil (l)) return l;
  return rectangles (l->item / d, l->next / d);
}

rectangles
correct (const rectangles& l) {
  // 迭代 + 尾槽位直挂：避免深度等于列表长度的递归，也避免 reverse 的二次分配
  rectangles  out;
  rectangles* tail= &out;
  for (rectangles p= l; !is_nil (p); p= p->next) {
    rectangle& r= p->item;
    if ((r->x1 < r->x2) && (r->y1 < r->y2)) rectangles::append (tail, r);
  }
  return out;
}

rectangles
simplify_bis (rectangles l) {
  if (is_nil (l) || is_atom (l)) return l;
  return simplify_bis (l->next) | rectangles (l->item);
}

rectangles
simplify (rectangles l) {
  if (N (l) > 25) return copy (l);
  else return simplify_bis (l);
}

rectangle
least_upper_bound (rectangles l) {
  ASSERT (!is_nil (l), "no rectangles in list");
  rectangle r1= copy (l->item);
  while (!is_nil (l->next)) {
    l           = l->next;
    rectangle r2= l->item;
    r1->x1      = min (r1->x1, r2->x1);
    r1->y1      = min (r1->y1, r2->y1);
    r1->x2      = max (r1->x2, r2->x2);
    r1->y2      = max (r1->y2, r2->y2);
  }
  return r1;
}

rectangle
least_upper_bound (array<rectangle> l) {
  ASSERT (N (l) != 0, "no rectangles in list");
  rectangle r1= l[0];
  for (int i= 1; i < N (l); i++) {
    rectangle r2= l[i];
    r1->x1      = min (r1->x1, r2->x1);
    r1->y1      = min (r1->y1, r2->y1);
    r1->x2      = max (r1->x2, r2->x2);
    r1->y2      = max (r1->y2, r2->y2);
  }
  return r1;
}

double
area (rectangles r) {
  double sum= 0.0;
  while (!is_nil (r)) {
    sum+= area (r->item);
    r= r->next;
  }
  return sum;
}
