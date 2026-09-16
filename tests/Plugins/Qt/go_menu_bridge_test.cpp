/******************************************************************************
 * MODULE     : go_menu_bridge_test.cpp
 * DESCRIPTION: GoMenuBridge 单元测试
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/GoMenuBridge.hpp"
#include "base.hpp"

#include <QDialog>
#include <QtTest/QtTest>

class TestGoMenuBridge : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_bridge_meta_defaults ();
  void test_bridge_actions ();
};

void
TestGoMenuBridge::test_bridge_meta_defaults () {
  QDialog      host;
  GoMenuBridge bridge (&host);
  QVariantMap  meta= bridge.meta ();

  QVERIFY (meta.contains ("can_back"));
  QVERIFY (meta.contains ("can_forward"));
  QVERIFY (meta.contains ("label_back"));
  QVERIFY (meta.contains ("label_forward"));
  QVERIFY (meta.contains ("label_save"));
  QVERIFY (meta.contains ("label_buffers"));
  QVERIFY (meta.contains ("label_recent"));
  QVERIFY (meta.contains ("buffers"));
  QVERIFY (meta.contains ("recent"));
}

void
TestGoMenuBridge::test_bridge_actions () {
  QDialog      host;
  GoMenuBridge bridge (&host);

  // 验证各 invokable 方法能安全调用且不崩溃
  bridge.goBack ();
  bridge.goForward ();
  bridge.savePosition ();
  bridge.switchToBuffer ("");
  bridge.loadBuffer ("");
  bridge.closeMenu ();
}

#ifdef QTTEXMACS
QTEST_MAIN (TestGoMenuBridge)
#else
int
main () {
  return 0;
}
#endif

#include "go_menu_bridge_test.moc"
