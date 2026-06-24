#include "a_lolly_test.hpp"
#include "rel_hashmap.hpp"

TEST_CASE ("test contains") {
  rel_hashmap<int, int> t (0);

  SUBCASE ("test case is empty") { CHECK (t->contains (1) == false); }

  SUBCASE ("test case is not empty") {
    hashmap<int, int> hm1 (0, 10);
    hm1 (1)= 10;
    hm1 (2)= 20;
    t->change (hm1);

    CHECK (t->contains (1) == true);
    CHECK (t->contains (2) == true);
    CHECK (t->contains (3) == false);

    hashmap<int, int> hm2 (0, 10);
    hm2 (3)= 30;
    hm2 (4)= 40;
    t->extend ();
    t->change (hm2);

    CHECK (t->contains (1) == true);
    CHECK (t->contains (2) == true);
    CHECK (t->contains (3) == true);
    CHECK (t->contains (5) == false);
  }
}

TEST_CASE ("test extend") {
  rel_hashmap<int, int> t (0);

  hashmap<int, int> hm1 (0, 10);
  hm1 (1)= 10;
  hm1 (2)= 20;
  t->change (hm1);
  t->extend ();

  CHECK (t->contains (1) == true);
  CHECK (t->contains (2) == true);
  CHECK (t->contains (3) == false);
}

TEST_CASE ("test shorten") {
  rel_hashmap<int, int> t (0);

  SUBCASE ("test Shorten the empty rel_hashmap") {
    TM_CHECK_THROWS (t->shorten ());
  }

  SUBCASE ("test not Shorten the empty rel_hashmap") {

    SUBCASE ("test not throw") {
      t->extend ();
      CHECK_NOTHROW (t->shorten ());
    }

    SUBCASE ("test the content") {
      t->extend ();
      hashmap<int, int> hm1 (0, 10);
      hm1 (1)= 10;
      hm1 (2)= 20;
      t->change (hm1);
      t->shorten ();

      CHECK (t->contains (1) == false);
      CHECK (t->contains (2) == false);
      CHECK (t->contains (4) == false);
      CHECK (is_nil (t->next));
    }
  }
}

TEST_CASE ("test merge") {
  rel_hashmap<int, int> t (0);

  SUBCASE ("test merge the empty rel_hashmap") {
    TM_CHECK_THROWS (t->merge ());
  }

  hashmap<int, int> hm1 (0, 10);
  hm1 (1)= 10;
  hm1 (2)= 20;
  t->change (hm1);
  t->extend ();

  SUBCASE ("test not merge the empty rel_hashmap") {

    SUBCASE ("test not throw") {
      t->extend ();
      CHECK_NOTHROW (t->merge ());
    }

    SUBCASE ("test the content") {
      t->extend ();
      hashmap<int, int> hm2 (0, 10);
      hm2 (3)= 30;
      hm2 (4)= 40;
      t->change (hm2);
      t->merge ();

      CHECK (t->contains (1) == true);
      CHECK (t->contains (2) == true);
      CHECK (t->contains (3) == true);
      CHECK (t->contains (4) == true);
      CHECK (t->contains (5) == false);
    }
  }
}

TEST_CASE ("test find_changes") {
  rel_hashmap<int, int> t (0);
  hashmap<int, int>     hm1 (0, 10);
  hm1 (1)= 10;
  hm1 (2)= 20;
  t->change (hm1);

  SUBCASE ("test if all the same") {
    hashmap<int, int> hm2 (0, 10);
    hm2 (1)= 10;
    hm2 (2)= 20;
    t->find_changes (hm2);

    CHECK (hm2->contains (1) == false);
    CHECK (hm2->contains (2) == false);
  }

  SUBCASE ("test if not same") {
    hashmap<int, int> hm2 (0, 10);
    hm2 (1)= 10;
    hm2 (2)= 30;
    t->find_changes (hm2);

    CHECK (hm2->contains (1) == false);
    CHECK (hm2[2] == 30);
  }

  SUBCASE ("test if all different") {
    hashmap<int, int> hm2 (0, 10);
    hm2 (1)= 20;
    hm2 (2)= 30;
    t->find_changes (hm2);

    CHECK (hm2[1] == 20);
    CHECK (hm2[2] == 30);
  }
}

TEST_CASE ("test find_differences") {
  rel_hashmap<int, int> t (0);
  hashmap<int, int>     hm1 (0, 10);
  hm1 (1)= 10;
  hm1 (2)= 20;
  t->change (hm1);

  SUBCASE ("test if all the same") {
    hashmap<int, int> hm2 (0, 10);
    hm2 (1)= 10;
    hm2 (2)= 20;
    t->find_differences (hm2);

    CHECK (hm2->contains (1) == false);
    CHECK (hm2->contains (2) == false);
  }

  t->extend ();
  hashmap<int, int> hm2 (0, 10);
  hm1 (3)= 30;
  hm1 (4)= 40;
  t->change (hm2);

  SUBCASE ("test if key different") {

    SUBCASE ("test if different from first, but the same as next") {
      hashmap<int, int> hm3 (0, 10);
      hm3 (3)= 30;
      hm3 (4)= 40;
      t->find_differences (hm3);

      CHECK (hm3->contains (1) == false);
      CHECK (hm3->contains (2) == false);
      CHECK (hm3->contains (1) == false);
      CHECK (hm3->contains (2) == false);
    }

    SUBCASE ("test if different from all the others") {
      hashmap<int, int> hm3 (0, 10);
      hm3 (5)= 30;
      hm3 (6)= 40;
      t->find_differences (hm3);

      CHECK (hm3->contains (1) == false);
      CHECK (hm3->contains (2) == false);
      CHECK (hm3[5] == 30);
      CHECK (hm3[6] == 40);

      hashmap<int, int> hm4 (0, 10);
      hm4 (3)= 40;
      hm4 (4)= 50;
      t->find_differences (hm4);

      CHECK (hm4->contains (1) == false);
      CHECK (hm4->contains (2) == false);
      CHECK (hm4[3] == 40);
      CHECK (hm4[4] == 50);
    }
  }
}

TEST_CASE ("test change") {
  rel_hashmap<int, int> t (0);
  hashmap<int, int>     hm1 (0, 10);
  hm1 (1)= 10;
  hm1 (2)= 20;
  t->change (hm1);

  CHECK (t->contains (1));
  CHECK (t->contains (2));

  SUBCASE ("test change the same") {
    hashmap<int, int> hm2 (0, 10);
    hm2 (1)= 10;
    hm2 (2)= 20;
    t->change (hm2);

    CHECK (t->contains (1));
    CHECK (t->contains (2));

    hashmap<int, int> hm3 (0, 10);
    hm3 (1)= 20;
    hm3 (2)= 30;
    t->change (hm3);

    CHECK (t[1] == 20);
    CHECK (t[2] == 30);
  }

  SUBCASE ("test change the different") {
    hashmap<int, int> hm2 (0, 10);
    hm2 (3)= 30;
    hm2 (4)= 40;
    t->change (hm2);

    CHECK (t[1] == 10);
    CHECK (t[2] == 20);
    CHECK (t[3] == 30);
    CHECK (t[4] == 40);
  }

  SUBCASE ("test change the next") {
    t->extend ();
    hashmap<int, int> hm2 (0, 10);
    hm2 (3)= 30;
    hm2 (4)= 40;
    t->change (hm2);

    CHECK (t[1] == 10);
    CHECK (t[2] == 20);
    CHECK (t[3] == 30);
    CHECK (t[4] == 40);
  }
}