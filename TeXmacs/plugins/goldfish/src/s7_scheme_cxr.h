/* s7_scheme_cxr.h - c[ad]+r optimizer function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_CXR_H
#define S7_SCHEME_CXR_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

/* typed-arg (pl_p) versions of the c[ad]+r functions, used by the
   s7 optimizer via function pointer comparison, so they must be
   extern (single definition in s7_scheme_cxr.c) */

s7_pointer caar_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cadr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cdar_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cddr_p_p(s7_scheme *sc, s7_pointer lst);

s7_pointer caaar_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer caadr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cadar_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cdaar_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer caddr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cdddr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cdadr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cddar_p_p(s7_scheme *sc, s7_pointer lst);

s7_pointer caaddr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cadddr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cadadr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer caddar_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cddddr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cddadr_p_p(s7_scheme *sc, s7_pointer lst);
s7_pointer cdddar_p_p(s7_scheme *sc, s7_pointer lst);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_CXR_H */
