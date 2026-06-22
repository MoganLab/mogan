/* s7_ctables.h - character classification table declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_CTABLES_H
#define S7_CTABLES_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

#define S7_CTABLE_SIZE 256

extern bool *exponent_table;
extern bool *slashify_table;
extern bool *char_ok_in_a_name;
extern bool *white_space;
extern bool *number_table;
extern bool *symbol_slashify_table;
extern int32_t *digits;

void init_ctables(void);

#ifdef __cplusplus
}
#endif

#endif /* S7_CTABLES_H */
