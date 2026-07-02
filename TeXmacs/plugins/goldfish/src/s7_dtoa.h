/* s7_dtoa.h - double-to-ASCII conversion (Grisu2 algorithm)
 *
 * derived from fpconv (MIT License)
 * SPDX-License-Identifier: MIT
 */

#ifndef S7_DTOA_H
#define S7_DTOA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int fpconv_dtoa(double value, char *buffer);

#ifdef __cplusplus
}
#endif

#endif /* S7_DTOA_H */
