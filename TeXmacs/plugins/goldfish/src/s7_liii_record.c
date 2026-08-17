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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See
// the License for the specific language governing permissions and limitations
// under the License.
//

/* s7_liii_record.c - C implementation of define-record-type (SRFI-9 / R7RS)
 *
 * The macro is implemented in C to avoid the expansion-time cost of the old
 * define-macro version (Scheme-level map/memq/quasiquote for every record
 * type at library load time).  The generated expansion is intentionally the
 * same let-based one the Scheme macro produced: s7 heavily optimizes inlet
 * and let-ref with constant keys, so the let representation is faster at run
 * time than a vector-based one (see devel/0116.md for the benchmarks).
 * Keeping the let representation also means zero observable behavior change:
 * records are still lets, print the same, and let?/let-ref keep working on
 * them.
 */

#include "s7_liii_record.h"

static s7_pointer
record_error (s7_scheme* sc, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, "wrong-type-arg"), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

static bool
record_symbol_list (s7_scheme* sc, s7_pointer lst) {
  if (s7_list_length (sc, lst) < 0) return false; /* improper or circular */
  for (s7_pointer p= lst; s7_is_pair (p); p= s7_cdr (p))
    if (!s7_is_symbol (s7_car (p))) return false;
  return true;
}

static bool
record_memq_sym (s7_pointer sym, s7_pointer lst) {
  for (s7_pointer p= lst; s7_is_pair (p); p= s7_cdr (p))
    if (s7_car (p) == sym) return true; /* symbols: pointer equality */
  return false;
}

/* append form to the tail of the list headed by *tail (update *tail) */
static void
record_append (s7_scheme* sc, s7_pointer* tail, s7_pointer form) {
  s7_pointer cell= s7_cons (sc, form, s7_nil (sc));
  s7_set_cdr (*tail, cell);
  *tail= cell;
}

/* (define-record-type type-name (constructor field ...) predicate
 *   (field-name accessor [modifier]) ...)
 *
 * expands to
 *   (begin
 *     (define (predicate obj)
 *       (and (let? obj) (eq? (let-ref obj 'typ) 'type-name)))
 *     (define (constructor arg ...)
 *       (inlet 'typ 'type-name 'field value ...))
 *     (define (accessor obj) (let-ref obj 'field))
 *     (define (modifier obj val) (let-set! obj 'field val))
 *     ...
 *     'type-name)
 * where obj and typ are fresh gensyms and the constructor values are given in
 * field declaration order (constructor arguments are reordered as needed,
 * fields missing from the constructor spec default to #f).
 */
s7_pointer
g_define_record_type (s7_scheme* sc, s7_pointer args) {
  s7_gc_protect_via_stack (sc, args);

  s7_pointer type_name= s7_car (args);
  s7_pointer ctor_spec= s7_cadr (args);
  s7_pointer pred_name= s7_caddr (args);
  s7_pointer fields   = s7_cdddr (args);

  if (!s7_is_symbol (type_name)) {
    s7_gc_unprotect_via_stack (sc, args);
    return record_error (sc, "define-record-type: type name should be a symbol, but got ~S", type_name);
  }
  if ((!s7_is_pair (ctor_spec)) || (!s7_is_symbol (s7_car (ctor_spec))) ||
      (!record_symbol_list (sc, s7_cdr (ctor_spec)))) {
    s7_gc_unprotect_via_stack (sc, args);
    return record_error (sc, "define-record-type: constructor spec should be (name field ...), but got ~S", ctor_spec);
  }
  if (!s7_is_symbol (pred_name)) {
    s7_gc_unprotect_via_stack (sc, args);
    return record_error (sc, "define-record-type: predicate name should be a symbol, but got ~S", pred_name);
  }
  if (s7_list_length (sc, fields) < 0) {
    s7_gc_unprotect_via_stack (sc, args);
    return record_error (sc, "define-record-type: field specs should be a proper list, but got ~S", fields);
  }
  for (s7_pointer p= fields; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer spec    = s7_car (p);
    s7_int     rest_len= (s7_is_pair (spec)) ? s7_list_length (sc, s7_cdr (spec)) : -1;
    if ((!s7_is_pair (spec)) || (!s7_is_symbol (s7_car (spec))) || (rest_len < 0) || (rest_len > 2) ||
        (!record_symbol_list (sc, s7_cdr (spec)))) {
      s7_gc_unprotect_via_stack (sc, args);
      return record_error (sc, "define-record-type: field spec should be (name [accessor [modifier]]), but got ~S",
                           spec);
    }
  }

  s7_pointer begin_sym  = s7_make_symbol (sc, "begin");
  s7_pointer define_sym = s7_make_symbol (sc, "define");
  s7_pointer lambda_sym = s7_make_symbol (sc, "lambda");
  s7_pointer quote_sym  = s7_make_symbol (sc, "quote");
  s7_pointer and_sym    = s7_make_symbol (sc, "and");
  s7_pointer eq_sym     = s7_make_symbol (sc, "eq?");
  s7_pointer is_let_sym = s7_make_symbol (sc, "let?");
  s7_pointer let_ref_sym= s7_make_symbol (sc, "let-ref");
  s7_pointer let_set_sym= s7_make_symbol (sc, "let-set!");
  s7_pointer inlet_sym  = s7_make_symbol (sc, "inlet");
  s7_pointer obj_sym    = s7_gensym (sc, "obj");
  s7_pointer typ_sym    = s7_gensym (sc, "typ");
  s7_pointer val_sym    = s7_make_symbol (sc, "val");

  /* nested s7_list builds are covered by s7's GC_TEMPS lag (see s7.h): a
   * freshly created object cannot be collected before several further
   * allocations, and each form is attached to the protected result list
   * immediately after being built */
  s7_gc_protect_via_stack (sc, obj_sym);
  s7_gc_protect_via_stack (sc, typ_sym);
  s7_pointer result= s7_gc_protect_via_stack (sc, s7_list (sc, 1, begin_sym));
  s7_pointer tail  = result;

  /* predicate: (define (pred obj) (and (let? obj) (eq? (let-ref obj 'typ) 'type))) */
  record_append (sc, &tail,
                 s7_list (sc, 3, define_sym, s7_list (sc, 2, pred_name, obj_sym),
                          s7_list (sc, 3, and_sym, s7_list (sc, 2, is_let_sym, obj_sym),
                                   s7_list (sc, 3, eq_sym,
                                            s7_list (sc, 3, let_ref_sym, obj_sym, s7_list (sc, 2, quote_sym, typ_sym)),
                                            s7_list (sc, 2, quote_sym, type_name)))));

  /* constructor: (define (ctor arg ...) (inlet 'typ 'type 'field value ...))
   * with values in field declaration order */
  {
    s7_pointer call= s7_gc_protect_via_stack (
        sc, s7_list (sc, 3, inlet_sym, s7_list (sc, 2, quote_sym, typ_sym), s7_list (sc, 2, quote_sym, type_name)));
    s7_pointer ctail= s7_cdr (s7_cdr (call));
    for (s7_pointer p= fields; s7_is_pair (p); p= s7_cdr (p)) {
      s7_pointer field_name= s7_car (s7_car (p));
      s7_pointer value     = (record_memq_sym (field_name, s7_cdr (ctor_spec))) ? field_name : s7_f (sc);
      record_append (sc, &ctail, s7_list (sc, 2, quote_sym, field_name));
      record_append (sc, &ctail, value);
    }
    record_append (sc, &tail, s7_list (sc, 3, define_sym, ctor_spec, call));
    s7_gc_unprotect_via_stack (sc, call);
  }

  /* accessors and modifiers */
  for (s7_pointer p= fields; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer rest= s7_cdr (s7_car (p));
    if (!s7_is_pair (rest)) continue;
    record_append (sc, &tail,
                   s7_list (sc, 3, define_sym, s7_list (sc, 2, s7_car (rest), obj_sym),
                            s7_list (sc, 3, let_ref_sym, obj_sym, s7_list (sc, 2, quote_sym, s7_car (s7_car (p))))));
    if (s7_is_pair (s7_cdr (rest)))
      record_append (
          sc, &tail,
          s7_list (sc, 3, define_sym, s7_list (sc, 3, s7_cadr (rest), obj_sym, val_sym),
                   s7_list (sc, 4, let_set_sym, obj_sym, s7_list (sc, 2, quote_sym, s7_car (s7_car (p))), val_sym)));
  }

  record_append (sc, &tail, s7_list (sc, 2, quote_sym, type_name));

  s7_gc_unprotect_via_stack (sc, result);
  s7_gc_unprotect_via_stack (sc, typ_sym);
  s7_gc_unprotect_via_stack (sc, obj_sym);
  s7_gc_unprotect_via_stack (sc, args);
  return result;
}

void
glue_liii_record (s7_scheme* sc) {
  s7_define_macro (sc, "define-record-type", g_define_record_type, 3, 0, true,
                   "(define-record-type type-name (constructor field ...) predicate (field-name accessor "
                   "[modifier]) ...) defines a new record type");
}
