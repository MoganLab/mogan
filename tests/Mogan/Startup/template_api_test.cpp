
/******************************************************************************
 * MODULE     : template_api_test.cpp
 * DESCRIPTION: Full regression tests for TemplateAPI
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "base.hpp"
#include <QtTest/QtTest>

#include "template_api.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTimer>

// 简易 HTTP Mock Server：收到完整 HTTP 请求后返回固定响应
// 支持两种模式：
// 1. 普通模式：收到请求后立即返回完整 response
// 2. 分块模式：先发送 header，然后通过 QTimer 分块发送 body，
//    模拟真实网络传输，确保 downloadProgress 信号被触发
class MiniHttpServer : public QTcpServer {
public:
  explicit MiniHttpServer (const QByteArray& response,
                           QObject*          parent= nullptr);
  explicit MiniHttpServer (const QByteArray& header, const QByteArray& body,
                           int chunkSize, QObject* parent= nullptr);

  QString    url () const;
  QByteArray lastRequest () const;

private:
  void scheduleNextChunk (QTcpSocket* socket);

  QByteArray response_;
  QByteArray lastRequest_;
  QByteArray header_;
  QByteArray body_;
  int        chunkSize_= 0;
  int        bodySent_ = 0;
};

class TestTemplateAPI : public QObject {
  Q_OBJECT

private:
  QTcpServer hangServer_;

private slots:
  void initTestCase () {
    QStandardPaths::setTestModeEnabled (true);
    init_lolly ();
    QVERIFY (hangServer_.listen (QHostAddress::LocalHost, 0));
  }

  void cleanup () { hangServer_.close (); }

  // --- 基础配置与状态 ---

  void test_api_base_url () {
    TemplateAPI api;
    QCOMPARE (api.apiBaseUrl (), QString ("https://liiistem.cn/template-api"));
    api.setApiBaseUrl ("http://example.com/api");
    QCOMPARE (api.apiBaseUrl (), QString ("http://example.com/api"));
  }

  void test_network_state_changed_signal () {
    TemplateAPI api;
    QSignalSpy  spy (&api, &TemplateAPI::networkStateChanged);
    QVERIFY (spy.isValid ());

    api.setOfflineMode (true);
    QCOMPARE (spy.count (), 1);
    QCOMPARE (spy.takeFirst ()[0].toBool (), false);

    api.setOfflineMode (false);
    QCOMPARE (spy.count (), 1);
    QCOMPARE (spy.takeFirst ()[0].toBool (), true);
  }

  void test_is_online () {
    TemplateAPI api;
    QVERIFY (api.isOnline ());
    api.setOfflineMode (true);
    QVERIFY (!api.isOnline ());
    api.setOfflineMode (false);
    QVERIFY (api.isOnline ());
  }

  // --- 离线模式阻断 ---

  void test_offline_mode_blocks_download () {
    TemplateAPI api;
    api.setOfflineMode (true);
    QSignalSpy spy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (spy.isValid ());

    api.downloadTemplate ("id", "http://example.com/file", "/tmp/test.tmu");
    QCoreApplication::processEvents ();

    QCOMPARE (spy.count (), 1);
    QList<QVariant> args= spy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("id"));
    QVERIFY (args[1].toString ().contains ("Offline"));
  }

  void test_offline_mode_blocks_metadata () {
    TemplateAPI api;
    api.setOfflineMode (true);
    QSignalSpy spy (&api, &TemplateAPI::metadataLoadFailed);
    QVERIFY (spy.isValid ());

    api.fetchMetadata ();
    QCoreApplication::processEvents ();

    QCOMPARE (spy.count (), 1);
    QVERIFY (spy.takeFirst ()[0].toString ().contains ("Offline"));
  }

  // --- 下载成功与进度 ---

  void test_download_success () {
    QByteArray body= "Hello Template!";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;

    QSignalSpy completedSpy (&api, &TemplateAPI::downloadCompleted);
    QSignalSpy failedSpy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (completedSpy.isValid ());
    QVERIFY (failedSpy.isValid ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.filePath ("test.tmu");

    api.downloadTemplate ("test-tmpl", server.url () + "/file", targetPath);
    QVERIFY (completedSpy.wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (completedSpy.count (), 1);
    QList<QVariant> args= completedSpy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("test-tmpl"));
    QCOMPARE (args[1].toString (), targetPath);

    QFile file (targetPath);
    QVERIFY (file.open (QIODevice::ReadOnly));
    QCOMPARE (file.readAll (), body);
  }

  void test_download_progress () {
    QByteArray body (65536, 'X');
    QByteArray header= QByteArray ("HTTP/1.1 200 OK\r\n") +
                       "Content-Length: " + QByteArray::number (body.size ()) +
                       "\r\n" + "\r\n";

    MiniHttpServer server (header, body, 4096);
    TemplateAPI    api;

    QSignalSpy progressSpy (&api, &TemplateAPI::downloadProgress);
    QSignalSpy completedSpy (&api, &TemplateAPI::downloadCompleted);
    QVERIFY (progressSpy.isValid ());
    QVERIFY (completedSpy.isValid ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.filePath ("prog.tmu");

    api.downloadTemplate ("prog-tmpl", server.url () + "/file", targetPath);
    QVERIFY (completedSpy.wait (5000));

    QCOMPARE (completedSpy.count (), 1);
    QVERIFY (progressSpy.count () >= 1);
    QList<QVariant> args= progressSpy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("prog-tmpl"));
    QVERIFY (args[1].toLongLong () >= 0);
    QCOMPARE (args[2].toLongLong (), qint64 (65536));
  }

  // --- 下载失败场景 ---

  void test_download_network_error () {
    // 空响应：服务器收到请求后直接断开，触发网络错误
    MiniHttpServer server ("");
    TemplateAPI    api;

    QSignalSpy spy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (spy.isValid ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.filePath ("err.tmu");

    api.downloadTemplate ("err-tmpl", server.url () + "/file", targetPath);
    QVERIFY (spy.wait (1000));

    QCOMPARE (spy.count (), 1);
    QList<QVariant> args= spy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("err-tmpl"));
    QVERIFY (args[1].toString ().contains ("failed", Qt::CaseInsensitive));
  }

  void test_download_cannot_write_file () {
    QByteArray body= "data";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;

    QSignalSpy spy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (spy.isValid ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.path ();

    api.downloadTemplate ("write-err", server.url () + "/file", targetPath);
    QVERIFY (spy.wait (1000));

    QCOMPARE (spy.count (), 1);
    QVERIFY (spy.takeFirst ()[1].toString ().contains ("Cannot save",
                                                       Qt::CaseInsensitive));
  }

  // 测试 HTTP 404 响应：服务器返回 404 时应触发 downloadFailed 信号
  void test_download_http_error_404 () {
    QByteArray response= "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    MiniHttpServer server (response);
    TemplateAPI    api;

    QSignalSpy spy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (spy.isValid ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.filePath ("404.tmu");

    api.downloadTemplate ("not-found", server.url () + "/file", targetPath);
    QVERIFY (spy.wait (1000));

    QCOMPARE (spy.count (), 1);
    QList<QVariant> args= spy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("not-found"));
    QVERIFY (args[1].toString ().contains ("failed", Qt::CaseInsensitive));
  }

  // 测试并发下载多个不同 templateId：验证 downloadReplies_ 哈希表隔离互不干扰
  void test_download_concurrent_templates () {
    QByteArray bodyA= "TemplateA";
    QByteArray responseA=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (bodyA.size ()) + "\r\n" +
        "\r\n" + bodyA;
    QByteArray bodyB= "TemplateB";
    QByteArray responseB=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (bodyB.size ()) + "\r\n" +
        "\r\n" + bodyB;

    MiniHttpServer serverA (responseA);
    MiniHttpServer serverB (responseB);
    TemplateAPI    api;

    QSignalSpy completedSpy (&api, &TemplateAPI::downloadCompleted);
    QSignalSpy failedSpy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (completedSpy.isValid ());
    QVERIFY (failedSpy.isValid ());

    QTemporaryDir tempDir;
    QString       pathA= tempDir.filePath ("a.tmu");
    QString       pathB= tempDir.filePath ("b.tmu");

    api.downloadTemplate ("tmpl-a", serverA.url () + "/file", pathA);
    api.downloadTemplate ("tmpl-b", serverB.url () + "/file", pathB);

    for (int i= 0; i < 20 && completedSpy.count () < 2; ++i) {
      completedSpy.wait (100);
      QCoreApplication::processEvents ();
    }

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (completedSpy.count (), 2);

    QSet<QString> ids;
    QSet<QString> paths;
    for (int i= 0; i < 2; ++i) {
      QList<QVariant> args= completedSpy.takeFirst ();
      ids.insert (args[0].toString ());
      paths.insert (args[1].toString ());
    }
    QVERIFY (ids.contains ("tmpl-a"));
    QVERIFY (ids.contains ("tmpl-b"));
    QVERIFY (paths.contains (pathA));
    QVERIFY (paths.contains (pathB));

    QFile fileA (pathA);
    QVERIFY (fileA.open (QIODevice::ReadOnly));
    QCOMPARE (fileA.readAll (), bodyA);

    QFile fileB (pathB);
    QVERIFY (fileB.open (QIODevice::ReadOnly));
    QCOMPARE (fileB.readAll (), bodyB);
  }

  // 测试下载到已存在的文件路径：验证 QFile 会覆盖旧内容而非失败
  void test_download_overwrite_existing_file () {
    QByteArray body= "new content";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;

    QSignalSpy completedSpy (&api, &TemplateAPI::downloadCompleted);
    QSignalSpy failedSpy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (completedSpy.isValid ());
    QVERIFY (failedSpy.isValid ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.filePath ("existing.tmu");

    QFile existing (targetPath);
    QVERIFY (existing.open (QIODevice::WriteOnly));
    existing.write ("old content");
    existing.close ();

    api.downloadTemplate ("overwrite", server.url () + "/file", targetPath);
    QVERIFY (completedSpy.wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (completedSpy.count (), 1);

    QFile result (targetPath);
    QVERIFY (result.open (QIODevice::ReadOnly));
    QCOMPARE (result.readAll (), body);
  }

  // --- 取消逻辑（1009 修复核心）---

  void test_cancel_download_emits_failed () {
    TemplateAPI api;
    QSignalSpy  spy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (spy.isValid ());

    QString url=
        QString ("http://127.0.0.1:%1/hang").arg (hangServer_.serverPort ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.filePath ("cancel.tmu");

    api.downloadTemplate ("test-tmpl", url, targetPath);
    api.cancelDownload ("test-tmpl");
    QCoreApplication::processEvents ();

    QCOMPARE (spy.count (), 1);
    QList<QVariant> args= spy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("test-tmpl"));
    QVERIFY (args[1].toString ().contains ("cancelled", Qt::CaseInsensitive));
  }

  void test_download_template_reuse_aborts_old_without_signal () {
    TemplateAPI api;
    QSignalSpy  failedSpy (&api, &TemplateAPI::downloadFailed);
    QSignalSpy  completedSpy (&api, &TemplateAPI::downloadCompleted);
    QVERIFY (failedSpy.isValid ());
    QVERIFY (completedSpy.isValid ());

    QString hangUrl=
        QString ("http://127.0.0.1:%1/hang").arg (hangServer_.serverPort ());

    QTemporaryDir tempDir;
    QString       targetPath= tempDir.filePath ("reuse.tmu");

    // 启动一个会 hang 的旧下载
    api.downloadTemplate ("test-tmpl", hangUrl, targetPath);
    QCoreApplication::processEvents ();

    // 再次下载同一个 templateId，内部 abort 旧请求，新请求应成功
    QByteArray body= "Template Reuse Success";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;
    MiniHttpServer server (response);
    QString        newPath= tempDir.filePath ("reuse2.tmu");

    api.downloadTemplate ("test-tmpl", server.url () + "/file", newPath);
    QVERIFY (completedSpy.wait (1000));

    // 旧请求不应触发 downloadFailed，新请求应成功完成
    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (completedSpy.count (), 1);
    QList<QVariant> args= completedSpy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("test-tmpl"));
    QCOMPARE (args[1].toString (), newPath);

    QFile file (newPath);
    QVERIFY (file.open (QIODevice::ReadOnly));
    QCOMPARE (file.readAll (), body);
  }

  // 测试取消不存在的 templateId：验证不会崩溃也不会误发射 downloadFailed 信号
  void test_cancel_nonexistent_download () {
    TemplateAPI api;
    QSignalSpy  spy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (spy.isValid ());

    api.cancelDownload ("nonexistent-id");
    QCoreApplication::processEvents ();

    QCOMPARE (spy.count (), 0);
  }

  // --- Metadata 获取 ---

  void test_fetch_metadata_success () {
    QJsonObject stats;
    stats["downloads"]= 42;
    stats["rating"]   = 4.5;

    QJsonObject compat;
    compat["mogan_min_version"]= "1.0";

    QJsonArray tags;
    tags.append ("math");
    tags.append ("physics");

    QJsonObject tmplObj;
    tmplObj["id"]           = "tmpl1";
    tmplObj["name"]         = "Template 1";
    tmplObj["description"]  = "A template";
    tmplObj["category"]     = "cat1";
    tmplObj["author"]       = "Author";
    tmplObj["version"]      = "1.0";
    tmplObj["license"]      = "MIT";
    tmplObj["thumbnail_url"]= "";
    tmplObj["preview_url"]  = "";
    tmplObj["download_url"] = "http://example.com/file";
    tmplObj["file_size"]    = 100;
    tmplObj["file_md5"]     = "abc123";
    tmplObj["created_at"]   = "2024-01-01T00:00:00Z";
    tmplObj["updated_at"]   = "2024-06-01T00:00:00Z";
    tmplObj["language"]     = "zh-CN";
    tmplObj["tags"]         = tags;
    tmplObj["compatibility"]= compat;
    tmplObj["statistics"]   = stats;

    QJsonArray templates;
    templates.append (tmplObj);

    QJsonObject category;
    category["id"]         = "cat1";
    category["name"]       = "Category 1";
    category["description"]= "Desc";
    category["icon"]       = "icon";
    category["order"]      = 1;
    category["templates"]  = templates;

    QJsonArray categories;
    categories.append (category);

    QJsonObject root;
    root["categories"]= categories;

    QByteArray body= QJsonDocument (root).toJson (QJsonDocument::Compact);

    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "ETag: \"test-etag-123\"\r\n" + "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QHash<QString, TemplateMetadataPtr> receivedMetadata;
    QList<TemplateCategory>             receivedCategories;
    connect (&api, &TemplateAPI::metadataLoaded,
             [&] (const QHash<QString, TemplateMetadataPtr>& m,
                  const QList<TemplateCategory>&             c) {
               receivedMetadata  = m;
               receivedCategories= c;
             });

    QSignalSpy failedSpy (&api, &TemplateAPI::metadataLoadFailed);
    QVERIFY (failedSpy.isValid ());

    api.fetchMetadata ();
    QVERIFY (QSignalSpy (&api, &TemplateAPI::metadataLoaded).wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (receivedCategories.size (), 1);
    QCOMPARE (receivedCategories[0].id, QString ("cat1"));
    QCOMPARE (receivedCategories[0].name, QString ("Category 1"));
    QCOMPARE (receivedCategories[0].order, 1);

    QVERIFY (receivedMetadata.contains ("tmpl1"));
    auto tmpl= receivedMetadata["tmpl1"];
    QVERIFY (tmpl);
    QCOMPARE (tmpl->id, QString ("tmpl1"));
    QCOMPARE (tmpl->name, QString ("Template 1"));
    QCOMPARE (tmpl->author, QString ("Author"));
    QCOMPARE (tmpl->fileSize, qint64 (100));
    QCOMPARE (tmpl->downloadCount, 42);
    QCOMPARE (tmpl->rating, 4.5);
    QCOMPARE (tmpl->tags, QStringList ({"math", "physics"}));
    QCOMPARE (tmpl->moganMinVersion, QString ("1.0"));

    QCOMPARE (api.lastMetadataEtag (), QString ("\"test-etag-123\""));
  }

  void test_fetch_metadata_304_not_modified () {
    QByteArray     response= "HTTP/1.1 304 Not Modified\r\n\r\n";
    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QSignalSpy spy (&api, &TemplateAPI::metadataNotModified);
    QVERIFY (spy.isValid ());

    api.fetchMetadata ();
    QVERIFY (spy.wait (1000));
    QCOMPARE (spy.count (), 1);
  }

  void test_fetch_metadata_network_error () {
    MiniHttpServer server ("");
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QSignalSpy spy (&api, &TemplateAPI::metadataLoadFailed);
    QVERIFY (spy.isValid ());

    api.fetchMetadata ();
    QVERIFY (spy.wait (1000));
    QCOMPARE (spy.count (), 1);
  }

  void test_fetch_metadata_invalid_json () {
    QByteArray body= "not json";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QSignalSpy spy (&api, &TemplateAPI::metadataLoadFailed);
    QVERIFY (spy.isValid ());

    // 抑制 parseMetadataResponse 中预期内的 qWarning
    QtMessageHandler oldHandler= qInstallMessageHandler (
        [] (QtMsgType, const QMessageLogContext&, const QString&) {});

    api.fetchMetadata ();
    QVERIFY (spy.wait (1000));

    qInstallMessageHandler (oldHandler);

    QCOMPARE (spy.count (), 1);
    QVERIFY (spy.takeFirst ()[0].toString ().contains ("Invalid"));
  }

  void test_fetch_metadata_conditional_request () {
    QByteArray body= R"({"categories":[],"templates":[]})";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "ETag: \"etag-456\"\r\n" + "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());
    api.setMetadataEtag ("\"prev-etag\"");

    QSignalSpy spy (&api, &TemplateAPI::metadataLoaded);
    QVERIFY (spy.isValid ());

    api.fetchMetadata ();
    QVERIFY (spy.wait (1000));

    QCOMPARE (spy.count (), 1);
    QCOMPARE (api.lastMetadataEtag (), QString ("\"etag-456\""));
    QVERIFY (server.lastRequest ().contains ("if-none-match"));
  }

  // 测试 API 返回空的 categories 和
  // templates：验证正常解析为空的元数据和分类列表
  void test_fetch_metadata_empty_result () {
    QByteArray body= R"({"categories":[],"templates":[]})";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "ETag: \"empty-etag\"\r\n" + "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QHash<QString, TemplateMetadataPtr> receivedMetadata;
    QList<TemplateCategory>             receivedCategories;
    connect (&api, &TemplateAPI::metadataLoaded,
             [&] (const QHash<QString, TemplateMetadataPtr>& m,
                  const QList<TemplateCategory>&             c) {
               receivedMetadata  = m;
               receivedCategories= c;
             });

    QSignalSpy failedSpy (&api, &TemplateAPI::metadataLoadFailed);
    QVERIFY (failedSpy.isValid ());

    api.fetchMetadata ();
    QVERIFY (QSignalSpy (&api, &TemplateAPI::metadataLoaded).wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (receivedCategories.size (), 0);
    QCOMPARE (receivedMetadata.size (), 0);
    QCOMPARE (api.lastMetadataEtag (), QString ("\"empty-etag\""));
  }

  // 测试模板字段缺失或 id 为空：验证空 id 模板被忽略且解析过程不会崩溃
  void test_fetch_metadata_missing_required_fields () {
    QJsonObject tmplObj;
    tmplObj["id"]          = "";
    tmplObj["name"]        = "";
    tmplObj["download_url"]= "";

    QJsonArray templates;
    templates.append (tmplObj);

    QJsonObject category;
    category["id"]       = "cat1";
    category["name"]     = "";
    category["templates"]= templates;

    QJsonArray categories;
    categories.append (category);

    QJsonObject root;
    root["categories"]= categories;

    QByteArray body= QJsonDocument (root).toJson (QJsonDocument::Compact);

    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QHash<QString, TemplateMetadataPtr> receivedMetadata;
    QList<TemplateCategory>             receivedCategories;
    connect (&api, &TemplateAPI::metadataLoaded,
             [&] (const QHash<QString, TemplateMetadataPtr>& m,
                  const QList<TemplateCategory>&             c) {
               receivedMetadata  = m;
               receivedCategories= c;
             });

    QSignalSpy failedSpy (&api, &TemplateAPI::metadataLoadFailed);
    QVERIFY (failedSpy.isValid ());

    api.fetchMetadata ();
    QVERIFY (QSignalSpy (&api, &TemplateAPI::metadataLoaded).wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (receivedCategories.size (), 1);
    QCOMPARE (receivedCategories[0].id, QString ("cat1"));

    QVERIFY (!receivedMetadata.contains (""));
    QCOMPARE (receivedMetadata.size (), 0);
  }

  // --- 生命周期安全 ---

  void test_destructor_with_active_download () {
    QString url=
        QString ("http://127.0.0.1:%1/hang").arg (hangServer_.serverPort ());

    {
      TemplateAPI   api;
      QTemporaryDir tempDir;
      QString       targetPath= tempDir.filePath ("destructor.tmu");
      api.downloadTemplate ("test-tmpl", url, targetPath);
      // api 离开作用域，destructor 被调用
    }
    QVERIFY (true);
  }

  void test_destructor_with_active_metadata_fetch () {
    QString url=
        QString ("http://127.0.0.1:%1/hang").arg (hangServer_.serverPort ());

    {
      TemplateAPI api;
      api.setApiBaseUrl (url);
      api.fetchMetadata ();
      // api 离开作用域，destructor 被调用
    }
    QVERIFY (true);
  }
};

// MiniHttpServer 方法定义（放在 TestTemplateAPI 之后，避免 moc 被嵌套 lambda
// 中的大括号干扰）
MiniHttpServer::MiniHttpServer (const QByteArray& response, QObject* parent)
    : QTcpServer (parent), response_ (response), chunkSize_ (0), bodySent_ (0) {
  connect (this, &QTcpServer::newConnection, this, [this] () {
    QTcpSocket* socket         = nextPendingConnection ();
    auto        handleReadyRead= [this, socket] () {
      lastRequest_.append (socket->readAll ());
      if (lastRequest_.contains ("\r\n\r\n")) {
        if (!response_.isEmpty ()) {
          socket->write (response_);
          socket->flush ();
        }
        socket->close ();
      }
    };
    connect (socket, &QTcpSocket::readyRead, handleReadyRead);
    if (socket->bytesAvailable () > 0) handleReadyRead ();
  });
  QVERIFY (listen (QHostAddress::LocalHost, 0));
}

MiniHttpServer::MiniHttpServer (const QByteArray& header,
                                const QByteArray& body, int chunkSize,
                                QObject* parent)
    : QTcpServer (parent), header_ (header), body_ (body),
      chunkSize_ (chunkSize), bodySent_ (0) {
  connect (this, &QTcpServer::newConnection, this, [this] () {
    QTcpSocket* socket         = nextPendingConnection ();
    auto        handleReadyRead= [this, socket] () {
      lastRequest_.append (socket->readAll ());
      if (lastRequest_.contains ("\r\n\r\n")) {
        socket->write (header_);
        socket->flush ();
        bodySent_= 0;
        scheduleNextChunk (socket);
      }
    };
    connect (socket, &QTcpSocket::readyRead, handleReadyRead);
    if (socket->bytesAvailable () > 0) handleReadyRead ();
  });
  QVERIFY (listen (QHostAddress::LocalHost, 0));
}

void
MiniHttpServer::scheduleNextChunk (QTcpSocket* socket) {
  if (bodySent_ >= body_.size ()) {
    socket->close ();
    return;
  }
  int len= qMin (chunkSize_, body_.size () - bodySent_);
  socket->write (body_.mid (bodySent_, len));
  socket->flush ();
  bodySent_+= len;
  QTimer::singleShot (0, this, [this, socket] () {
    if (socket->state () == QAbstractSocket::ConnectedState) {
      scheduleNextChunk (socket);
    }
  });
}

QString
MiniHttpServer::url () const {
  return QString ("http://127.0.0.1:%1").arg (serverPort ());
}

QByteArray
MiniHttpServer::lastRequest () const {
  return lastRequest_;
}

QTEST_MAIN (TestTemplateAPI)
#include "template_api_test.moc"
