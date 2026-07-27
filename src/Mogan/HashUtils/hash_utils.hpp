
/******************************************************************************
 * MODULE     : hash_utils.hpp
 * DESCRIPTION: Binary-safe hashing utilities (MD5 etc.)
 * COPYRIGHT  : (C) 2026  Mogan STEM authors
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef HASH_UTILS_HPP
#define HASH_UTILS_HPP

#include "string.hpp"

/**
 * @brief Compute binary-safe MD5 of a mogan string.
 *
 * mogan's `string` may contain embedded NUL bytes (e.g. PNG bytes stored
 * inside a texmacs raw-data node). The returned hex string is safe to use
 * as a cache key.
 *
 * @param s input bytes (length determined by N(s), not by strlen)
 * @return 32-character lowercase hex MD5 digest
 */
string md5_binary (string s);

#endif // HASH_UTILS_HPP
