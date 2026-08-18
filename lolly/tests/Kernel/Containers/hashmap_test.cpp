#include "a_lolly_test.hpp"
#include "hashmap.hpp"
#include "string.hpp"
#include <string>

TEST_CASE ("test_resize") {
  auto hm= hashmap<int, int> (0, 10);
  hm (1) = 10;
  hm (2) = 20;

  hm->resize (1);
  CHECK_EQ (hm[1] == 10, true);
  CHECK_EQ (hm[2] == 20, true);

  hm->resize (20);
  CHECK_EQ (hm[1] == 10, true);
  CHECK_EQ (hm[2] == 20, true);
}

TEST_CASE ("test reset") {
  auto hm= hashmap<int, int> (0, 10);
  hm (1) = 10;
  hm (11)= 20;
  hm->reset (1);

  CHECK_EQ (hm->contains (1), false);
  CHECK_EQ (hm->contains (11), true);
}

auto hm_generate= hashmap<int, int> (0, 10);
void
routine (int key) {
  CHECK_EQ (hm_generate->contains (key), true);
}

TEST_CASE ("test generate") {
  hm_generate (1)= 10;
  hm_generate (2)= 20;
  hm_generate->generate (routine);
}

TEST_CASE ("test contains") {
  auto hm= hashmap<int, void*> (nullptr, 2, 2);
  hm (1) = nullptr;
  CHECK_EQ (hm->contains (1), true);
  CHECK_EQ (hm->contains (3), false);
}

TEST_CASE ("test empty") {
  auto hm= hashmap<int, int> ();
  CHECK_EQ (hm->empty (), true);

  hm (1);
  CHECK_EQ (hm->empty (), false);
}

TEST_CASE ("test join") {
  auto hm1= hashmap<int, int> ();
  auto hm2= hashmap<int, int> ();
  hm1 (1) = 10;
  hm1 (2) = 20;
  hm2 (2) = -20;
  hm2 (3) = -30;
  hm1->join (hm2);

  CHECK_EQ (hm1[1] == 10, true);
  CHECK_EQ (hm1[2] == -20, true);
  CHECK_EQ (hm1[3] == -30, true);
}

TEST_CASE ("test write back") {
  auto hm1= hashmap<int, int> (0, 10);
  auto hm2= hashmap<int, int> (0, 10);
  hm1 (1) = 10;
  hm1 (2) = 20;
  hm2 (2) = -20;

  hm1->write_back (2, hm2);
  CHECK_EQ (hm1[2] == 20, true);

  hm1->write_back (3, hm2);
  CHECK_EQ (hm1[3] == 0, true);

  hm2 (4)= -40;
  hm1->write_back (4, hm2);
  CHECK_EQ (hm2[4] == -40, true);
}

TEST_CASE ("test pre patch") {
  auto hm      = hashmap<int, int> ();
  auto hm_patch= hashmap<int, int> ();
  auto hm_base = hashmap<int, int> ();

  hm (1)= 10;
  hm_patch (1);
  hm->pre_patch (hm_patch, hm_base);
  CHECK_EQ (hm[1] == 10, true);

  hm (2)      = 20;
  hm_patch (2)= -20;
  hm_base (2) = 20;
  hm->pre_patch (hm_patch, hm_base);
  CHECK_EQ (hm[2] == 0, true);

  hm_patch (3)= -30;
  hm->pre_patch (hm_patch, hm_base);
  CHECK_EQ (hm[3] == -30, true);
}

TEST_CASE ("test post patch") {
  auto hm      = hashmap<int, int> ();
  auto hm_patch= hashmap<int, int> ();
  auto hm_base = hashmap<int, int> ();

  hm (1)= 10;
  hm_patch (1);
  hm->post_patch (hm_patch, hm_base);
  CHECK_EQ (hm[1] == 0, true);

  hm_patch (2)= -20;
  hm->pre_patch (hm_patch, hm_base);
  CHECK_EQ (hm[2] == -20, true);
}

TEST_CASE ("test copy") {
  auto hm  = hashmap<int, int> ();
  auto hm_c= hashmap<int, int> (0, 10, 2);
  hm_c (1) = 10;
  hm_c (11)= 110;
  hm_c (2) = 20;

  auto res_hm= copy (hm_c);
  CHECK_EQ (res_hm[1] == 10, true);
  CHECK_EQ (res_hm[11] == 110, true);
  CHECK_EQ (res_hm[2] == 20, true);
}

TEST_CASE ("test equality") {
  auto hm1= hashmap<int, int> (0, 10, 3);
  auto hm2= hashmap<int, int> (0, 100, 30);
  hm1 (1) = 10;
  hm2 (1) = 10;
  CHECK_EQ (hm1 == hm2, true);

  hm2 (2)= 20;
  CHECK_EQ (hm1 != hm2, true);
}

TEST_CASE ("test changes") {
  auto base_m = hashmap<int, int> ();
  auto patch_m= hashmap<int, int> ();
  base_m (1)  = 10;
  base_m (2)  = 20;
  patch_m (2) = -20;
  patch_m (3) = -30;
  auto res    = changes (patch_m, base_m);
  CHECK_EQ (N (res) == 2, true);
  CHECK_EQ (res[2] == -20, true);
  CHECK_EQ (res[3] == -30, true);
}

TEST_CASE ("test invert") {
  auto base_m = hashmap<int, int> ();
  auto patch_m= hashmap<int, int> ();
  base_m (1)  = 10;
  base_m (2)  = 20;
  patch_m (2) = -20;
  patch_m (3) = -30;
  auto res    = invert (patch_m, base_m);
  CHECK_EQ (N (res) == 2, true);
  CHECK_EQ (res[2] == 20, true);
  CHECK_EQ (res[3] == 0, true);
}

TEST_CASE ("test size") {
  auto empty_hm= hashmap<int, void*> ();
  CHECK_EQ (N (empty_hm) == 0, true);

  auto non_empty_hm= hashmap<int, void*> ();
  non_empty_hm (1) = nullptr;
  CHECK_EQ (N (non_empty_hm) == 1, true);
}

TEST_CASE ("test auto resize with clustered keys") {
  // 等差键低位聚集:hash(int) 为恒等,步长 64 的键在旧实现下挤进同一桶,
  // 触发 insert_node 内的自动扩容路径
  auto hm= hashmap<int, int> (0);
  for (int i= 0; i < 1000; i++)
    hm (i * 64)= i;
  CHECK_EQ (N (hm), 1000);
  for (int i= 0; i < 1000; i++) {
    CHECK_EQ (hm->contains (i * 64), true);
    CHECK_EQ (hm[i * 64], i);
  }
  CHECK_EQ (hm->contains (1000 * 64), false);

  for (int i= 0; i < 1000; i+= 2)
    hm->reset (i * 64); // 触发缩容路径
  CHECK_EQ (N (hm), 500);
  for (int i= 0; i < 1000; i++)
    CHECK_EQ (hm->contains (i * 64), (i % 2) == 1);
  for (int i= 1; i < 1000; i+= 2)
    CHECK_EQ (hm[i * 64], i);
}

TEST_CASE ("test shrink to empty and rebuild") {
  auto hm= hashmap<int, int> (0);
  for (int i= 0; i < 200; i++)
    hm (i)= i;
  for (int i= 0; i < 200; i++)
    hm->reset (i);
  CHECK_EQ (hm->empty (), true);
  CHECK_EQ (N (hm), 0);
  CHECK_EQ (hm->contains (0), false);
  CHECK_EQ (hm[42], 0); // 空表读缺失键返回 init

  for (int i= 0; i < 50; i++)
    hm (i)= -i; // 缩到 1 桶后再重建
  CHECK_EQ (N (hm), 50);
  for (int i= 0; i < 50; i++)
    CHECK_EQ (hm[i], -i);
}

TEST_CASE ("test reset missing key is noop") {
  auto hm= hashmap<int, int> (0);
  hm (1) = 10;
  hm->reset (999);
  hm->reset (-1);
  CHECK_EQ (N (hm), 1);
  CHECK_EQ (hm[1], 10);
}

TEST_CASE ("test manual resize with clustered keys") {
  auto hm= hashmap<int, int> (0, 1);
  for (int i= 0; i < 100; i++)
    hm (i * 128)= i;
  hm->resize (1);
  for (int i= 0; i < 100; i++) {
    CHECK_EQ (hm->contains (i * 128), true);
    CHECK_EQ (hm[i * 128], i);
  }
  hm->resize (512);
  for (int i= 0; i < 100; i++) {
    CHECK_EQ (hm->contains (i * 128), true);
    CHECK_EQ (hm[i * 128], i);
  }
  CHECK_EQ (N (hm), 100);
}

TEST_CASE ("test join triggers destination resize") {
  auto src= hashmap<int, int> (0);
  for (int i= 0; i < 500; i++)
    src (i)= i * 3;
  auto dst= hashmap<int, int> (0); // 单桶,join 过程必然多次扩容
  dst (0) = -1;
  dst->join (src);
  CHECK_EQ (N (dst), 500);
  for (int i= 0; i < 500; i++)
    CHECK_EQ (dst[i], i * 3);
  CHECK_EQ (dst == src, true);
}

TEST_CASE ("test join equality against rebuilt map") {
  auto hm1= hashmap<int, int> (0, 4);
  auto hm2= hashmap<int, int> (0, 256, 4); // 桶数不同,== 复用 code 跨桶查找
  for (int i= 0; i < 300; i++) {
    hm1 (i * 16)= i;
    hm2 (i * 16)= i;
  }
  CHECK_EQ (hm1 == hm2, true);
  hm2 (299 * 16)= -1;
  CHECK_EQ (hm1 == hm2, false);
}

TEST_CASE ("test string keys across resize") {
  auto hm= hashmap<string, int> (0);
  for (int i= 0; i < 300; i++)
    hm ("key_" * as_string (i))= i;
  CHECK_EQ (N (hm), 300);
  auto hc= copy (hm);
  CHECK_EQ (hc == hm, true);
  for (int i= 0; i < 300; i+= 3)
    hc->reset ("key_" * as_string (i));
  CHECK_EQ (hc == hm, false);
  CHECK_EQ (hc->contains ("key_" * as_string (1)), true);
  CHECK_EQ (hc->contains ("key_" * as_string (3)), false);
}

TEST_CASE ("hashmap default") {
  // Create a hashmap object with integer keys and string values
  hashmap<int, std::string> map ("default");
  // Test the comparison operators
  hashmap<int, std::string> equal_map ("default");
  equal_map (1)= "one";
  equal_map (2)= "two";
  hashmap<int, std::string> not_equal_map ("default");
  not_equal_map (1)= "one";
  not_equal_map (2)= "three";
  CHECK_EQ (map == equal_map, false);
  CHECK (map != not_equal_map);
}
