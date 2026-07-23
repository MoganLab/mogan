#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#ifdef LORO_ENABLED
#include "../../../src/Plugins/WebSocket/libcurl/tm_curl_websocket_client.hpp"
#include "base.hpp"
#include "mock_ws_server.hpp"

// Forward declaration
void init_lolly ();

class TestWsClient : public tm_curl_websocket_client {
public:
  bool   connected_flag   = false;
  bool   disconnected_flag= false;
  string last_message;
  bool   last_is_binary= false;
  string last_error;

  void on_connect () override { connected_flag= true; }
  void on_disconnect () override { disconnected_flag= true; }
  void on_message (string data, bool is_binary) override {
    last_message  = data;
    last_is_binary= is_binary;
  }
  void on_error (string msg) override { last_error= msg; }
};

class tm_websocket_client_test : public QObject {
  Q_OBJECT

private:
  MockWsServer* server;
  QString       serverUrl;

private slots:
  void initTestCase () {
    init_lolly ();
    server= new MockWsServer (this);
    QVERIFY (server->listen (QHostAddress::LocalHost, 0));
    serverUrl= QString ("ws://127.0.0.1:%1").arg (server->serverPort ());
  }

  void cleanupTestCase () { server->close (); }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_connect_and_echo () {
    TestWsClient client;
    client.connect (string (serverUrl.toStdString ().c_str ()));

    // Wait for connection
    int retries= 50;
    while (!client.connected_flag && retries > 0) {
      client.poll ();
      QTest::qWait (100);
      retries--;
    }
    QVERIFY (client.connected_flag);
    QVERIFY (client.connected ());

    // Send a message
    string test_msg= "Hello WebSocket";
    client.send (test_msg, false); // Text message

    // Wait for echo
    retries= 50;
    while (client.last_message != test_msg && retries > 0) {
      client.poll ();
      QTest::qWait (100);
      retries--;
    }

    QVERIFY (client.last_message == "Hello WebSocket");
    QCOMPARE (client.last_is_binary, false);

    // Send binary message
    string bin_msg= "BinaryData";
    client.send (bin_msg, true);

    retries= 50;
    while (client.last_message != bin_msg && retries > 0) {
      client.poll ();
      QTest::qWait (100);
      retries--;
    }

    QVERIFY (client.last_message == "BinaryData");
    QCOMPARE (client.last_is_binary, true);

    client.disconnect ();

    // Wait for disconnect event
    retries= 50;
    while (!client.disconnected_flag && retries > 0) {
      client.poll ();
      QTest::qWait (100);
      retries--;
    }
    QVERIFY (client.disconnected_flag);
  }
};
QTEST_MAIN (tm_websocket_client_test)
#else
class DummyTest : public QObject {
    Q_OBJECT
};
QTEST_MAIN (DummyTest)
#endif // LORO_ENABLED

#include "tm_websocket_client_test.moc"
