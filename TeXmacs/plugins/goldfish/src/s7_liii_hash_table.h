/* s7_liii_hash_table.h - hash-table utility declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_LIII_HASH_TABLE_H
#define S7_LIII_HASH_TABLE_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_is_hash_table(s7_scheme *sc, s7_pointer args);

s7_pointer g_hash_table_size(s7_scheme *sc, s7_pointer args);
s7_int hash_table_size_i_7p(s7_scheme *sc, s7_pointer table);

s7_pointer g_hash_table_ref_2(s7_scheme *sc, s7_pointer args);

s7_pointer g_hash_table_key_typer(s7_scheme *sc, s7_pointer args);
s7_pointer g_hash_table_value_typer(s7_scheme *sc, s7_pointer args);

s7_pointer g_hash_table_set(s7_scheme *sc, s7_pointer args);

s7_pointer g_hash_table_ref(s7_scheme *sc, s7_pointer args);

s7_pointer g_hash_table(s7_scheme *sc, s7_pointer args);
s7_pointer g_hash_table_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_weak_hash_table(s7_scheme *sc, s7_pointer args);

s7_pointer g_make_hash_table(s7_scheme *sc, s7_pointer args);
s7_pointer g_make_weak_hash_table(s7_scheme *sc, s7_pointer args);

s7_pointer g_is_weak_hash_table(s7_scheme *sc, s7_pointer args);
s7_pointer g_hash_code(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_LIII_HASH_TABLE_H */
