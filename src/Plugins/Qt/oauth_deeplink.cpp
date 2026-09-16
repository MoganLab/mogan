/******************************************************************************
 * MODULE     : oauth_deeplink.cpp
 * DESCRIPTION: liiistem:// 自定义协议深链的接收、解析与实例路由
 * COPYRIGHT  : (C) 2026  MoonL79
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "oauth_deeplink.hpp"

#include "tm_ostream.hpp"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMainWindow>
#include <QUrl>
#include <QUrlQuery>
#include <QWidget>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

namespace oauth_deeplink {

bool
is_oauth_callback (const QString& url) {
  QUrl parsed (url);
  // QUrl::scheme() 本身返回小写；显式比较是为覆盖传入串大小写混杂的情况
  if (parsed.scheme ().compare (QString::fromLatin1 (kScheme),
                                Qt::CaseInsensitive) != 0)
    return false;

  // Windows 规范化后 query 前会多一个 `/`，host 也可能被转小写，因此不碰
  // host 与 path，只认这两个查询参数
  QUrlQuery query (parsed);
  return !query.queryItemValue ("code").isEmpty () &&
         !query.queryItemValue ("state").isEmpty ();
}

QString
instance_id_from_url (const QString& url) {
  const QString state= QUrlQuery (QUrl (url)).queryItemValue ("state");
  // state 形如 "<instanceId>.<一次性随机数>"，实例标识在第一个 "." 之前
  const int dot= state.indexOf ('.');
  if (dot <= 0) return QString ();
  return state.left (dot);
}

QString
find_url (const QStringList& arguments) {
  const QString prefix= QString::fromLatin1 (kScheme) + "://";
  const int     size  = arguments.size ();
  for (int i= 1; i < size; i++) {
    if (arguments[i].startsWith (prefix, Qt::CaseInsensitive))
      return arguments[i];
  }
  return QString ();
}

QString
redirect_uri () {
  return QString::fromLatin1 (kRedirectUri);
}

void
bring_to_front () {
  // 优先主窗口：顶层 widget 里还混着搜索栏、各种 popup，随便抓一个会把它们
  // 提到前面。拿不到主窗口时退回第一个可见顶层窗口
  QWidget*          target = nullptr;
  const QWidgetList windows= QApplication::topLevelWidgets ();
  for (QWidget* window : windows) {
    if (!window || !window->isWindow () || !window->isVisible ()) continue;
    if (qobject_cast<QMainWindow*> (window)) {
      target= window;
      break;
    }
    if (!target) target= window;
  }
  if (!target) return;

  // 最小化时先还原：activateWindow () 对最小化窗口不生效
  if (target->isMinimized ()) target->showNormal ();
  target->activateWindow ();
}

#ifdef Q_OS_WINDOWS

bool
try_forward (const QString& url) {
  const QString instance_id= instance_id_from_url (url);
  if (instance_id.isEmpty ()) return false;

  QLocalSocket socket;
  socket.connectToServer (QString::fromLatin1 (kUrlServerPrefix) + instance_id);
  if (!socket.waitForConnected (300)) return false;

  // 前台权交接：本进程由浏览器拉起、有权抢前台，但要立刻退出；目标实例收到 URL
  // 时焦点仍在浏览器上，而 Windows 默认拒绝对非前台进程的 SetForegroundWindow，
  // 结果会是浏览器亮着、软件在后台静默登录完成。权限随这次进程间通信传递过去。
  //
  // 该调用有前置条件，可能失败（本进程当时未必是前台进程）。失败时软件只会在
  // 后台登录，界面不弹出来——记一条日志，免得真机上只能看到「窗口没动静」
  if (!AllowSetForegroundWindow (ASFW_ANY))
    debug_boot << "OAuth deep link: cannot hand over foreground rights\n";

  socket.write (url.toUtf8 ());
  // 必须加换行：多条 URL 可能前后到达同一实例，接收端以换行切分
  socket.write ("\n");
  socket.flush ();
  socket.waitForBytesWritten (300);
  socket.disconnectFromServer ();

  // 已转发，本进程的使命结束：调用方不应再继续启动，否则会多开一个空窗口
  return true;
}

void
start_receiver (QObject* owner, const QString& instance_id, QObject* target,
                const UrlHandler& handler) {
  // QLocalServer 以 owner 为父对象，随实例析构。名字含实例标识，多开时天然
  // 不撞，因此不需要「主实例」概念，也不改变现有允许多开的策略
  QLocalServer* server= new QLocalServer (owner);
  const QString name  = QString::fromLatin1 (kUrlServerPrefix) + instance_id;
  QLocalServer::removeServer (name);
  if (!server->listen (name)) {
    debug_boot << "OAuth deep link server failed to listen" << "\n";
    return;
  }

  QObject::connect (
      server, &QLocalServer::newConnection, target, [server, target, handler] {
        while (QLocalSocket* socket= server->nextPendingConnection ()) {
          // 跨 readyRead 累积半个 URL：buffer 按值捕获，副本由连接对象持有，
          // socket 析构时连接一并断开，不需要额外的生命周期管理
          QByteArray buffer;
          QObject::connect (socket, &QLocalSocket::readyRead, target,
                            [socket, buffer, handler] () mutable {
                              buffer.append (socket->readAll ());
                              int newline;
                              while ((newline= buffer.indexOf ('\n')) >= 0) {
                                const QString url=
                                    QString::fromUtf8 (buffer.left (newline));
                                buffer.remove (0, newline + 1);
                                if (!url.isEmpty ()) handler (url);
                              }
                            });
          QObject::connect (socket, &QLocalSocket::disconnected, socket,
                            &QLocalSocket::deleteLater);
        }
      });
}

#else // 非 Windows：深链接收与路由留待 macOS（QFileOpenEvent）与 Linux 接入

bool
try_forward (const QString&) {
  return false;
}

void
start_receiver (QObject*, const QString&, QObject*, const UrlHandler&) {}

#endif

} // namespace oauth_deeplink
