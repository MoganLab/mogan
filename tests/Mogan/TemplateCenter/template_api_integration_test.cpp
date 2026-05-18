
/******************************************************************************
 * MODULE     : template_api_integration_test.cpp
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
    QCOMPARE (api.apiBaseUrl (), QString ("https://liiistem.cn"));
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

  void test_offline_mode_blocks_categories () {
    TemplateAPI api;
    api.setOfflineMode (true);
    QSignalSpy spy (&api, &TemplateAPI::categoriesLoadFailed);
    QVERIFY (spy.isValid ());

    api.fetchCategories ();
    QCoreApplication::processEvents ();

    QCOMPARE (spy.count (), 1);
    QVERIFY (spy.takeFirst ()[0].toString ().contains ("Offline"));
  }

  void test_offline_mode_blocks_templates () {
    TemplateAPI api;
    api.setOfflineMode (true);
    QSignalSpy spy (&api, &TemplateAPI::templatesLoadFailed);
    QVERIFY (spy.isValid ());

    api.fetchTemplates ("cat1");
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

    api.downloadTemplate ("test-tmpl", hangUrl, targetPath);
    QCoreApplication::processEvents ();

    QByteArray body= "Template Reuse Success";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;
    MiniHttpServer server (response);
    QString        newPath= tempDir.filePath ("reuse2.tmu");

    api.downloadTemplate ("test-tmpl", server.url () + "/file", newPath);
    QVERIFY (completedSpy.wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (completedSpy.count (), 1);
    QList<QVariant> args= completedSpy.takeFirst ();
    QCOMPARE (args[0].toString (), QString ("test-tmpl"));
    QCOMPARE (args[1].toString (), newPath);

    QFile file (newPath);
    QVERIFY (file.open (QIODevice::ReadOnly));
    QCOMPARE (file.readAll (), body);
  }

  void test_cancel_nonexistent_download () {
    TemplateAPI api;
    QSignalSpy  spy (&api, &TemplateAPI::downloadFailed);
    QVERIFY (spy.isValid ());

    api.cancelDownload ("nonexistent-id");
    QCoreApplication::processEvents ();

    QCOMPARE (spy.count (), 0);
  }

  // --- 分类获取 ---

  void test_fetch_categories_success () {
    QJsonArray  categories;
    QJsonObject cat1;
    cat1["categoryKey"]  = "thesis";
    cat1["name"]         = u8"论文";
    cat1["nameEn"]       = "Thesis";
    cat1["description"]  = u8"学位论文模板";
    cat1["order"]        = 1;
    cat1["templateCount"]= 15;
    categories.append (cat1);

    QJsonObject cat2;
    cat2["categoryKey"]  = "report";
    cat2["name"]         = u8"报告";
    cat2["nameEn"]       = "Report";
    cat2["description"]  = u8"实验报告模板";
    cat2["order"]        = 2;
    cat2["templateCount"]= 10;
    categories.append (cat2);

    QJsonObject root;
    root["code"]   = 0;
    root["success"]= true;
    root["message"]= "ok";
    root["data"]   = categories;

    QByteArray body= QJsonDocument (root).toJson (QJsonDocument::Compact);

    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QList<TemplateCategory> receivedCategories;
    connect (&api, &TemplateAPI::categoriesLoaded,
             [&] (const QList<TemplateCategory>& c) { receivedCategories= c; });

    QSignalSpy failedSpy (&api, &TemplateAPI::categoriesLoadFailed);
    QVERIFY (failedSpy.isValid ());

    api.fetchCategories ();
    QVERIFY (QSignalSpy (&api, &TemplateAPI::categoriesLoaded).wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (receivedCategories.size (), 2);
    QCOMPARE (receivedCategories[0].id, QString ("thesis"));
    QCOMPARE (receivedCategories[0].nameEn, QString ("Thesis"));
    QCOMPARE (receivedCategories[0].order, 1);
    QCOMPARE (receivedCategories[1].id, QString ("report"));
    QCOMPARE (receivedCategories[1].order, 2);
  }

  void test_fetch_categories_network_error () {
    MiniHttpServer server ("");
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QSignalSpy spy (&api, &TemplateAPI::categoriesLoadFailed);
    QVERIFY (spy.isValid ());

    api.fetchCategories ();
    QVERIFY (spy.wait (1000));
    QCOMPARE (spy.count (), 1);
  }

  void test_fetch_categories_invalid_json () {
    QByteArray body= "not json";
    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QSignalSpy spy (&api, &TemplateAPI::categoriesLoadFailed);
    QVERIFY (spy.isValid ());

    QtMessageHandler oldHandler= qInstallMessageHandler (
        [] (QtMsgType, const QMessageLogContext&, const QString&) {});

    api.fetchCategories ();
    QVERIFY (spy.wait (1000));

    qInstallMessageHandler (oldHandler);

    QCOMPARE (spy.count (), 1);
    QVERIFY (spy.takeFirst ()[0].toString ().contains ("Invalid"));
  }

  // --- 模板获取 ---

  void test_fetch_templates_success () {
    QJsonArray  items;
    QJsonObject tmpl;
    tmpl["templateKey"] = "nsfc-ysf-c";
    tmpl["name"]        = u8"国自然青年C类";
    tmpl["description"] = u8"申请书模板";
    tmpl["author"]      = "Liii Network";
    tmpl["version"]     = "20260424";
    tmpl["license"]     = "GPL-3.0";
    tmpl["thumbnailUrl"]= "https://cdn.liiistem.cn/images/thumb.png";
    tmpl["fileSize"]    = 1024;
    tmpl["fileMd5"]     = "53213b7dd8736afbf9a927cccac16533";
    tmpl["language"]    = "zh-CN";

    QJsonObject categoryObj;
    categoryObj["categoryKey"]= "resume-report-application";
    categoryObj["name"]       = u8"简历报告申请";
    tmpl["category"]          = categoryObj;

    tmpl["url"]       = "https://cdn.liiistem.cn/library/file.tmu";
    tmpl["pdfUrl"]    = "https://cdn.liiistem.cn/library/file.pdf";
    tmpl["createTime"]= "2026-04-24T00:00:00Z";
    tmpl["updateTime"]= "2026-04-25T12:00:00Z";

    QJsonArray tags;
    tags.append ("NSFC");
    tags.append (u8"国自然");
    tmpl["tags"]= tags;

    QJsonObject compat;
    compat["mogan_min_version"]= "1.2.0";
    tmpl["compatibility"]      = compat;

    QJsonObject stats;
    stats["downloads"]= 100;
    stats["rating"]   = 4.5;
    tmpl["statistics"]= stats;

    items.append (tmpl);

    QJsonObject dataObj;
    dataObj["items"]= items;

    QJsonObject root;
    root["code"]   = 0;
    root["success"]= true;
    root["message"]= "ok";
    root["data"]   = dataObj;

    QByteArray body= QJsonDocument (root).toJson (QJsonDocument::Compact);

    QByteArray response=
        QByteArray ("HTTP/1.1 200 OK\r\n") +
        "Content-Length: " + QByteArray::number (body.size ()) + "\r\n" +
        "\r\n" + body;

    MiniHttpServer server (response);
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QHash<QString, TemplateMetadataPtr> receivedMetadata;
    connect (&api, &TemplateAPI::templatesLoaded,
             [&] (const QHash<QString, TemplateMetadataPtr>& m) {
               receivedMetadata= m;
             });

    QSignalSpy failedSpy (&api, &TemplateAPI::templatesLoadFailed);
    QVERIFY (failedSpy.isValid ());

    api.fetchTemplates ("resume-report-application");
    QVERIFY (QSignalSpy (&api, &TemplateAPI::templatesLoaded).wait (1000));

    QCOMPARE (failedSpy.count (), 0);
    QCOMPARE (receivedMetadata.size (), 1);

    auto ptr= receivedMetadata["nsfc-ysf-c"];
    QVERIFY (!ptr.isNull ());
    QCOMPARE (ptr->id, QString ("nsfc-ysf-c"));
    QCOMPARE (ptr->category, QString ("resume-report-application"));
    QCOMPARE (ptr->fileSize, qint64 (1024));
    QCOMPARE (ptr->downloadCount, 100);
  }

  void test_fetch_templates_network_error () {
    MiniHttpServer server ("");
    TemplateAPI    api;
    api.setApiBaseUrl (server.url ());

    QSignalSpy spy (&api, &TemplateAPI::templatesLoadFailed);
    QVERIFY (spy.isValid ());

    api.fetchTemplates ("cat1");
    QVERIFY (spy.wait (1000));
    QCOMPARE (spy.count (), 1);
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
    }
    QVERIFY (true);
  }

  void test_destructor_with_active_categories_fetch () {
    QString url=
        QString ("http://127.0.0.1:%1/hang").arg (hangServer_.serverPort ());

    {
      TemplateAPI api;
      api.setApiBaseUrl (url);
      api.fetchCategories ();
    }
    QVERIFY (true);
  }

  void test_destructor_with_active_templates_fetch () {
    QString url=
        QString ("http://127.0.0.1:%1/hang").arg (hangServer_.serverPort ());

    {
      TemplateAPI api;
      api.setApiBaseUrl (url);
      api.fetchTemplates ("cat1");
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
#include "template_api_integration_test.moc"
