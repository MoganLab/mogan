/******************************************************************************
 * MODULE     : oauth_deeplink_registry.cpp
 * DESCRIPTION: Windows 上 liiistem:// 协议注册的运行时写入
 * COPYRIGHT  : (C) 2026  MoonL79
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "oauth_deeplink.hpp"

#include "tm_debug.hpp"
#include "tm_ostream.hpp"
#include "tm_sys_utils.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QString>

#ifdef Q_OS_WINDOWS

namespace oauth_deeplink {

// QSettings 把键路径的最后一段当值名：写 .../shell/open/command 得到的
// 是 shell\open 下名为 command 的值，而 Windows 要的是 command 子键。
// 末段取 "Default" 时 Qt 折叠成空值名（qsettings_win.cpp 里
// res == "Default" || res == "." 即置空），正好落到子键的 (默认) 值上。
// 落点错了浏览器既不弹窗也不报错，只是静默无反应，最难查的那种失败
static const char* const kDefaultValueSuffix= "/Default";

/**
 * @brief 读出注册表里 shell\open\command 指向的可执行文件；不可用时返回空串
 *
 * 命令形如 `"C:\...\LiiiSTEM.exe" "%1"`，取第一个引号对之间的路径。要求路径
 * 为绝对路径且文件仍在——安装目录被挪走或卸载残留时它就成了死链。
 */
static QString
registered_command_exe () {
  // QSettings 继承 QObject、不可拷贝，只能各自就地构造，不抽公共的取值函数
  QSettings     classes ("HKEY_CURRENT_USER\\Software\\Classes",
                         QSettings::NativeFormat);
  const QString scheme= QString::fromLatin1 (kScheme);
  const QString commandKey=
      scheme + "/shell/open/command" + kDefaultValueSuffix;

  // 缺 URL Protocol 空值标记时 Shell 根本不认这是协议，浏览器点链接会直接
  // 失败（实测返回 SE_ERR_NOASSOC），此时哪怕 command 写得再对也没用
  if (!classes.contains (scheme + "/URL Protocol") ||
      !classes.contains (commandKey))
    return QString ();

  const QString command= classes.value (commandKey).toString ().trimmed ();
  const int     first  = command.indexOf ('"');
  const int     last   = command.lastIndexOf ('"');
  if (first < 0 || last <= first) return QString ();

  const QString exe= command.mid (first + 1, last - first - 1);
  if (!QDir::isAbsolutePath (exe) || !QFile::exists (exe)) return QString ();
  return exe;
}

/**
 * @brief liiistem:// 协议注册的运行时写入
 *
 * 本函数是这个键的唯一写入方，安装器（tauri-installer）不再碰它。改到运行时的
 * 原因是安装器只在安装时跑一次：Velopack 自动更新不会执行安装器，静默更新用户
 * 与老 NSIS 用户（从不写这个键）都覆盖不到。写在运行时，这些路径就只剩一条。
 *
 * 只在「键确实不可用」时才写：键缺失、缺 URL Protocol 标记、command 取不出
 * 路径，或指向的 exe 已不存在。指向的 exe 仍然存在时一律不碰——便携版、双版本
 * 共存、开发构建同时存在时不该被后来者覆盖，先来后到决定归属。
 *
 * 值的落点必须符合 Windows 对协议注册的要求：command 与 DefaultIcon 是子键的
 * (默认) 值，描述是 scheme 键的 (默认) 值，只有 URL Protocol 与 ApplicationName
 * 是命名值。前三处写成同名命名值 Windows 找不到，浏览器表现为静默无反应
 * （见 kDefaultValueSuffix）。
 *
 * 只有商业版执行：社区版没有登录能力，深链对它毫无意义（见
 * is_community_stem）。
 */
void
ensure_registered () {
  // 社区版写进去会占住这个键：没有商业版时它指向一个收到深链也做不了什么的进程，
  // 而商业版后装也拿不回来——本函数在注册可用时一律不改写，先来后到决定归属
  if (is_community_stem ()) return;

  const QString registered= registered_command_exe ();
  if (!registered.isEmpty ()) {
    // 注册可用就不动它。这里仍然记一条：真机上「协议压根没被拉起」与「注册指向
    // 了另一个 exe」现象完全一样，不记就只能去翻注册表
    debug_boot << "OAuth protocol registration kept: "
               << registered.toUtf8 ().constData () << "\n";
    return;
  }

  QSettings     classes ("HKEY_CURRENT_USER\\Software\\Classes",
                         QSettings::NativeFormat);
  const QString scheme= QString::fromLatin1 (kScheme);
  const QString exe=
      QDir::toNativeSeparators (QCoreApplication::applicationFilePath ());
  const QString quoted= QString ("\"%1\"").arg (exe);

  // 描述、“URL Protocol” 空值标记、浏览器确认框展示的应用名。描述与
  // ApplicationName 对外可见：资源管理器与浏览器确认框都会显示，
  // 改文案前先想清楚用户看到什么
  classes.setValue (scheme + kDefaultValueSuffix, "URL:Liii STEM");
  classes.setValue (scheme + "/URL Protocol", QString ());
  classes.setValue (scheme + "/ApplicationName", "Liii STEM");
  classes.setValue (scheme + "/DefaultIcon" + kDefaultValueSuffix, quoted);
  // %1 必须带引号：URL 可能含特殊字符，引号保证它作为单个参数传给进程
  classes.setValue (scheme + "/shell/open/command" + kDefaultValueSuffix,
                    quoted + " \"%1\"");
  classes.sync ();

  debug_boot << "OAuth protocol registration repaired\n";
}

} // namespace oauth_deeplink

#else

namespace oauth_deeplink {
void
ensure_registered () {}
} // namespace oauth_deeplink

#endif
