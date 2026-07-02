
/******************************************************************************
 * MODULE     : uri.cpp
 * DESCRIPTION: URI
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "uri.hpp"

namespace lolly {
namespace data {

string
uri_host (url u) {
  return as_string (u[2][1]);
}

string
uri_path (url u) {
  return string ("/") * as_string (u[2][2], URL_STANDARD);
}

} // namespace data
} // namespace lolly
