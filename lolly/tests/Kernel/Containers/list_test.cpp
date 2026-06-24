
/******************************************************************************
 * MODULE     : list_test.cpp
 * DESCRIPTION: test on linked lists with reference counting
 * COPYRIGHT  : (C) 2018 Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "list.hpp"
#include "string.hpp"

static list<long>
gen (int64_t n) {
  auto normal= list<long> ();
  for (long i= 0; i < n; i++)
    normal << i;
  return normal;
}

auto the_nil_list = list<string> ();
auto the_atom_list= list<long> (1);
auto normal       = list<long> (1, 2, 3, list<long> ());

TEST_CASE ("is nil") {
  CHECK_EQ (is_nil (the_nil_list), true);
  CHECK_EQ (is_nil (the_atom_list), false);
}

TEST_CASE ("is atom") {
  CHECK_EQ (is_atom (the_nil_list), false);
  CHECK_EQ (is_atom (the_atom_list), true);
}

/******************************************************************************
 * tests on output and convertion
 ******************************************************************************/

TEST_CASE ("access") {
  // QVERIFY_EXCEPTION_THROWN (the_nil_list[0], string);

  CHECK_EQ (the_atom_list[0], 1L);

  CHECK_EQ (normal[0], 1L);
  CHECK_EQ (normal[1], 2L);
  CHECK_EQ (normal[2], 3L);
}

/******************************************************************************
 * tests on insertion and suppression
 ******************************************************************************/

TEST_CASE ("operate on the last") {
  // QVERIFY_EXCEPTION_THROWN (access_last (the_nil_list), string);
  // QVERIFY_EXCEPTION_THROWN (suppress_last (the_nil_list), string);
  // QVERIFY_EXCEPTION_THROWN (last_item (the_nil_list), string);

  auto the_atom_list_copy         = copy (the_atom_list);
  access_last (the_atom_list_copy)= 2L;
  CHECK_EQ (the_atom_list_copy, list<long> (2L));
  suppress_last (the_atom_list_copy);
  CHECK_EQ (the_atom_list_copy, list<long> ());
  CHECK_EQ (last_item (the_atom_list), 1L);

  auto normal_copy         = copy (normal);
  access_last (normal_copy)= 4;
  CHECK_EQ (normal_copy, list<long> (1, 2, 4, list<long> ()));
  suppress_last (normal_copy);
  CHECK_EQ (normal_copy, list<long> (1, 2, list<long> ()));
  CHECK_EQ (last_item (normal), 3L);
}

/******************************************************************************
 * tests on computations with list<T> structures
 ******************************************************************************/

TEST_CASE ("size") {
  CHECK_EQ (N (the_nil_list), 0);
  CHECK_EQ (N (the_atom_list), 1);
  CHECK_EQ (N (normal), 3);
  for (auto i= 4; i <= 100; i++) {
    auto list= gen (i);
    CHECK_EQ (N (list), i);
  }
}

TEST_CASE ("copy") {
  CHECK_EQ (copy (the_nil_list), the_nil_list);
  CHECK_EQ (copy (the_atom_list), the_atom_list);
  CHECK_EQ (copy (normal), normal);
}

TEST_CASE ("append") {
  auto appended= the_nil_list * string ("a");
  CHECK_EQ (appended, list<string> (string ("a")));

  auto to_append= list<string> ("a");
  CHECK_EQ (the_nil_list * to_append, to_append);

  auto nil_to_append= list<string> ();
  CHECK_EQ (the_nil_list * nil_to_append, nil_to_append);

  CHECK_EQ (the_atom_list * list<long> (), the_atom_list);

  CHECK_EQ (list<long> (1L, 2L, list<long> ()) * list<long> (3L), normal);
  CHECK_EQ (list<long> (1L) * list<long> (2L, 3L, list<long> ()), normal);
}

TEST_CASE ("head and tail") {
  // QVERIFY_EXCEPTION_THROWN (head (the_nil_list), string);
  // QVERIFY_EXCEPTION_THROWN (tail (the_nil_list), string);

  CHECK_EQ (head (the_atom_list), the_atom_list);
  CHECK_EQ (tail (the_atom_list), list<long> ());

  CHECK_EQ (head (normal), list<long> (1));
  CHECK_EQ (tail (normal), list<long> (2, 3, list<long> ()));
}

TEST_CASE ("reverse") {
  CHECK_EQ (reverse (the_nil_list), the_nil_list);
  CHECK_EQ (reverse (the_atom_list), the_atom_list);
  CHECK_EQ (reverse (normal), list<long> (3, 2, 1, list<long> ()));
}

TEST_CASE ("remove") {
  CHECK_EQ (remove (the_nil_list, string ("a")), the_nil_list);
  CHECK_EQ (remove (the_atom_list, 1L), list<long> ());
  CHECK_EQ (remove (normal, 2L), list<long> (1, 3, list<long> ()));
}

TEST_CASE ("contains") {
  CHECK_EQ (contains (the_nil_list, string ("a")), false);
  CHECK_EQ (contains (the_atom_list, 1L), true);
  CHECK_EQ (contains (normal, 1L), true);

  CHECK_EQ (contains (normal, 2L), true);
  CHECK_EQ (contains (normal, 3L), true);
}
