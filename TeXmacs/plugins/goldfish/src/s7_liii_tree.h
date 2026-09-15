/* s7_liii_tree.h - tree utility declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_LIII_TREE_H
#define S7_LIII_TREE_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Scheme-callable g_ procedures */
s7_pointer g_tree_is_cyclic(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_leaves(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_memq(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_member(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_set_memq(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_set_memq_syms(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_count(s7_scheme *sc, s7_pointer args);

/* Core tree functions called across interpreter and optimizer */
bool tree_is_cyclic(s7_scheme *sc, s7_pointer tree);
s7_int tree_len(s7_scheme *sc, s7_pointer tree);
s7_int tree_leaves_i_7p(s7_scheme *sc, s7_pointer tree);
s7_pointer tree_leaves_p_p(s7_scheme *sc, s7_pointer tree);

bool s7_tree_memq(s7_scheme *sc, s7_pointer sym, s7_pointer tree);
bool tree_memq_1(s7_scheme *sc, s7_pointer sym, s7_pointer tree);
bool tree_memq_b_7pp(s7_scheme *sc, s7_pointer sym, s7_pointer tree);
bool tree_including_quote_memq(s7_scheme *sc, s7_pointer sym, s7_pointer tree);

bool tree_member(s7_scheme *sc, s7_pointer obj, s7_pointer tree, s7_pointer compare);

bool tree_set_memq_b_7pp(s7_scheme *sc, s7_pointer syms, s7_pointer tree);
s7_pointer tree_set_memq_p_pp(s7_scheme *sc, s7_pointer syms, s7_pointer tree);
s7_pointer tree_set_memq_syms_direct(s7_scheme *sc, s7_pointer syms, s7_pointer tree);
s7_pointer tree_set_memq_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr);

s7_int tree_count(s7_scheme *sc, s7_pointer obj, s7_pointer tree, s7_int count);
s7_int tree_count_at_least(s7_scheme *sc, s7_pointer obj, s7_pointer tree, s7_int count, s7_int top);

/* Documentation strings and signatures */
#define H_tree_is_cyclic "(tree-cyclic? tree) returns #t if the tree has a cycle."
#define Q_tree_is_cyclic sc->pl_bt

#define H_tree_leaves "(tree-leaves tree) returns the number of leaves in the tree"
#define Q_tree_leaves s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_list_symbol)

#define H_tree_memq "(tree-memq? obj tree) is a tree-oriented version of memq, but returning #t if the object is in the tree."
#define Q_tree_memq s7_make_signature(sc, 3, sc->is_boolean_symbol, sc->T, sc->is_list_symbol)

#define H_tree_member "(tree-member? obj tree [compare]) returns #t if obj is in tree, using compare (defaulting to equal?)"
#define Q_tree_member s7_make_signature(sc, 4, sc->is_boolean_symbol, sc->T, sc->is_list_symbol, sc->is_procedure_symbol)

#define H_tree_set_memq "(tree-set-memq symbols tree) returns #t if any of the list of symbols is in the tree"
#define Q_tree_set_memq s7_make_signature(sc, 3, sc->is_boolean_symbol, sc->is_list_symbol, sc->is_list_symbol)

#define H_tree_count "(tree-count obj tree max-count) returns how many times obj is in tree (using eq?), stopping at max-count (if specified)"
#define Q_tree_count s7_make_signature(sc, 4, sc->is_integer_symbol, sc->T, sc->is_list_symbol, sc->is_integer_symbol)

#ifdef __cplusplus
}
#endif

#endif /* S7_LIII_TREE_H */
