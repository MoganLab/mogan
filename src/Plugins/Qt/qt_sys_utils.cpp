/******************************************************************************
 * MODULE     : qt_sys_utils.cpp
 * DESCRIPTION: external command launcher
 * COPYRIGHT  : (C) 2009, 2016  David MICHEL, Denis Raux
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_sys_utils.hpp"
#include "basic.hpp"
#include "file.hpp"
#include "qt_utilities.hpp"
#include "string.hpp"
#include "tm_configure.hpp"
#include "tm_debug.hpp"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFile>
#include <QNetworkInterface>
#include <QProcess>
#include <QString>
#include <QSysInfo>
#include <QUrl>

#ifdef Q_OS_WINDOWS
#include <qt_windows.h>
#include <windows.h>

// 声明 RtlGetVersion 函数
extern "C" {
    typedef LONG NTSTATUS;
    typedef struct _RTL_OSVERSIONINFOW {
        ULONG dwOSVersionInfoSize;
        ULONG dwMajorVersion;
        ULONG dwMinorVersion;
        ULONG dwBuildNumber;
        ULONG dwPlatformId;
        WCHAR szCSDVersion[128];
    } RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;
    
    NTSTATUS WINAPI RtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInformation);
}

#endif

string
qt_get_current_cpu_arch () {
  return from_qstring (QSysInfo::currentCpuArchitecture ());
}

string
qt_get_pretty_os_name () {
  return from_qstring (QSysInfo::prettyProductName ());
}

#ifdef Q_OS_WINDOWS
QString
get_windows_detailed_version () {
  RTL_OSVERSIONINFOW osvi;
  osvi.dwOSVersionInfoSize= sizeof (osvi);
  if (RtlGetVersion (&osvi) != 0) {
    return QSysInfo::prettyProductName ();
  }

  QString productName;
  if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0) {
    if (osvi.dwBuildNumber >= 22000) productName= "Windows 11";
    else productName= "Windows 10";
  }
  else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3) {
    productName= "Windows 8.1";
  }
  else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2) {
    productName= "Windows 8";
  }
  else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1) {
    productName= "Windows 7";
  }
  else {
    productName= QString ("Windows %1.%2")
                     .arg (osvi.dwMajorVersion)
                     .arg (osvi.dwMinorVersion);
  }

  return QString ("%1 %2.%3.%4")
      .arg (productName)
      .arg (osvi.dwMajorVersion)
      .arg (osvi.dwMinorVersion)
      .arg (osvi.dwBuildNumber)
      .replace (" ", "_");
}
#endif

#ifdef Q_OS_MACOS
QString
get_macos_detailed_version () {
  QProcess    p;
  QStringList env= QProcess::systemEnvironment ();
  p.setEnvironment (env);
  p.start ("sh",
           QStringList ()
               << "-c"
               << "sw_vers -productVersion; sw_vers -productName; uname -m");
  if (!p.waitForFinished (1000)) return QSysInfo::prettyProductName ();

  QString     output= p.readAllStandardOutput ().trimmed ();
  QStringList lines = output.split ("\n");
  if (lines.size () < 3) return QSysInfo::prettyProductName ();

  return QString ("%1 %2 (%3)")
      .arg (lines[1])
      .arg (lines[0])
      .arg (lines[2])
      .replace (" ", "_");
}
#endif

#ifdef Q_OS_LINUX
QString
get_linux_detailed_version () {
  QFile file ("/etc/os-release");
  if (!file.open (QIODevice::ReadOnly)) return QSysInfo::prettyProductName ();

  QString prettyName;
  while (!file.atEnd ()) {
    QString line= file.readLine ().trimmed ();
    if (line.startsWith ("PRETTY_NAME=")) {
      prettyName= line.section ('=', 1).remove ('"');
      break;
    }
  }
  file.close ();

  if (prettyName.isEmpty ()) return QSysInfo::prettyProductName ();
  return prettyName.replace (" ", "_");
}
#endif

// User-Agent 格式:
// LiiiSTEM-v2026.2.1 Windows_11_10.0.26100 x86_64
// LiiiSTEM-v2026.2.1 macOS_15.3_(arm64) arm64
// LiiiSTEM-v2026.2.1 Debian_GNU/Linux_13_(trixie) x86_64

string
qt_stem_user_agent () {
  QString appVersion= QString ("LiiiSTEM-v") + XMACS_VERSION;
#ifdef Q_OS_WINDOWS
  QString osName= get_windows_detailed_version ();
#elif defined(Q_OS_MACOS)
  QString osName= get_macos_detailed_version ();
#elif defined(Q_OS_LINUX)
  QString osName= get_linux_detailed_version ();
#else
  QString osName= QSysInfo::prettyProductName ();
#endif
  QString arch= QSysInfo::currentCpuArchitecture ();

  return from_qstring (
      QString ("%1 %2 %3").arg (appVersion).arg (osName).arg (arch));
}

string
qt_stem_device_id () {
  QByteArray combinedData;

  QList<QNetworkInterface> interfaces= QNetworkInterface::allInterfaces ();
  for (const QNetworkInterface& interface : interfaces) {
    if (!(interface.flags () & QNetworkInterface::IsLoopBack) &&
        (interface.flags () & QNetworkInterface::IsUp)) {
      combinedData.append (interface.hardwareAddress ().toUtf8 ());
    }
  }

  QByteArray hashed=
      QCryptographicHash::hash (combinedData, QCryptographicHash::Sha256);
  return from_qstring (QString (hashed.toHex ()));
}

void
qt_open_url (url u) {
  debug_io << "open-url\t" << u << LF;
  if (is_local_and_single (u)) {
    QString link= to_qstring ("file:///" * as_string (u));
    QDesktopServices::openUrl (QUrl (link));
  }
  else {
    QString link= to_qstring (as_string (u));
    QDesktopServices::openUrl (QUrl (link));
  }
}
