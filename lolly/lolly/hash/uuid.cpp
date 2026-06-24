
/******************************************************************************
 * MODULE     : uuid.cpp
 * DESCRIPTION: UUID generation
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "uuid.hpp"
#include "basic.hpp"

#include <tbox/tbox.h>

namespace lolly {
namespace hash {
string
uuid_make () {
  tb_char_t        uuid[37];
  const tb_char_t* ret= tb_uuid4_make_cstr (uuid, tb_null);
  if (ret == NULL) {
    TM_FAILED ("Failed to generate UUID");
  }
  return string (ret);
}
} // namespace hash
} // namespace lolly
