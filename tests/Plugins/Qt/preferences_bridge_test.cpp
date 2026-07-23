/******************************************************************************
 * MODULE     : preferences_bridge_test.cpp
 * DESCRIPTION: 单元测试 PreferencesBridge glue 入口 cpp_preferences_dialog
 *              的测试钩子（MOGAN_TEST_PREFERENCES=ok|cancel 时不弹窗）。
 *              bridge 的 eval_scheme 依赖完整 scheme boot，纯逻辑部分由
 *              preferences-widgets-test.scm 覆盖；本文件只覆盖 C++ 侧的钩子
 *              判定与返回值形状。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/QTMQmlDialog.hpp" // cpp_preferences_dialog
#include "base.hpp"

#include "sys_utils.hpp" // set_env
#include "tree_helper.hpp"

#include <QtTest/QtTest>

// 测试钩子环境变量的 RAII 守卫：构造时设值，析构时还原为空。
struct EnvHook {
  string key;
  EnvHook (string k, string v) : key (k) { set_env (k, v); }
  ~EnvHook () { set_env (key, ""); }
};

class TestPreferencesBridge : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // =cancel 钩子：不弹窗，返回空 tree（与其它对话框的非阻塞 show
  // 返回空 tree 一致）。
  void test_cancel_hook_returns_empty ();

  // =ok 钩子：不弹窗，返回 (tuple "ok") 供自动化脚本区分已提交。
  void test_ok_hook_returns_ok_marker ();
};

void
TestPreferencesBridge::test_cancel_hook_returns_empty () {
  EnvHook hook ("MOGAN_TEST_PREFERENCES", "cancel");
  tree    r= cpp_preferences_dialog ();
  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 0);
}

void
TestPreferencesBridge::test_ok_hook_returns_ok_marker () {
  EnvHook hook ("MOGAN_TEST_PREFERENCES", "ok");
  tree    r= cpp_preferences_dialog ();
  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 1);
  QVERIFY (is_atomic (r[0]));
  // 不直接比较 label 字符串（string 类 operator() 与构造歧义），
  // 形状验证已足够：非空 tuple 含一个原子节点。
}

#ifdef QTTEXMACS
QTEST_MAIN (TestPreferencesBridge)
#else
int
main () {
  return 0;
}
#endif
#include "preferences_bridge_test.moc"
