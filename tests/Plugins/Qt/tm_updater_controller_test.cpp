/******************************************************************************
 * MODULE     : tm_updater_controller_test.cpp
 * DESCRIPTION: TmUpdaterController 的确定性单测：idle 默认值 + 未可用/未就绪时
 *              download/apply 返回 #f。不触发网络（不调 checkNow），不依赖
 *              worker 线程。
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/TmUpdaterController.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestTmUpdaterController : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  // 单例非空。
  void test_singleton_non_null () {
    QVERIFY (get_updater_controller () != nullptr);
  }

  // idle 默认值：state=0 / progress=0 / version 空 / errorCode 空。
  void test_idle_defaults () {
    TmUpdaterController* c= get_updater_controller ();
    QCOMPARE (c->state (), 0);
    QCOMPARE (c->progress (), 0);
    QVERIFY (c->version ().isEmpty ());
    QVERIFY (c->releaseNotes ().isEmpty ());
    QVERIFY (c->errorCode ().isEmpty ());
  }

  // 未 available / 未 ready 时 download/apply 均为 false（tm_updater 基础
  // 实现与 tm_velopack 在非 AVAILABLE/READY 状态下都返回 false，全平台一致）。
  void test_download_apply_idle_false () {
    TmUpdaterController* c= get_updater_controller ();
    QCOMPARE (c->download (), false);
    QCOMPARE (c->apply (), false);
  }
};

QTEST_MAIN (TestTmUpdaterController)
#include "tm_updater_controller_test.moc"
