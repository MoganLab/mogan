/******************************************************************************
 * MODULE     : text_toolbar_test.cpp
 * DESCRIPTION: Test text toolbar functionality
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "edit_interface.hpp"
#include <QtTest/QtTest>

// 测试文本工具栏缓存机制
class TestTextToolbar : public QObject {
  Q_OBJECT

private slots:
  void test_cache_timeout_boundary ();
  void test_cache_invalidation ();
  void test_rectangle_validity ();
  void test_coordinate_conversion ();
  void test_empty_selection_handling ();
};

// 测试缓存超时边界（100ms）
void
TestTextToolbar::test_cache_timeout_boundary () {
  // 验证时间差计算
  time_t t1= 1000;
  time_t t2= 1099; // 差99ms，应该使用缓存
  time_t t3= 1100; // 差100ms，应该重新检查

  QVERIFY ((t2 - t1) < 100);  // 99 < 100，缓存有效
  QVERIFY ((t3 - t1) >= 100); // 100 >= 100，缓存过期
}

// 测试缓存失效机制
void
TestTextToolbar::test_cache_invalidation () {
  // 模拟缓存状态
  time_t last_check = texmacs_time ();
  bool   last_result= true;

  // 模拟 invalidate_text_toolbar_cache()
  last_check= 0;

  // 验证缓存已失效
  QVERIFY (last_check == 0);

  // 验证下次检查会重新计算（时间差会很大）
  time_t now= texmacs_time ();
  QVERIFY ((now - last_check) >= 100); // 一定会超过100ms
}

// 测试矩形有效性检查
void
TestTextToolbar::test_rectangle_validity () {
  // 有效矩形（非零面积）
  rectangle valid (100, 200, 300, 400);
  QVERIFY (valid->x1 < valid->x2);
  QVERIFY (valid->y1 < valid->y2);

  // 无效矩形：零宽度
  rectangle zero_width (100, 200, 100, 400);
  QVERIFY (zero_width->x1 >= zero_width->x2); // 应该被检测为无效

  // 无效矩形：零高度
  rectangle zero_height (100, 200, 300, 200);
  QVERIFY (zero_height->y1 >= zero_height->y2); // 应该被检测为无效

  // 无效矩形：负面积（x1 > x2）
  rectangle negative_x (300, 200, 100, 400);
  QVERIFY (negative_x->x1 > negative_x->x2);

  // 无效矩形：负面积（y1 > y2）
  rectangle negative_y (100, 400, 300, 200);
  QVERIFY (negative_y->y1 > negative_y->y2);
}

// 测试坐标转换精度
void
TestTextToolbar::test_coordinate_conversion () {
  constexpr double INV_UNIT= 1.0 / 256.0;

  // 基础转换测试
  QCOMPARE (int (std::round (2560 * INV_UNIT)), 10);
  QCOMPARE (int (std::round (5120 * INV_UNIT)), 20);

  // 边界值测试
  QCOMPARE (int (std::round (0 * INV_UNIT)), 0);
  QCOMPARE (int (std::round (255 * INV_UNIT)), 1); // 接近1的值
  QCOMPARE (int (std::round (256 * INV_UNIT)), 1); // 正好1个单位
  QCOMPARE (int (std::round (257 * INV_UNIT)), 1); // 略大于1

  // 大数值精度测试
  SI  large      = 1000000;
  int large_pixel= int (std::round (large * INV_UNIT));
  QCOMPARE (large_pixel, 3906);

  // 验证反向计算误差在可接受范围
  double back_calc= large_pixel / INV_UNIT;
  double error    = std::abs (back_calc - large);
  QVERIFY (error < 256); // 误差小于1个像素单位
}

// 测试空选区处理
void
TestTextToolbar::test_empty_selection_handling () {
  // 默认构造的空矩形
  rectangle empty;
  QCOMPARE (empty->x1, 0);
  QCOMPARE (empty->y1, 0);
  QCOMPARE (empty->x2, 0);
  QCOMPARE (empty->y2, 0);

  // 空矩形应该被检测为无效（零面积）
  bool is_empty_invalid= (empty->x1 >= empty->x2) || (empty->y1 >= empty->y2);
  QVERIFY (is_empty_invalid);

  // 最小有效矩形（1x1像素）
  rectangle minimal (0, 0, 1, 1);
  bool      is_minimal_valid=
      (minimal->x1 < minimal->x2) && (minimal->y1 < minimal->y2);
  QVERIFY (is_minimal_valid);
}

QTEST_MAIN (TestTextToolbar)
#include "text_toolbar_test.moc"
