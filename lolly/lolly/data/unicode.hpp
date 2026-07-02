
/******************************************************************************
 * MODULE     : unicode.hpp
 * DESCRIPTION: Unicode support
 * COPYRIGHT  : (C) 2023  Darcy Shen
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
 * @brief Encode 4 bytes as string_u8
 * @param code The 4 bytes to encode
 * @return The UTF-8 string in string_u8
 */
string_u8 encode_as_utf8 (uint32_t code);

/**
 * @brief Decode string_u8 string as 4 bytes at position i
 * @param s The string_u8 string
 * @param i The position
 * @return The 4 bytes in uint32_t
 */
uint32_t decode_from_utf8 (string_u8 s, int& i);

string unicode_get_range (int code);

/**
 * @brief Checks if a string contains only CJK Unified Ideographs.
 *
 * @param s The string to check.
 * @return True if all characters in the string are CJK Unified Ideographs,
 * otherwise false.
 * @note This function expects the CJK Unified Ideographs to be in a specific
 * encoded format.
 */
bool is_cjk_unified_ideographs (string s);

/**
 * @brief Checks if a string contains any CJK Unified Ideographs.
 *
 * @param s The string to check.
 * @return True if the string contains at least one CJK Unified Ideograph,
 * otherwise false.
 * @note This function expects the CJK Unified Ideographs to be in a specific
 * encoded format.
 */
bool has_cjk_unified_ideographs (string s);

/**
 * @brief Convert UTF-16 string to UTF-8 string
 * @param s_u16 the string using the UTF-16 encoding
 * @return the string using the UTF-8 encoding
 * @note For invalid UTF-16 string, only the valid part will be converted
 */
string utf16_to_utf8 (string s_u16);

#if defined(OS_MINGW) || defined(_WIN32) || defined(_WIN64)
/**
 * @brief Convert wchar_t* to UTF-8 string
 * @param s_u16 the string using the UTF-16 encoding
 * @return the string using the UTF-8 encoding
 * @note For invalid UTF-16 string, only the valid part will be converted
 */
string wchar_to_utf8 (const wchar_t* s_u16);
#endif

/**
 * @brief Convert UTF-8 string to UTF-16 string
 * @param s_u8 the string using the UTF-8 encoding
 * @return the string using the UTF-16 encoding
 * @note For invalid UTF-8 string, only the valid part will be converted
 */
string utf8_to_utf16 (string s_u8);
} // namespace data
} // namespace lolly
