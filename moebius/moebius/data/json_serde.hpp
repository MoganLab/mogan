/******************************************************************************
 * MODULE     : json_serde.hpp
 * DESCRIPTION: JSON serialization/deserialization for modification and patch,
 *              enabling network transport for collaborative editing
 * COPYRIGHT  : (C) 2026  cc-fuyu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/
#pragma once
#include "modification.hpp"
#include "patch.hpp"

namespace moebius {
namespace data {

/**
 * @brief Serialize a modification to a JSON-formatted string.
 *
 * The output format is:
 *   {"type":"assign","path":[0,1],"tree":"..."}
 *
 * This is designed for transmitting OT operations over WebSocket
 * in a collaborative editing session.
 *
 * @param mod The modification to serialize.
 * @return A JSON string representing the modification.
 */
string modification_to_json (modification mod);

/**
 * @brief Deserialize a modification from a JSON-formatted string.
 *
 * @param s A JSON string produced by modification_to_json.
 * @return The deserialized modification.
 */
modification json_to_modification (string s);

/**
 * @brief Serialize a path to a JSON array string.
 *
 * Uses JSON array of integers, e.g. "[0,1,3]".
 * An empty path is represented as "[]".
 *
 * @param p The path to serialize.
 * @return A JSON array string representation.
 */
string path_to_json_string (path p);

/**
 * @brief Deserialize a path from a JSON array string.
 *
 * @param s A JSON array string, e.g. "[0,1,3]".
 * @return The deserialized path.
 */
path json_string_to_path (string s);

/**
 * @brief Deserialize a path from a dot-separated string.
 *
 * @param s A dot-separated string, e.g. "0.1.3".
 * @return The deserialized path.
 */
path json_string_to_path (string s);

/**
 * @brief Serialize a tree to a JSON-compatible string.
 *
 * Uses the existing scheme serialization as the transport format.
 *
 * @param t The tree to serialize.
 * @return A scheme-formatted string representation.
 */
string tree_to_json_string (tree t);

/**
 * @brief Deserialize a tree from a scheme-formatted string.
 *
 * @param s A scheme-formatted string.
 * @return The deserialized tree.
 */
tree json_string_to_tree (string s);

} // namespace data
} // namespace moebius
