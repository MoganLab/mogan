/* s7_liii_vector.h - vector utility declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_LIII_VECTOR_H
#define S7_LIII_VECTOR_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_is_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_float_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_int_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_byte_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_complex_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_string_to_byte_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_byte_vector_to_string(s7_scheme *sc, s7_pointer args);
s7_pointer g_float_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_int_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_byte_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_complex_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_rank(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_dimension(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_dimensions(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_subvector(s7_scheme *sc, s7_pointer args);
s7_pointer g_subvector_position(s7_scheme *sc, s7_pointer args);
s7_pointer g_subvector_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_typer(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_3(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_ref(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_ref_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_cv_ref_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_fv_ref_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_iv_ref_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_cv_set_3(s7_scheme *sc, s7_pointer args);

#if !WITH_PURE_S7
s7_pointer g_list_to_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_length(s7_scheme *sc, s7_pointer args);
s7_pointer g_make_vector(s7_scheme *sc, s7_pointer args);
s7_pointer g_vector_fill(s7_scheme *sc, s7_pointer args);
s7_int vector_length_i_7p(s7_scheme *sc, s7_pointer vec);
s7_pointer vector_length_p_p(s7_scheme *sc, s7_pointer vec);
#endif

#ifdef __cplusplus
}
#endif

#endif /* S7_LIII_VECTOR_H */
