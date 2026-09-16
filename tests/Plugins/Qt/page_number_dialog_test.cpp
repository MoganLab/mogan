/******************************************************************************
 * MODULE     : page_number_dialog_test.cpp
 * DESCRIPTION: 单元测试 PageNumberDialog glue 入口 cpp_page_number_dialog
 *              的测试钩子（MOGAN_TEST_PAGE_NUMBER=ok|cancel 时不弹窗）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/QTMQmlDialog.hpp" // cpp_page_number_dialog
#include "base.hpp"

#include "sys_utils.hpp" // set_env
#include "tree_helper.hpp"

#include <QtTest/QtTest>

class TestPageNumberDialog : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // =cancel 钩子：不弹窗，返回空 tree。
  void test_cancel_hook_returns_empty ();

  // =ok 钩子：不弹窗，返回 (tuple "ok")。
  void test_ok_hook_returns_ok_marker ();
};

void
TestPageNumberDialog::test_cancel_hook_returns_empty () {
  EnvHook hook ("MOGAN_TEST_PAGE_NUMBER", "cancel");
  tree    r= cpp_page_number_dialog ();
  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 0);
}

void
TestPageNumberDialog::test_ok_hook_returns_ok_marker () {
  EnvHook hook ("MOGAN_TEST_PAGE_NUMBER", "ok");
  tree    r= cpp_page_number_dialog ();
  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 1);
  QVERIFY (is_atomic (r[0]));
}

#ifdef QTTEXMACS
QTEST_MAIN (TestPageNumberDialog)
#else
int
main () {
  return 0;
}
#endif
#include "page_number_dialog_test.moc"
