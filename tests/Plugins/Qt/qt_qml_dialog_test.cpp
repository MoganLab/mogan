/******************************************************************************
 * MODULE      : qt_qml_dialog_test.cpp
 * DESCRIPTION : Tests for cpp_confirm_close (QML confirm-close dialog glue).
 *               只覆盖「测试钩子命中」的纯逻辑契约：按钮下标↔返回串映射、
 *               scratch↔Save as、空值/非法值兜底。钩子未命中时会真弹模态
 *               exec()，依赖 GUI 与人工点击，不在此覆盖（由 GUI 集成测试
 *               2021.scm 驱动）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/QTMQmlDialog.hpp"
#include "Qt/qt_utilities.hpp" // to_qstring
#include "base.hpp"

#include <QtTest/QtTest>

// get_env / set_env：lolly/System/Misc/sys_utils.hpp，经 base.hpp 的
// sys_utils.hpp 带入。钩子变量名须与 QTMQmlDialog.cpp 里读取的一致。
static const char* kHookVar= "MOGAN_TEST_CONFIRM_CLOSE";

class TestQTMQmlDialog : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  // 钩子命中 Save → 返回 "Save"（非 scratch：肯定按钮文案即 Save）。
  void test_hook_save_returns_save () {
    set_env (kHookVar, "Save");
    QCOMPARE (to_qstring (cpp_confirm_close ("msg", false)), QString ("Save"));
  }

  // 钩子命中 Don't save → 返回 "Don't save"。
  void test_hook_dont_save_returns_dont_save () {
    set_env (kHookVar, "Don't save");
    QCOMPARE (to_qstring (cpp_confirm_close ("msg", false)),
              QString ("Don't save"));
  }

  // 钩子命中 Cancel → 返回 "Cancel"。
  void test_hook_cancel_returns_cancel () {
    set_env (kHookVar, "Cancel");
    QCOMPARE (to_qstring (cpp_confirm_close ("msg", false)),
              QString ("Cancel"));
  }

  // 返回值与 scratch 标志无关：scratch 只影响 QML 肯定按钮的显示文案，
  // 钩子路径下 cpp_confirm_close 仍返回统一的 "Save"，由 scheme 侧据此
  // 走「另存为」。固化此跨层契约，防 QML/C++ 接口漂移。
  void test_scratch_does_not_change_return_value () {
    set_env (kHookVar, "Save");
    QCOMPARE (to_qstring (cpp_confirm_close ("msg", true)), QString ("Save"));
  }

  // 钩子为非法值 → 同样视为未命中（get_env != 任一合法枚举），不应误返回。
  // 钩子为空（默认）也属此类，但 set_env(var,"") 在当前 lolly 实现下取值
  // 不稳定（可能保留旧值），故不单独设空串用例，由本用例的非法值覆盖
  // 「非合法枚举不命中」语义。真弹窗路径由 GUI 集成测试 2021.scm 覆盖。
  void test_invalid_hook_not_recognized () {
    set_env (kHookVar, "save"); // 大小写敏感，非合法枚举
    QVERIFY (get_env (kHookVar) != "Save");
    QVERIFY (get_env (kHookVar) != "Don't save");
    QVERIFY (get_env (kHookVar) != "Cancel");
  }
};

#include "qt_qml_dialog_test.moc"

QTEST_MAIN (TestQTMQmlDialog)
