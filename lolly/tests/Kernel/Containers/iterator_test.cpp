#include "a_lolly_test.hpp"
#include "iterator.hpp"
#include "string.hpp"

TEST_CASE ("iterate on empty object") {

  SUBCASE ("test empty hashset") {
    hashset<int>  set;
    iterator<int> it= iterate (set);
    CHECK (!it->busy ());
  }

  SUBCASE ("test empty hashmap") {
    hashmap<int, int> map;
    iterator<int>     it= iterate (map);
    CHECK (!it->busy ());
  }
}

TEST_CASE ("iterate on hashset") {
  hashset<int> set;
  set->insert (1);
  set->insert (2);
  hashset<string> set1;
  set1->insert (string ("string1"));
  set1->insert (string ("string2"));
  SUBCASE ("Test iterator busy") {
    iterator<int>    it = iterate (set);
    iterator<string> it1= iterate (set1);
    CHECK (it->busy ());
    CHECK (it1->busy ());
  }
  SUBCASE ("Test iterator remains") {
    iterator<int>    it = iterate (set);
    iterator<string> it1= iterate (set1);
    CHECK (it->remains () == -1);
    CHECK (it1->remains () == -1);
  }
  SUBCASE ("Test iterator next") {
    iterator<int>    it = iterate (set);
    iterator<string> it1= iterate (set1);
    CHECK (it->next ());
    CHECK_EQ (it1->next () == string ("string1"), true);
  }
}

TEST_CASE ("iterate on hashmap") {
  hashmap<int, int> h;
  h (1)           = 1;
  h (2)           = 4;
  h (3)           = 6;
  iterator<int> it= iterate (h);
  auto          a = string ();
  while (it->busy ()) {
    a= a * as_string (it->next ());
  }
  CHECK_EQ (a == string ("123"), true);
}

TEST_CASE ("ranged for on hashmap") {
  SUBCASE ("ordinary") {
    hashmap<int, int> h;
    h (1)= 1;
    h (2)= 4;
    h (3)= 6;
    string a;
    for (auto i : iterate (h)) {
      a= a * as_string (i);
    }
    string_eq (a, "123");
  };
  SUBCASE ("empty") {
    hashmap<int, int> h;
    string            a;
    for (auto i : iterate (h)) {
      a= a * as_string (i);
    }
    string_eq (a, "");
  };
}

TEST_CASE ("iterate on hashset") {
  hashset<int> h;
  h->insert (1);
  h->insert (2);
  h->insert (3);
  iterator<int> it= iterate (h);
  string        a ("");
  while (it->busy ()) {
    a << as_string (it->next ());
  }
  CHECK_EQ (a == string ("123"), true);
}

TEST_CASE ("ranged for on hashset") {
  SUBCASE ("ordinary") {
    hashset<int> h;
    h->insert (1);
    h->insert (2);
    h->insert (3);
    string a;
    for (auto i : iterate (h)) {
      a << as_string (i);
    }
    string_eq (a, "123");
  };
  SUBCASE ("empty") {
    hashset<int> h;
    string       a;
    for (auto i : iterate (h)) {
      a= a * as_string (i);
    }
    string_eq (a, "");
  }
}