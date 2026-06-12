
/******************************************************************************
 * MODULE     : glue_l2.cpp
 * DESCRIPTION: L2 Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "init_glue_l2.hpp"
#include "object_l1.hpp"
#include "object_l2.hpp"
#include "s7_tm.hpp"

#include "analyze.hpp"
#include "file.hpp"
#include "locale.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tree.hpp"

#include <lolly/data/base64.hpp>
#include <lolly/data/numeral.hpp>
#include <lolly/data/unicode.hpp>
#include <lolly/io/http.hpp>
#include <lolly/system/subprocess.hpp>

#include "scheme.hpp"

using lolly::io::http_head;
using lolly::io::http_label;

tmscm
blackboxP (tmscm t) {
  bool b= tmscm_is_blackbox (t);
  return bool_to_tmscm (b);
}

tmscm
treeP (tmscm t) {
  bool b= tmscm_is_blackbox (t) &&
          (type_box (tmscm_to_blackbox (t)) == type_helper<tree>::id);
  return bool_to_tmscm (b);
}

tmscm
urlP (tmscm t) {
  bool b= tmscm_is_url (t);
  return bool_to_tmscm (b);
}

url
url_ref (url u, int i) {
  return u[i];
}

string
lolly_version () {
  return string (LOLLY_VERSION);
}

long
http_status_code (url u) {
  long status_code= open_box<long> (
      http_response_ref (http_head (u), http_label::STATUS_CODE)->data);
  return status_code;
}

tmscm
binary_load (tmscm u) {
  TMSCM_ASSERT_URL (u, TMSCM_ARG1, "binary-load");
  url      file_url= tmscm_to_url (u);
  string   data    = tm_string_load_bytes (file_url);
  s7_int   len     = N (data);
  s7_pointer bv    = s7_make_byte_vector (tm_s7, len, 1, NULL);
  if (len > 0) {
    uint8_t* dest= s7_byte_vector_elements (bv);
    memcpy (dest, data.begin (), len);
  }
  return bv;
}

tmscm
binary_save (tmscm u, tmscm bv) {
  TMSCM_ASSERT_URL (u, TMSCM_ARG1, "binary-save");
  if (!s7_is_byte_vector (bv)) {
    TMSCM_ASSERT (false, bv, TMSCM_ARG2, "binary-save");
  }
  url        file_url= tmscm_to_url (u);
  s7_int     len     = s7_vector_length (bv);
  string     data;
  data->resize (len);
  if (len > 0) {
    uint8_t* src= s7_byte_vector_elements (bv);
    for (s7_int i= 0; i < len; i++) {
      data[i]= src[i];
    }
  }
  tm_string_save_bytes (data, file_url);
  return TMSCM_UNSPECIFIED;
}

#include "cork.hpp"
#include "glue_lolly.cpp"

void
initialize_glue_l2 () {
  tmscm_install_procedure ("blackbox?", blackboxP, 1, 0, 0);
  tmscm_install_procedure ("tree?", treeP, 1, 0, 0);
  tmscm_install_procedure ("url?", urlP, 1, 0, 0);
  tmscm_install_procedure ("binary-load", binary_load, 1, 0, 0);
  tmscm_install_procedure ("binary-save", binary_save, 2, 0, 0);
  initialize_glue_lolly ();
}
