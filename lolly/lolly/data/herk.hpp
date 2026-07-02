
/******************************************************************************
 * MODULE     : herk.hpp
 * DESCRIPTION: Herk encoding conversions
 * COPYRIGHT  : (C) 2026  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#pragma once

#include "string.hpp"

namespace lolly {
namespace data {

/**
 * @brief Convert a Herk-encoded string to UTF-8.
 * @param input The Herk-encoded string.
 * @return The UTF-8 string.
 */
string_u8 herk_to_utf8 (string input);

/**
 * @brief Convert a UTF-8 string to Herk encoding.
 * @param input The UTF-8 string.
 * @return The Herk-encoded string.
 */
string utf8_to_herk (string_u8 input);

} // namespace data
} // namespace lolly
