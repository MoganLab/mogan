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

TEST_CASE ("test int keys with clustered low bits") {
  // hash(int) 为恒等,步长 64 的键低位相同,覆盖 hashset 的桶下标混合与
  // 自动扩容/缩容路径
  auto set= hashset<int> ();
  for (int i= 0; i < 500; i++)
    set << (i * 64);
  CHECK_EQ (N (set), 500);
  for (int i= 0; i < 500; i++)
    CHECK_EQ (set->contains (i * 64), true);
  CHECK_EQ (set->contains (500 * 64), false);

  auto sub= hashset<int> ();
  for (int i= 0; i < 500; i+= 2)
    sub << (i * 64);
  CHECK_EQ (sub <= set, true);

  for (int i= 0; i < 500; i+= 2)
    set->remove (i * 64);
  CHECK_EQ (N (set), 250);
  for (int i= 0; i < 500; i++)
    CHECK_EQ (set->contains (i * 64), (i % 2) == 1);
}

TEST_CASE ("test remove missing is noop") {
  auto set= hashset<int> ();
  set << 1;
  set->remove (999);
  CHECK_EQ (N (set), 1);
  CHECK_EQ (set->contains (1), true);
}

TEST_CASE ("test repeated resize with duplicates") {
  // max=8 触发多次扩容;穿插重复插入与删除,验证节点重挂后条目不丢不重
  auto set= hashset<int> (1, 8);
  for (int round= 0; round < 3; round++) {
    for (int i= 0; i < 300; i++)
      set << (i * 97);
    for (int i= 0; i < 300; i++)
      set << (i * 97); // 重复插入为 no-op
    CHECK_EQ (N (set), 300);
    for (int i= 0; i < 150; i++)
      set->remove (i * 97);
    CHECK_EQ (N (set), 150);
    for (int i= 0; i < 150; i++)
      CHECK_EQ (set->contains (i * 97), false);
    for (int i= 150; i < 300; i++)
      CHECK_EQ (set->contains (i * 97), true);
  }
}

TEST_CASE ("test equality after copy and remove") {
  auto s1= hashset<int> ();
  for (int i= 0; i < 100; i++)
    s1 << (i * 32);
  auto s2= copy (s1);
  CHECK_EQ (s1 == s2, true);
  s2->remove (50 * 32);
  CHECK_EQ (s1 == s2, false);
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
