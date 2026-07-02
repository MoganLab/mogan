/******************************************************************************
 * MODULE     : glue_l2_extra.hpp
 * DESCRIPTION: helper functions used by glue_lolly (generated, standalone)
 *              and by init_glue_l2.cpp's own procedure registrations.
 *              Extracted from init_glue_l2.cpp so that glue_lolly.cpp can
 *              be compiled as an independent translation unit.
 ******************************************************************************/

#ifndef GLUE_L2_EXTRA_HPP
#define GLUE_L2_EXTRA_HPP

#include "object_l1.hpp"
#include "object_l2.hpp"
#include "s7_tm.hpp"

#include "analyze.hpp"
#include "file.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tree.hpp"
#include "url.hpp"

#include <lolly/io/http.hpp>

#include "scheme.hpp"

using lolly::io::http_head;
using lolly::io::http_label;

inline tmscm
blackboxP (tmscm t) {
  bool b= tmscm_is_blackbox (t);
  return bool_to_tmscm (b);
}

inline tmscm
treeP (tmscm t) {
  bool b= tmscm_is_blackbox (t) &&
          (type_box (tmscm_to_blackbox (t)) == type_helper<tree>::id);
  return bool_to_tmscm (b);
}

inline tmscm
urlP (tmscm t) {
  bool b= tmscm_is_url (t);
  return bool_to_tmscm (b);
}

inline url
url_ref (url u, int i) {
  return u[i];
}

inline string
lolly_version () {
  return string (LOLLY_VERSION);
}

inline long
http_status_code (url u) {
  long status_code= open_box<long> (
      http_response_ref (http_head (u), http_label::STATUS_CODE)->data);
  return status_code;
}

#endif
