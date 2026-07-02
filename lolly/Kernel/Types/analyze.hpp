
/******************************************************************************
 * MODULE     : analyze.hpp
 * DESCRIPTION: Properties of characters and strings
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef ANALYZE_H
#define ANALYZE_H

#include <stdint.h>

#include "array.hpp"
#include "hashset.hpp"
#include "ntuple.hpp"
#include "string.hpp"

class object;

inline bool
is_alpha (char c) {
  return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'));
}
inline bool
is_locase (char c) {
  return (c >= 'a') && (c <= 'z');
}
inline bool
is_upcase (char c) {
  return (c >= 'A') && (c <= 'Z');
}
inline bool
is_digit (char c) {
  return (c >= '0') && (c <= '9');
}
inline bool
is_binary_digit (char c) {
  return c == '0' || c == '1';
}
inline bool
is_octal_digit (char c) {
  return c >= '0' && c <= '7';
}
inline bool
is_hex_digit (char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
         (c >= 'a' && c <= 'f');
}
inline bool
is_numeric (char c) {
  return ((c >= '0') && (c <= '9')) || (c == '.');
}
inline bool
is_punctuation (char c) {
  return (c == '.') || (c == ',') || (c == ':') || (c == '\'') || (c == '`') ||
         (c == ';') || (c == '!') || (c == '?');
}
inline bool
is_space (char c) {
  return (c == ' ') || (c == '\11') || (c == '\12') || (c == '\15');
}

/**
 * @brief Checks if a character is an ISO alphabetic character
 *
 * @param c The character to be checked.
 * @return True if the character is an ISO alphabetic character; otherwise,
 * returns False.
 */
bool is_iso_alpha (char c);

/**
 * @brief Checks if a character is an ISO lowercase alphabetic character.
 *
 * @param c The character to check.
 * @return True if the character is ISO lowercase alphabetic, otherwise false.
 */
bool is_iso_locase (char c);

/**
 * @brief Checks if a character is an ISO uppercase alphabetic character.
 *
 * @param c The character to check.
 * @return True if the character is ISO uppercase alphabetic, otherwise false.
 */
bool is_iso_upcase (char c);

/**
 * @brief Checks if a string contains only alphabetic characters.
 *
 * @param s The string to check.
 * @return True if all characters in the string are alphabetic, otherwise false.
 */
bool is_alpha (string s);

bool is_alphanum (string s);

/**
 * @brief Checks if a string contains only lowercase alphabetic characters.
 *
 * @param s The string to check.
 * @return True if all characters in the string are lowercase alphabetic,
 * otherwise false.
 */
bool is_locase_alpha (string s);

/**
 * @brief Checks if a string contains only ISO alphabetic characters.
 *
 * @param s The string to check.
 * @return True if all characters in the string are ISO alphabetic, otherwise
 * false.
 */
bool is_iso_alpha (string s);

/**
 * @brief Checks if a string contains only numeric characters.
 *
 * @param s The string to check.
 * @return True if all characters in the string are numeric, otherwise false.
 */
bool is_numeric (string s);

/**
 * @brief Converts a lowercase character to uppercase.
 *
 * @param c The character to convert.
 * @return The uppercase version of the character if it is an ISO lowercase
 * alphabetic character; otherwise, returns the original character.
 */
char upcase (char s);

/**
 * @brief Converts an uppercase character to lowercase.
 *
 * @param c The character to convert.
 * @return The lowercase version of the character if it is an ISO uppercase
 * alphabetic character; otherwise, returns the original character.
 */
char locase (char s);

/**
 * @brief Finds the closing delimiter corresponding to the given opening
 * delimiter.
 *
 * @param c The opening delimiter character.
 * @return The corresponding closing delimiter if known, otherwise returns the
 * original character.
 */
char closing_delimiter (char c);

/**
 * @brief Converts the first character of a string to uppercase.
 *
 * @param s The string to convert.
 * @return A new string where the first character is converted to uppercase, if
 * it is an ISO lowercase alphabetic character.
 */
string upcase_first (string s);

/**
 * @brief Converts the first character of a string to lowercase.
 *
 * @param s The string to convert.
 * @return A new string where the first character is converted to lowercase, if
 * it is an ISO uppercase alphabetic character.
 */
string locase_first (string s);

/**
 * @brief Converts all lowercase characters in a string to uppercase.
 *
 * @param s The string to convert.
 * @return A new string where all ISO lowercase alphabetic characters are
 * converted to uppercase.
 */
string upcase_all (string s);

/**
 * @brief Converts all uppercase characters in a string to lowercase.
 *
 * @param s The string to convert.
 * @return A new string where all ISO uppercase alphabetic characters are
 * converted to lowercase.
 */
string locase_all (string s);

/**
 * @brief Union of two strings.
 *
 * This function performs the union operation between two strings.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @return A new string containing the union of s1 and s2.
 */
string string_union (string s1, string s2);

/**
 * @brief Remove characters from one string that are in another string.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @return A new string containing s1 - s2.
 */
string string_minus (string s1, string s2);

/**
 * @brief Remove the prefix from s if matches
 * @param s the string
 * @param prefix the prefix
 * @return If the prefix matches, return s with prefix removed,
 * otherwise, return s
 */
string remove_prefix (string s, string prefix);

/**
 * @brief Remove the suffix from s if matches
 * @param s the string
 * @param prefix the suffix
 * @return If the suffix matches, return s with suffix removed,
 * otherwise, return s
 */
string remove_suffix (string s, string suffix);

string il2_to_cork (string s);
string cork_to_il2 (string s);

/**
 * @brief Convert ispanish string to Spanish string.
 *
 * @param s The ispanish string.
 * @return The converted Spanish string.
 */
string ispanish_to_spanish (string s);

/**
 * @brief Convert Spanish string to ispanish string.
 *
 * @param s The Spanish string.
 * @return The converted ispanish string.
 */
string spanish_to_ispanish (string s);

/**
 * @brief Convert igerman string to german string.
 *
 * @param s The igerman string.
 * @return The converted german string.
 */
string igerman_to_german (string s);

/**
 * @brief Convert german string to igerman string.
 *
 * @param s The german string.
 * @return The converted igerman string.
 */
string german_to_igerman (string s);

/**
 * @brief Converts tabs in a string to spaces.
 *
 * @param s The original string with tabs.
 * @param tw The tab width.
 * @return A new string with tabs replaced by spaces.
 */
string convert_tabs_to_spaces (string s, int w);

/**
 * @brief Generates an alphabetic string for an integer.
 *
 * @param nr The integer to be converted to an alphabetic string.
 * @return A string representing the alphabetic character.
 */
string alpha_nr (int nr);

/**
 * @brief Generates an uppercase alphabetic string for an integer.
 *
 * @param nr The integer to be converted to an alphabetic string.
 * @return A string representing the uppercase alphabetic character.
 */
string Alpha_nr (int nr);

/**
 * @brief Generates footnote symbols for a given integer.
 *
 * @param nr The integer to be converted to a footnote symbol.
 * @return A string representing the footnote symbol.
 */
string fnsymbol_nr (int nr);

/**
 * @brief Add quotes around a string to indicate it's a string, not a symbol.
 *
 * This function is used for marking the label of a STRING tree as representing
 * a string and not a symbol.
 *
 * @param s The input string.
 * @return A new string with quotes around it.
 */
string raw_quote (string s);

/**
 * @brief Remove quotes from a string label.
 *
 * This function is used to get the string value of a STRING tree label
 * representing a string.
 *
 * @param s The input string.
 * @return The string without the quotes.
 */
string raw_unquote (string s);

/**
 * @brief Escape a string for use in shell scripts.
 *
 * @param s The input string.
 * @return An escaped string.
 */
string escape_sh (string s);

/**
 * @brief Escape a string with generic escape sequences.
 *
 * @param s The input string.
 * @return An escaped string.
 */
string escape_generic (string s);

/**
 * @brief Escape a string to be displayed verbatim.
 *
 * @param s The input string.
 * @return An escaped string.
 */
string escape_verbatim (string s);

/**
 * @brief Escape spaces in a string with a backslash.
 *
 * @param s The input string.
 * @return A string with escaped spaces.
 */
string escape_spaces (string s);

/**
 * @brief Unescape a Guile-syntax string.
 *
 * @param s The input string.
 * @return A string with special characters unescaped.
 */
string unescape_guile (string s);

/**
 * @brief Convert DOS line endings to more standard line endings.
 *
 * @param s The input string.
 * @return A string with Unix-style line endings.
 */
string dos_to_better (string s);

bool test (string s, int i, const char* test);
bool test (string s, int i, string test);
bool starts (string s, const char* test);
bool starts (string s, const string test);
bool ends (string s, const char* test);
bool ends (string s, const string test);
bool read (string s, int& i, const char* test);
bool read (string s, int& i, string test);
bool read (string s, string test);
bool read_line (string s, int& i, string& result);
bool read_int (string s, int& i, int& result);
bool read_double (string s, int& i, double& result);
bool read_word (string s, int& i, string& result);
bool is_whitespace (string s);
void skip_spaces (string s, int& i);
void skip_whitespace (string s, int& i);
void skip_line (string s, int& i);
void skip_symbol (string s, int& i);

void parse (string s, int& pos, QI& ret);
void parse (string s, int& pos, QN& ret);
void parse (string s, int& pos, HI& ret);
void parse (string s, int& pos, HN& ret);
void parse (string s, int& pos, SI& ret);
void parse (string s, int& pos, SI*& a, int len);

int index_of (string s, char c);

/**
 * Searches for a substring in a string.
 *
 * @param what The substring to search for.
 * @param in The string to search in.
 * @return Position where the substring was found, or -1 if not found.
 */
int search_forwards (string what, string in);

/**
 * Searches for a substring in a string starting from a specified position.
 *
 * @param what The substring to search for.
 * @param pos The starting position in the string to search from.
 * @param in The string to search in.
 * @return Position where the substring was found, or -1 if not found.
 */
int search_forwards (string what, int pos, string in);

/**
 * Searches for a substring in a string starting from a specified position, in
 * reverse.
 *
 * @param what_list The substring to search for.
 * @param pos The starting position in the string to search from.
 * @param in The string to search in.
 * @return Position where the substring was found, or -1 if not found.
 */
int search_forwards (array<string> what_list, int pos, string in);

/**
 * Searches for a substring in a string, in reverse.
 *
 * @param s The substring to search for.
 * @param in The string to search in.
 * @return Position where the substring was found, or -1 if not found.
 */
int search_backwards (string what, string in);

/**
 * Searches for a substring in a string starting from a specified position, in
 * reverse.
 *
 * @param s The substring to search for.
 * @param pos The starting position in the string to search from.
 * @param in The string to search in.
 * @return Position where the substring was found, or -1 if not found.
 */
int search_backwards (string what, int pos, string in);

/**
 * Counts the occurrences of a substring in a string.
 *
 * @param s The substring to count.
 * @param in The string to search in.
 * @return Number of occurrences of the substring.
 */
int count_occurrences (string what, string in);

/**
 * Checks whether a substring occurs within another string.
 *
 * @param what The string to find.
 * @param in The string in which to search.
 * @return True if the string occurs, otherwise false.
 */
bool occurs (string what, string in);

bool contains (string s, string what);
bool contains (string s, char c);

/**
 * Finds the length of the longest string that is both a suffix of the first
 * string and a prefix of the second string.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @return The length of the longest overlapping string.
 */
int overlapping (string s1, string s2);

/**
 * Replaces all occurrences of a specific substring within a string.
 *
 * @param s The original string.
 * @param what The substring to replace.
 * @param by The string with which to replace the substring.
 * @return The new string with all occurrences of the substring replaced.
 */
string replace (string s, string what, string by);

/**
 * Matches a string against a wildcard pattern.
 *
 * @param s The string to match.
 * @param w The wildcard pattern.
 * @return True if the string matches the wildcard pattern, otherwise false.
 */
bool match_wildcard (string s, string w);

/**
 * @brief Calculates a fuzzy matching score between a pattern and target string.
 *
 * This implements a fuzzy matching algorithm similar to those used in
 * Sublime Text, fzf, and VS Code. It scans from left to right to match
 * input characters and scores based on match positions (consecutive matches,
 * word boundaries, uppercase letters get higher scores).
 *
 * @param pattern The pattern string to match (e.g., "npd").
 * @param target The target string to match against (e.g., "NumPyData").
 * @return A score where higher values indicate better matches, -1 for no match.
 *
 * Example: "npd" matching "NumPyData":
 * - 'n' matches 'N' at start (high score for first letter)
 * - 'p' matches 'P' at camelCase boundary (high score)
 * - 'd' matches 'D' at camelCase boundary (high score)
 */
int fuzzy_match_score (string pattern, string target);

/**
 * Finds the position of the first non-alphabetic character in a string.
 *
 * @param s The string to search.
 * @param pos The position from which to start the search.
 * @param forward True to search forward, false to search backward.
 * @return The position of the first non-alphabetic character; -1 if not found.
 */
int find_non_alpha (string s, int pos, bool forward);

/**
 * Splits a string into an array of strings based on a separator string.
 *
 * @param s The string to split.
 * @param sep The separator string.
 * @return An array of strings split based on the separator.
 */
array<string> tokenize (string s, string sep);

/**
 * Joins an array of strings into a single string, separated by a specific
 * string.
 *
 * @param a The array of strings to join.
 * @param sep The separator string.
 * @return A single string consisting of the array elements separated by the
 * separator.
 */
string recompose (array<string> a, string sep);

/**
 * Removes all leading spaces from a string.
 *
 * @param s The string to trim.
 * @return The string without leading spaces.
 */
string trim_spaces_left (string s);

/**
 * Removes all leading spaces from a string array.
 *
 * @param a The string array to trim.
 * @return The string without leading spaces.
 */
array<string> trim_spaces_left (array<string> a);

/**
 * Removes all trailing spaces from a string.
 *
 * @param s The string to trim.
 * @return The string without trailing spaces.
 */
string trim_spaces_right (string s);

/**
 * Removes all trailing spaces from a string array.
 *
 * @param s The string array to trim.
 * @return The string without trailing spaces.
 */
array<string> trim_spaces_right (array<string> a);

/**
 * Removes all leading and trailing spaces from a string.
 *
 * @param s The string to trim.
 * @return The string without leading and trailing spaces.
 */
string trim_spaces (string s);

/**
 * Removes all leading and trailing spaces from an array of strings.
 *
 * @param a The array of strings to trim.
 * @return An array of strings with all leading and trailing spaces removed.
 */
array<string> trim_spaces (array<string> a);

/**
 * @brief the differences between two strings by identifying the common
 * substrings and returning the different sections' indices.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @return An array containing sets of four integers:
 */
array<int> differences (string s1, string s2);

/**
 * @brief a measure of difference (distance) between two strings.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @return An integer representing the sum of lengths of all differing sections
 * between the two strings.
 */
int distance (string s1, string s2);

/**
 * @brief Parses a string containing a length value and its unit.
 *
 * @param[in] s    The string to parse, e.g., "12.34cm".
 * @param[out] len The parsed numerical value of the length.
 * @param[out] unit The unit of the length as a string, e.g., "cm".
 */
void parse_length (string s, double& len, string& unit);

#endif // defined ANALYZE_H
