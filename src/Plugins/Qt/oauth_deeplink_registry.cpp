/******************************************************************************
 * MODULE     : oauth_deeplink_registry.cpp
 * DESCRIPTION: Windows 上 liiistem:// 协议注册的运行时补写
 * COPYRIGHT  : (C) 2026  MoonL79
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "oauth_deeplink.hpp"

#include "tm_ostream.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QString>

#ifdef Q_OS_WINDOWS

namespace oauth_deeplink {

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

  // 缺 URL Protocol 空值标记时 Shell 根本不认这是协议，浏览器点链接会直接
  // 失败（实测返回 SE_ERR_NOASSOC），此时哪怕 command 写得再对也没用
  if (!classes.contains (scheme + "/URL Protocol") ||
      !classes.contains (scheme + "/shell/open/command"))
    return QString ();

  const QString command=
      classes.value (scheme + "/shell/open/command").toString ().trimmed ();
  const int first= command.indexOf ('"');
  const int last = command.lastIndexOf ('"');
  if (first < 0 || last <= first) return QString ();

  const QString exe= command.mid (first + 1, last - first - 1);
  if (!QDir::isAbsolutePath (exe) || !QFile::exists (exe)) return QString ();
  return exe;
}

/**
 * @brief 协议注册的运行时自愈
 *
 * 协议注册的主责在安装器（tauri-installer 写 HKCU\Software\Classes\liiistem）。
 * 但 **Velopack 自动更新不会执行安装器**：register_url_protocol 只在
 * install_latest 中调用，因此
 *
 *   - 老 NSIS 用户（research.nsis 从不写这个键）升级到新版后键是空的；
 *   - 走静默自动更新的用户永远不会跑安装器。
 *
 * 运行时补写因此是老用户迁移的必经路径，而不只是为了方便开发调试。
 *
 * 与安装器共用同一组键，为避免两边互相顶掉，只在「键确实不可用」时才写：
 * 键缺失、缺 URL Protocol 标记、command 取不出路径、或指向的 exe 已不存在。
 * 指向的 exe 仍然存在时一律不碰——便携版 / 双版本共存时不该被后来者覆盖。
 */
void
ensure_registered () {
  if (!registered_command_exe ().isEmpty ())
    return; // 注册可用，保持安装器写入的现状

  QSettings     classes ("HKEY_CURRENT_USER\\Software\\Classes",
                         QSettings::NativeFormat);
  const QString scheme= QString::fromLatin1 (kScheme);
  const QString exe=
      QDir::toNativeSeparators (QCoreApplication::applicationFilePath ());
  const QString quoted= QString ("\"%1\"").arg (exe);

  // 描述、“URL Protocol” 空值标记、浏览器确认框展示的应用名。
  // 两处文案与安装器（tauri-installer lib.rs 的 URL_SCHEME_DESCRIPTION /
  // URL_APP_DISPLAY_NAME）逐字一致：同一组键由两个写入方维护，值不同会让
  // 「谁最后写的」决定用户看到什么
  classes.setValue (scheme, "URL:Liii STEM");
  classes.setValue (scheme + "/URL Protocol", QString ());
  classes.setValue (scheme + "/ApplicationName", "Liii STEM");
  classes.setValue (scheme + "/DefaultIcon", quoted);
  // %1 必须带引号：URL 可能含特殊字符，引号保证它作为单个参数传给进程
  classes.setValue (scheme + "/shell/open/command", quoted + " \"%1\"");
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
