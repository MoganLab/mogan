
/******************************************************************************
 * MODULE     : sha.cpp
 * DESCRIPTION: sha digest
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "sha.hpp"
#include "analyze.hpp"
#include "file.hpp"
#include "lolly/data/numeral.hpp"
#include "string.hpp"
#include "url.hpp"

#include <tbox/tbox.h>

namespace lolly {
namespace hash {
string
sha_hexdigest (url u, sha_mode mode) {
  if (!is_local_and_single (u)) {
    return string ("");
  }
  string      name= as_string (u);
  const char* path= as_charp (name);
  if (!tb_file_access (path, TB_FILE_MODE_RO)) {
    return string ("");
  }
  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return string ("");
  }
  tb_size_t i_size= tb_file_size (file);
  if (i_size == 0) {
    switch (mode) {
    case sha_mode::SHA2_224:
      return "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f";
    case sha_mode::SHA2_256:
      return "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    default:
      TM_FAILED ("Unsupported SHA mode");
    }
  }
  tb_file_sync (file); // lock file
  tb_byte_t* i_buffer   = tm_new_array<tb_byte_t> (i_size);
  tb_size_t  real_size  = tb_file_read (file, i_buffer, i_size);
  bool       read_sz_equ= (real_size == i_size);
  bool       exit_suc   = tb_file_exit (file); // exit file
  if (read_sz_equ && exit_suc) {
    tb_byte_t     o_buffer[32];
    int           o_size     = 32;
    tb_sha_mode_t tb_sha_mode= tb_sha_mode_t::TB_SHA_MODE_SHA2_256;
    switch (mode) {
    case sha_mode::SHA2_224:
      o_size     = 28;
      tb_sha_mode= tb_sha_mode_t::TB_SHA_MODE_SHA2_224;
      break;
    case sha_mode::SHA2_256:
      o_size     = 32;
      tb_sha_mode= tb_sha_mode_t::TB_SHA_MODE_SHA2_256;
      break;
    default:
      TM_FAILED ("Unsupported SHA mode");
    }
    tb_size_t sha_size= tb_sha_make (mode, i_buffer, i_size, o_buffer, o_size);
    if (sha_size != o_size) {
      return string ("");
    }

    string ret= string ();
    for (int i= 0; i < o_size; ++i) {
      ret << data::to_padded_hex (o_buffer[i]);
    }
    return ret;
  }
  else {
    return string ("");
  }
}

string
sha224_hexdigest (url u) {
  return sha_hexdigest (u, sha_mode::SHA2_224);
}

string
sha256_hexdigest (url u) {
  return sha_hexdigest (u, sha_mode::SHA2_256);
}
} // namespace hash
} // namespace lolly
