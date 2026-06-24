#include "a_lolly_test.hpp"
#include "hashset.hpp"
#include "string.hpp"

TEST_CASE ("test contains") {
  auto set= hashset<string> ();
  set->insert ("Hello");
  CHECK_EQ (set->contains ("Hello"), true);
  CHECK_EQ (set->contains ("hello"), false);
  auto empty= hashset<string> ();
  CHECK_EQ (set->contains ("hello"), false);
}

TEST_CASE ("test init") {
  auto set= hashset<string> ();
  set << string ("hello") << string ("world");
  CHECK_EQ (set->contains ("hello"), true);
  CHECK_EQ (set->contains ("world"), true);
  CHECK_EQ (N (set), 2);
}

TEST_CASE ("test remove") {
  auto set1= hashset<string> ();
  auto set2= hashset<string> ();
  CHECK_EQ (N (set1), N (set2));
  set1 << string ("aaa") << string ("bbb");
  CHECK_EQ (N (set1), N (set2) + 2);
  set2 << string ("aaa") << string ("bbb") << string ("ccc");
  set2->remove (string ("bbb"));
  CHECK_EQ (N (set1), N (set2));
}

TEST_CASE ("test operator <=") {
  auto out= tm_ostream ();
  auto set= hashset<string> ();
  set << string ("aaa") << string ("bbb");
  auto set1= hashset<string> ();
  set1 << string ("aaa") << string ("bbb") << string ("ccc");
  auto out1= tm_ostream ();
  CHECK_EQ (set <= set1, true);
}

TEST_CASE ("test copy") {
  auto set1= hashset<string> (), set2= hashset<string> ();
  set1 << string ("a") << string ("b") << string ("c");
  set2= copy (set1);
  CHECK_EQ (set2->contains (string ("a")), true);
  CHECK_EQ (set2->contains (string ("b")), true);
  CHECK_EQ (set2->contains (string ("c")), true);
  CHECK_EQ (N (set1) == N (set2), true);
  // Test utf-8 for Chinese
  set1 << string ("你好") << string ("世界");
  set2= copy (set1);
  CHECK_EQ (set2->contains (string ("你好")), true);
  CHECK_EQ (set2->contains (string ("世界")), true);
}
