
/******************************************************************************
 * MODULE     : http.hpp
 * DESCRIPTION: HTTP related routines
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#pragma once

#include "blackbox.hpp"
#include "hashmap.hpp"
#include "lolly/data/lolly_tree.hpp"
#include "url.hpp"

namespace lolly {
namespace io {

typedef lolly_tree<blackbox> http_tree;

enum http_label : int {
  STATUS_CODE= 1,
  TEXT,
  URL,
  ELAPSED,
  HEADER,
  PARAMETERS,
  PAYLOAD,
  MULTIPART,
  TUPLE,
  ROOT,
};

using http_headers= hashmap<string, string>;

template <typename T> inline http_tree blackbox_tree (int label, T data);

inline http_tree
http_response_init () {
  http_tree ret= http_tree (http_label::ROOT, 0);

  ret << blackbox_tree<long> (http_label::STATUS_CODE, 404);
  ret << blackbox_tree<string> (http_label::TEXT, string (""));
  ret << blackbox_tree<string> (http_label::URL, string (""));
  ret << blackbox_tree<double> (http_label::STATUS_CODE, 0.0);
  ret << blackbox_tree<hashmap<string, string>> (http_label::HEADER,
                                                 hashmap<string, string> ());
  return ret;
}

inline http_tree
http_response_ref (http_tree r, http_label op) {
  return r[op - 1];
}

inline void
http_response_set (http_tree r, http_label op, http_tree t) {
  r[op - 1]= t;
}

http_tree http_get (url u, http_headers headers= http_headers ());
http_tree http_head (url u, http_headers headers= http_headers ());
http_tree download (url from, url to, http_headers headers= http_headers ());

} // namespace io
} // namespace lolly
