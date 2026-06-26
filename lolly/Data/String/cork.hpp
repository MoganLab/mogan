
/******************************************************************************
 * MODULE     : cork.hpp
 * DESCRIPTION: Routines on Cork Encoding
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TM_CORK_H
#define TM_CORK_H

#include "array.hpp"
#include "string.hpp"

/**
 * @brief Encodes a string by converting special characters to TeXmacs encoding.
 *
 * @param s The input string to be encoded.
 * @return The encoded string.
 */
string tm_encode (string s);

/**
 * @brief Decodes a TeXmacs encoded string.
 *
 * @param s The input string to be decoded.
 * @return The decoded string.
 */
string tm_decode (string s);

/**
 * @brief Encodes a string with special TeXmacs variables.
 *
 * @param s The input string to be encoded.
 * @return The encoded string.
 */
string tm_var_encode (string s);

/**
 * @brief Corrects a TeXmacs string by removing or correcting malformed
 * sequences.
 *
 * @param s The input string to be corrected.
 * @return The corrected string.
 */
string tm_correct (string s);

/**
 * @brief Moves the position one TeXmacs character forwards in a string.
 *
 * @param s The input string.
 * @param pos The current position in the string.
 */
void tm_char_forwards (string s, int& pos);

/**
 * @brief Moves the position one TeXmacs character backwards in a string.
 *
 * @param s The input string.
 * @param pos The current position in the string.
 */
void tm_char_backwards (string s, int& pos);

/**
 * @brief Gets the next TeXmacs character position in a string.
 *
 * @param s The input string.
 * @param pos The current position in the string.
 * @return The next TeXmacs character position.
 */
int tm_char_next (string s, int pos);

/**
 * @brief Gets the previous TeXmacs character position in a string.
 *
 * @param s The input string.
 * @param pos The current position in the string.
 * @return The previous TeXmacs character position.
 */
int tm_char_previous (string s, int pos);

/**
 * @brief Accesses the k-th TeXmacs character in a string, moving forwards.
 *
 * @param s The input string.
 * @param k The k-th position.
 * @return The k-th TeXmacs character.
 */
string tm_forward_access (string s, int i);

/**
 * @brief Accesses the k-th TeXmacs character in a string, moving backwards.
 *
 * @param s The input string.
 * @param k The k-th position.
 * @return The k-th TeXmacs character.
 */
string tm_backward_access (string s, int i);

/**
 * @brief Gets the length of a TeXmacs string.
 *
 * @param s The input string.
 * @return The length of the TeXmacs string.
 */
int tm_string_length (string s);

/**
 * @brief Tokenizes a TeXmacs string.
 *
 * @param s The input string.
 * @return An array containing the tokens.
 */
array<string> tm_tokenize (string s);

/**
 * @brief Splits a TeXmacs string based on its characteristics.
 *
 * @param s The input string.
 * @return An array containing the split substrings.
 */
array<string> tm_string_split (string s);

/**
 * @brief Recomposes a TeXmacs string from an array of tokens.
 *
 * @param a The input array of tokens.
 * @return The recomposed string.
 */
string tm_recompose (array<string> a);

/**
 * @brief Searches for a substring forwards in a given TeXmacs string.
 *
 * @param s The substring to search for.
 * @param pos The position in the string to start searching from.
 * @param in The string in which to search.
 * @return The starting position of the substring, or -1 if not found.
 */
int tm_search_forwards (string s, int pos, string in);

/**
 * @brief Searches for a substring backwards in a given TeXmacs string.
 *
 * @param s The substring to search for.
 * @param pos The position in the string to start searching from.
 * @param in The string in which to search.
 * @return The starting position of the substring, or -1 if not found.
 */
int tm_search_backwards (string s, int pos, string in);

/**
 * @brief Checks if the string contains unicode characters.
 *
 * @param s The string to check.
 * @return True if the string contains unicode characters, False otherwise.
 */
bool contains_unicode_char (string s);

/**
 * @brief Downgrades certain LaTeX-like mathematical letters to plain text.
 *
 * @param s The original string.
 * @return A new string with certain LaTeX-like letters downgraded.
 */
string downgrade_math_letters (string s);

#endif