/******************************************************************************
 * MODULE     : cork.cpp
 * DESCRIPTION: UTF-8 <-> Cork encoding conversions
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "cork.hpp"
#include "numeral.hpp"
#include "unicode.hpp"

#include <cstring>

namespace lolly {
namespace data {

#include "cork_data.inc"

// Look up a multi-byte codepoint (>= 128) in the sorted table. Returns -1 if
// no entry exists.
static inline int
utf8_to_cork_cp_index (uint32_t code) {
  int lo= 0, hi= utf8_to_cork_cp_count - 1;
  while (lo <= hi) {
    int mid= (lo + hi) >> 1;
    if (utf8_to_cork_cp_codepoints[mid] == code) return mid;
    if (utf8_to_cork_cp_codepoints[mid] < code) lo= mid + 1;
    else hi= mid - 1;
  }
  return -1;
}

// Scan a window starting at `pos` for the next '>' (no further than the
// longest entity key, 24 bytes). On success sets *key_len to the length
// including '>' and returns true; otherwise returns false.
static inline bool
cork_scan_entity_window (const char* base, int n, int pos, int* key_len) {
  int end  = pos;
  int limit= n;
  if (limit > pos + 24) limit= pos + 24;
  while (end < limit && base[end] != '>')
    end++;
  if (end >= limit) return false;
  *key_len= end - pos + 1;
  return true;
}

// Compare a candidate entity (offset/len into the pool) against the query
// bytes [p, p+key_len). Returns <0/0/>0 in the usual lexicographic sense,
// treating the shorter prefix as smaller when one side is a prefix of the
// other (matching how the entity table is sorted).
static inline int
cork_cmp_entity (const unsigned char* pool, int off, int mid_len,
                 const unsigned char* p, int key_len) {
  int min_len= mid_len < key_len ? mid_len : key_len;
  int cmp    = min_len == 0 ? 0 : memcmp (pool + off, p, min_len);
  if (cmp != 0) return cmp;
  return mid_len - key_len; // equal prefix -> shorter sorts first
}

// Binary-search the sorted `<name>` entity table for the bytes starting at
// `pos`. On success returns the entity index and sets *consumed to the key
// length; otherwise returns -1.
static int
cork_to_utf8_entity_at (const char* base, int n, int pos, int* consumed) {
  int key_len;
  if (!cork_scan_entity_window (base, n, pos, &key_len)) return -1;
  const unsigned char* p = reinterpret_cast<const unsigned char*> (base + pos);
  int                  lo= 0, hi= cork_to_utf8_entity_count - 1;
  while (lo <= hi) {
    int mid= (lo + hi) >> 1;
    int cmp= cork_cmp_entity (cork_to_utf8_entity_key_pool,
                              cork_to_utf8_entity_keys[mid].off,
                              cork_to_utf8_entity_keys[mid].len, p, key_len);
    if (cmp == 0) {
      *consumed= key_len;
      return mid;
    }
    if (cmp < 0) lo= mid + 1;
    else hi= mid - 1;
  }
  return -1;
}

// Same as above but restricted to the strict subset (no fallback). Returns
// the index into the full entity table on hit.
static int
strict_cork_to_utf8_entity_at (const char* base, int n, int pos,
                               int* consumed) {
  int key_len;
  if (!cork_scan_entity_window (base, n, pos, &key_len)) return -1;
  const unsigned char* p = reinterpret_cast<const unsigned char*> (base + pos);
  int                  lo= 0, hi= strict_cork_to_utf8_entity_count - 1;
  while (lo <= hi) {
    int mid= (lo + hi) >> 1;
    int idx= strict_cork_to_utf8_entity_indices[mid];
    int cmp= cork_cmp_entity (cork_to_utf8_entity_key_pool,
                              cork_to_utf8_entity_keys[idx].off,
                              cork_to_utf8_entity_keys[idx].len, p, key_len);
    if (cmp == 0) {
      *consumed= key_len;
      return idx;
    }
    if (cmp < 0) lo= mid + 1;
    else hi= mid - 1;
  }
  return -1;
}

// Check the 3 non-entity multi-byte keys (`%\x18`, `%\x18\x18`, `...`).
static int
cork_to_utf8_special_at (const char* base, int n, int pos, int* consumed) {
  for (int i= 0; i < cork_to_utf8_special_count; i++) {
    int len= cork_to_utf8_special_keys[i].len;
    if (pos + len > n) continue;
    if (memcmp (base + pos,
                cork_to_utf8_special_key_pool +
                    cork_to_utf8_special_keys[i].off,
                len) == 0) {
      *consumed= len;
      return i;
    }
  }
  return -1;
}

string
utf8_to_cork (string input) {
  int    i, n= N (input);
  string output;
  for (i= 0; i < n;) {
    int      start= i;
    uint32_t code = decode_from_utf8 (input, i);
    // ASCII (code < 128), or invalid UTF-8 bytes that decode_from_utf8
    // shrunk to a single byte (lone continuation/lead bytes). Both reach
    // the 1-byte table; entries with len==0 pass the byte through unchanged,
    // reproducing the original copy_unmatched behaviour.
    if (code < 128 || (i - start == 1 && code < 256)) {
      const cork_slice* sl= code < 128 ? &utf8_to_cork_ascii[code] : nullptr;
      if (sl != nullptr && sl->len > 0) {
        output << string ((char*) utf8_to_cork_ascii_pool + sl->off, sl->len);
      }
      else {
        output << input (start, i);
      }
      continue;
    }
    int idx= utf8_to_cork_cp_index (code);
    if (idx >= 0) {
      const cork_slice& sl= utf8_to_cork_cp_values[idx];
      output << string ((char*) utf8_to_cork_cp_value_pool + sl.off, sl.len);
    }
    else {
      // Unmapped codepoint >= 256: emit as <#XXXX>. Mirrors the
      // `r == s && code >= 256` fallback in the original utf8_to_cork.
      output << "<#" * to_Hex ((int) code) * ">";
    }
  }
  return output;
}

// Emit one Cork byte through the per-byte UTF-8 table. Bytes without a
// mapping (i.e. '<' and '>' alone) pass through unchanged.
static inline void
cork_to_utf8_emit_byte (string_u8& r, unsigned char b) {
  const cork_slice& sl= cork_to_utf8_byte[b];
  if (sl.len > 0) {
    r << string ((char*) cork_to_utf8_byte_pool + sl.off, sl.len);
  }
  else {
    r << string ((char) b, 1);
  }
}

// Shared body for cork_to_utf8 and strict_cork_to_utf8. `strict` selects
// between the full entity table and the fallback-excluded subset.
static string_u8
cork_to_utf8_impl (string input, bool strict) {
  int         i= 0, n= N (input);
  const char* base= input.begin ();
  string_u8   r;
  while (i < n) {
    char c= input[i];

    // 1) <#XXXX> hexadecimal escape (highest precedence -- emits utf8 of
    //    the explicit codepoint, bypassing the entity / byte tables).
    if (c == '<' && i + 1 < n && input[i + 1] == '#') {
      int hex_start= i + 2;
      int j        = hex_start;
      while (j < n && input[j] != '>')
        j++;
      string hex_str= input (hex_start, j);
      r << encode_as_utf8 ((uint32_t) from_hex (hex_str));
      i= (j < n) ? j + 1 : n;
      continue;
    }

    // 2) `<name>` entity. Only attempted when current byte is '<'.
    if (c == '<') {
      int consumed= 0;
      int idx= strict ? strict_cork_to_utf8_entity_at (base, n, i, &consumed)
                      : cork_to_utf8_entity_at (base, n, i, &consumed);
      if (idx >= 0) {
        const cork_slice& sl= cork_to_utf8_entity_values[idx];
        r << string ((char*) cork_to_utf8_entity_value_pool + sl.off, sl.len);
        i+= consumed;
        continue;
      }
      // No entity matched at this position; fall through to per-byte handling.
    }

    // 3) Non-entity multi-byte key (`%\x18`, `%\x18\x18`, `...`). Tried
    //    before the 1-byte table so the longest-prefix match wins, matching
    //    the original trie's behaviour.
    if (c == '%' || c == '.') {
      int consumed= 0;
      int idx     = cork_to_utf8_special_at (base, n, i, &consumed);
      if (idx >= 0) {
        const cork_slice& sl= cork_to_utf8_special_values[idx];
        r << string ((char*) cork_to_utf8_special_value_pool + sl.off, sl.len);
        i+= consumed;
        continue;
      }
    }

    // 4) Single-byte Cork -> UTF-8, or passthrough if no entry.
    cork_to_utf8_emit_byte (r, (unsigned char) c);
    i++;
  }
  return r;
}

string_u8
cork_to_utf8 (string input) {
  return cork_to_utf8_impl (input, /*strict=*/false);
}

string_u8
strict_cork_to_utf8 (string input) {
  return cork_to_utf8_impl (input, /*strict=*/true);
}

} // namespace data
} // namespace lolly
