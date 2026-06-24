#include "a_lolly_test.hpp"
#include "hashmap.hpp"
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
