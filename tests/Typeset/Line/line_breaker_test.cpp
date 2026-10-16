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
#include "line_breaker.hpp"
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
  void display (renderer) override {}
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
  void test_atomic_line_breaks ();
  void test_forbidden_break_at_penalty ();
  void test_single_cjk_char_atomic ();
  void test_empty_or_single_item ();
  void test_overflow_pass2_breaks ();
  void test_large_scale_dense_breaks ();
  void test_stretchable_space_breaks ();
};

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

void
TestLineBreaker::test_overflow_pass2_breaks () {
  // 构造 5 个超宽图元，中间图元全部设为禁止换行（HYPH_INVALID）
  // 迫使 line_breaker 在第一阶段（pass 1）无法找到有效断点，进入第二阶段（pass
  // 2 溢出强行断行）
  array<line_item> a= make_std_items (5, 5000);
  for (int i= 0; i < 5; i++)
    a[i]->penalty= HYPH_INVALID;

  SI line_width = 3000;
  SI large_width= 4000;

  array<path> ap= line_breaks (a, 0, 5, line_width, large_width, 0, 0);

  // 在 pass 2 兜底下仍应生成首尾完整的断点序列
  QVERIFY (N (ap) >= 2);
  QCOMPARE (ap[0], path (0));
  QCOMPARE (ap[N (ap) - 1], path (5));
  verify_atomic_breaks (ap, 5);

  // 断点序列必须严格单调递增
  for (int k= 1; k < N (ap); k++) {
    QVERIFY (ap[k]->item > ap[k - 1]->item);
  }
}

void
TestLineBreaker::test_large_scale_dense_breaks () {
  // 测试 60 个图元密集断行，验证 atom_best（改为 lb_info_rep
  // 连续存储后）的大规模索引与追溯
  const int        n= 60;
  array<line_item> a= make_std_items (n, 500);

  SI line_width = 2500; // 每行大约 5 个图元
  SI large_width= 3500;

  array<path> ap= line_breaks (a, 0, n, line_width, large_width, 0, 0);

  QVERIFY (N (ap) >= 10);
  QCOMPARE (ap[0], path (0));
  QCOMPARE (ap[N (ap) - 1], path (n));
  verify_atomic_breaks (ap, n);

  // 验证断点严格单调递增，且覆盖整个范围
  for (int k= 1; k < N (ap); k++) {
    QVERIFY (ap[k]->item > ap[k - 1]->item);
  }
}

void
TestLineBreaker::test_stretchable_space_breaks () {
  // 测试带有伸缩空间的图元换行
  const int        n= 8;
  array<line_item> a (n);
  for (int i= 0; i < n; i++) {
    box b= empty_box (path (i), 0, 0, 1000, 1000);
    a[i] = line_item (STD_ITEM, OP_TEXT, b, 0);
    // 给空格附加弹性范围：[800, 1000, 1500]
    a[i]->spc= space (800, 1000, 1500);
  }

  // 行宽设为 2800，介于 2 个图元与 3 个图元弹性区间之间
  SI line_width = 2800;
  SI large_width= 3500;

  array<path> ap= line_breaks (a, 0, n, line_width, large_width, 0, 0);

  QVERIFY (N (ap) >= 2);
  QCOMPARE (ap[0], path (0));
  QCOMPARE (ap[N (ap) - 1], path (n));
  verify_atomic_breaks (ap, n);
}

QTEST_MAIN (TestLineBreaker)
#include "line_breaker_test.moc"
