/* s7_scheme_cxr.c - c[ad]+r optimizer function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_cxr.h"
#include "s7_internal_helpers.h"

#include <stddef.h>

/* the corresponding g_caar etc are in s7_liii_list.c; these are the
   typed-arg (pl_p) versions used by the optimizer */

#define CXR_A_LIST_STRING(sc, nm)                                                                                      \
  s7i_wrap_string (sc, "a pair whose " nm " is also a pair", (s7_int) (28 + sizeof (nm) - 1))
#define CXR_METHOD_OR_BUST(sc, nm, lst) s7i_sole_arg_method_or_bust (sc, lst, nm, s7i_set_plist_1 (sc, lst), "a pair")
#define CXR_WRONG_TYPE_ERROR_NR(sc, nm, lst)                                                                           \
  sole_arg_wrong_type_error_nr (sc, s7_make_symbol (sc, nm), lst, CXR_A_LIST_STRING (sc, nm))

/* -------- caar -------- */

s7_pointer
caar_p_p (s7_scheme* sc, s7_pointer lst) {
  if ((s7_is_pair (lst)) && (s7_is_pair (s7_car (lst)))) return (s7_caar (lst));
  if (s7_is_pair (lst)) CXR_WRONG_TYPE_ERROR_NR (sc, "caar", lst);
  return (CXR_METHOD_OR_BUST (sc, "caar", lst));
}

/* -------- cadr -------- */

s7_pointer
cadr_p_p (s7_scheme* sc, s7_pointer lst) {
  if ((s7_is_pair (lst)) && (s7_is_pair (s7_cdr (lst)))) return (s7_cadr (lst));
  if (s7_is_pair (lst)) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  return (CXR_METHOD_OR_BUST (sc, "cadr", lst));
}

/* -------- cdar -------- */

s7_pointer
cdar_p_p (s7_scheme* sc, s7_pointer lst) {
  if ((s7_is_pair (lst)) && (s7_is_pair (s7_car (lst)))) return (s7_cdar (lst));
  if (!s7_is_pair (lst)) CXR_WRONG_TYPE_ERROR_NR (sc, "car", lst);
  return (CXR_METHOD_OR_BUST (sc, "cdar", lst));
}

/* -------- cddr -------- */

s7_pointer
cddr_p_p (s7_scheme* sc, s7_pointer lst) {
  if ((s7_is_pair (lst)) && (s7_is_pair (s7_cdr (lst)))) return (s7_cddr (lst));
  if (s7_is_pair (lst)) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  return (CXR_METHOD_OR_BUST (sc, "cddr", lst));
}

/* -------- caaar -------- */

s7_pointer
caaar_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "caaar", lst));
  if (!s7_is_pair (s7_car (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "car", lst);
  if (!s7_is_pair (s7_caar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "caar", lst);
  if (!s7_is_pair (s7_caar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "caar", lst);
  return (s7_caaar (lst));
}

/* -------- caadr -------- */

s7_pointer
caadr_p_p (s7_scheme* sc, s7_pointer lst) {
  if ((s7_is_pair (lst)) && (s7_is_pair (s7_cdr (lst))) && (s7_is_pair (s7_cadr (lst)))) return (s7_caadr (lst));
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "caadr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  CXR_WRONG_TYPE_ERROR_NR (sc, "cadr", lst);
  return (NULL);
}

/* -------- cadar -------- */

s7_pointer
cadar_p_p (s7_scheme* sc, s7_pointer lst) {
  if ((s7_is_pair (lst)) && (s7_is_pair (s7_car (lst))) && (s7_is_pair (s7_cdar (lst)))) return (s7_cadar (lst));
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cadar", lst));
  if (!s7_is_pair (s7_car (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "car", lst);
  CXR_WRONG_TYPE_ERROR_NR (sc, "cdar", lst);
  return (NULL);
}

/* -------- cdaar -------- */

s7_pointer
cdaar_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cdaar", lst));
  if (!s7_is_pair (s7_car (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "car", lst);
  if (!s7_is_pair (s7_caar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "caar", lst);
  return (s7_cdaar (lst));
}

/* -------- caddr -------- */

s7_pointer
caddr_p_p (s7_scheme* sc, s7_pointer lst) {
  if ((s7_is_pair (lst)) && (s7_is_pair (s7_cdr (lst))) && (s7_is_pair (s7_cddr (lst)))) return (s7_caddr (lst));
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "caddr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  CXR_WRONG_TYPE_ERROR_NR (sc, "cddr", lst);
  return (NULL);
}

/* -------- cdddr -------- */

s7_pointer
cdddr_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cdddr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  if (!s7_is_pair (s7_cddr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cddr", lst);
  return (s7_cdddr (lst));
}

/* -------- cdadr -------- */

s7_pointer
cdadr_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cdadr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  if (!s7_is_pair (s7_cadr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cadr", lst);
  return (s7_cdadr (lst));
}

/* -------- cddar -------- */

s7_pointer
cddar_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cddar", lst));
  if (!s7_is_pair (s7_car (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "car", lst);
  if (!s7_is_pair (s7_cdar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdar", lst);
  return (s7_cddar (lst));
}

/* -------- caaddr -------- */

s7_pointer
caaddr_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "caaddr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  if (!s7_is_pair (s7_cddr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cddr", lst);
  if (!s7_is_pair (s7_caddr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "caddr", lst);
  return (s7_caaddr (lst));
}

/* -------- cadddr -------- */

s7_pointer
cadddr_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cadddr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  if (!s7_is_pair (s7_cddr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cddr", lst);
  if (!s7_is_pair (s7_cdddr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdddr", lst);
  return (s7_cadddr (lst));
}

/* -------- cadadr -------- */

s7_pointer
cadadr_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cadadr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  if (!s7_is_pair (s7_cadr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cadr", lst);
  if (!s7_is_pair (s7_cdadr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdadr", lst);
  return (s7_cadadr (lst));
}

/* -------- caddar -------- */

s7_pointer
caddar_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "caddar", lst));
  if (!s7_is_pair (s7_car (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "car", lst);
  if (!s7_is_pair (s7_cdar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdar", lst);
  if (!s7_is_pair (s7_cddar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cddar", lst);
  return (s7_caddar (lst));
}

/* -------- cddddr -------- */

s7_pointer
cddddr_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cddddr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  if (!s7_is_pair (s7_cddr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cddr", lst);
  if (!s7_is_pair (s7_cdddr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdddr", lst);
  return (s7_cddddr (lst));
}

/* -------- cddadr -------- */

s7_pointer
cddadr_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cddadr", lst));
  if (!s7_is_pair (s7_cdr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdr", lst);
  if (!s7_is_pair (s7_cadr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cadr", lst);
  if (!s7_is_pair (s7_cdadr (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdadr", lst);
  return (s7_cddadr (lst));
}

/* -------- cdddar -------- */

s7_pointer
cdddar_p_p (s7_scheme* sc, s7_pointer lst) {
  if (!s7_is_pair (lst)) return (CXR_METHOD_OR_BUST (sc, "cdddar", lst));
  if (!s7_is_pair (s7_car (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "car", lst);
  if (!s7_is_pair (s7_cdar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cdar", lst);
  if (!s7_is_pair (s7_cddar (lst))) CXR_WRONG_TYPE_ERROR_NR (sc, "cddar", lst);
  return (s7_cdddar (lst));
}
