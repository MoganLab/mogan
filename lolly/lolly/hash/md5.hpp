
/******************************************************************************
 * MODULE     : md5.hpp
 * DESCRIPTION: md5 digest
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#pragma once

#include "string.hpp"
#include "url.hpp"

namespace lolly {
namespace hash {
string md5_hexdigest (url u);
}
} // namespace lolly
