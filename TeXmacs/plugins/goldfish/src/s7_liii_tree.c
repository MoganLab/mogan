/* s7_liii_tree.c - tree implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#include "s7_internal.h"
#include "s7_liii_tree.h"
#include "s7_internal_helpers.h"

#define make_integer(Sc, N)              s7_make_integer(Sc, N)
#define s7_integer_clamped_if_gmp(Sc, P) integer(P)

#define TREE_NOT_CYCLIC 0
#define TREE_CYCLIC 1
#define TREE_HAS_PAIRS 2

#ifndef T_SHORT_TREE_COLLECTED
#define T_SHORT_TREE_COLLECTED         (1 << 3)
#define tree_is_collected(p)           has_high_type_bit(T_Pair(p), T_SHORT_TREE_COLLECTED)
#define tree_set_collected(p)          set_high_type_bit(T_Pair(p), T_SHORT_TREE_COLLECTED)
#define tree_clear_collected(p)        clear_high_type_bit(T_Pair(p), T_SHORT_TREE_COLLECTED)
#endif

#define begin_small_symbol_set(Sc)              s7i_begin_small_symbol_set(Sc)
#define end_small_symbol_set(Sc)                s7i_end_small_symbol_set(Sc)
#define add_symbol_to_small_symbol_set(Sc, Sym) s7i_add_symbol_to_small_symbol_set(Sc, Sym)
#define symbol_is_in_small_symbol_set(Sc, Sym)  s7i_symbol_is_in_small_symbol_set(Sc, Sym)

extern s7_pointer a_list_string;

/* -------------------------------- tree-cyclic? -------------------------------- */

static int32_t tree_is_cyclic_or_has_pairs(s7_scheme *sc, s7_pointer tree)
{
  s7_pointer fast = tree, slow = tree; /* we assume tree is a pair */
  bool has_pairs = false;
  while (true)
    {
      if (tree_is_collected(fast)) return(TREE_CYCLIC);
      if ((!has_pairs) && (is_unquoted_pair(sc, car(fast)))) has_pairs = true;
      fast = cdr(fast);
      if (!is_pair(fast)) return((has_pairs) ? TREE_HAS_PAIRS : TREE_NOT_CYCLIC);

      if (tree_is_collected(fast)) return(TREE_CYCLIC);
      if ((!has_pairs) && (is_unquoted_pair(sc, car(fast)))) has_pairs = true;
      fast = cdr(fast);
      if (!is_pair(fast)) return((has_pairs) ? TREE_HAS_PAIRS : TREE_NOT_CYCLIC);

      slow = cdr(slow);
      if (fast == slow) return(TREE_CYCLIC);
    }
  return(TREE_HAS_PAIRS); /* not reached */
}

/* we can't use shared_info here because tree_is_cyclic may be called in the midst of output that depends on sc->circle_info */

static bool tree_is_cyclic_1(s7_scheme *sc, s7_pointer tree)
{
  for (s7_pointer p = tree; is_pair(p); p = cdr(p))
    {
      tree_set_collected(p);
      if (sc->tree_pointers_top == sc->tree_pointers_size)
	{
	  if (sc->tree_pointers_size == 0)
	    {
	      sc->tree_pointers_size = 8;
	      sc->tree_pointers = (s7_pointer *)Malloc(sc->tree_pointers_size * sizeof(s7_pointer));
	    }
	  else
	    {
	      sc->tree_pointers_size *= 2;
	      sc->tree_pointers = (s7_pointer *)Realloc(sc->tree_pointers, sc->tree_pointers_size * sizeof(s7_pointer));
	    }}
      sc->tree_pointers[sc->tree_pointers_top++] = p;
      if (is_unquoted_pair(sc, car(p)))
	{
	  const int32_t old_top = sc->tree_pointers_top;
	  const int32_t result = tree_is_cyclic_or_has_pairs(sc, car(p));
	  if ((result == TREE_CYCLIC) || (tree_is_cyclic_1(sc, car(p))))
	    return(true);
	  for (int32_t i = old_top; i < sc->tree_pointers_top; i++)
	    tree_clear_collected(sc->tree_pointers[i]);
	  sc->tree_pointers_top = old_top;
	}}
  return(false);
}

bool tree_is_cyclic(s7_scheme *sc, s7_pointer tree)
{
  int32_t result;
  if (!is_pair(tree)) return(false);
  result = tree_is_cyclic_or_has_pairs(sc, tree);
  if (result == TREE_NOT_CYCLIC) return(false);
  if (result == TREE_CYCLIC) return(true);
  result = tree_is_cyclic_1(sc, tree);
  for (int32_t i = 0; i < sc->tree_pointers_top; i++)
    tree_clear_collected(sc->tree_pointers[i]);
  sc->tree_pointers_top = 0;
  return(result);
}

s7_pointer g_tree_is_cyclic(s7_scheme *sc, s7_pointer args)
{
  return(make_boolean(sc, tree_is_cyclic(sc, car(args))));
}

/* ---------------- tree-leaves ---------------- */

static inline s7_int tree_len_1(s7_scheme *sc, s7_pointer p)
{
  s7_int sum;
  for (sum = 0; is_pair(p); p = cdr(p))
    {
      s7_pointer cp = car(p);
      if ((!is_pair(cp)) ||
	  (is_quote(sc, car(cp))))
	sum++;
      else
	{
	  do {
	    s7_pointer ccp = car(cp);
	    if ((!is_pair(ccp)) ||
		(is_quote(sc, car(ccp))))
	      sum++;
	    else
	      {
		do {
		  s7_pointer cccp = car(ccp);
		  if ((!is_pair(cccp)) ||
		      (is_quote(sc, car(cccp))))
		    sum++;
		  else sum += tree_len_1(sc, cccp);
		  ccp = cdr(ccp);
		} while (is_pair(ccp));
		if (!is_null(ccp)) sum++;
	      }
	    cp = cdr(cp);
	    } while (is_pair(cp));
	  if (!is_null(cp)) sum++;
	}}
  return((is_null(p)) ? sum : sum + 1);
}

s7_int tree_len(s7_scheme *sc, s7_pointer tree)
{
  if (is_null(tree))
    return(0);
  if ((!is_pair(tree)) || (is_quote(sc, car(tree))))
    return(1);
  return(tree_len_1(sc, tree));
}

s7_int tree_leaves_i_7p(s7_scheme *sc, s7_pointer tree)
{
  if (!is_pair(tree))
    {
      if (is_null(tree)) return(0);
      if (!has_active_methods(sc, tree))
	sole_arg_wrong_type_error_nr(sc, sc->tree_leaves_symbol, tree, a_list_string);
      return(integer(find_and_apply_method(sc, tree, sc->tree_leaves_symbol, set_mlist_1(sc, tree))));
    }
  if ((sc->safety > no_safety) && (tree_is_cyclic(sc, tree)))
    error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-leaves: tree is cyclic: ~S", 31), tree));
  return(tree_len(sc, tree));
}

s7_pointer tree_leaves_p_p(s7_scheme *sc, s7_pointer tree)
{
  if (is_list(tree))
    {
      if ((sc->safety > no_safety) && (tree_is_cyclic(sc, tree)))
	error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-leaves: tree is cyclic: ~S", 31), tree));
      return(make_integer(sc, tree_len(sc, tree)));
    }
  return(method_or_bust_p(sc, tree, sc->tree_leaves_symbol, a_list_string));
}

s7_pointer g_tree_leaves(s7_scheme *sc, s7_pointer args)
{
  return(tree_leaves_p_p(sc, car(args)));
}

/* ---------------- tree-memq ---------------- */

bool tree_memq_1(s7_scheme *sc, s7_pointer sym, s7_pointer tree)    /* sym need not be a symbol */
{
  if (is_quote(sc, car(tree)))
    return((!is_symbol(sym)) && (!is_pair(sym)) && (is_pair(cdr(tree))) && (sym == cadr(tree)));
  do {
    if (sym == car(tree))
      return(true);
    if (is_pair(car(tree)))
      {
	s7_pointer cp = car(tree);
	if (is_quote(sc, car(cp)))
	  {
	    if ((!is_symbol(sym)) && (!is_pair(sym)) && (is_pair(cdr(cp))) && (sym == cadr(cp)))
	      return(true);
	  }
	else
	  do {
	      if (sym == car(cp))
		return(true);
	      if ((is_pair(car(cp))) && (tree_memq_1(sc, sym, car(cp))))
		return(true);
	      cp = cdr(cp);
	      if (sym == cp)
		return(true);
	    } while (is_pair(cp));
      }
    tree = cdr(tree);
    if (sym == tree)
      return(true);
  } while (is_pair(tree));
  return(false);
}

bool s7_tree_memq(s7_scheme *sc, s7_pointer sym, s7_pointer tree)
{
  if (sym == tree) return(true);
  if (!is_pair(tree)) return(false); /* this happens a lot */
  if ((sc->safety > no_safety) && (tree_is_cyclic(sc, tree)))
    error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-memq?: tree is cyclic: ~S", 30), tree));
  return(tree_memq_1(sc, sym, tree));
}

bool tree_memq_b_7pp(s7_scheme *sc, s7_pointer sym, s7_pointer tree)
{
  if (!is_list(tree))
    {
      if (!has_active_methods(sc, tree))
	wrong_type_error_nr(sc, sc->tree_memq_symbol, 2, tree, a_list_string);
      return(find_and_apply_method(sc, tree, sc->tree_memq_symbol, set_mlist_2(sc, sym, tree)) != sc->F);
    }
  return(s7_tree_memq(sc, sym, tree));
}

s7_pointer g_tree_memq(s7_scheme *sc, s7_pointer args)
{
  return(make_boolean(sc, tree_memq_b_7pp(sc, car(args), cadr(args))));
}

bool tree_including_quote_memq(s7_scheme *sc, s7_pointer sym, s7_pointer tree)    /* sym need not be a symbol */
{
  do {
    if (sym == car(tree))
      return(true);
    if (is_pair(car(tree)))
      {
	s7_pointer cp = car(tree);
	do {
	  if (sym == car(cp))
	    return(true);
	  if ((is_pair(car(cp))) && (tree_including_quote_memq(sc, sym, car(cp))))
	    return(true);
	  cp = cdr(cp);
	  if (sym == cp)
	    return(true);
	} while (is_pair(cp));
      }
    tree = cdr(tree);
    if (sym == tree)
      return(true);
  } while (is_pair(tree));
  return(false);
}

/* ---------------- tree-member ---------------- */

static inline bool tree_node_matches(s7_scheme *sc, s7_pointer obj, s7_pointer node, s7_pointer compare)
{
  if (!compare)
    return s7_is_equal(sc, obj, node);

  if (is_pair(node) && !is_pair(obj))
    return false;
  if (is_null(node) && !is_null(obj))
    return false;

  return s7_call(sc, compare, s7_list(sc, 2, obj, node)) != sc->F;
}

static bool tree_member_1(s7_scheme *sc, s7_pointer obj, s7_pointer tree, s7_pointer compare)
{
  if (tree_node_matches(sc, obj, tree, compare))
    return true;
  if (!is_pair(tree))
    return false;
  do {
    if (tree_node_matches(sc, obj, car(tree), compare))
      return true;
    if (is_pair(car(tree)))
      {
	s7_pointer cp = car(tree);
	do {
	  if (tree_node_matches(sc, obj, car(cp), compare))
	    return true;
	  if (is_pair(car(cp)))
	    {
	      if (tree_member_1(sc, obj, car(cp), compare))
		return true;
	    }
	  cp = cdr(cp);
	  if (tree_node_matches(sc, obj, cp, compare))
	    return true;
	} while (is_pair(cp));
      }
    tree = cdr(tree);
    if (tree_node_matches(sc, obj, tree, compare))
      return true;
  } while (is_pair(tree));
  return false;
}

bool tree_member(s7_scheme *sc, s7_pointer obj, s7_pointer tree, s7_pointer compare)
{
  if (!is_list(tree))
    {
      if (!has_active_methods(sc, tree))
	wrong_type_error_nr(sc, sc->tree_member_symbol, 2, tree, a_list_string);
      s7_pointer m_args = (compare) ? s7_list(sc, 3, obj, tree, compare) : set_mlist_2(sc, obj, tree);
      return(find_and_apply_method(sc, tree, sc->tree_member_symbol, m_args) != sc->F);
    }
  if ((sc->safety > no_safety) && (tree_is_cyclic(sc, tree)))
    error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-member?: tree is cyclic: ~S", 32), tree));
  return tree_member_1(sc, obj, tree, compare);
}

s7_pointer g_tree_member(s7_scheme *sc, s7_pointer args)
{
  s7_pointer obj = car(args);
  s7_pointer tree = cadr(args);
  s7_pointer compare = NULL;
  if (is_pair(cddr(args)))
    {
      compare = caddr(args);
      if (!s7_is_procedure(compare))
	wrong_type_error_nr(sc, sc->tree_member_symbol, 3, compare, wrap_string(sc, "a procedure", 11));
    }
  return make_boolean(sc, tree_member(sc, obj, tree, compare));
}

/* ---------------- tree-set-memq ---------------- */

static inline bool pair_set_memq(s7_scheme *sc, s7_pointer tree)
{
  while (true)
    {
      s7_pointer p = car(tree);
      if (is_symbol(p))
	{
	  if (symbol_is_in_small_symbol_set(sc, p))
	    return(true);
	}
      else
	if ((is_unquoted_pair(sc, p)) &&
	    (pair_set_memq(sc, p)))
	  return(true);
      tree = cdr(tree);
      if (!is_pair(tree)) break;
    }
  return((is_symbol(tree)) && (symbol_is_in_small_symbol_set(sc, tree)));
}

bool tree_set_memq_b_7pp(s7_scheme *sc, s7_pointer syms, s7_pointer tree)
{
  bool non_symbols = false;
  if (!is_list(syms))
    {
      if (!has_active_methods(sc, syms))
	wrong_type_error_nr(sc, sc->tree_set_memq_symbol, 1, syms, a_list_string);
      return(find_and_apply_method(sc, syms, sc->tree_set_memq_symbol, set_mlist_2(sc, syms, tree)) != sc->F);
    }
  if (!is_pair(tree))
    {
      if (is_null(tree)) return(false);
      /* (define (func) (do ((i 0 (+ i 1)) (var #f)) ((= i 1) var) (set! var (tree-set-memq (list) (block))))) (func) */
      if (!has_active_methods(sc, tree))
	wrong_type_error_nr(sc, sc->tree_set_memq_symbol, 2, tree, a_list_string);
      return(find_and_apply_method(sc, tree, sc->tree_set_memq_symbol, set_mlist_2(sc, syms, tree)) != sc->F);
    }
  if (is_null(syms)) return(false);
  if (sc->safety > no_safety)
    {
      if (tree_is_cyclic(sc, syms))
	error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-set-memq: symbol list is cyclic: ~S", 40), syms));
      if (tree_is_cyclic(sc, tree))
	error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-set-memq: tree is cyclic: ~S", 33), tree));
    }
  begin_small_symbol_set(sc);
  for (s7_pointer p = syms; is_pair(p); p = cdr(p))
    if (is_symbol(car(p)))
      add_symbol_to_small_symbol_set(sc, car(p));
    else non_symbols = true;
  {
    bool result = pair_set_memq(sc, tree);
    end_small_symbol_set(sc);
    if (result) return(true);
  }
  if (non_symbols)
    for (s7_pointer p = syms; is_pair(p); p = cdr(p))
      if ((!is_symbol(car(p))) &&
	  (s7_tree_memq(sc, car(p), tree)))
	return(true);
  return(false);
}

s7_pointer tree_set_memq_p_pp(s7_scheme *sc, s7_pointer syms, s7_pointer tree)
{
  return(make_boolean(sc, tree_set_memq_b_7pp(sc, syms, tree)));
}

s7_pointer g_tree_set_memq(s7_scheme *sc, s7_pointer args)
{
  return(make_boolean(sc, tree_set_memq_b_7pp(sc, car(args), cadr(args))));
}

s7_pointer tree_set_memq_syms_direct(s7_scheme *sc, s7_pointer syms, s7_pointer tree)
{
  if (!is_pair(tree))
    {
      if (is_null(tree)) return(sc->F);
      if (!has_active_methods(sc, tree))
	wrong_type_error_nr(sc, sc->tree_set_memq_symbol, 2, tree, a_list_string);
      return(find_and_apply_method(sc, tree, sc->tree_set_memq_symbol, set_mlist_2(sc, syms, tree)));
    }
  if (is_quote(sc, car(tree))) return(sc->F);
  if ((sc->safety > no_safety) && (tree_is_cyclic(sc, tree)))
    error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-set-memq: tree is cyclic: ~S", 33), tree));
  begin_small_symbol_set(sc);
  for (s7_pointer p = syms; is_pair(p); p = cdr(p))
    add_symbol_to_small_symbol_set(sc, car(p));
  {
    bool result = pair_set_memq(sc, tree);
    end_small_symbol_set(sc);
    return(make_boolean(sc, result));
  }
}

s7_pointer g_tree_set_memq_syms(s7_scheme *sc, s7_pointer args)
{
  return(tree_set_memq_syms_direct(sc, car(args), cadr(args)));
}

s7_pointer tree_set_memq_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr)
{
  if ((is_proper_quote(sc, cadr(expr))) &&   /* not (tree-set-memq (quote) ...) */
      (is_pair(cadadr(expr))))               /*  (tree-set-memq '(...)...) */
    {
      for (s7_pointer p = cadadr(expr); is_pair(p); p = cdr(p))
	if (!is_symbol(car(p)))
	  return(func);
      return(sc->tree_set_memq_syms);
    }
  return(func);
}

/* ---------------- tree-count ---------------- */

s7_int tree_count(s7_scheme *sc, s7_pointer obj, s7_pointer tree, s7_int count)
{
  if (tree == obj) return(count + 1);
  if ((!is_pair(tree)) || (is_quote(sc, car(tree)))) return(count);
  return(tree_count(sc, obj, cdr(tree), tree_count(sc, obj, car(tree), count)));
}

s7_int tree_count_at_least(s7_scheme *sc, s7_pointer obj, s7_pointer tree, s7_int count, s7_int top)
{
  if (tree == obj) return(count + 1);
  if ((!is_pair(tree)) || (is_quote(sc, car(tree)))) return(count);
  do {
    count = tree_count_at_least(sc, obj, car(tree), count, top);
    if (count >= top) return(count);
    tree = cdr(tree);
    if (tree == obj) return(count + 1);
  } while (is_pair(tree));
  return(count);
}

s7_pointer g_tree_count(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer obj = car(args), tree = cadr(args);
  s7_pointer count;

  if (!is_pair(tree))
    {
      if ((is_pair(cddr(args))) &&
	  (!s7_is_integer(caddr(args))))
	wrong_type_error_nr(sc, sc->tree_count_symbol, 3, caddr(args), sc->type_names[T_INTEGER]);
      if (is_null(tree)) return(s7_make_integer(sc, 0));
      if (!has_active_methods(sc, tree))
	wrong_type_error_nr(sc, sc->tree_count_symbol, 2, tree, a_list_string);
      return(find_and_apply_method(sc, tree, sc->tree_count_symbol, set_mlist_2(sc, obj, tree)));
    }
  if ((sc->safety > no_safety) && (tree_is_cyclic(sc, tree)))
    error_nr(sc, sc->wrong_type_arg_symbol, set_elist_2(sc, wrap_string(sc, "tree-count: tree is cyclic: ~S", 30), tree));
  if (is_null(cddr(args)))
    return(make_integer(sc, tree_count(sc, obj, tree, 0)));
  count = caddr(args);
  if (!s7_is_integer(count))
    wrong_type_error_nr(sc, sc->tree_count_symbol, 3, count, sc->type_names[T_INTEGER]);
  return(make_integer(sc, tree_count_at_least(sc, obj, tree, 0, s7_integer_clamped_if_gmp(sc, count))));
}

/* ---------------- compatibility bridges ---------------- */

bool s7i_tree_is_cyclic(s7_scheme *sc, s7_pointer p) {return(tree_is_cyclic(sc, p));}
s7_int s7i_tree_len(s7_scheme *sc, s7_pointer p) {return(tree_len(sc, p));}
bool s7i_tree_is_cyclic_checked(s7_scheme *sc, s7_pointer tree) {return((sc->safety > no_safety) && (tree_is_cyclic(sc, tree)));}
s7_pointer s7i_tree_leaves_p_p(s7_scheme *sc, s7_pointer p) {return(tree_leaves_p_p(sc, p));}
bool s7i_tree_memq_b_7pp(s7_scheme *sc, s7_pointer sym, s7_pointer tree) {return(tree_memq_b_7pp(sc, sym, tree));}
bool s7i_tree_set_memq_b_7pp(s7_scheme *sc, s7_pointer syms, s7_pointer tree) {return(tree_set_memq_b_7pp(sc, syms, tree));}
s7_pointer s7i_tree_set_memq_syms_direct(s7_scheme *sc, s7_pointer a, s7_pointer b) {return(tree_set_memq_syms_direct(sc, a, b));}
