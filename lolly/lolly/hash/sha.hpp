
/******************************************************************************
 * MODULE     : sha.hpp
 * DESCRIPTION: sha digest
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
enum sha_mode : long { SHA2_224= 224, SHA2_256= 256 };
string sha_hexdigest (url u, sha_mode mode);
string sha224_hexdigest (url u);
string sha256_hexdigest (url u);
} // namespace hash
} // namespace lolly
