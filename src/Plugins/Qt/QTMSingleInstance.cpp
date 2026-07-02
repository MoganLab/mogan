/******************************************************************************
 * MODULE     : QTMSingleInstance.cpp
 * DESCRIPTION: 单实例实现，详见 QTMSingleInstance.hpp。
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMSingleInstance.hpp"

// 注意：Q_OS_* 由 Qt 的 qsystemdetection.h 定义，必须在包含任意 Qt 头文件
// （此处先包含 qglobal.h）之后才能用于条件编译。下方实现整段被平台宏包住；
// 不支持的平台提供空桩，保证链接器总能解析符号（MOC 生成的元对象代码会引用
// QTMSingleInstanceServer 的成员，即便在空桩平台也需存在定义）。
#include <QtGlobal>

// 覆盖平台：Linux（Unix domain socket）+ Windows（命名管道）。两者
// QLocalServer/QLocalSocket 均为 Qt 原生支持。macOS 走 QFileOpenEvent，
// 不需要本模块。
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)

#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "tm_url.hpp"

#include <QByteArray>
#include <QLocalServer>
#include <QLocalSocket>

// 协议详见 QTMSingleInstance.hpp 顶部注释。

// 单实例 socket 路径（Linux）/ 命名管道名（Windows）。
//   Linux：$TEXMACS_HOME_PATH/system/mogan_socket。该目录每个用户/配置独立，
//          天然隔离多账号；在 immediate_options() 里 init_texmacs_home_path()
//          之后即建立，客户端/服务端两侧都能拿到。
//   Windows：QLocalServer 在 Windows 上忽略路径只取 name 当管道名
//            （\\.\pipe\<name>）。name 不允许含 '\' '/'，故用一个固定短名。
//            多账号隔离由 Windows 命名管道的会话命名空间自动提供
//            （同一台机不同登录会话看不到彼此的 \\.\pipe\）。
static QString
single_instance_socket_path () {
#if defined(Q_OS_WIN)
  return QStringLiteral ("moganstem-single-instance");
#else
  url home= url_system ("$TEXMACS_HOME_PATH/system");
  url sock= home * url ("mogan_socket");
  return to_qstring (concretize (sock));
#endif
}

/******************************************************************************
 * 客户端：转发到已运行实例
 ******************************************************************************/

// 从 argv 中挑出位置参数文件（与 init_texmacs.cpp 同款判定），解析成绝对路径。
// 复用 init_texmacs.cpp 的 url_pwd() 解析逻辑：未 rooted 的相对当前目录。
// 含 '\n' 的路径会被跳过——发过去会破坏消息边界，宁可让本进程自己打开。
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
    QByteArray raw= to_qstring (as_string (u)).toUtf8 ();
    if (raw.contains ('\n')) continue; // 路径含换行符会破坏协议，跳过
    files.append (raw);
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
  // 确保所有字节都进入内核缓冲（服务端可异步读），无需等服务端处理完——
  // 服务端以"客户端关闭连接"作为一条完整消息的边界。
  socket.flush ();
  socket.waitForBytesWritten (3000);
  socket.disconnectFromServer ();
  if (socket.state () != QLocalSocket::UnconnectedState)
    socket.waitForDisconnected (1000);
  return true;
}

/******************************************************************************
 * 服务端：在已运行实例侧接收转发
 ******************************************************************************/

QTMSingleInstanceServer::QTMSingleInstanceServer (QObject* parent)
    : QObject (parent), server (nullptr), pending (nullptr), buffer (nullptr),
      timer_id (0) {}

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
  // 单实例语义下转发场景是顺序的（用户一次双击一个文件），同时只处理一条连接
  // 足够。若上条还没读完就来了新的，丢弃新的——客户端会超时走本地启动路径。
  if (pending != nullptr) {
    QLocalSocket* stale= server->nextPendingConnection ();
    if (stale != nullptr) delete stale;
    return;
  }
  QLocalSocket* client= server->nextPendingConnection ();
  if (client == nullptr) return;
  pending  = client;
  buffer   = new QByteArray ();
  timer_id = startTimer (10000); // 兜底：客户端崩溃不 disconnect 时回收
  // 异步驱动：readyRead 累加数据并切行处理；disconnected 收尾。全程不调用
  // waitFor*——那会阻塞 GUI 主线程导致卡顿。
  connect (client, &QLocalSocket::readyRead, this,
           &QTMSingleInstanceServer::read_from_client);
  connect (client, &QLocalSocket::disconnected, this,
           &QTMSingleInstanceServer::discard_client);
  // 可能 connect 之前已经 readyRead：手动触发一次读取。
  if (client->bytesAvailable () > 0) read_from_client ();
}

void
QTMSingleInstanceServer::read_from_client () {
  if (pending == nullptr) return;
  // 累加新到的数据，按 '\n' 切行；最后一个不完整行（无 '\n'）留在缓冲等下次。
  buffer->append (pending->readAll ());
  int idx;
  while ((idx= buffer->indexOf ('\n')) >= 0) {
    QByteArray line= buffer->mid (0, idx);
    buffer->remove (0, idx + 1);
    if (line.isEmpty ()) continue;
    string path= from_qstring_utf8 (QString::fromUtf8 (line));
    if (is_empty (path)) continue;
    // 默认 :current-window 路径：load-buffer-open 在当前窗口新开标签页，
    // 并按既有逻辑关掉未修改的欢迎 scratch buffer。不要用 :new-window。
    exec_delayed (scheme_cmd (list_object (symbol_object ("load-buffer"),
                                           object (url_system (path)),
                                           eval (":current-window"))));
  }
  // 客户端发完路径会立即关闭连接：缓冲里若已无未完整行（无残留 '\n'-less
  // 尾巴），且 socket 不会再有新数据（state 表明对端已关），即可立即清理，
  // 不必等 disconnected 信号回调。
  if (buffer->isEmpty () &&
      pending->state () == QLocalSocket::UnconnectedState) {
    discard_client ();
  }
}

void
QTMSingleInstanceServer::discard_client () {
  if (pending == nullptr) return;
  // 兜底定时器可能还没触发，先取消掉。
  if (timer_id != 0) {
    killTimer (timer_id);
    timer_id= 0;
  }
  // 收尾前再做一次读取，防止客户端发完立刻 disconnect、最后一帧还没被
  // readyRead 触发处理。
  if (pending->bytesAvailable () > 0) read_from_client ();
  // 不发 ACK：协议以客户端关闭连接为完成边界，ACK 在此场景下冗余。
  // disconnect 信号回调里绝对不能 waitForDisconnected——那会阻塞 GUI 主线程。
  pending->deleteLater ();
  pending= nullptr;
  delete buffer;
  buffer= nullptr;
}

void
QTMSingleInstanceServer::timerEvent (QTimerEvent* e) {
  if (e->timerId () == timer_id && pending != nullptr) {
    // 兜底：客户端崩溃/异常不 disconnect，定时回收这条悬挂连接。
    discard_client ();
  }
  QObject::timerEvent (e);
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

#else // 不支持单实例的平台（macOS 等）：空桩，仅满足链接。

bool
mogan_forward_to_running_instance (int, char**) {
  return false;
}

void
mogan_start_single_instance_server () {}

QTMSingleInstanceServer::QTMSingleInstanceServer (QObject* parent)
    : QObject (parent), server (nullptr), pending (nullptr), buffer (nullptr),
      timer_id (0) {}

bool
QTMSingleInstanceServer::start () {
  return false;
}

void
QTMSingleInstanceServer::handle_connection () {}

void
QTMSingleInstanceServer::read_from_client () {}

void
QTMSingleInstanceServer::discard_client () {}

void
QTMSingleInstanceServer::timerEvent (QTimerEvent*) {}

#endif // Q_OS_LINUX || Q_OS_WIN
