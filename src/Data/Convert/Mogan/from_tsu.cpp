
/******************************************************************************
 * MODULE     : from_tsu.cpp
 * DESCRIPTION: Conversion from the TSU format
 * COPYRIGHT  : (C) 2024  Darcy Shen
 *              (C) 2026  Eshaan Gupta
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "convert.hpp"
#include "tree_helper.hpp"

#include <moebius/vars.hpp>

using namespace moebius;

/******************************************************************************
 * Conversion of TSU strings to TeXmacs trees
 ******************************************************************************/

tree
tsu_document_to_tree (string s) {
  tree error (ERROR, "bad format or data");

  if (starts (s, "<TSU|<tuple|")) {
    int           i            = index_of (s, '>');
    string        version_tuple= s (N ("<TSU|<tuple|"), i);
    array<string> version_arr  = tokenize (version_tuple, "|");
    string        tsu_version  = version_arr[0];
    string        xmacs_version= version_arr[1];
    tree          doc          = tmu_to_tree (s, xmacs_version);

    if (is_compound (doc, "TeXmacs", 1) || is_expand (doc, "TeXmacs", 1) ||
        is_apply (doc, "TeXmacs", 1))
      doc= tree (DOCUMENT, doc);

    if (!is_document (doc)) return error;

    if (N (doc) == 0 || !is_compound (doc[0], "TeXmacs", 1)) {
      tree d (DOCUMENT);
      d << compound ("TeXmacs", string (TEXMACS_VERSION));
      d << A (doc);
      doc= d;
    }

    return doc;
  }
  return error;
}
