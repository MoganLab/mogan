
/******************************************************************************
 * MODULE     : startup_bridge_test.cpp
 * DESCRIPTION: Unit tests for StartupBridge data logic
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "StartupBridge.hpp"
#include "base.hpp"
#include "template_manager.hpp"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestStartupBridge : public QObject {
  Q_OBJECT

private:
  QTemporaryDir tempHome_;

private slots:
  void initTestCase () {
    QStandardPaths::setTestModeEnabled (true);
    init_lolly ();

    // 将 TEXMACS_HOME_PATH 指向临时目录，隔离 recent-files.json 读写
    QVERIFY (tempHome_.isValid ());
    qputenv ("TEXMACS_HOME_PATH", tempHome_.path ().toUtf8 ());

    // 抑制 TemplateManager 初始化时因 Scheme 解释器未就绪产生的预期警告
    QtMessageHandler oldHandler= qInstallMessageHandler (
        [] (QtMsgType, const QMessageLogContext&, const QString&) {});
    TemplateManager::instance ()->setOffline (true);
    TemplateManager::instance ()->initialize ();
    qInstallMessageHandler (oldHandler);
  }

  void init () {
    // 每次测试前清理 recent-files.json
    QDir systemDir (tempHome_.path () + "/system");
    if (systemDir.exists ()) systemDir.removeRecursively ();
  }

  // ---- 构造与初始化 ----

  void test_construct_and_initialize () {
    StartupBridge bridge;
    QVERIFY (bridge.recentDocs ().isEmpty ());
    QVERIFY (bridge.styleCards ().isEmpty ());
    QVERIFY (bridge.categories ().isEmpty ());
    QVERIFY (bridge.activeCategoryId ().isEmpty ());
    QVERIFY (bridge.activeCategoryName ().isEmpty ());
    QVERIFY (bridge.categoryTemplates ().isEmpty ());
    QCOMPARE (bridge.categoryLoading (), false);
  }

  // ---- Recent docs 管理 ----

  void test_add_recent_doc () {
    StartupBridge bridge;
    bridge.addRecentDoc ("/tmp/test-doc-1.tmu");
    QCOMPARE (bridge.recentDocs ().size (), 1);

    auto m= bridge.recentDocs ().first ().toMap ();
    QCOMPARE (m["filePath"].toString (), QString ("/tmp/test-doc-1.tmu"));
    QCOMPARE (m["fileName"].toString (), QString ("test-doc-1.tmu"));
    QVERIFY (!m["openedAt"].toString ().isEmpty ());
    QVERIFY (m.contains ("timestamp"));
  }

  void test_add_recent_doc_dedup () {
    StartupBridge bridge;
    bridge.addRecentDoc ("/tmp/doc-a.tmu");
    bridge.addRecentDoc ("/tmp/doc-b.tmu");
    bridge.addRecentDoc ("/tmp/doc-a.tmu"); // 重复：应移到最前
    QCOMPARE (bridge.recentDocs ().size (), 2);
    QCOMPARE (bridge.recentDocs ().first ().toMap ()["filePath"].toString (),
              QString ("/tmp/doc-a.tmu"));
  }

  void test_remove_recent_doc () {
    StartupBridge bridge;
    bridge.addRecentDoc ("/tmp/doc-x.tmu");
    bridge.addRecentDoc ("/tmp/doc-y.tmu");
    bridge.removeRecentDoc ("/tmp/doc-x.tmu");
    QCOMPARE (bridge.recentDocs ().size (), 1);
    QCOMPARE (bridge.recentDocs ().first ().toMap ()["filePath"].toString (),
              QString ("/tmp/doc-y.tmu"));
  }

  void test_clear_all_recent_docs () {
    StartupBridge bridge;
    bridge.addRecentDoc ("/tmp/a.tmu");
    bridge.addRecentDoc ("/tmp/b.tmu");
    bridge.clearAllRecentDocs ();
    QVERIFY (bridge.recentDocs ().isEmpty ());
  }

  // ---- 最近文档 JSON 持久化 ----

  void test_recent_docs_persistence () {
    // 写入
    {
      StartupBridge bridge1;
      bridge1.addRecentDoc ("/tmp/persist-test.tmu");

      // 处理延迟执行的 Scheme 调用 (QTimer::singleShot(0, ...))
      QCoreApplication::processEvents ();
    }

    // 验证 JSON 文件内容
    QString jsonPath=
        QDir (tempHome_.path () + "/system").filePath ("recent-files.json");
    QVERIFY (QFile::exists (jsonPath));

    QFile file (jsonPath);
    QVERIFY (file.open (QIODevice::ReadOnly));
    QJsonDocument doc= QJsonDocument::fromJson (file.readAll ());
    file.close ();

    QVERIFY (doc.isObject ());
    QJsonArray files= doc.object ()["files"].toArray ();
    QCOMPARE (files.size (), 1);
    QCOMPARE (files[0].toObject ()["path"].toString (),
              QString ("/tmp/persist-test.tmu"));
    QVERIFY (files[0].toObject ()["last_open"].toDouble () > 0);
  }

  // ---- 分类操作 ----

  void test_select_category_idempotent () {
    StartupBridge bridge;

    QSignalSpy spy (&bridge, &StartupBridge::activeCategoryChanged);

    // 首次选择（未初始化 templateManager_，走安全路径：仅更新内部状态）
    bridge.selectCategory ("cat-a");
    QCOMPARE (bridge.activeCategoryId (), QString ("cat-a"));
    QCOMPARE (bridge.categoryLoading (), true);
    QCOMPARE (spy.count (), 1);

    // 重复选择同一分类不应 emit 信号
    bridge.selectCategory ("cat-a");
    QCOMPARE (spy.count (), 1);

    // 切换分类应再次 emit
    bridge.selectCategory ("cat-b");
    QCOMPARE (bridge.activeCategoryId (), QString ("cat-b"));
    QCOMPARE (spy.count (), 2);
  }

  // ---- 属性访问 ----

  void test_style_cards_initially_empty () {
    StartupBridge bridge;
    QVERIFY (bridge.styleCards ().isEmpty ());
  }

  void test_category_templates_initially_empty () {
    StartupBridge bridge;
    QVERIFY (bridge.categoryTemplates ().isEmpty ());
  }

  // ---- tr 工具方法 ----

  void test_tr_returns_non_empty_for_valid_key () {
    StartupBridge bridge;
    QString       result= bridge.tr ("File");
    QVERIFY (!result.isEmpty ());
  }
};

QTEST_MAIN (TestStartupBridge)
#include "startup_bridge_test.moc"
