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

/* s7_r7rs_library.c - R7RS library registry for Goldfish Scheme
 *
 * The registry maps an R7RS library name (a proper list of symbols and
 * non-negative integers, e.g. (liii base)) to the let environment holding
 * the library's exported bindings.  It lives in the rootlet variable
 * *r7rs-libraries* so that it is reachable by the garbage collector.
 */

#include "s7_r7rs_library.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define R7RS_LIBRARIES_NAME "*r7rs-libraries*"

static s7_pointer
r7rs_library_error (s7_scheme* sc, const char* kind, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, kind), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

static s7_pointer
r7rs_library_registry (s7_scheme* sc) {
  return s7_name_to_value (sc, R7RS_LIBRARIES_NAME);
}

/* R7RS: a library name is a proper list whose elements are symbols or
 * exact non-negative integers.  Returns true iff name is valid. */
static bool
r7rs_library_name_valid (s7_scheme* sc, s7_pointer name) {
  if ((!s7_is_null (sc, name)) && (!s7_is_pair (name))) return false; /* not a list at all */
  if (s7_list_length (sc, name) < 0) return false;                    /* improper or circular list */
  for (s7_pointer p= name; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer elt= s7_car (p);
    if (s7_is_symbol (elt)) continue;
    if (s7_is_integer (elt) && s7_integer (elt) >= 0) continue;
    return false;
  }
  return true;
}

static s7_pointer
r7rs_library_check_name (s7_scheme* sc, const char* caller, s7_pointer name) {
  if (!r7rs_library_name_valid (sc, name))
    return r7rs_library_error (sc, "wrong-type-arg",
                               "library name should be a proper list of symbols or non-negative integers, but got ~S",
                               name);
  return NULL;
}

static s7_pointer
g_library_defined_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer name= s7_car (args);
  s7_pointer err = r7rs_library_check_name (sc, "g_library-defined?", name);
  if (err) return err;
  return s7_make_boolean (sc, s7_is_let (s7_hash_table_ref (sc, r7rs_library_registry (sc), name)));
}

static s7_pointer
g_library_ref (s7_scheme* sc, s7_pointer args) {
  s7_pointer name= s7_car (args);
  s7_pointer err = r7rs_library_check_name (sc, "g_library-ref", name);
  if (err) return err;
  s7_pointer env= s7_hash_table_ref (sc, r7rs_library_registry (sc), name);
  return s7_is_let (env) ? env : s7_f (sc);
}

static s7_pointer
g_library_register (s7_scheme* sc, s7_pointer args) {
  s7_pointer name= s7_car (args);
  s7_pointer err = r7rs_library_check_name (sc, "g_library-register!", name);
  if (err) return err;
  s7_pointer env= s7_cadr (args);
  if (!s7_is_let (env))
    return r7rs_library_error (sc, "wrong-type-arg", "library environment should be a let, but got ~S", env);
  s7_hash_table_set (sc, r7rs_library_registry (sc), name, env);
  return env;
}

static s7_pointer
g_library_unregister (s7_scheme* sc, s7_pointer args) {
  s7_pointer name= s7_car (args);
  s7_pointer err = r7rs_library_check_name (sc, "g_library-unregister!", name);
  if (err) return err;
  /* s7 hash tables have no delete; storing #f marks the entry as absent
   * (g_library-defined?/g_library-ref only accept let values). */
  s7_hash_table_set (sc, r7rs_library_registry (sc), name, s7_f (sc));
  return s7_unspecified (sc);
}

/* -------- define-library helpers (defined below) -------- */

static bool       r7rs_decl_named (s7_pointer decl, const char* name);
static s7_pointer r7rs_export_spec_names (s7_scheme* sc, s7_pointer spec, s7_pointer* internal);
static s7_pointer r7rs_entries_find (s7_pointer entries, s7_pointer sym);
static char*      r7rs_library_name_to_path (s7_scheme* sc, s7_pointer libname);

/* -------- cond-expand (library declarations) -------- */

/* is the library available: registered, or present as a file in some *load-path* directory? */
static bool
r7rs_library_available (s7_scheme* sc, s7_pointer libname) {
  if (s7_is_let (s7_hash_table_ref (sc, r7rs_library_registry (sc), libname))) return true;
  char*      relpath  = r7rs_library_name_to_path (sc, libname);
  bool       found    = false;
  s7_pointer load_path= s7_name_to_value (sc, "*load-path*");
  for (s7_pointer p= load_path; (s7_is_pair (p)) && (!found); p= s7_cdr (p)) {
    if (!s7_is_string (s7_car (p))) continue;
    const char* dir= s7_string (s7_car (p));
    size_t      n  = strlen (dir) + strlen (relpath) + 2;
    char*       full= (char*) malloc (n);
    snprintf (full, n, "%s/%s", dir, relpath);
    FILE* fp= fopen (full, "r");
    if (fp) {
      fclose (fp);
      found= true;
    }
    free (full);
  }
  free (relpath);
  return found;
}

/* evaluate an R7RS feature requirement: a feature identifier, (library name),
 * or (and/or/not ...) compositions */
static bool
r7rs_feature_satisfied (s7_scheme* sc, s7_pointer req) {
  if (s7_is_symbol (req)) return s7_is_provided (sc, s7_symbol_name (req));
  if (!s7_is_pair (req)) {
    r7rs_library_error (sc, "syntax-error", "cond-expand: invalid feature requirement ~S", req);
    return false;
  }
  s7_pointer head= s7_car (req);
  if (!s7_is_symbol (head)) {
    r7rs_library_error (sc, "syntax-error", "cond-expand: invalid feature requirement ~S", req);
    return false;
  }
  const char* op= s7_symbol_name (head);
  if (strcmp (op, "library") == 0) {
    if ((s7_list_length (sc, req) != 2) || (!r7rs_library_name_valid (sc, s7_cadr (req)))) {
      r7rs_library_error (sc, "syntax-error", "cond-expand: malformed (library ...) requirement ~S", req);
      return false;
    }
    return r7rs_library_available (sc, s7_cadr (req));
  }
  if (strcmp (op, "and") == 0) {
    for (s7_pointer p= s7_cdr (req); s7_is_pair (p); p= s7_cdr (p))
      if (!r7rs_feature_satisfied (sc, s7_car (p))) return false;
    return true;
  }
  if (strcmp (op, "or") == 0) {
    for (s7_pointer p= s7_cdr (req); s7_is_pair (p); p= s7_cdr (p))
      if (r7rs_feature_satisfied (sc, s7_car (p))) return true;
    return false;
  }
  if (strcmp (op, "not") == 0) {
    if (s7_list_length (sc, req) != 2) {
      r7rs_library_error (sc, "syntax-error", "cond-expand: (not ...) takes exactly one requirement, got ~S", req);
      return false;
    }
    return !r7rs_feature_satisfied (sc, s7_cadr (req));
  }
  r7rs_library_error (sc, "syntax-error", "cond-expand: unknown feature requirement ~S", req);
  return false;
}

/* choose the body (a list of declarations) of the first matching clause of a
 * cond-expand declaration; returns NULL (no allocation) if nothing matches */
static s7_pointer
r7rs_cond_expand_choose (s7_scheme* sc, s7_pointer clauses) {
  for (s7_pointer p= clauses; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer clause= s7_car (p);
    if (!s7_is_pair (clause)) {
      r7rs_library_error (sc, "syntax-error", "cond-expand: clause is not a list: ~S", clause);
      return NULL;
    }
    s7_pointer req= s7_car (clause);
    if ((s7_is_symbol (req)) && (strcmp (s7_symbol_name (req), "else") == 0)) return s7_cdr (clause);
    if (r7rs_feature_satisfied (sc, req)) return s7_cdr (clause);
  }
  return NULL;
}

/* R7RS cond-expand, replacing s7's built-in read-time expansion (which neither
 * supports (library ...) requirements nor clauses with multiple body forms).
 * Registered as a plain c-macro, so the reader no longer expands cond-expand
 * forms and define-library bodies can process them as declarations. */
static s7_pointer
g_cond_expand_r7rs (s7_scheme* sc, s7_pointer args) {
  s7_gc_protect_via_stack (sc, args);
  s7_pointer chosen   = r7rs_cond_expand_choose (sc, args);
  s7_pointer expansion=
    chosen ? s7_cons (sc, s7_make_symbol (sc, "begin"), chosen) : s7_unspecified (sc);
  s7_gc_unprotect_via_stack (sc, args);
  return expansion;
}

/* -------- define-library passes -------- */

enum { R7RS_PASS_EVAL, R7RS_PASS_VALIDATE, R7RS_PASS_POPULATE };

typedef struct {
  s7_pointer lib_env;
  s7_pointer entries;
  s7_pointer export_env;
  bool       saw_export;
} r7rs_define_library_ctx;

static void r7rs_walk_library_decls (s7_scheme* sc, s7_pointer decls, int mode, r7rs_define_library_ctx* ctx);

/* -------- include -------- */

static bool
r7rs_file_readable (const char* path) {
  FILE* fp= fopen (path, "r");
  if (!fp) return false;
  fclose (fp);
  return true;
}

static char*
r7rs_str_dup (const char* s) {
  size_t n= strlen (s) + 1;
  char*  r= (char*) malloc (n);
  memcpy (r, s, n);
  return r;
}

/* resolve an include file: first relative to the directory of the file
 * currently being loaded, then each *load-path* directory, then the name
 * as-is.  Returns a malloc'd path, or NULL if not found. */
static char*
r7rs_find_include_file (s7_scheme* sc, const char* name) {
  s7_pointer port= s7_current_input_port (sc);
  const char* current= s7_port_filename (sc, port);
  if (current) {
    const char* sep= strrchr (current, '/');
    if (!sep) sep= strrchr (current, '\\');
    if (sep) {
      size_t dir_len= (size_t) (sep - current);
      char* candidate= (char*) malloc (dir_len + 1 + strlen (name) + 1);
      memcpy (candidate, current, dir_len);
      candidate[dir_len]= '/';
      strcpy (candidate + dir_len + 1, name);
      if (r7rs_file_readable (candidate)) return candidate;
      free (candidate);
    }
  }
  s7_pointer load_path= s7_name_to_value (sc, "*load-path*");
  for (s7_pointer p= load_path; s7_is_pair (p); p= s7_cdr (p)) {
    if (!s7_is_string (s7_car (p))) continue;
    const char* dir= s7_string (s7_car (p));
    size_t      n  = strlen (dir) + strlen (name) + 2;
    char* candidate= (char*) malloc (n);
    snprintf (candidate, n, "%s/%s", dir, name);
    if (r7rs_file_readable (candidate)) return candidate;
    free (candidate);
  }
  if (r7rs_file_readable (name)) return r7rs_str_dup (name);
  return NULL;
}

/* read an entire file; returns a malloc'd NUL-terminated string, or NULL */
static char*
r7rs_read_file_text (const char* path) {
  FILE* fp= fopen (path, "rb");
  if (!fp) return NULL;
  fseek (fp, 0, SEEK_END);
  long size= ftell (fp);
  if (size < 0) {
    fclose (fp);
    return NULL;
  }
  rewind (fp);
  char* text= (char*) malloc ((size_t) size + 1);
  size_t got= fread (text, 1, (size_t) size, fp);
  fclose (fp);
  text[got]= '\0';
  return text;
}

/* validate the filename arguments of an include declaration */
static s7_pointer
r7rs_include_check_files (s7_scheme* sc, s7_pointer files) {
  for (s7_pointer p= files; s7_is_pair (p); p= s7_cdr (p))
    if (!s7_is_string (s7_car (p)))
      return r7rs_library_error (sc, "wrong-type-arg", "include: filename should be a string, got ~S", s7_car (p));
  return NULL;
}

/* (include "file" ...) / (include-ci "file" ...): the file contents are library
 * body forms, as if wrapped in begin.  Note: the s7 reader has no case-folding
 * mode, so include-ci currently behaves exactly like include. */
static void
r7rs_include_files (s7_scheme* sc, s7_pointer files, s7_pointer lib_env) {
  s7_pointer err= r7rs_include_check_files (sc, files);
  if (err) return;
  for (s7_pointer p= files; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer name= s7_car (p);
    char*      path= r7rs_find_include_file (sc, s7_string (name));
    if (!path) {
      r7rs_library_error (sc, "io-error", "include: cannot find file ~S", name);
      return;
    }
    char* text= r7rs_read_file_text (path);
    free (path);
    if (!text) {
      r7rs_library_error (sc, "io-error", "include: cannot read file ~S", name);
      return;
    }
    s7_pointer port= s7_open_input_string (sc, text); /* the port references text */
    s7_gc_protect_via_stack (sc, port);
    s7_pointer eof= s7_eof_object (sc);
    s7_pointer form;
    while ((form= s7_read (sc, port)) != eof)
      s7_eval (sc, form, lib_env);
    s7_gc_unprotect_via_stack (sc, port);
    free (text);
  }
}

/* (include-library-declarations "file" ...): the file contents are library
 * declarations, spliced into the declaration stream */
static void
r7rs_include_library_declarations (s7_scheme* sc, s7_pointer files, int mode, r7rs_define_library_ctx* ctx) {
  s7_pointer err= r7rs_include_check_files (sc, files);
  if (err) return;
  for (s7_pointer p= files; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer name= s7_car (p);
    char*      path= r7rs_find_include_file (sc, s7_string (name));
    if (!path) {
      r7rs_library_error (sc, "io-error", "include-library-declarations: cannot find file ~S", name);
      return;
    }
    char* text= r7rs_read_file_text (path);
    free (path);
    if (!text) {
      r7rs_library_error (sc, "io-error", "include-library-declarations: cannot read file ~S", name);
      return;
    }
    s7_pointer port= s7_open_input_string (sc, text); /* the port references text */
    s7_gc_protect_via_stack (sc, port);
    s7_pointer eof  = s7_eof_object (sc);
    s7_pointer forms= s7_nil (sc);
    s7_gc_protect_via_stack (sc, forms);
    s7_pointer form;
    while ((form= s7_read (sc, port)) != eof) {
      s7_gc_protect_via_stack (sc, form);
      forms= s7_cons (sc, form, forms); /* reversed */
      s7_gc_unprotect_via_stack (sc, form);
      s7_gc_unprotect_via_stack (sc, forms);
      s7_gc_protect_via_stack (sc, forms);
    }
    /* reverse in place (no allocation, so no GC risk): restore source order */
    s7_pointer rev= s7_nil (sc);
    for (s7_pointer q= forms; s7_is_pair (q);) {
      s7_pointer next= s7_cdr (q);
      s7_set_cdr (q, rev);
      rev= q;
      q  = next;
    }
    /* the last cons cell (old head) is still on the protect stack and anchors
     * the whole chain, which now starts at rev */
    r7rs_walk_library_decls (sc, rev, mode, ctx);
    s7_gc_unprotect_via_stack (sc, forms);
    s7_gc_unprotect_via_stack (sc, port);
    free (text);
  }
}

/* pass 2 helper: every export spec must be well-formed and name a binding of the
 * library body, or a binding that falls through to the rootlet */
static void
r7rs_validate_export_decl (s7_scheme* sc, s7_pointer decl, s7_pointer entries) {
  for (s7_pointer specs= s7_cdr (decl); s7_is_pair (specs); specs= s7_cdr (specs)) {
    s7_pointer internal= NULL;
    r7rs_export_spec_names (sc, s7_car (specs), &internal);
    if ((!r7rs_entries_find (entries, internal)) && (!s7_is_defined (sc, s7_symbol_name (internal))))
      r7rs_library_error (sc, "unbound-variable", "define-library: cannot export ~S: it is not defined in "
                          "the library body",
                          internal);
  }
}

/* pass 3 helper: materialize the exported bindings into export_env */
static void
r7rs_populate_export_decl (s7_scheme* sc, s7_pointer decl, s7_pointer entries, s7_pointer export_env, s7_pointer lib_env) {
  for (s7_pointer specs= s7_cdr (decl); s7_is_pair (specs); specs= s7_cdr (specs)) {
    s7_pointer internal= NULL;
    s7_pointer external= r7rs_export_spec_names (sc, s7_car (specs), &internal);
    s7_pointer entry   = r7rs_entries_find (entries, internal);
    /* only the library body's own bindings are materialized in the export
     * environment.  Names that fall through to the rootlet (pass 2 allows
     * them, e.g. (scheme base) re-exporting eqv?) stay virtual: the export
     * environment's outlet chain resolves them, exactly like the old
     * Scheme implementation.  Copying hundreds of rootlet bindings into
     * every export environment (and from there into every importer) would
     * be a measurable slowdown. */
    if (entry) s7_varlet (sc, export_env, external, s7_cdr (entry));
    else
      if (external != internal) {
        /* a renamed rootlet re-export (export (rename eqv? same?)): the new
         * name does not exist in the rootlet, so it must be materialized */
        s7_pointer value= s7_let_ref (sc, lib_env, internal);
        /* syntactic rootlet bindings (define* etc.) cannot be let slots in s7;
         * skip them, matching the old Scheme implementation */
        if (!s7_is_syntax (value)) s7_varlet (sc, export_env, external, value);
      }
  }
}

static void
r7rs_walk_library_decls (s7_scheme* sc, s7_pointer decls, int mode, r7rs_define_library_ctx* ctx) {
  for (s7_pointer p= decls; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer decl= s7_car (p);
    if (r7rs_decl_named (decl, "cond-expand")) {
      s7_pointer chosen= r7rs_cond_expand_choose (sc, s7_cdr (decl));
      if (chosen) r7rs_walk_library_decls (sc, chosen, mode, ctx);
      continue;
    }
    /* include-library-declarations splices declarations: handle in every pass */
    if (r7rs_decl_named (decl, "include-library-declarations")) {
      r7rs_include_library_declarations (sc, s7_cdr (decl), mode, ctx);
      continue;
    }
    /* include/include-ci splice body forms: only the eval pass cares */
    if ((r7rs_decl_named (decl, "include")) || (r7rs_decl_named (decl, "include-ci"))) {
      if (mode == R7RS_PASS_EVAL) r7rs_include_files (sc, s7_cdr (decl), ctx->lib_env);
      continue;
    }
    switch (mode) {
    case R7RS_PASS_EVAL:
      if (!r7rs_decl_named (decl, "export")) s7_eval (sc, decl, ctx->lib_env);
      break;
    case R7RS_PASS_VALIDATE:
      if (r7rs_decl_named (decl, "export")) {
        ctx->saw_export= true;
        r7rs_validate_export_decl (sc, decl, ctx->entries);
      }
      break;
    case R7RS_PASS_POPULATE:
      if (r7rs_decl_named (decl, "export"))
        r7rs_populate_export_decl (sc, decl, ctx->entries, ctx->export_env, ctx->lib_env);
      break;
    }
  }
}

static bool
r7rs_decl_named (s7_pointer decl, const char* name) {
  return (s7_is_pair (decl)) && (s7_is_symbol (s7_car (decl))) &&
         (strcmp (s7_symbol_name (s7_car (decl)), name) == 0);
}

/* An export spec is either a symbol (external == internal) or (rename old new).
 * On success stores the internal name in *internal and returns the external name;
 * on a malformed spec an error is signalled. */
static s7_pointer
r7rs_export_spec_names (s7_scheme* sc, s7_pointer spec, s7_pointer* internal) {
  if (s7_is_symbol (spec)) {
    *internal= spec;
    return spec;
  }
  if ((s7_is_pair (spec)) && (r7rs_decl_named (spec, "rename")) && (s7_list_length (sc, spec) == 3) &&
      (s7_is_symbol (s7_cadr (spec))) && (s7_is_symbol (s7_caddr (spec)))) {
    *internal= s7_cadr (spec);
    return s7_caddr (spec);
  }
  return r7rs_library_error (sc, "syntax-error", "define-library: invalid export spec ~S", spec);
}

/* Find the binding entry (a (symbol . value) pair of s7_let_to_list) for sym
 * among the own slots of env, or NULL if sym is not bound in env itself
 * (outlets are deliberately not searched: only definitions and imports of the
 * library body count). */
static s7_pointer
r7rs_entries_find (s7_pointer entries, s7_pointer sym) {
  for (s7_pointer p= entries; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer entry= s7_car (p);
    if (s7_car (entry) == sym) return entry;
  }
  return NULL;
}

static s7_pointer
g_define_library (s7_scheme* sc, s7_pointer args) {
  s7_gc_protect_via_stack (sc, args);
  s7_pointer libname= s7_car (args);
  if (!r7rs_library_name_valid (sc, libname))
    return r7rs_library_error (sc, "wrong-type-arg",
                               "define-library: invalid library name ~S (a proper list of symbols or non-negative "
                               "integers expected)",
                               libname);

  r7rs_define_library_ctx ctx;
  ctx.saw_export= false;

  /* the working environment: definitions and imports of the library body land here */
  ctx.lib_env= s7_sublet (sc, s7_rootlet (sc), s7_nil (sc));
  s7_gc_protect_via_stack (sc, ctx.lib_env);

  /* pass 1: evaluate every declaration except export in lib_env
   * (cond-expand clauses are resolved and recursed into by the walker) */
  r7rs_walk_library_decls (sc, s7_cdr (args), R7RS_PASS_EVAL, &ctx);

  /* the own slots of lib_env, as (symbol . value) entries */
  ctx.entries= s7_let_to_list (sc, ctx.lib_env);
  s7_gc_protect_via_stack (sc, ctx.entries);

  /* pass 2: validate export specs before mutating any visible state */
  r7rs_walk_library_decls (sc, s7_cdr (args), R7RS_PASS_VALIDATE, &ctx);

  ctx.export_env= s7_inlet (sc, s7_nil (sc));
  s7_gc_protect_via_stack (sc, ctx.export_env);

  /* make the library reachable before populating it: the registry entry and the
   * compatibility global symbol (used by the Scheme import implementation) */
  s7_hash_table_set (sc, r7rs_library_registry (sc), libname, ctx.export_env);
  char* name_str= s7_object_to_c_string (sc, libname);
  s7_define (sc, s7_rootlet (sc), s7_make_symbol (sc, name_str), ctx.export_env);
  free (name_str);

  /* populate: with no export declaration every binding is exported */
  if (!ctx.saw_export) {
    for (s7_pointer p= ctx.entries; s7_is_pair (p); p= s7_cdr (p)) {
      s7_pointer entry= s7_car (p);
      s7_varlet (sc, ctx.export_env, s7_car (entry), s7_cdr (entry));
    }
  }
  else r7rs_walk_library_decls (sc, s7_cdr (args), R7RS_PASS_POPULATE, &ctx);

  s7_gc_unprotect_via_stack (sc, ctx.export_env);
  s7_gc_unprotect_via_stack (sc, ctx.entries);
  s7_gc_unprotect_via_stack (sc, ctx.lib_env);
  s7_gc_unprotect_via_stack (sc, args);
  return s7_t (sc);
}

/* -------- import -------- */

/* map a library name (liii base) to the file path "liii/base.scm" */
static char*
r7rs_library_name_to_path (s7_scheme* sc, s7_pointer libname) {
  size_t len= 1; /* NUL */
  for (s7_pointer p= libname; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer elt= s7_car (p);
    len+= (s7_is_symbol (elt) ? strlen (s7_symbol_name (elt)) : 24) + 1; /* '/' or ".scm" */
  }
  char* path= (char*) malloc (len + 4);
  char* w   = path;
  for (s7_pointer p= libname; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer elt= s7_car (p);
    if (w != path) *w++= '/';
    if (s7_is_symbol (elt)) {
      size_t n= strlen (s7_symbol_name (elt));
      memcpy (w, s7_symbol_name (elt), n);
      w+= n;
    }
    else w+= sprintf (w, "%lld", (long long) s7_integer (elt));
  }
  memcpy (w, ".scm", 5); /* with NUL */
  return path;
}

/* return the exported environment of libname, loading its file on first use */
static s7_pointer
r7rs_library_env (s7_scheme* sc, s7_pointer libname) {
  s7_pointer env= s7_hash_table_ref (sc, r7rs_library_registry (sc), libname);
  if (s7_is_let (env)) return env;
  char* path= r7rs_library_name_to_path (sc, libname);
  s7_load (sc, path); /* errors if the file cannot be found */
  free (path);
  env= s7_hash_table_ref (sc, r7rs_library_registry (sc), libname);
  if (!s7_is_let (env))
    return r7rs_library_error (sc, "unbound-variable", "import: loading did not define the library ~S", libname);
  return env;
}

static s7_pointer r7rs_import_set_env (s7_scheme* sc, s7_pointer iset);

/* is sym a member of the symbol list names? */
static bool
r7rs_symbol_member (s7_pointer names, s7_pointer sym) {
  for (s7_pointer p= names; s7_is_pair (p); p= s7_cdr (p))
    if (s7_car (p) == sym) return true;
  return false;
}

static s7_pointer
r7rs_import_check_names (s7_scheme* sc, s7_pointer names) {
  for (s7_pointer p= names; s7_is_pair (p); p= s7_cdr (p))
    if (!s7_is_symbol (s7_car (p)))
      return r7rs_library_error (sc, "wrong-type-arg", "import: expected an identifier, got ~S", s7_car (p));
  return NULL;
}

/* (only import-set identifier ...) */
static s7_pointer
r7rs_import_only (s7_scheme* sc, s7_pointer iset) {
  s7_pointer rest= s7_cdr (iset);
  if (!s7_is_pair (rest))
    return r7rs_library_error (sc, "syntax-error", "import: (only ...) needs an import set, got ~S", iset);
  s7_pointer err= r7rs_import_check_names (sc, s7_cdr (rest));
  if (err) return err;
  s7_pointer sub= r7rs_import_set_env (sc, s7_car (rest));
  s7_gc_protect_via_stack (sc, sub);
  s7_pointer env= s7_inlet (sc, s7_nil (sc));
  s7_gc_protect_via_stack (sc, env);
  for (s7_pointer names= s7_cdr (rest); s7_is_pair (names); names= s7_cdr (names)) {
    s7_pointer name= s7_car (names);
    s7_varlet (sc, env, name, s7_let_ref (sc, sub, name)); /* let-ref errors if name is missing */
  }
  s7_gc_unprotect_via_stack (sc, env);
  s7_gc_unprotect_via_stack (sc, sub);
  return env;
}

/* (except import-set identifier ...) */
static s7_pointer
r7rs_import_except (s7_scheme* sc, s7_pointer iset) {
  s7_pointer rest= s7_cdr (iset);
  if (!s7_is_pair (rest))
    return r7rs_library_error (sc, "syntax-error", "import: (except ...) needs an import set, got ~S", iset);
  s7_pointer err= r7rs_import_check_names (sc, s7_cdr (rest));
  if (err) return err;
  s7_pointer sub= r7rs_import_set_env (sc, s7_car (rest));
  s7_gc_protect_via_stack (sc, sub);
  s7_pointer env= s7_inlet (sc, s7_nil (sc));
  s7_gc_protect_via_stack (sc, env);
  s7_pointer entries= s7_let_to_list (sc, sub);
  s7_gc_protect_via_stack (sc, entries);
  s7_pointer names= s7_cdr (rest);
  for (s7_pointer p= entries; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer entry= s7_car (p);
    if (!r7rs_symbol_member (names, s7_car (entry))) s7_varlet (sc, env, s7_car (entry), s7_cdr (entry));
  }
  s7_gc_unprotect_via_stack (sc, entries);
  s7_gc_unprotect_via_stack (sc, env);
  s7_gc_unprotect_via_stack (sc, sub);
  return env;
}

/* (prefix import-set prefix-identifier) */
static s7_pointer
r7rs_import_prefix (s7_scheme* sc, s7_pointer iset) {
  if ((s7_list_length (sc, iset) != 3) || (!s7_is_symbol (s7_caddr (iset))))
    return r7rs_library_error (sc, "syntax-error",
                               "import: (prefix ...) needs an import set and a prefix identifier, got ~S", iset);
  s7_pointer sub= r7rs_import_set_env (sc, s7_cadr (iset));
  s7_gc_protect_via_stack (sc, sub);
  s7_pointer env= s7_inlet (sc, s7_nil (sc));
  s7_gc_protect_via_stack (sc, env);
  s7_pointer entries= s7_let_to_list (sc, sub);
  s7_gc_protect_via_stack (sc, entries);
  const char* pre= s7_symbol_name (s7_caddr (iset));
  size_t      pre_len= strlen (pre);
  for (s7_pointer p= entries; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer  entry= s7_car (p);
    const char* name = s7_symbol_name (s7_car (entry));
    size_t      name_len= strlen (name);
    char*       buf= (char*) malloc (pre_len + name_len + 1);
    memcpy (buf, pre, pre_len);
    memcpy (buf + pre_len, name, name_len + 1);
    s7_varlet (sc, env, s7_make_symbol (sc, buf), s7_cdr (entry));
    free (buf);
  }
  s7_gc_unprotect_via_stack (sc, entries);
  s7_gc_unprotect_via_stack (sc, env);
  s7_gc_unprotect_via_stack (sc, sub);
  return env;
}

/* (rename import-set (old new) ...) */
static s7_pointer
r7rs_import_rename (s7_scheme* sc, s7_pointer iset) {
  s7_pointer rest= s7_cdr (iset);
  if (!s7_is_pair (rest))
    return r7rs_library_error (sc, "syntax-error", "import: (rename ...) needs an import set, got ~S", iset);
  for (s7_pointer specs= s7_cdr (rest); s7_is_pair (specs); specs= s7_cdr (specs)) {
    s7_pointer spec= s7_car (specs);
    if ((s7_list_length (sc, spec) != 2) || (!s7_is_symbol (s7_car (spec))) || (!s7_is_symbol (s7_cadr (spec))))
      return r7rs_library_error (sc, "syntax-error", "import: rename expects (old new) pairs, got ~S", spec);
  }
  s7_pointer sub= r7rs_import_set_env (sc, s7_car (rest));
  s7_gc_protect_via_stack (sc, sub);
  s7_pointer env= s7_inlet (sc, s7_nil (sc));
  s7_gc_protect_via_stack (sc, env);
  s7_pointer entries= s7_let_to_list (sc, sub);
  s7_gc_protect_via_stack (sc, entries);
  s7_pointer specs= s7_cdr (rest);
  for (s7_pointer p= entries; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer entry= s7_car (p);
    s7_pointer name = s7_car (entry);
    for (s7_pointer q= specs; s7_is_pair (q); q= s7_cdr (q)) {
      s7_pointer spec= s7_car (q);
      if (s7_car (spec) == name) {
        name= s7_cadr (spec);
        break;
      }
    }
    s7_varlet (sc, env, name, s7_cdr (entry));
  }
  s7_gc_unprotect_via_stack (sc, entries);
  s7_gc_unprotect_via_stack (sc, env);
  s7_gc_unprotect_via_stack (sc, sub);
  return env;
}

/* resolve an import set to the environment of bindings it denotes */
static s7_pointer
r7rs_import_set_env (s7_scheme* sc, s7_pointer iset) {
  if (r7rs_decl_named (iset, "only")) return r7rs_import_only (sc, iset);
  if (r7rs_decl_named (iset, "except")) return r7rs_import_except (sc, iset);
  if (r7rs_decl_named (iset, "prefix")) return r7rs_import_prefix (sc, iset);
  if (r7rs_decl_named (iset, "rename")) return r7rs_import_rename (sc, iset);
  /* plain library name */
  if (!r7rs_library_name_valid (sc, iset))
    return r7rs_library_error (sc, "wrong-type-arg", "import: invalid import set ~S", iset);
  return r7rs_library_env (sc, iset);
}

static s7_pointer
g_import (s7_scheme* sc, s7_pointer args) {
  s7_gc_protect_via_stack (sc, args);
  /* s7 applies a c-macro without changing sc->curlet, so the current let is
   * exactly the environment in which the import form appears. */
  s7_pointer target= s7_curlet (sc);
  for (s7_pointer sets= args; s7_is_pair (sets); sets= s7_cdr (sets)) {
    s7_pointer env= r7rs_import_set_env (sc, s7_car (sets));
    s7_gc_protect_via_stack (sc, env);
    s7_pointer entries= s7_let_to_list (sc, env);
    s7_gc_protect_via_stack (sc, entries);
    /* varlet prepends slots, so bindings of later import sets shadow earlier ones */
    for (s7_pointer p= entries; s7_is_pair (p); p= s7_cdr (p)) {
      s7_pointer entry= s7_car (p);
      s7_varlet (sc, target, s7_car (entry), s7_cdr (entry));
    }
    s7_gc_unprotect_via_stack (sc, entries);
    s7_gc_unprotect_via_stack (sc, env);
  }
  s7_gc_unprotect_via_stack (sc, args);
  return s7_t (sc); /* the "expansion" #t evaluates to itself */
}

void
glue_r7rs_library (s7_scheme* sc) {
  s7_define_variable (sc, R7RS_LIBRARIES_NAME, s7_make_hash_table (sc, 64));
  s7_define_safe_function (sc, "g_library-defined?", g_library_defined_p, 1, 0, false,
                           "(g_library-defined? libname) returns #t if the R7RS library named libname is registered");
  s7_define_safe_function (sc, "g_library-ref", g_library_ref, 1, 0, false,
                           "(g_library-ref libname) returns the exported environment of the R7RS library named libname, "
                           "or #f if it is not registered");
  s7_define_safe_function (sc, "g_library-register!", g_library_register, 2, 0, false,
                           "(g_library-register! libname env) registers env as the exported environment of the R7RS "
                           "library named libname, replacing any previous registration");
  s7_define_safe_function (sc, "g_library-unregister!", g_library_unregister, 1, 0, false,
                           "(g_library-unregister! libname) removes the R7RS library named libname from the registry");
  s7_define_macro (sc, "define-library", g_define_library, 1, 0, true,
                   "(define-library libname decl ...) defines the R7RS library libname from the given declarations "
                   "(export, import, begin, ...) and registers its exported environment");
  s7_define_macro (sc, "import", g_import, 0, 0, true,
                   "(import import-set ...) imports the bindings denoted by each import set (a library name, "
                   "optionally modified by only/except/prefix/rename) into the current environment");
  s7_define_macro (sc, "cond-expand", g_cond_expand_r7rs, 0, 0, true,
                   "(cond-expand clause ...) chooses the first clause whose feature requirement (a feature "
                   "identifier, (library name), or an and/or/not composition) is satisfied");
}
