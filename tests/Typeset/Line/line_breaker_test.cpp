/******************************************************************************
 * MODULE     : line_breaker_test.cpp
 * DESCRIPTION: Unit tests for paragraph line breaker and atomic paths
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Boxes/construct.hpp"
#include "Format/line_item.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "font.hpp"
#include "line_breaker.hpp"
#include "smart_font.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>

class MockCjkBoxRep : public box_rep {
public:
  string str;
  MockCjkBoxRep (path ip, string s, SI w) : box_rep (ip), str (s) {
    x1= 0;
    y1= 0;
    x2= w;
    y2= w;
  }
  string get_leaf_string () override { return str; }
         operator tree () override { return tree ("mock"); }
  void   display (renderer) override {}
};

box
mock_cjk_box (path ip, string s, SI w) {
  return tm_new<MockCjkBoxRep> (ip, s, w);
}

static array<line_item>
make_std_items (int n, SI w) {
  array<line_item> a (n);
  for (int i= 0; i < n; i++)
    a[i]= line_item (STD_ITEM, OP_TEXT, empty_box (path (i), 0, 0, w, w), 0);
  return a;
}

static void
verify_atomic_breaks (array<path> ap, int end) {
  for (int k= 0; k < N (ap); k++) {
    QVERIFY (is_atom (ap[k]));
    QVERIFY (ap[k]->item >= 0 && ap[k]->item <= end);
  }
}

class TestLineBreaker : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
  }

  void test_hyphenation_keeps_lines_within_width ();
  void test_hyphenation_long_paragraph ();
  void test_hyphenation_german_compounds ();
  void test_atomic_line_breaks ();
  void test_forbidden_break_at_penalty ();
  void test_single_cjk_char_atomic ();
  void test_empty_or_single_item ();
};

/**
 * @brief 回归 [c87f08744]：断字候选位置曾共用同一份
 * lb_info，使最优断点链被串乱，
 *        段落整体不折行（单行超宽，只在某个断字点才换行）。
 * @note 用真实字体和断字模式排一个段落并扫描一批行宽。每个结果须满足：
 *       断点单调；行数不少于总宽度所需；任何一行（不含两端断字残余）都不得明显
 *       超过 large_width（容差 25% 用于吸收窄行时允许的轻微超宽）。
 */
static void
check_paragraph_wraps (const char* lang, const char** words, int n) {
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);
  font fn= smart_font ("roman", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn));

  language         lan= text_language (lang);
  array<line_item> a (n);
  SI               total= 0;
  for (int i= 0; i < n; i++) {
    box b    = text_box (path (i), 0, string (words[i]), fn, pencil (black));
    a[i]     = line_item (STRING_ITEM, OP_TEXT, b, i == n - 1 ? 0 : 1, lan);
    a[i]->spc= i == n - 1 ? space (0) : fn->spc;
    total+= b->w () + a[i]->spc->def;
  }

  for (int k= 150; k <= 450; k+= 3) {
    SI          line_width = total * 100 / k;
    SI          large_width= (SI) (line_width / 0.9);
    SI          limit      = large_width + large_width / 4;
    array<path> ap= line_breaks (a, 0, n, line_width, large_width, 0, 0);
    QCOMPARE (ap[0], path (0));
    QCOMPARE (ap[N (ap) - 1], path (n));
    QVERIFY2 (
        N (ap) - 1 >= (int) (total / limit),
        qPrintable (QString ("k=%1: only %2 lines").arg (k).arg (N (ap) - 1)));
    for (int j= 0; j + 1 < N (ap); j++) {
      QVERIFY (ap[j]->item <= ap[j + 1]->item);
      // 起点为断字残余或终点为断字时少算一个词，结果偏保守
      int from= is_atom (ap[j]) ? ap[j]->item : ap[j]->item + 1;
      int to  = ap[j + 1]->item;
      SI  w   = 0;
      for (int i= from; i < to; i++)
        w+= a[i]->b->w () + (i + 1 < to ? a[i]->spc->def : 0);
      QVERIFY2 (w <= limit, qPrintable (QString ("k=%1 line %2 width %3 > %4")
                                            .arg (k)
                                            .arg (j)
                                            .arg (w)
                                            .arg (limit)));
    }
  }
}

// 最小复现：去掉 "alonggggg " 或 "boundarie " 任一词，旧代码就恢复正常
void
TestLineBreaker::test_hyphenation_keeps_lines_within_width () {
  const char* words[]= {"where",     "are",       "Schwartz",   "functions",
                        "on",        "the",       "real",       "line.",
                        "This",      "operator",  "arises",     "in",
                        "diverse",   "contexts:", "averages",   "of",
                        "over",      "yield",     "relevant",   "correction",
                        "terms",     "for",       "evaluating", "Cauchy",
                        "integrals", "alonggggg", "boundarie",  "boundaries"};
  check_paragraph_wraps ("english", words, sizeof (words) / sizeof (words[0]));
}

// 较长段落：断字点更多，错误的最优链在更多行宽下出现
void
TestLineBreaker::test_hyphenation_long_paragraph () {
  const char* words[]= {
      "evaluating",   "Cauchy",       "integrals",    "along",
      "boundaries",   "of",           "Lipschitz",    "domains",
      "and",          "for",          "inverting",    "elliptic",
      "operators",    "with",         "non-constant", "coefficients;",
      "fiberwise",    "applications", "of",           "bounds",
      "on",           "can",          "be",           "used",
      "to",           "bound",        "multilinear",  "singular",
      "integral",     "opertors",     "with",         "full",
      "symmetry.",    "We",           "elaborate",    "and",
      "provide",      "references",   "for",          "these",
      "applications", "further",      "on.",          "One",
      "is",           "interested",   "in",           "showing",
      "the",          "bounds",       "with",         "a",
      "constant",     "that",         "does",         "not",
      "depend",       "on",           "the",          "functions."};
  check_paragraph_wraps ("english", words, sizeof (words) / sizeof (words[0]));
}

// 德语复合词：几乎每个词都有多个断字候选，另一套断字模式
void
TestLineBreaker::test_hyphenation_german_compounds () {
  const char* words[]= {"Die",
                        "Donaudampfschifffahrtsgesellschaft",
                        "verwendet",
                        "Rindfleischetikettierungsueberwachungsaufgaben",
                        "und",
                        "Haftpflichtversicherungsbedingungen",
                        "fuer",
                        "alle",
                        "Geschwindigkeitsbegrenzungen",
                        "in",
                        "der",
                        "Bundesrepublik",
                        "Deutschland",
                        "sowie",
                        "Arbeitsunfaehigkeitsbescheinigungen",
                        "der",
                        "Krankenversicherung"};
  check_paragraph_wraps ("german", words, sizeof (words) / sizeof (words[0]));
}

void
TestLineBreaker::test_atomic_line_breaks () {
  // 构造 10 个宽度为 1000 tmpt 的图元，换行罚分为 0（允许换行）
  array<line_item> a= make_std_items (10, 1000);

  // 行宽 3000 tmpt（每行可容纳约 3 个图元）
  SI line_width = 3000;
  SI large_width= 4500;

  array<path> ap= line_breaks (a, 0, 10, line_width, large_width, 0, 0);

  // 结果应包含起始点、中间换行点、终止点
  QVERIFY (N (ap) >= 2);
  QCOMPARE (ap[0], path (0));
  QCOMPARE (ap[N (ap) - 1], path (10));

  // 验证所有返回的断点均为深度为 1 的原子断点
  verify_atomic_breaks (ap, 10);
}

void
TestLineBreaker::test_forbidden_break_at_penalty () {
  // 构造 6 个图元，在第 2 个图元后设置 HYPH_INVALID 禁止断行
  array<line_item> a= make_std_items (6, 1000);
  a[2]->penalty     = HYPH_INVALID;

  SI line_width = 2500;
  SI large_width= 3500;

  array<path> ap= line_breaks (a, 0, 6, line_width, large_width, 0, 0);

  QVERIFY (N (ap) >= 2);
  // 断点不应切在禁断位置 path(3)（即第 2 个图元之后）
  for (int k= 1; k < N (ap) - 1; k++) {
    QVERIFY (is_atom (ap[k]));
    QVERIFY (ap[k] != path (3));
  }
}

void
TestLineBreaker::test_single_cjk_char_atomic () {
  // 模拟 CJK 单字图元（如 <#4E00>, <#4E01> 等单个汉字）
  array<line_item> a (8);
  string           cjk_chars[]= {"<#4E00>", "<#4E01>", "<#4E02>", "<#4E03>",
                                 "<#4E04>", "<#4E05>", "<#4E06>", "<#4E07>"};
  for (int i= 0; i < 8; i++) {
    box b= mock_cjk_box (path (i), cjk_chars[i], 1000);
    a[i] = line_item (STRING_ITEM, OP_TEXT, b, 0);
  }

  SI line_width = 3000;
  SI large_width= 4500;

  array<path> ap= line_breaks (a, 0, 8, line_width, large_width, 0, 0);

  QVERIFY (N (ap) >= 2);
  // 验证所有汉字断点均为整字边界的原子断点，绝不产生复合路径
  verify_atomic_breaks (ap, 8);
}

void
TestLineBreaker::test_empty_or_single_item () {
  // 单图元边界测试
  array<line_item> a= make_std_items (1, 1000);

  array<path> ap= line_breaks (a, 0, 1, 2000, 3000, 0, 0);

  QCOMPARE (N (ap), 2);
  QCOMPARE (ap[0], path (0));
  QCOMPARE (ap[1], path (1));
}

QTEST_MAIN (TestLineBreaker)
#include "line_breaker_test.moc"
