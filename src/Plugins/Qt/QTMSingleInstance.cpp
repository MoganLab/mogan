/******************************************************************************
 * MODULE     : QTMSingleInstance.cpp
 * DESCRIPTION: Linux 单实例实现，详见 QTMSingleInstance.hpp。
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMSingleInstance.hpp"

// 注意：Q_OS_LINUX 由 Qt 的 qsystemdetection.h 定义，必须在包含任意 Qt 头文件
// （此处先包含 qglobal.h）之后才能用于条件编译。下方 Linux 实现整段被它包住；
// 非 Linux 平台提供空桩，保证链接器总能解析符号（MOC 生成的元对象代码会引用
// QTMSingleInstanceServer 的成员，即便在非 Linux 上也需存在定义）。
#include <QtGlobal>

#if defined(Q_OS_LINUX)

#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "tm_url.hpp"

#include <QByteArray>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

// 协议：每条消息以一行 UTF-8 文本（待打开文件的绝对路径）表示，行尾 '\n'。
// 客户端发送完毕后等服务端回一个字节（任意）作为 ACK，然后关闭连接。
// 路径里出现 '\n' 的概率极低；此处不做转义，必要时可后续加固。

// 单实例 socket 路径：$TEXMACS_HOME_PATH/system/mogan_socket。
// 该目录每个用户/配置独立，天然隔离多账号；在 immediate_options() 里
// init_texmacs_home_path() 之后即建立，客户端/服务端两侧都能拿到。
static QString
single_instance_socket_path () {
  url home= url_system ("$TEXMACS_HOME_PATH/system");
  url sock= home * url ("mogan_socket");
  return to_qstring (concretize (sock));
}

/******************************************************************************
 * 客户端：转发到已运行实例
 ******************************************************************************/

// 从 argv 中挑出位置参数文件（与 init_texmacs.cpp 同款判定），解析成绝对路径。
// 复用 init_texmacs.cpp 的 url_pwd() 解析逻辑：未 rooted 的相对当前目录。
static QList<QByteArray>
collect_files_to_open (int argc, char** argv) {
  QList<QByteArray> files;
  for (int i= 1; i < argc; i++) {
    if (argv[i] == nullptr || argv[i][0] == '\0') continue;
    string s= argv[i];
    // 与启动参数解析一致："--foo" 折成 "-foo" 再判断。
    if ((N (s) >= 2) && (s (0, 2) == "--")) s= s (1, N (s));
    if (s[0] == '-' || s[0] == '+') continue; // 选项，跳过
    url u= url_system (s);
    if (!is_rooted (u)) u= resolve (url_pwd (), "") * u;
    // 项目自定义 string 无 c_str()；经 to_qstring 转 UTF-8 QByteArray。
    files.append (to_qstring (as_string (u)).toUtf8 ());
  }
  return files;
}

bool
mogan_forward_to_running_instance (int argc, char** argv) {
  QList<QByteArray> files= collect_files_to_open (argc, argv);
  // 没有待打开文件（例如纯启动、或全是选项）时，不转发：让本进程正常启动，
  // 否则无文件的"再开一次"会被吞掉、用户看不到第二个窗口也无从知晓。
  if (files.isEmpty ()) return false;

  QLocalSocket socket;
  socket.connectToServer (single_instance_socket_path ());
  // 短超时：已运行实例应几乎立即 accept；连不上（无实例）就快速放弃。
  if (!socket.waitForConnected (1000)) return false;

  for (const QByteArray& path : files) {
    socket.write (path + '\n');
  }
  socket.flush ();
  // 等 ACK（任意一个字节即可），最多等 5s 防止服务端卡死拖死客户端。
  socket.waitForReadyRead (5000);
  socket.disconnectFromServer ();
  if (socket.state () != QLocalSocket::UnconnectedState)
    socket.waitForDisconnected (1000);
  return true;
}

/******************************************************************************
 * 服务端：在已运行实例侧接收转发
 ******************************************************************************/

QTMSingleInstanceServer::QTMSingleInstanceServer (QObject* parent)
    : QObject (parent), server (nullptr) {}

bool
QTMSingleInstanceServer::start () {
  server= new QLocalServer (this);
  // 上次崩溃可能留下残留 socket 文件，先清理再监听。
  QLocalServer::removeServer (single_instance_socket_path ());
  if (!server->listen (single_instance_socket_path ())) {
    // 监听失败（路径过长/权限等）：退化为"每次双击开新进程"，不影响功能。
    delete server;
    server= nullptr;
    return false;
  }
  connect (server, &QLocalServer::newConnection, this,
           &QTMSingleInstanceServer::handle_connection);
  return true;
}

void
QTMSingleInstanceServer::handle_connection () {
  if (server == nullptr) return;
  QLocalSocket* client= server->nextPendingConnection ();
  if (client == nullptr) return;
  // 等数据到达；客户端一发完就会等 ACK，超时兜底以免悬挂连接。
  client->waitForReadyRead (5000);
  QByteArray        data = client->readAll ();
  QList<QByteArray> lines= data.split ('\n');
  for (const QByteArray& line : lines) {
    if (line.isEmpty ()) continue;
    string path= from_qstring_utf8 (QString::fromUtf8 (line));
    if (is_empty (path)) continue;
    // 默认 :current-window 路径：load-buffer-open 在当前窗口新开标签页，
    // 并按既有逻辑关掉未修改的欢迎 scratch buffer。不要用 :new-window。
    exec_delayed (scheme_cmd (list_object (symbol_object ("load-buffer"),
                                           object (url_system (path)),
                                           eval (":current-window"))));
  }
  // ACK：回一个字节让客户端放心退出。
  client->write ("1");
  client->flush ();
  client->disconnectFromServer ();
  if (client->state () != QLocalSocket::UnconnectedState)
    client->waitForDisconnected (1000);
  client->deleteLater ();
}

void
mogan_start_single_instance_server () {
  static QTMSingleInstanceServer* the_server= nullptr;
  if (the_server != nullptr) return; // 防止重复注册
  the_server= new QTMSingleInstanceServer ();
  if (!the_server->start ()) {
    delete the_server;
    the_server= nullptr;
  }
}

#else // 非 Linux：空桩，仅满足链接（MOC 元对象代码会引用这些符号）。

bool
mogan_forward_to_running_instance (int, char**) {
  return false;
}

void
mogan_start_single_instance_server () {}

QTMSingleInstanceServer::QTMSingleInstanceServer (QObject* parent)
    : QObject (parent), server (nullptr) {}

bool
QTMSingleInstanceServer::start () {
  return false;
}

void
QTMSingleInstanceServer::handle_connection () {}

#endif // Q_OS_LINUX
