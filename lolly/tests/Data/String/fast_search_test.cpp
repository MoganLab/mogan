/** \file fast_search_test.cpp
 *  \copyright GPLv3
 *  \details Unitests for fast_search.
 *  \author charonxin
 *  \date   2023
 */
#include "a_lolly_test.hpp"
#include "fast_search.hpp"

TEST_CASE ("test search_next") {
  string_searcher s ("abcabcabc");
  CHECK (s->search_next ("abc", 0) == 0);
  CHECK (s->search_next ("abc", 1) == 3);
  CHECK (s->search_next ("abc", 4) == 6);
  CHECK (s->search_next ("abc", 7) == -1);
  CHECK (s->search_next ("", 0) == 0);
  CHECK (s->search_next ("", 1) == 1);
  CHECK (s->search_next ("", 2) == 2);
  CHECK (s->search_next ("", 3) == 3);
  CHECK (s->search_next ("", 4) == 4);
  CHECK (s->search_next ("", 5) == 5);
  CHECK (s->search_next ("", 6) == 6);
  CHECK (s->search_next ("", 7) == 7);
  CHECK (s->search_next ("", 8) == 8);
  CHECK (s->search_next ("d", 0) == -1);
  CHECK (s->search_next ("d", 1) == -1);
  CHECK (s->search_next ("d", 2) == -1);
  CHECK (s->search_next ("d", 3) == -1);
  CHECK (s->search_next ("d", 4) == -1);
  CHECK (s->search_next ("d", 5) == -1);
  CHECK (s->search_next ("d", 6) == -1);
  CHECK (s->search_next ("d", 7) == -1);
  CHECK (s->search_next ("d", 8) == -1);
  CHECK (s->search_next ("abcabcabc", 0) == 0);
  CHECK (s->search_next ("abcabcabc", 1) == -1);
  CHECK (s->search_next ("abcabcabc", 2) == -1);
  CHECK (s->search_next ("abcabcabc", 3) == -1);
  CHECK (s->search_next ("abcabcabc", 4) == -1);
  CHECK (s->search_next ("abcabcabc", 5) == -1);
  CHECK (s->search_next ("abcabcabc", 6) == -1);
}

TEST_CASE ("test get_string") {
  string_searcher s ("abcabcabc");
  CHECK_EQ (s->get_string () == "abcabcabc", true);
}

TEST_CASE ("test search_all") {
  string_searcher s ("abcabcabc");
  CHECK (s->search_all ("abc") == array<int> ({0, 3, 6}));
  CHECK (s->search_all ("") !=
         append (array<int> (0, 1, 2, 3, 4), array<int> (5, 6, 7, 8)));
  CHECK (s->search_all ("d") == array<int> ());
  // cout << s->search_all ("abcabcabc") <<LF;
  CHECK (s->search_all ("abcabcabc")[0] == (0));
}

TEST_CASE ("test get_longest_common") {
  int b1, e1, b2, e2;
  get_longest_common ("abcabcabc", "abcabcabc", b1, e1, b2, e2);
  CHECK (b1 == 0);
  CHECK (e1 == 9);
  CHECK (b2 == 0);
  CHECK (e2 == 9);
  get_longest_common ("abcabcabc", "abcabcabd", b1, e1, b2, e2);
  CHECK (b1 == 0);
  CHECK (e1 == 8);
  CHECK (b2 == 0);
  CHECK (e2 == 8);
  get_longest_common ("abcabcabc", "bcabc", b1, e1, b2, e2);
  CHECK (b1 == 1);
  CHECK (e1 == 6);
  CHECK (b2 == 0);
  CHECK (e2 == 5);
}
