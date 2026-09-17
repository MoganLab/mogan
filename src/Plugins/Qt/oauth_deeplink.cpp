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

#include "server.hpp"
#include "tm_debug.hpp"
#include "tm_ostream.hpp"
#include "tm_server.hpp"

#include <QApplication>
#include <QCoreApplication>
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
is_wake (const QString& url) {
  QUrl parsed (url);
  // QUrl::scheme() 本身返回小写；显式比较是为覆盖传入串大小写混杂的情况
  if (parsed.scheme ().compare (QString::fromLatin1 (kScheme),
                                Qt::CaseInsensitive) != 0)
    return false;

  // Windows 规范化后 query 前会多一个 `/`，host 也可能被转小写，因此不碰
  // host 与 path，只认这一个查询参数
  return !QUrlQuery (parsed).queryItemValue ("instance").isEmpty ();
}

QString
instance_id_from_url (const QString& url) {
  return QUrlQuery (QUrl (url)).queryItemValue ("instance");
}

bool
is_wake_for (const QString& url, const QString& instance_id) {
  // 还不知道自己是哪个实例（本进程的标识尚未登记）时不能当成「就是给本进程的」
  if (instance_id.isEmpty ()) return false;
  return is_wake (url) && instance_id_from_url (url) == instance_id;
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

bool
handle_launch () {
  // 非 Windows 上是空操作，社区版也在这里被挡掉（见 ensure_registered）
  ensure_registered ();

  // arguments() 而非 argv：Windows 命令行是 UTF-16，QApplication 已在构造时完成
  // 转换并摘掉 Qt 自身参数，比直接取 argv 干净
  const QString url= find_url (QCoreApplication::arguments ());
  if (url.isEmpty ()) return false;

  // 命令行里带深链 URL 就说明本进程是被浏览器拉起来送唤醒的，唯一职责是转给发起
  // 实例。送不到（那个实例已退出）也不接管启动：用户要的是原来那个窗口，再开一个
  // 新的既非他所求，也不是登录所需——登录走环回地址，与本进程无关
  if (!try_forward (url))
    debug_boot << "OAuth deep link: no running instance to wake\n";
  return true;
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

#else // 非 Windows：没有转发进程，URL 由系统直接投递（见 handle_open_url）

bool
try_forward (const QString&) {
  return false;
}

void
start_receiver (QObject*, const QString&, QObject*, const UrlHandler&) {}

#endif

#ifdef Q_OS_MACOS

#include <QDateTime>

// 本进程的实例标识，由 QTMOAuth 构造时登记。深链到达时靠它判断这条 URL 是不是
// 给本进程的（见 handle_open_url）
static QString s_localInstanceId;

// 事件循环开始的时刻（0 = 还没开始）。见 mark_loop_started
static qint64 s_loopStartMs= 0;

// 深链在事件循环开始后多久之内到达，仍算「本进程刚被拉起」：被拉起的进程，
// LaunchServices 在事件循环开始后约 0.1 秒投递 URL（实测），5 秒是给慢机器
// 与冷启动留的余量
static const qint64 kLaunchGraceMs= 5000;

void
set_local_instance_id (const QString& instance_id) {
  s_localInstanceId= instance_id;
}

void
mark_loop_started () {
  s_loopStartMs= QDateTime::currentMSecsSinceEpoch ();
}

bool
handle_open_url (const QUrl& url) {
  if (url.scheme ().compare (QString::fromLatin1 (kScheme),
                             Qt::CaseInsensitive) != 0)
    return false;

  if (is_wake_for (url.toString (), s_localInstanceId)) {
    bring_to_front ();
    return true;
  }

  // 不指向本进程的唤醒有两种来历，只能靠「本进程是不是刚起来」区分：
  //
  // - 刚起来（事件循环才开始几百毫秒）→ 本进程就是被这条深链拉起的。发起登录的
  //   实例已经没了（用户关掉或崩溃），登录早已作废，用户要的是原来那个窗口，不该
  //   由这里再留一个全新的界面——直接退出。
  // - 已经跑了一阵 → 用户正在用的实例收到了别人的唤醒（多开，或有人直接打开了
  //   一条唤醒链接）。既不置前也不退出：退出会连带杀掉用户手里未保存的工作。
  //
  // 界面「已经出现过」是 macOS 的下限，不是能调好的时序：LaunchServices 要等
  // App 启动完成才投递 URL 事件，实测比主窗口晚约 0.3 秒，早于此无从判断。
  //
  // 两个标识都记：真机上「窗口没弹出来」与「唤醒投给了别的实例」现象一样，
  // 不记就没法归因（与 Windows 侧 try_forward 失败时的日志同一个作用）。
  // 存 QByteArray 而不是取 constData () 存指针：toUtf8 () 的返回值是临时对象
  const QByteArray target = instance_id_from_url (url.toString ()).toUtf8 ();
  const QByteArray local  = s_localInstanceId.toUtf8 ();
  const qint64     now    = QDateTime::currentMSecsSinceEpoch ();
  const qint64     elapsed= s_loopStartMs == 0 ? 0 : now - s_loopStartMs;

  if (elapsed >= kLaunchGraceMs) {
    debug_boot << "OAuth deep link: not for this instance (want "
               << target.constData () << ", have " << local.constData ()
               << "), ignored\n";
    return true;
  }

  debug_boot << "OAuth deep link: launched for a dead instance (want "
             << target.constData () << ", have " << local.constData ()
             << "), quitting\n";
  if (is_server_started ())
    get_server ()->quit (); // 关管道 + scheme 退出钩子 + exit
  QCoreApplication::quit ();
  return true;
}

#else // Linux 等的深链接入留待后续；这些平台没有 FileOpen 形态的深链

void
set_local_instance_id (const QString&) {}

void
mark_loop_started () {}

bool
handle_open_url (const QUrl&) {
  return false;
}

#endif // Q_OS_MACOS

} // namespace oauth_deeplink
