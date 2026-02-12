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

#ifdef Q_OS_WINDOWS
#include <qt_windows.h>
#include <windows.h>

#undef IsUp
#undef IsLoopBack
#endif

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFile>
#include <QNetworkInterface>
#include <QProcess>
#include <QString>
#include <QSysInfo>
#include <QUrl>

string
qt_get_current_cpu_arch () {
  return from_qstring (QSysInfo::currentCpuArchitecture ());
}

string
qt_get_pretty_os_name () {
  return from_qstring (QSysInfo::prettyProductName ());
}

#ifdef Q_OS_WINDOWS

// Helper function to get Windows version info using dynamic loading
static bool
GetWindowsVersionInfo (ULONG& major, ULONG& minor, ULONG& build) {
  HMODULE hNtDll= ::GetModuleHandleW (L"ntdll.dll");
  if (!hNtDll) return false;

  // Define the function pointer type - RtlGetVersion returns LONG and takes a
  // pointer to OSVERSIONINFO
  typedef LONG (WINAPI * RtlGetVersionFunc) (void*);
  auto pRtlGetVersion= reinterpret_cast<RtlGetVersionFunc> (
      ::GetProcAddress (hNtDll, "RtlGetVersion"));

  if (!pRtlGetVersion) return false;

  OSVERSIONINFOW osvi;
  osvi.dwOSVersionInfoSize= sizeof (osvi);
  LONG status             = pRtlGetVersion (&osvi);

  // RtlGetVersion returns 0 (STATUS_SUCCESS) on success
  if (status != 0) return false;

  major= osvi.dwMajorVersion;
  minor= osvi.dwMinorVersion;
  build= osvi.dwBuildNumber;
  return true;
}

QString
get_windows_detailed_version () {
  ULONG major= 0, minor= 0, build= 0;

  if (!GetWindowsVersionInfo (major, minor, build)) {
    return QSysInfo::prettyProductName ();
  }

  QString productName;
  if (major == 10 && minor == 0) {
    if (build >= 22000) productName= "Windows 11";
    else productName= "Windows 10";
  }
  else if (major == 6 && minor == 3) {
    productName= "Windows 8.1";
  }
  else if (major == 6 && minor == 2) {
    productName= "Windows 8";
  }
  else if (major == 6 && minor == 1) {
    productName= "Windows 7";
  }
  else {
    productName= QString ("Windows %1.%2").arg (major).arg (minor);
  }

  return QString ("%1 %2.%3.%4")
      .arg (productName)
      .arg (major)
      .arg (minor)
      .arg (build)
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

  return QString ("%1 %2").arg (lines[1]).arg (lines[0]).replace (" ", "_");
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

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
QString
get_linux_or_macos_device_id () {
  QByteArray               combinedData;
  QList<QNetworkInterface> interfaces= QNetworkInterface::allInterfaces ();
  for (int i= 0; i < interfaces.size (); ++i) {
    const QNetworkInterface& interface= interfaces.at (i);
    if (!(interface.flags () & QNetworkInterface::IsLoopBack) &&
        (interface.flags () & QNetworkInterface::IsUp)) {
      combinedData.append (interface.hardwareAddress ().toUtf8 ());
    }
  }
  QByteArray hashed=
      QCryptographicHash::hash (combinedData, QCryptographicHash::Sha256);
  return QString (hashed.toHex ());
}
#endif

#ifdef Q_OS_WINDOWS
QString
get_windows_device_id () {
  DWORD serialNumber= 0;
  BOOL  success= GetVolumeInformationW (L"C:\\", NULL, 0, &serialNumber, NULL,
                                        NULL, NULL, 0);

  if (success && serialNumber != 0) {
    QByteArray data= QByteArray::number (serialNumber, 16).toUpper ();
    QByteArray hashed=
        QCryptographicHash::hash (data, QCryptographicHash::Sha256);
    return QString (hashed.toHex ());
  }

  return "";
}
#endif

string
qt_stem_device_id () {
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
  return from_qstring (get_linux_or_macos_device_id ());
#elif defined(Q_OS_WINDOWS)
  return from_qstring (get_windows_device_id ());
#endif
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
