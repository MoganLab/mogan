/******************************************************************************
 * MODULE     : chinese_language_test.cpp
 * DESCRIPTION: Unit tests for Chinese language advance and typeset properties
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "impl_language.hpp"
#include "language.hpp"
#include <QtTest/QtTest>
#include <lolly/data/herk.hpp>

class TestChineseLanguage : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void test_cjk_unified_ideographs_fast_path ();
  void test_cjk_punctuation_do_not_start ();
  void test_cjk_punctuation_do_not_end ();
  void test_ascii_word_and_space ();
  void test_boundary_and_out_of_range ();
};

void
TestChineseLanguage::test_cjk_unified_ideographs_fast_path () {
  language lan= text_language ("chinese");

  // 测试常用汉字十六进制编码（4E00..9FFF 范围）：
  // <#4E00> (一), <#4E2D> (中), <#6587> (文), <#5B57> (字), <#9FA5>
  // 以及 4000..EFFF 范围内的首字符：4..9, A..E, a..e
  string s= "<#4E00><#4E2D><#6587><#5B57><#9FA5><#4000><#A000><#EFFF>";
  tree   t (s);
  int    pos= 0;

  while (pos < N (s)) {
    int           old_pos= pos;
    text_property prop   = lan->advance (t, pos);

    // 每一个都必须匹配 tp_cjk_normal_rep (类型为 TP_CJK_NORMAL)
    QCOMPARE (prop, &tp_cjk_normal_rep);
    QCOMPARE (prop->type, TP_CJK_NORMAL);
    // 每次 advance 应该前进一个完整字符
    QVERIFY (pos > old_pos);
  }
  QCOMPARE (pos, N (s));
}

void
TestChineseLanguage::test_cjk_punctuation_do_not_start () {
  language lan= text_language ("chinese");

  // 行首禁则标点（不能出现在行首，对应 tp_cjk_period_rep）：
  // 句号、逗号、顿号、叹号、问号、右双引号、右单引号、破折号、居中点等
  array<string> puncts;
  puncts << lolly::data::utf8_to_herk ("。") << lolly::data::utf8_to_herk ("，")
         << lolly::data::utf8_to_herk ("、") << lolly::data::utf8_to_herk ("！")
         << lolly::data::utf8_to_herk ("？") << lolly::data::utf8_to_herk ("”")
         << lolly::data::utf8_to_herk ("’") << lolly::data::utf8_to_herk ("）")
         << lolly::data::utf8_to_herk ("》") << string ("<#2014>")
         << string ("<centerdot>");

  for (int i= 0; i < N (puncts); i++) {
    string s= puncts[i];
    tree   t (s);
    int    pos= 0;

    text_property prop= lan->advance (t, pos);
    QCOMPARE (prop, &tp_cjk_period_rep);
    QCOMPARE (prop->type, TP_CJK_PERIOD);
    QCOMPARE (pos, N (s));
  }
}

void
TestChineseLanguage::test_cjk_punctuation_do_not_end () {
  language lan= text_language ("chinese");

  // 行尾禁则标点（不能出现在行尾，对应 tp_cjk_no_break_rep）：
  // 左双引号、左单引号、左书名号、左括号等
  array<string> puncts;
  puncts << lolly::data::utf8_to_herk ("“") << lolly::data::utf8_to_herk ("‘")
         << lolly::data::utf8_to_herk ("《") << lolly::data::utf8_to_herk ("（")
         << lolly::data::utf8_to_herk ("【")
         << lolly::data::utf8_to_herk ("「");

  for (int i= 0; i < N (puncts); i++) {
    string s= puncts[i];
    tree   t (s);
    int    pos= 0;

    text_property prop= lan->advance (t, pos);
    QCOMPARE (prop, &tp_cjk_no_break_rep);
    QCOMPARE (prop->type, TP_CJK_NO_BREAK);
    QCOMPARE (pos, N (s));
  }
}

void
TestChineseLanguage::test_ascii_word_and_space () {
  language lan= text_language ("chinese");

  string s= "hello world";
  tree   t (s);
  int    pos= 0;

  // "hello" 应作为一个整体前进，返回 tp_cjk_no_break_rep
  text_property prop1= lan->advance (t, pos);
  QCOMPARE (prop1, &tp_cjk_no_break_rep);
  QCOMPARE (pos, 5);

  // 空格应前进 1，返回 tp_space_rep
  text_property prop2= lan->advance (t, pos);
  QCOMPARE (prop2, &tp_space_rep);
  QCOMPARE (pos, 6);

  // "world" 前进到末尾
  text_property prop3= lan->advance (t, pos);
  QCOMPARE (prop3, &tp_cjk_no_break_rep);
  QCOMPARE (pos, 11);
}

void
TestChineseLanguage::test_boundary_and_out_of_range () {
  language lan= text_language ("chinese");

  // 1. pos 超出字符串长度
  string s= "test";
  tree   t (s);
  int    pos = 10;
  auto   prop= lan->advance (t, pos);
  QCOMPARE (prop, &tp_normal_rep);

  // 2. <# 开头但不足 3 个字符的情况
  string s_short= "<#";
  tree   t_short (s_short);
  pos = 0;
  prop= lan->advance (t_short, pos);
  // 不能走 fast path，应走正常解析
  QVERIFY (pos > 0);

  // 3. 不在 U+4000..U+EFFF 范围内的字符，如全角空格 <#3000>
  string s_space= "<#3000>";
  tree   t_space (s_space);
  pos = 0;
  prop= lan->advance (t_space, pos);
  // <#3000> 开头为 '3'，不走 fast path，但既不在 do_not_start 也不在 do_not_end
  QCOMPARE (prop, &tp_cjk_normal_rep);
  QCOMPARE (pos, N (s_space));
}

QTEST_MAIN (TestChineseLanguage)
#include "chinese_language_test.moc"
