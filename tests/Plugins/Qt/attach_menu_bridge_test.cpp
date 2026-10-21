/******************************************************************************
 * MODULE     : attach_menu_bridge_test.cpp
 * DESCRIPTION: AttachMenuBridge 单元测试
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/AttachMenuBridge.hpp"
#include "base.hpp"

#include <QDialog>
#include <QtTest/QtTest>

class TestAttachMenuBridge : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_bridge_meta_defaults ();
  void test_bridge_actions ();
  void test_choose_files_defers_menu_deletion ();
};

void
TestAttachMenuBridge::test_bridge_meta_defaults () {
  QDialog          host;
  AttachMenuBridge bridge (&host);
  QVariantMap      meta= bridge.meta ();

  QVERIFY (meta.contains ("label_recent"));
  QVERIFY (meta.contains ("label_open"));
  QVERIFY (meta.contains ("recent"));
}

void
TestAttachMenuBridge::test_bridge_actions () {
  QDialog          host;
  AttachMenuBridge bridge (&host);

  // 验证各 invokable 方法能安全调用且不崩溃
  bridge.loadBuffer ("");
  bridge.closeMenu ();

  qputenv ("MOGAN_TEST_CHOOSE_FILES", "cancel");
  bridge.openFile ();
  qunsetenv ("MOGAN_TEST_CHOOSE_FILES");
}

void
TestAttachMenuBridge::test_choose_files_defers_menu_deletion () {
  auto* host= new QWidget ();
  host->setAttribute (Qt::WA_DeleteOnClose);
  host->show ();
  QPointer<QWidget> guard (host);
  auto*             bridge= new AttachMenuBridge (host);

  qputenv ("MOGAN_TEST_CHOOSE_FILES", "event-loop-cancel");
  bridge->openFile ();
  qunsetenv ("MOGAN_TEST_CHOOSE_FILES");

  // 对话框的嵌套事件循环返回后，QML 回调退出前菜单仍须存活。
  QVERIFY (!guard.isNull ());
  QCoreApplication::sendPostedEvents (nullptr, QEvent::DeferredDelete);
  QVERIFY (guard.isNull ());
}

#ifdef QTTEXMACS
QTEST_MAIN (TestAttachMenuBridge)
#else
int
main () {
  return 0;
}
#endif

#include "attach_menu_bridge_test.moc"
