
/******************************************************************************
 * MODULE     : glue_l2.cpp
 * DESCRIPTION: L2 Glue for linking TeXmacs commands to scheme
 * COPYRIGHT   : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
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
#include <lolly/data/herk.hpp>
#include <lolly/data/numeral.hpp>
#include <lolly/data/unicode.hpp>
#include <lolly/io/http.hpp>
#include <lolly/system/subprocess.hpp>

#include "glue_l2_extra.hpp"
#include "scheme.hpp"

void
initialize_glue_l2 () {
  tmscm_install_procedure ("blackbox?", blackboxP, 1, 0, 0);
  tmscm_install_procedure ("tree?", treeP, 1, 0, 0);
  tmscm_install_procedure ("url?", urlP, 1, 0, 0);
  initialize_glue_lolly ();
}
