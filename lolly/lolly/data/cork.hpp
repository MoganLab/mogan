/******************************************************************************
 * MODULE     : cork.hpp
 * DESCRIPTION: UTF-8 <-> Cork encoding conversions
 * COPYRIGHT  : (C) 2026  Da Shen
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
 * @brief Convert a UTF-8 string to Cork encoding.
 *
 * Codepoints without a direct Cork byte (and codepoints >= 256 that the
 * named-entity tables do not cover) are emitted as the literal sequence
 * "<#XXXX>" with the hex codepoint. ASCII control bytes without a mapping
 * pass through unchanged.
 *
 * @param input The UTF-8 string.
 * @return The Cork-encoded string.
 */
string utf8_to_cork (string input);

/**
 * @brief Convert a Cork-encoded string to UTF-8.
 *
 * Honours "<#XXXX>" escapes and decodes registered named entities
 * ("<less>", "<alpha>", ...). Cork bytes without a mapping pass through
 * unchanged.
 *
 * @param input The Cork-encoded string.
 * @return The UTF-8 string.
 */
string_u8 cork_to_utf8 (string input);

/**
 * @brief Convert a Cork-encoded string to UTF-8 using the strict table.
 *
 * Same as cork_to_utf8 but excludes the symbol-unicode-fallback table, so
 * long-arrow fallbacks ("<longuparrow>" etc.) are not decoded.
 *
 * @param input The Cork-encoded string.
 * @return The UTF-8 string.
 */
string_u8 strict_cork_to_utf8 (string input);

} // namespace data
} // namespace lolly
