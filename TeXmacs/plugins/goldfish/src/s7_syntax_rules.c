//
// Copyright (C) 2026 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "s7_syntax_rules.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct syntactic_closure_t {
  s7_pointer env;
  s7_pointer free_vars;
  s7_pointer expr;
  s7_pointer rename;
} syntactic_closure_t;

static s7_int synclo_type_tag= 0;

static inline bool
is_synclo (s7_scheme* sc, s7_pointer x) {
  return s7_is_c_object (x) && s7_c_object_type (x) == synclo_type_tag;
}

static s7_pointer
synclo_free (s7_scheme* sc, s7_pointer obj) {
  syntactic_closure_t* sc_val= (syntactic_closure_t*) s7_c_object_value (obj);
  if (sc_val) {
    free (sc_val);
  }
  return NULL;
}

static s7_pointer
synclo_mark (s7_scheme* sc, s7_pointer obj) {
  syntactic_closure_t* sc_val= (syntactic_closure_t*) s7_c_object_value (obj);
  if (sc_val) {
    if (sc_val->env) s7_mark (sc_val->env);
    if (sc_val->free_vars) s7_mark (sc_val->free_vars);
    if (sc_val->expr) s7_mark (sc_val->expr);
    if (sc_val->rename && sc_val->rename != s7_f (sc)) s7_mark (sc_val->rename);
  }
  return NULL;
}

static s7_pointer
synclo_to_string (s7_scheme* sc, s7_pointer args) {
  syntactic_closure_t* sc_val= (syntactic_closure_t*) s7_c_object_value (s7_car (args));
  char                 buf[256];
  if (sc_val && sc_val->expr && s7_is_symbol (sc_val->expr)) {
    snprintf (buf, sizeof (buf), "#<syntactic-closure %s>", s7_symbol_name (sc_val->expr));
  }
  else {
    snprintf (buf, sizeof (buf), "#<syntactic-closure>");
  }
  return s7_make_string (sc, buf);
}

static s7_pointer
synclo_is_equal (s7_scheme* sc, s7_pointer args) {
  s7_pointer obj1= s7_car (args);
  s7_pointer obj2= s7_cadr (args);
  if (obj1 == obj2) return s7_t (sc);
  if (!s7_is_c_object (obj1) || !s7_is_c_object (obj2)) return s7_f (sc);
  if (s7_c_object_type (obj1) != synclo_type_tag || s7_c_object_type (obj2) != synclo_type_tag) return s7_f (sc);
  syntactic_closure_t* s1= (syntactic_closure_t*) s7_c_object_value (obj1);
  syntactic_closure_t* s2= (syntactic_closure_t*) s7_c_object_value (obj2);
  if (s1 == s2) return s7_t (sc);
  if (s1 && s2 && s1->env == s2->env && s1->expr == s2->expr && s1->free_vars == s2->free_vars) return s7_t (sc);
  return s7_f (sc);
}

static s7_pointer
g_make_syntactic_closure (s7_scheme* sc, s7_pointer args) {
  s7_pointer           env = s7_car (args);
  s7_pointer           fv  = s7_cadr (args);
  s7_pointer           expr= s7_caddr (args);
  syntactic_closure_t* s   = (syntactic_closure_t*) malloc (sizeof (syntactic_closure_t));
  if (!s) return s7_error (sc, s7_make_symbol (sc, "out-of-memory"), s7_nil (sc));
  s->env      = env;
  s->free_vars= fv;
  s->expr     = expr;
  s->rename   = s7_f (sc);
  return s7_make_c_object (sc, synclo_type_tag, (void*) s);
}

static s7_pointer
g_syntactic_closure_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer x= s7_car (args);
  return s7_make_boolean (sc, is_synclo (sc, x));
}

static s7_pointer
synclo_expr (s7_scheme* sc, s7_pointer x) {
  while (is_synclo (sc, x)) {
    syntactic_closure_t* s= (syntactic_closure_t*) s7_c_object_value (x);
    x                     = s->expr;
  }
  return x;
}

static s7_pointer
synclo_expr_env (s7_scheme* sc, s7_pointer x, s7_pointer* eff_env) {
  while (is_synclo (sc, x)) {
    syntactic_closure_t* s= (syntactic_closure_t*) s7_c_object_value (x);
    if (s->env && s->env != s7_nil (sc) && s->env != s7_f (sc)) {
      *eff_env= s->env;
    }
    x= s->expr;
  }
  return x;
}

static syntactic_closure_t*
require_synclo (s7_scheme* sc, s7_pointer x) {
  if (!is_synclo (sc, x)) {
    s7_error (sc, s7_make_symbol (sc, "type-error"),
              s7_list (sc, 2, s7_make_string (sc, "expected syntactic-closure"), x));
    return NULL;
  }
  return (syntactic_closure_t*) s7_c_object_value (x);
}

static s7_pointer
g_syntactic_closure_env (s7_scheme* sc, s7_pointer args) {
  return require_synclo (sc, s7_car (args))->env;
}

static s7_pointer
g_syntactic_closure_expr (s7_scheme* sc, s7_pointer args) {
  return require_synclo (sc, s7_car (args))->expr;
}

static s7_pointer
g_syntactic_closure_free_vars (s7_scheme* sc, s7_pointer args) {
  return require_synclo (sc, s7_car (args))->free_vars;
}

static s7_pointer
g_syntactic_closure_rename (s7_scheme* sc, s7_pointer args) {
  return require_synclo (sc, s7_car (args))->rename;
}

static s7_pointer
g_syntactic_closure_set_rename_b (s7_scheme* sc, s7_pointer args) {
  s7_pointer x   = s7_car (args);
  s7_pointer proc= s7_cadr (args);
  require_synclo (sc, x)->rename= proc;
  return x;
}

static s7_pointer
g_identifier_p (s7_scheme* sc, s7_pointer args) {
  return s7_make_boolean (sc, s7_is_symbol (synclo_expr (sc, s7_car (args))));
}

static s7_pointer
g_identifier_to_symbol (s7_scheme* sc, s7_pointer args) {
  s7_pointer x= s7_car (args);
  s7_pointer y= synclo_expr (sc, x);
  if (s7_is_symbol (y)) return y;
  return s7_error (sc, s7_make_symbol (sc, "type-error"),
                   s7_list (sc, 2, s7_make_string (sc, "expected identifier"), x));
}

static s7_pointer
strip_synclos (s7_scheme* sc, s7_pointer x) {
  x= synclo_expr (sc, x);
  if (s7_is_pair (x)) {
    s7_pointer kar= strip_synclos (sc, s7_car (x));
    s7_pointer kdr= strip_synclos (sc, s7_cdr (x));
    if (kar == s7_car (x) && kdr == s7_cdr (x)) {
      return x;
    }
    return s7_cons (sc, kar, kdr);
  }
  if (s7_is_byte_vector (x) || s7_is_int_vector (x) || s7_is_float_vector (x) || s7_is_complex_vector (x)) {
    return x;
  }
  if (s7_is_vector (x)) {
    s7_int     len= s7_vector_length (x);
    s7_pointer v  = s7_make_vector (sc, len);
    for (s7_int i= 0; i < len; i++) {
      s7_vector_set (sc, v, i, strip_synclos (sc, s7_vector_ref (sc, x, i)));
    }
    return v;
  }
  return x;
}

static s7_pointer
g_strip_syntactic_closures (s7_scheme* sc, s7_pointer args) {
  return strip_synclos (sc, s7_car (args));
}

static bool is_quote_form (s7_scheme* sc, s7_pointer p);

static s7_pointer
g_identifier_eq_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer e1 = s7_car (args);
  s7_pointer id1= s7_cadr (args);
  s7_pointer e2 = s7_caddr (args);
  s7_pointer id2= s7_cadddr (args);

  if (is_quote_form (sc, id1) && is_quote_form (sc, id2)) {
    return s7_t (sc);
  }

  s7_pointer eff_e1= e1;
  s7_pointer sym1  = synclo_expr_env (sc, id1, &eff_e1);

  s7_pointer eff_e2= e2;
  s7_pointer sym2  = synclo_expr_env (sc, id2, &eff_e2);

  if (!s7_is_symbol (sym1) || !s7_is_symbol (sym2)) {
    return s7_make_boolean (sc, s7_is_equal (sc, sym1, sym2));
  }

  s7_pointer val1= (s7_is_let (eff_e1) ? s7_let_ref (sc, eff_e1, sym1) : s7_undefined (sc));
  s7_pointer val2= (s7_is_let (eff_e2) ? s7_let_ref (sc, eff_e2, sym2) : s7_undefined (sc));

  if (val1 != s7_undefined (sc) && val2 != s7_undefined (sc)) {
    return s7_make_boolean (sc, val1 == val2);
  }
  if (val1 == s7_undefined (sc) && val2 == s7_undefined (sc)) {
    return s7_make_boolean (sc, sym1 == sym2);
  }
  return s7_f (sc);
}

typedef struct synclo_rename_entry {
  s7_pointer                  key;
  s7_pointer                  env;
  s7_pointer                  expr;
  s7_pointer                  gensym;
  struct synclo_rename_entry* next;
} synclo_rename_entry;

static bool
is_quote_form (s7_scheme* sc, s7_pointer p) {
  p= synclo_expr (sc, p);
  if (s7_is_symbol (p)) {
    return strcmp (s7_symbol_name (p), "quote") == 0;
  }
  if (s7_is_syntax (p)) {
    char* str = s7_object_to_c_string (sc, p);
    bool  is_q= (str && (strcmp (str, "#_quote") == 0 || strcmp (str, "quote") == 0));
    if (str) free (str);
    return is_q;
  }
  return false;
}

static bool
is_auxiliary_syntax (const char* name) {
  return (strcmp (name, "=>") == 0 || strcmp (name, "else") == 0 || strcmp (name, "_") == 0 ||
          strcmp (name, "...") == 0);
}

static s7_pointer
resolve_ast (s7_scheme* sc, s7_pointer x, s7_pointer def_env, synclo_rename_entry** memo_head) {
  if (is_synclo (sc, x)) {
    syntactic_closure_t* s   = (syntactic_closure_t*) s7_c_object_value (x);
    s7_pointer           expr= s->expr;
    s7_pointer           env = s->env;
    if (!s7_is_let (env) && s7_is_let (def_env)) {
      env= def_env;
    }
    if (s7_is_pair (expr) || s7_is_vector (expr)) {
      return resolve_ast (sc, expr, env, memo_head);
    }
    if (s7_is_symbol (expr)) {
      if (is_auxiliary_syntax (s7_symbol_name (expr))) {
        return expr;
      }
      s7_pointer root_val= s7_let_ref (sc, s7_rootlet (sc), expr);
      if (s7_is_let (env)) {
        s7_pointer val= s7_let_ref (sc, env, expr);
        if (val != s7_undefined (sc)) {
          if (root_val == s7_undefined (sc) || val != root_val) {
            if (s7_is_procedure (val) || s7_is_macro (sc, val)) {
              return val;
            }
          }
          return expr;
        }
      }
      if (root_val != s7_undefined (sc)) {
        return expr;
      }
      for (synclo_rename_entry* e= *memo_head; e; e= e->next) {
        if (e->key == x || (e->env == env && e->expr == expr)) {
          return e->gensym;
        }
      }
      s7_pointer           g= s7_gensym (sc, s7_symbol_name (expr));
      synclo_rename_entry* e= (synclo_rename_entry*) malloc (sizeof (synclo_rename_entry));
      if (e) {
        e->key    = x;
        e->env    = env;
        e->expr   = expr;
        e->gensym = g;
        e->next   = *memo_head;
        *memo_head= e;
      }
      return g;
    }
    return expr;
  }
  if (s7_is_pair (x)) {
    s7_pointer kar = s7_car (x);
    s7_pointer cdr_x= s7_cdr (x);
    if (is_quote_form (sc, kar) && s7_is_pair (cdr_x) && s7_is_null (sc, s7_cdr (cdr_x))) {
      s7_pointer resolved_kar= resolve_ast (sc, kar, def_env, memo_head);
      s7_pointer stripped_kdr= strip_synclos (sc, cdr_x);
      if (resolved_kar == kar && stripped_kdr == cdr_x) {
        return x;
      }
      return s7_cons (sc, resolved_kar, stripped_kdr);
    }
    s7_pointer res_kar= resolve_ast (sc, kar, def_env, memo_head);
    s7_pointer res_kdr= resolve_ast (sc, cdr_x, def_env, memo_head);
    if (res_kar == kar && res_kdr == cdr_x) {
      return x;
    }
    return s7_cons (sc, res_kar, res_kdr);
  }
  if (s7_is_byte_vector (x) || s7_is_int_vector (x) || s7_is_float_vector (x) || s7_is_complex_vector (x)) {
    return x;
  }
  if (s7_is_vector (x)) {
    s7_int     len= s7_vector_length (x);
    s7_pointer v  = s7_make_vector (sc, len);
    for (s7_int i= 0; i < len; i++) {
      s7_vector_set (sc, v, i, resolve_ast (sc, s7_vector_ref (sc, x, i), def_env, memo_head));
    }
    return v;
  }
  return x;
}

static s7_pointer
g_resolve_syntactic_closures (s7_scheme* sc, s7_pointer args) {
  s7_pointer           form     = s7_car (args);
  s7_pointer           def_env  = (s7_is_pair (s7_cdr (args)) ? s7_cadr (args) : s7_rootlet (sc));
  synclo_rename_entry* memo_head= NULL;
  s7_pointer           res      = resolve_ast (sc, form, def_env, &memo_head);
  while (memo_head) {
    synclo_rename_entry* next= memo_head->next;
    free (memo_head);
    memo_head= next;
  }
  return res;
}

void
glue_syntax_rules (s7_scheme* sc) {
  synclo_type_tag= s7_make_c_type (sc, "syntactic-closure");
  s7_c_type_set_gc_free (sc, synclo_type_tag, synclo_free);
  s7_c_type_set_gc_mark (sc, synclo_type_tag, synclo_mark);
  s7_c_type_set_to_string (sc, synclo_type_tag, synclo_to_string);
  s7_c_type_set_is_equal (sc, synclo_type_tag, synclo_is_equal);

  s7_define_function (sc, "make-syntactic-closure", g_make_syntactic_closure, 3, 0, false,
                      "(make-syntactic-closure env free-vars expr) => syntactic-closure");
  s7_define_function (sc, "syntactic-closure?", g_syntactic_closure_p, 1, 0, false,
                      "(syntactic-closure? x) => boolean");
  s7_define_function (sc, "syntactic-closure-env", g_syntactic_closure_env, 1, 0, false,
                      "(syntactic-closure-env sc) => environment");
  s7_define_function (sc, "syntactic-closure-expr", g_syntactic_closure_expr, 1, 0, false,
                      "(syntactic-closure-expr sc) => expr");
  s7_define_function (sc, "syntactic-closure-free-vars", g_syntactic_closure_free_vars, 1, 0, false,
                      "(syntactic-closure-free-vars sc) => list");
  s7_define_function (sc, "syntactic-closure-rename", g_syntactic_closure_rename, 1, 0, false,
                      "(syntactic-closure-rename sc) => procedure or #f");
  s7_define_function (sc, "syntactic-closure-set-rename!", g_syntactic_closure_set_rename_b, 2, 0, false,
                      "(syntactic-closure-set-rename! sc proc) => sc");
  s7_define_function (sc, "identifier?", g_identifier_p, 1, 0, false, "(identifier? x) => boolean");
  s7_define_function (sc, "identifier->symbol", g_identifier_to_symbol, 1, 0, false,
                      "(identifier->symbol x) => symbol");
  s7_define_function (sc, "strip-syntactic-closures", g_strip_syntactic_closures, 1, 0, false,
                      "(strip-syntactic-closures x) => expression without syntactic closures");
  s7_define_function (sc, "identifier=?", g_identifier_eq_p, 4, 0, false, "(identifier=? e1 id1 e2 id2) => boolean");
  s7_define_function (sc, "resolve-syntactic-closures", g_resolve_syntactic_closures, 1, 1, false,
                      "(resolve-syntactic-closures form [def-env]) => clean S-expression");
}
