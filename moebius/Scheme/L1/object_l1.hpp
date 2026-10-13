
/******************************************************************************
 * MODULE     : object_l1.hpp
 * DESCRIPTION: Implementation of scheme objects (L1)
 * COPYRIGHT  : (C) 1999-2011 Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef OBJECT_L1_HPP
#define OBJECT_L1_HPP

#include "command.hpp"
#include "modification.hpp"
#include "path.hpp"
#include "s7_tm.hpp"
#include "tree_helper.hpp"

#define content tree

/******************************************************************************
 * Basic assertions
 ******************************************************************************/

#define TMSCM_ASSERT_STRING(s, arg, rout)                                      \
  TMSCM_ASSERT (tmscm_is_string (s), s, arg, rout)
#define TMSCM_ASSERT_BOOL(flag, arg, rout)                                     \
  TMSCM_ASSERT (tmscm_is_bool (flag), flag, arg, rout)
#define TMSCM_ASSERT_INT(i, arg, rout)                                         \
  TMSCM_ASSERT (tmscm_is_int (i), i, arg, rout);
#define TMSCM_ASSERT_DI(i, arg, rout)                                          \
  TMSCM_ASSERT (tmscm_is_int (i), i, arg, rout);
#define TMSCM_ASSERT_DOUBLE(i, arg, rout)                                      \
  TMSCM_ASSERT (tmscm_is_double (i), i, arg, rout);
// TMSCM_ASSERT (SCM_REALP (i), i, arg, rout);
#define TMSCM_ASSERT_PATCH(p, arg, rout)                                       \
  TMSCM_ASSERT (tmscm_is_patch (p), p, arg, rout)
#define TMSCM_ASSERT_BLACKBOX(t, arg, rout)                                    \
  TMSCM_ASSERT (tmscm_is_blackbox (t), t, arg, rout)
#define TMSCM_ASSERT_SYMBOL(s, arg, rout)                                      \
  TMSCM_ASSERT (tmscm_is_symbol (s), s, arg, rout)
// TMSCM_ASSERT (SCM_NFALSEP (tmscm_symbol_p (s)), s, arg, rout)

#define TMSCM_ASSERT_OBJECT(a, b, c)
// no check

tmscm tree_to_tmscm (tree t);
tmscm tree_label_to_tmscm (tree_label l);
tmscm scheme_tree_to_tmscm (scheme_tree t);

tree        tmscm_to_tree (tmscm obj);
content     tmscm_to_content (tmscm obj);
tree_label  tmscm_to_tree_label (tmscm p);
scheme_tree tmscm_to_scheme_tree (tmscm p);

bool tmscm_is_tree (tmscm obj);
bool tmscm_is_content (tmscm p);

#define TMSCM_ASSERT_TREE(t, arg, rout)                                        \
  TMSCM_ASSERT (tmscm_is_tree (t), t, arg, rout)
#define TMSCM_ASSERT_SCHEME_TREE(p, arg, rout)
#define TMSCM_ASSERT_TREE_LABEL(p, arg, rout) TMSCM_ASSERT_SYMBOL (p, arg, rout)
#define TMSCM_ASSERT_CONTENT(p, arg, rout)                                     \
  TMSCM_ASSERT (tmscm_is_content (p), p, arg, rout)
#define content_to_tmscm tree_to_tmscm

typedef list<string> list_string;
typedef list<tree>   list_tree;

tmscm list_string_to_tmscm (list<string> l);
tmscm list_tree_to_tmscm (list<tree> l);

list<string> tmscm_to_list_string (tmscm obj);
list<tree>   tmscm_to_list_tree (tmscm obj);

bool tmscm_is_list_string (tmscm obj);
bool tmscm_is_list_tree (tmscm obj);

#define TMSCM_ASSERT_LIST_STRING(p, arg, rout)                                 \
  TMSCM_ASSERT (tmscm_is_list_string (p), p, arg, rout)
#define TMSCM_ASSERT_LIST_TREE(p, arg, rout)                                   \
  TMSCM_ASSERT (tmscm_is_list_tree (p), p, arg, rout)

tmscm command_to_tmscm (command o);
tmscm path_to_tmscm (path p);
tmscm modification_to_tmscm (modification m);

command      tmscm_to_command (tmscm o);
path         tmscm_to_path (tmscm obj);
modification tmscm_to_modification (tmscm obj);

bool tmscm_is_command (tmscm u);
bool tmscm_is_path (tmscm obj);
bool tmscm_is_modification (tmscm obj);

#define TMSCM_ASSERT_COMMAND(o, arg, rout)                                     \
  TMSCM_ASSERT (tmscm_is_command (o), o, arg, rout)
#define TMSCM_ASSERT_PATH(p, arg, rout)                                        \
  TMSCM_ASSERT (tmscm_is_path (p), p, arg, rout)
#define TMSCM_ASSERT_MODIFICATION(m, arg, rout)                                \
  TMSCM_ASSERT (tmscm_is_modification (m), m, arg, rout)

typedef array<DI>                   array_di;
typedef array<string>               array_string;
typedef array<tree>                 array_tree;
typedef array<path>                 array_path;
typedef array<double>               array_double;
typedef array<array<double>>        array_array_double;
typedef array<array<array<double>>> array_array_array_double;

tmscm array_di_to_tmscm (array<DI> a);
tmscm array_double_to_tmscm (array<double> a);
tmscm array_array_double_to_tmscm (array<array_double> a);
tmscm array_array_array_double_to_tmscm (array<array_array_double> a);
tmscm array_string_to_tmscm (array<string> a);
tmscm array_tree_to_tmscm (array<tree> a);
tmscm array_path_to_tmscm (array<path> a);

array<DI>                 tmscm_to_array_di (tmscm p);
array<double>             tmscm_to_array_double (tmscm p);
array<array_double>       tmscm_to_array_array_double (tmscm p);
array<array_array_double> tmscm_to_array_array_array_double (tmscm p);
array<string>             tmscm_to_array_string (tmscm p);
array<tree>               tmscm_to_array_tree (tmscm p);
array<path>               tmscm_to_array_path (tmscm p);

bool tmscm_is_array_di (tmscm p);
bool tmscm_is_array_double (tmscm p);
bool tmscm_is_array_array_double (tmscm p);
bool tmscm_is_array_array_array_double (tmscm p);
bool tmscm_is_array_string (tmscm p);
bool tmscm_is_array_tree (tmscm p);
bool tmscm_is_array_path (tmscm p);

#define TMSCM_ASSERT_ARRAY_DI(p, arg, rout)                                    \
  TMSCM_ASSERT (tmscm_is_array_di (p), p, arg, rout)
#define TMSCM_ASSERT_ARRAY_DOUBLE(p, arg, rout)                                \
  TMSCM_ASSERT (tmscm_is_array_double (p), p, arg, rout)
#define TMSCM_ASSERT_ARRAY_ARRAY_DOUBLE(p, arg, rout)                          \
  TMSCM_ASSERT (tmscm_is_array_array_double (p), p, arg, rout)
#define TMSCM_ASSERT_ARRAY_ARRAY_ARRAY_DOUBLE(p, arg, rout)                    \
  TMSCM_ASSERT (tmscm_is_array_array_array_double (p), p, arg, rout)
#define TMSCM_ASSERT_ARRAY_STRING(p, arg, rout)                                \
  TMSCM_ASSERT (tmscm_is_array_string (p), p, arg, rout)
#define TMSCM_ASSERT_ARRAY_TREE(p, arg, rout)                                  \
  TMSCM_ASSERT (tmscm_is_array_tree (p), p, arg, rout)
#define TMSCM_ASSERT_ARRAY_PATH(p, arg, rout)                                  \
  TMSCM_ASSERT (tmscm_is_array_path (p), p, arg, rout)

#endif
