/******************************************************************************
 * MODULE     : font_selector_bridge_test.cpp
 * DESCRIPTION: 单元测试字体选择器 glue 入口 cpp_font_selector_dialog 的测试钩子
 *              （MOGAN_TEST_FONT_SELECTOR=cancel 时不弹窗返回空 tree）。
 *              =ok 钩子经 font-selector-commit（需完整 scheme boot）走 GUI 集成
 *              测试（TeXmacs/tests/2027.scm）覆盖；真实弹窗路径（Qt
 *exec）无法在 C++ 单测里跑。bridge 的 Q_INVOKABLE 方法（eval_scheme 转 facade）
 *              同样需 boot，见 2027.scm。
 *              详见 record/qml/font-selector.md Phase 2。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/QTMQmlDialog.hpp" // cpp_font_selector_dialog
#include "base.hpp"

#include "sys_utils.hpp" // set_env
#include "tree_helper.hpp"

#include <QtTest/QtTest>

// 测试钩子环境变量的 RAII 守卫：构造时设值，析构时还原为空（不命中弹窗路径）。
struct EnvHook {
  string key;
  EnvHook (string k, string v) : key (k) { set_env (k, v); }
  ~EnvHook () { set_env (key, ""); }
};

class TestFontSelectorBridge : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // =cancel 钩子：不弹窗，直接返回空 tree（调用方 tree->stree 后 cdr 得 ()，
  // for-each no-op，不写回）。
  void test_cancel_hook_returns_empty ();

  // 未命中钩子（非法/空值）时不在此断言——真实弹窗路径会 exec() 阻塞，无法在
  // C++ 单测跑；仅确认钩子判定逻辑对未知值不短路（此处无法调用，留 GUI 测试）。
};

void
TestFontSelectorBridge::test_cancel_hook_returns_empty () {
  EnvHook hook ("MOGAN_TEST_FONT_SELECTOR", "cancel");
  tree    r= cpp_font_selector_dialog (0);
  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 0);
}

#ifdef QTTEXMACS
QTEST_MAIN (TestFontSelectorBridge)
#else
int
main () {
  return 0;
}
#endif
#include "font_selector_bridge_test.moc"
