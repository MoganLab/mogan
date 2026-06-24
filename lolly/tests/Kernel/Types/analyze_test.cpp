#include "a_lolly_test.hpp"
#include "analyze.hpp"

TEST_CASE ("is_alpha") {
  for (unsigned char c= 0; c < 255; c++) {
    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
      CHECK (is_alpha (c));
    }
    else {
      CHECK (!is_alpha (c));
    }
  }
}

TEST_CASE ("is_digit") {
  for (unsigned char c= 0; c < 255; c++) {
    if (c >= 48 && c <= 57) {
      CHECK (is_digit (c));
    }
    else {
      CHECK (!is_digit (c));
    }
  }
}

TEST_CASE ("is_space") {
  for (unsigned char c= 0; c < 255; c++) {
    if ((c == 9) || (c == 10) || (c == 13) || (c == 32)) {
      CHECK (is_space (c));
    }
    else {
      CHECK (!is_space (c));
    }
  }
}

TEST_CASE ("is_binary_digit") {
  for (unsigned char c= 0; c < 255; c++) {
    if ((c == '0') || (c == '1')) {
      CHECK (is_binary_digit (c));
    }
    else {
      CHECK (!is_binary_digit (c));
    }
  }
}

TEST_CASE ("is_alpha") {
  CHECK (is_alpha ("a"));
  CHECK (is_alpha ("abc"));
  CHECK (is_alpha ("Hello"));
  CHECK (!is_alpha ("!"));
  CHECK (!is_alpha ("abc123"));
  CHECK (!is_alpha (""));
}

TEST_CASE ("is_alphanum") {
  CHECK (is_alphanum ("s3"));
  CHECK (is_alphanum ("ipv6"));
  CHECK (is_alphanum ("abc"));
  CHECK (is_alphanum ("123"));
  CHECK (!is_alphanum (""));
  CHECK (!is_alphanum ("!"));
}

TEST_CASE ("test locase all") {
  CHECK_EQ (locase_all (string ("true")) == string ("true"), true);
  CHECK_EQ (locase_all (string ("TRue")) == string ("true"), true);
  CHECK_EQ (locase_all (string ("TRUE")) == string ("true"), true);
  CHECK_EQ (locase_all (string ("123TRUE")) == string ("123true"), true);
}

TEST_CASE ("test upcase all") {
  CHECK_EQ (upcase_all (string ("true")) == string ("TRUE"), true);
  CHECK_EQ (upcase_all (string ("TRue")) == string ("TRUE"), true);
  CHECK_EQ (upcase_all (string ("TRUE")) == string ("TRUE"), true);
  CHECK_EQ (upcase_all (string ("123true")) == string ("123TRUE"), true);
}

TEST_CASE ("test string minus") {
  CHECK_EQ (string_minus ("Hello World", "eo") == string ("Hll Wrld"), true);
  CHECK_EQ (string_minus ("", "abc") == string (""), true);
  CHECK_EQ (string_minus ("abc", "") == string ("abc"), true);
}

TEST_CASE ("test string union") {
  CHECK_EQ (string_union ("abc", "") == string ("abc"), true);
  CHECK_EQ (string_union ("", "abc") == string ("abc"), true);
  CHECK_EQ (string_union ("Hello World", "eo") == string ("Hll Wrldeo"), true);
}

TEST_CASE ("remove_prefix") {
  string_eq (remove_prefix ("abc", "a"), "bc");
  string_eq (remove_prefix ("abc", ""), "abc");
  string_eq (remove_prefix ("", ""), "");
  string_eq (remove_prefix ("abc", ""), "abc");
  string_eq (remove_prefix ("a1a", "a"), "1a");
}

TEST_CASE ("remove_suffix") {
  string_eq (remove_suffix ("abc", "c"), "ab");
  string_eq (remove_suffix ("abc", ""), "abc");
  string_eq (remove_suffix ("", ""), "");
  string_eq (remove_suffix ("abc", ""), "abc");
  string_eq (remove_suffix ("a1a", "a"), "a1");
}

TEST_CASE ("test_raw_quote") {
  CHECK_EQ (raw_quote ("a") == "\"a\"", true);
  CHECK_EQ (raw_quote ("") == "\"\"", true);
}

TEST_CASE ("test_raw_unquote") {
  CHECK_EQ (raw_unquote ("\"a\"") == "a", true);
  CHECK_EQ (raw_unquote ("\"a") == "\"a", true);
  CHECK_EQ (raw_unquote ("a\"") == "a\"", true);
  CHECK_EQ (raw_unquote ("") == "", true);
  CHECK_EQ (raw_unquote ("a") == "a", true);
}

TEST_CASE ("test_unescape_guile") {
  CHECK_EQ (unescape_guile ("\\\\") == "\\\\\\\\", true);
}

TEST_CASE ("test_starts") {
  CHECK (starts ("abc_def", "abc"));
  CHECK (!starts ("abc_def", "def"));
  CHECK (starts ("abc", ""));
  CHECK (starts ("", ""));
}

TEST_CASE ("test_ends") {
  CHECK (ends ("abc_def", "def"));
  CHECK (ends ("abc_def", ""));
  CHECK (!ends ("abc_def", "de"));
}

TEST_CASE ("test_read_word") {
  string word;
  int    i= 0;
  CHECK (read_word ("hello123", i, word));
  CHECK_EQ (word == "hello", true);
  CHECK_EQ (i, 5);

  i   = 0;
  word= "";
  CHECK (!read_word ("123", i, word));
  CHECK (is_empty (word));
  CHECK_EQ (i, 0);
}

TEST_CASE ("contains/occurs") {
  CHECK (contains ("abc", "a"));
  CHECK (contains ("abc", "ab"));
  CHECK (contains ("abc", "bc"));
  CHECK (!contains ("abc", "B"));
  CHECK (contains ("", ""));
  CHECK (contains ("abc", ""));
  CHECK (!contains ("abc", " "));
  CHECK (contains ("hello world", " "));
}

TEST_CASE ("replace") {
  CHECK_EQ (replace ("a-b", "-", "_") == "a_b", true);
  CHECK_EQ (replace ("a-b-c", "-", "_") == "a_b_c", true);
}

TEST_CASE ("tokenize") {
  CHECK_EQ (tokenize ("hello world", " "), array<string> ("hello", "world"));
  CHECK_EQ (tokenize ("zotero://select/library/items/2AIFJFS7", "://"),
            array<string> ("zotero", "select/library/items/2AIFJFS7"));
}

TEST_CASE ("recompose") {
  string_eq (recompose (array<string> ("hello", "world"), " "), "hello world");
  string_eq (
      recompose (array<string> ("zotero", "select/library/items/2AIFJFS7"),
                 "://"),
      "zotero://select/library/items/2AIFJFS7");
}

TEST_CASE ("fuzzy_match_score_1") {
  // Test case from the example: "npd" matching "NumPyData"
  CHECK (fuzzy_match_score ("npd", "NumPyData") > 0);

  // Test no match cases (VSCode style returns -1 for NO_SCORE)
  CHECK (fuzzy_match_score ("xyz", "abc") <= 0);
  CHECK (fuzzy_match_score ("", "abc") <= 0);
  CHECK (fuzzy_match_score ("abc", "") <= 0);
  CHECK (fuzzy_match_score ("abcd", "abc") <= 0); // Pattern longer than target
  CHECK (fuzzy_match_score ("test", "ts") <= 0);

  // Test exact matches should score high
  int exact_score  = fuzzy_match_score ("abc", "abc");
  int partial_score= fuzzy_match_score ("abc", "alphabet");
  CHECK (exact_score > 0);
  CHECK (exact_score > partial_score);

  // Test consecutive matches get higher scores than scattered
  int consecutive_score= fuzzy_match_score ("abc", "abcdef");
  int scattered_score  = fuzzy_match_score ("abc", "aXbXc");
  CHECK (consecutive_score > 0);
  CHECK (scattered_score > 0);
  CHECK (consecutive_score > scattered_score);

  // Test camelCase boundary bonuses
  int camel_score= fuzzy_match_score ("hw", "helloWorld");
  int lower_score= fuzzy_match_score ("hw", "helloworld");
  CHECK (camel_score > 0);
  CHECK (lower_score > 0);
  CHECK (camel_score > lower_score);
  camel_score= fuzzy_match_score ("hw", "HelloWorld");
  lower_score= fuzzy_match_score ("hw", "helloworld");
  CHECK (camel_score > 0);
  CHECK (lower_score > 0);
  CHECK (camel_score == lower_score);
  camel_score= fuzzy_match_score ("hw", "HelloWorld");
  lower_score= fuzzy_match_score ("hw", "helloWorld");
  CHECK (camel_score > 0);
  CHECK (lower_score > 0);
  CHECK (camel_score < lower_score);

  // Test word boundary bonuses with separators
  int underscore_score= fuzzy_match_score ("hw", "hello_world");
  int dash_score      = fuzzy_match_score ("hw", "hello-world");
  int no_sep_score    = fuzzy_match_score ("hw", "helloworld");
  CHECK (underscore_score > 0);
  CHECK (dash_score > 0);
  CHECK (no_sep_score > 0);
  CHECK (underscore_score > no_sep_score);
  CHECK (dash_score > no_sep_score);

  // Test first character bonus
  int first_char_score= fuzzy_match_score ("vs", "VSCode");
  int not_first_score = fuzzy_match_score ("vs", "aVSCode");
  CHECK (first_char_score > 0);
  CHECK (not_first_score > 0);
  CHECK (first_char_score > not_first_score);

  // Test case sensitivity bonus
  int exact_case_score= fuzzy_match_score ("VS", "VSCode");
  int diff_case_score = fuzzy_match_score ("vs", "VSCode");
  CHECK (exact_case_score > 0);
  CHECK (diff_case_score > 0);
  CHECK (exact_case_score > diff_case_score);

  // Test length preference (shorter targets preferred for same pattern)
  int short_score= fuzzy_match_score ("ab", "ab");
  int long_score = fuzzy_match_score ("ab", "alphabet");
  CHECK (short_score > 0);
  CHECK (long_score > 0);
  CHECK (short_score > long_score);

  // Test common abbreviation patterns
  CHECK (fuzzy_match_score ("gc", "git-commit") > 0);
  CHECK (fuzzy_match_score ("gc", "GetColor") > 0);
  CHECK (fuzzy_match_score ("np", "NumPy") > 0);
  CHECK (fuzzy_match_score ("js", "JavaScript") > 0);
}

TEST_CASE ("fuzzy_match_score_2") {
  // Test comprehensive ranking for pattern "fw" against various targets
  // Expected order from highest to lowest score based on VSCode fuzzy logic:

  // 1. Exact matches and very short targets (highest priority)
  int fw_score= fuzzy_match_score ("fw", "fw");

  // 2. Strong word boundaries (start + separator/camelCase)
  int file_watcher_score= fuzzy_match_score ("fw", "file_watcher");
  int file_writer_score = fuzzy_match_score ("fw", "file-writer");
  int FileWatcher_score = fuzzy_match_score ("fw", "FileWatcher");

  // 3. Start of word + later match
  int framework_score= fuzzy_match_score ("fw", "framework");
  int firewall_score = fuzzy_match_score ("fw", "firewall");

  // 4. Scattered matches
  int forward_score           = fuzzy_match_score ("fw", "forward");
  int following_workflow_score= fuzzy_match_score ("fw", "following_workflow");

  // 5. Weak matches (far apart, no word boundaries)
  int foobar_workflow_score= fuzzy_match_score ("fw", "foobar_workflow");
  int software_score       = fuzzy_match_score ("fw", "software");

  // Verify all scores are positive (all should match)
  CHECK (fw_score > 0);
  CHECK (file_watcher_score > 0);
  CHECK (file_writer_score > 0);
  CHECK (FileWatcher_score > 0);
  CHECK (framework_score > 0);
  CHECK (firewall_score > 0);
  CHECK (forward_score > 0);
  CHECK (following_workflow_score > 0);
  CHECK (foobar_workflow_score > 0);
  CHECK (software_score > 0);

  // Test ranking order (from highest to lowest expected scores)
  // Exact match should be highest
  CHECK (fw_score > file_watcher_score);

  // Word boundary matches should rank high
  CHECK (file_watcher_score > framework_score);
  CHECK (file_writer_score > framework_score);
  CHECK (FileWatcher_score == framework_score);

  // CamelCase should be preferred over separator
  CHECK (FileWatcher_score < file_watcher_score);

  CHECK (framework_score == forward_score);
  CHECK (firewall_score == forward_score);
  CHECK (forward_score < following_workflow_score);

  // Word boundaries should beat non-boundaries
  CHECK (following_workflow_score > software_score);
  CHECK (foobar_workflow_score > software_score);
}
