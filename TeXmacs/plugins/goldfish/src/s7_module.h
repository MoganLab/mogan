/* s7_module.h - module system declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_MODULE_H
#define S7_MODULE_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_load_path_set(s7_scheme *sc, s7_pointer args);
s7_pointer g_features_set(s7_scheme *sc, s7_pointer args);
s7_pointer g_libraries_set(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_MODULE_H */
