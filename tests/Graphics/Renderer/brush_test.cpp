/******************************************************************************
 * MODULE     : brush_test.cpp
 * DESCRIPTION: 回归测试 make_brush 对 PATTERN 树的参数个数校验。
 *              [201_72] 的防御性修复曾要求 N(p)==4，导致背景图片选择器
 *              生成的 3 参数 pattern（pattern image w h）被静默降级为
 *              no_brush，文档背景图片不显示（[2090]）。
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "brush.hpp"

#include <QtTest/QtTest>
#include <moebius/tree_label.hpp>

using namespace moebius;

class TestBrush : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_atomic_color ();
  void test_pattern_three_args ();
  void test_pattern_four_args ();
  void test_malformed_patterns ();
};

void
TestBrush::test_atomic_color () {
  QCOMPARE (brush (tree ("red"))->get_type (), brush_color);
  QCOMPARE (brush (tree ("none"))->get_type (), brush_none);
  QCOMPARE (brush (tree (""))->get_type (), brush_none);
}

void
TestBrush::test_pattern_three_args () {
  tree p (PATTERN, "paper.png", "100%", "100%");
  QCOMPARE (brush (p)->get_type (), brush_pattern);
}

void
TestBrush::test_pattern_four_args () {
  tree p (PATTERN, "paper.png", "100%", "100%", "white");
  QCOMPARE (brush (p)->get_type (), brush_pattern);
}

void
TestBrush::test_malformed_patterns () {
  // 缺宽度/高度参数：绘制时会读取 p[1]/p[2]，必须回退 no_brush
  QCOMPARE (brush (tree (PATTERN))->get_type (), brush_none);
  QCOMPARE (brush (tree (PATTERN, "paper.png"))->get_type (), brush_none);
  QCOMPARE (brush (tree (PATTERN, "paper.png", "100%"))->get_type (),
            brush_none);
  // 资源标识为空或为占位符
  QCOMPARE (brush (tree (PATTERN, "", "100%", "100%"))->get_type (),
            brush_none);
  QCOMPARE (brush (tree (PATTERN, "{}", "100%", "100%"))->get_type (),
            brush_none);
  // 非 PATTERN 复合树
  QCOMPARE (brush (tree (DOCUMENT, "a"))->get_type (), brush_none);
}

#ifdef QTTEXMACS
QTEST_MAIN (TestBrush)
#else
int
main () {
  return 0;
}
#endif
#include "brush_test.moc"
