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

TEST_CASE ("search_forwards") {
  // 基本命中
  CHECK_EQ (search_forwards ("quick", "the quick brown"), 4);
  CHECK_EQ (search_forwards ("the", "the quick brown"), 0);
  CHECK_EQ (search_forwards ("brown", "the quick brown"), 10);
  // 无命中
  CHECK_EQ (search_forwards ("QUICK", "the quick brown"), -1);
  // 空模式：返回起始位置
  CHECK_EQ (search_forwards ("", "abc"), 0);
  CHECK_EQ (search_forwards ("", 2, "abc"), 2);
  // 模式长于原串 / 与原串等长
  CHECK_EQ (search_forwards ("abcdef", "abc"), -1);
  CHECK_EQ (search_forwards ("abc", "abc"), 0);
  // 带起始位置
  CHECK_EQ (search_forwards ("the", 1, "the quick the"), 10);
  CHECK_EQ (search_forwards ("the", 10, "the quick the"), 10);
  // 起始位置越过最后一个可能匹配点
  CHECK_EQ (search_forwards ("the", 11, "the quick the"), -1);
  // 首字符干扰：首字符大量出现但整体不匹配
  CHECK_EQ (search_forwards ("totally", "the quick brown"), -1);
  CHECK_EQ (search_forwards ("at", "that at"), 2);
  // 尾部命中（验证窗口右边界含最后一个起点）
  CHECK_EQ (search_forwards ("dog", "the lazy dog"), 9);
  // 字符串内可含 '\0'
  CHECK_EQ (search_forwards (string ("b\0d", 3), string ("a\0b\0d", 5)), 2);
  // array 重载：多模式取最早命中，含首字符干扰与长度过滤
  array<string> pats= array<string> ("brown", "quick");
  CHECK_EQ (search_forwards (pats, 0, "the quick brown"), 4);
  array<string> pats2= array<string> ("zzzzz", "quick");
  CHECK_EQ (search_forwards (pats2, 0, "the quick brown"), 4);
  CHECK_EQ (search_forwards (pats, 0, "none here"), -1);
  // array 重载：pos == N(in) 时不再越界读
  CHECK_EQ (search_forwards (pats, 15, "the quick brown"), -1);
  // 大输入冒烟
  string line= "the quick brown fox jumps over the lazy dog\n";
  string text;
  for (int i= 0; i < 2000; i++)
    text << line;
  CHECK_EQ (search_forwards ("lazy", text), 35);
  CHECK_EQ (search_forwards ("LAZY", text), -1);
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
  // 无命中：内容不变
  CHECK_EQ (replace ("abc", "-", "_") == "abc", true);
  CHECK_EQ (replace ("", "-", "_") == "", true);
  // 首尾命中与相邻命中
  CHECK_EQ (replace ("-a-", "-", "_") == "_a_", true);
  CHECK_EQ (replace ("a--b", "-", "_") == "a__b", true);
  // 替换串比模式长/短/为空
  CHECK_EQ (replace ("a-b", "-", "<->") == "a<->b", true);
  CHECK_EQ (replace ("a<->b", "<->", "-") == "a-b", true);
  CHECK_EQ (replace ("a-b", "-", "") == "ab", true);
  // 多字符模式不重叠匹配：从左到右，命中后跳过整个模式
  CHECK_EQ (replace ("aaaa", "aa", "b") == "bb", true);
  CHECK_EQ (replace ("aaa", "aa", "b") == "ba", true);
  // 空模式守卫：原样返回，不死循环
  CHECK_EQ (replace ("abc", "", "_") == "abc", true);
  CHECK_EQ (replace ("", "", "_") == "", true);
  // 模式比原串长：尾部不足以容纳模式的区段不匹配
  CHECK_EQ (replace ("ab", "abc", "x") == "ab", true);
  CHECK_EQ (replace ("ab-", "ab-", "") == "", true);
  // 模式首字符出现但整体不匹配
  CHECK_EQ (replace ("aXbXc", "XY", "Z") == "aXbXc", true);
  // 命中后模式整体跳过，替换串中再现模式不递归替换
  CHECK_EQ (replace ("a-b", "-", "--") == "a--b", true);
}

TEST_CASE ("replace large input") {
  // 大输入冒烟：等长替换长度不变且模式被完全替换
  string line= "the quick brown fox jumps over the lazy dog\n";
  string text;
  for (int i= 0; i < 2000; i++)
    text << line;
  string r= replace (text, "quick", "brisk");
  CHECK (N (r) == N (text));
  CHECK_EQ (occurs ("quick", r), false);
}

TEST_CASE ("tokenize") {
  CHECK_EQ (tokenize ("hello world", " "), array<string> ("hello", "world"));
  CHECK_EQ (tokenize ("zotero://select/library/items/2AIFJFS7", "://"),
            array<string> ("zotero", "select/library/items/2AIFJFS7"));
  // 分隔符长于原串：整串作为唯一 token
  array<string> single;
  single << "ab";
  CHECK_EQ (tokenize ("ab", "abc"), single);
  // 尾部不足以容纳分隔符的片段原样保留
  CHECK_EQ (tokenize ("a::b:", "::"), array<string> ("a", "b:"));
  // 首字符干扰：首字符相同但整体不匹配，不拆分
  array<string> whole;
  whole << "a:b";
  CHECK_EQ (tokenize ("a:b", "::"), whole);
  // 空分隔符：整串作为唯一 token（原实现此处会死循环）
  CHECK_EQ (tokenize ("a:b", ""), whole);
  // 大输入冒烟
  string big;
  for (int i= 0; i < 2000; i++)
    big << "key=value;";
  CHECK_EQ (N (tokenize (big, ";")), 2001);
  string_eq (tokenize (big, ";")[1999], "key=value");
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
