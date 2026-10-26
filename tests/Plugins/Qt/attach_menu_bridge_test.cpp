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
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestAttachMenuBridge : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_bridge_meta_defaults ();
  void test_bridge_actions ();
  void test_choose_files_defers_menu_deletion ();
  void test_filter_recent ();
};

void
TestAttachMenuBridge::test_bridge_meta_defaults () {
  QDialog          host;
  AttachMenuBridge bridge (&host);
  QVariantMap      meta= bridge.meta ();

  QVERIFY (meta.contains ("label_recent"));
  QVERIFY (meta.contains ("label_open"));
  QVERIFY (meta.contains ("label_attached"));
  QVERIFY (meta.contains ("recent"));
  QVERIFY (meta.contains ("attached"));
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

void
TestAttachMenuBridge::test_filter_recent () {
  QTemporaryDir dir;
  QVERIFY (dir.isValid ());
  QList<QString> paths{dir.filePath ("a.txt"), dir.filePath ("b.tmu"),
                       dir.filePath (QString::fromUtf8 ("存在 文件.pdf"))};
  auto           makeItem= [] (const QString& url) {
    QVariantMap item;
    item["url"]  = url;
    item["title"]= url.section ('/', -1);
    return QVariant (item);
  };
  QVariantList recent;
  for (const QString& path : paths) {
    QFile file (path);
    QVERIFY (file.open (QIODevice::WriteOnly));
    file.write ("x");
    file.close ();
    recent << makeItem (path);
  }
  recent << makeItem (dir.filePath ("missing.txt"));

  // 已添加及失效条目被过滤，其余保留原顺序
  QSet<QString> attached;
  attached.insert (paths[1]);
  QVariantList out= AttachMenuBridge::filterRecent (recent, attached);
  QCOMPARE (out.size (), 2);
  QCOMPARE (out[0].toMap ()["url"].toString (), paths[0]);
  QCOMPARE (out[1].toMap ()["url"].toString (), paths[2]);

  // 空附件集合仍过滤失效条目；空 recent 得空列表
  QVariantList withoutAttached=
      AttachMenuBridge::filterRecent (recent, QSet<QString> ());
  QCOMPARE (withoutAttached.size (), 3);
  QCOMPARE (AttachMenuBridge::filterRecent (QVariantList (), attached).size (),
            0);
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
