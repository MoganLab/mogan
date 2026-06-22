/* s7_module.c - module system implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_module.h"
#include "s7_internal_helpers.h"

/* -------- *load-path* setter -------- */
s7_pointer g_load_path_set(s7_scheme *sc, s7_pointer args)
{
  s7_pointer strs;
  if (s7_is_null(sc, s7_cadr(args))) return s7_cadr(args);
  if (!s7_is_pair(s7_cadr(args)))
    return s7_error(sc, s7_make_symbol(sc, "wrong-type-arg"),
                    s7_list(sc, 2, s7_make_string(sc, "can't set *load-path* to ~S"), s7_cadr(args)));
  for (strs = s7_cadr(args); s7_is_pair(strs); strs = s7_cdr(strs))
    if (!s7_is_string(s7_car(strs)))
      return s7_error(sc, s7_make_symbol(sc, "wrong-type-arg"),
                      s7_list(sc, 3,
                              s7_make_string(sc, "can't set *load-path* to ~S, ~S is not a string"),
                              s7_cadr(args), s7_car(strs)));
  return s7_cadr(args);
}

/* -------- *features* setter -------- */
s7_pointer g_features_set(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer new_features = s7_cadr(args);
  if (s7_is_null(sc, new_features)) return s7_nil(sc);
  if (!s7_is_pair(new_features))
    return s7_error(sc, s7_make_symbol(sc, "wrong-type-arg"),
                    s7_list(sc, 2,
                            s7_make_string(sc, "can't set *features* to ~S (*features* must be a pair)"),
                            new_features));
  if (s7_list_length(sc, new_features) <= 0)
    return s7_error(sc, s7_make_symbol(sc, "wrong-type-arg"),
                    s7_list(sc, 2,
                            s7_make_string(sc, "can't set *features* to an improper or circular list ~S"),
                            new_features));
  for (s7_pointer features = new_features; s7_is_pair(features); features = s7_cdr(features))
    if (!s7_is_symbol(s7_car(features)))
      return s7_error(sc, s7_make_symbol(sc, "wrong-type-arg"),
                      s7_list(sc, 2,
                              s7_make_string(sc, "can't set *features* to ~S (each feature should be a symbol)"),
                              new_features));
  return new_features;
}

/* -------- *libraries* setter -------- */
s7_pointer g_libraries_set(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer new_libraries = s7_cadr(args);
  if (s7_is_null(sc, new_libraries)) return s7_nil(sc);
  if ((!s7_is_pair(new_libraries)) || (s7_list_length(sc, new_libraries) <= 0))
    return s7_error(sc, s7_make_symbol(sc, "wrong-type-arg"),
                    s7_list(sc, 2, s7_make_string(sc, "can't set *libraries* to ~S"), new_libraries));
  for (s7_pointer libraries = new_libraries; s7_is_pair(libraries); libraries = s7_cdr(libraries))
    if ((!s7_is_pair(s7_car(libraries))) ||
        (!s7_is_string(s7_car(s7_car(libraries)))) ||
        (!s7_is_let(s7_cdr(s7_car(libraries)))))
      return s7_wrong_type_arg_error(sc, "set! *libraries*", 1, s7_car(libraries),
                                     "a list of conses of the form (string . let)");
  return new_libraries;
}
