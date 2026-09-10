
/******************************************************************************
 * MODULE     : process_guard.cpp
 * DESCRIPTION: detect other Mogan instances sharing the install directory
 * COPYRIGHT  : (C) 2026 Mogan
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "process_guard.hpp"

#if defined(OS_WIN) || defined(OS_MACOS)

// 本文件在 lolly/Qt 初始化之前即被调用（research.cpp 启动钩子），只依赖
// 系统 C API；内部使用 std 字符串/容器（与 tm_velopack.cpp 桥接层同理的
// 例外，工作在全局设施就绪之前）。
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#ifdef OS_WIN
#include <windows.h>

// 依赖 windows.h 的类型定义，须置于其后（空行隔断，避免 gf fmt 按字母序
// 把它排到前面导致编译失败）
#include <tlhelp32.h>
#else
#include <libproc.h>
#include <unistd.h>
#endif

/******************************************************************************
 * 路径规范化与安装目录键
 ******************************************************************************/

// 统一比较形态：分隔符归一为 '/' 并小写化（Windows 与 macOS 文件系统默认
// 大小写不敏感）。模块路径与系统进程快照来源不同，先归一再比较。
static std::wstring
normalize (const wchar_t* p, size_t n) {
  std::wstring s (p, n);
  for (auto& c : s) {
    if (c == L'\\') c= L'/';
    c= static_cast<wchar_t> (towlower (c));
  }
  return s;
}

static std::string
normalize (const char* p, size_t n) {
  std::string s (p, n);
  for (auto& c : s) {
    if (c == '\\') c= '/';
    c= static_cast<char> (tolower (static_cast<unsigned char> (c)));
  }
  return s;
}

/**
 * @brief Windows：exe 路径 → 安装目录键。
 *
 * Velopack 布局为 <root>/current/<app>.exe，取路径中 /current/ 之前的段
 * （即安装根目录）。非该布局（开发构建等）退化为完整路径本身——同路径的
 * 进程仍互相识别，不同安装位置互不误伤。
 */
static std::wstring
install_key (const std::wstring& exe_path) {
  const std::wstring marker= L"/current/";
  size_t             pos   = exe_path.rfind (marker);
  if (pos != std::wstring::npos) return exe_path.substr (0, pos);
  return exe_path;
}

/**
 * @brief macOS：exe 路径 → 安装目录键。
 *
 * bundle 布局为 <...>/<app>.app/Contents/MacOS/<app>，取 .app 之前（含）
 * 的 bundle 路径；同一安装的并发实例经由稳定启动路径解析到同一 bundle，
 * 键相同。非该布局退化为完整路径本身。
 */
static std::string
install_key (const std::string& exe_path) {
  const std::string marker= "/.app/contents/macos/"; // 路径已小写化
  size_t            pos   = exe_path.rfind (marker);
  if (pos != std::string::npos) return exe_path.substr (0, pos + 4);
  return exe_path;
}

/******************************************************************************
 * 实现
 ******************************************************************************/

// 已规范化路径的文件名段（最后一个 '/' 之后）
static std::wstring
exe_name_of (const std::wstring& normalized_path) {
  size_t pos= normalized_path.rfind (L'/');
  return normalized_path.substr (pos == std::wstring::npos ? 0 : pos + 1);
}

bool
has_other_mogan_instances () {
#ifdef OS_WIN
  wchar_t self_path[1024];
  DWORD   n= GetModuleFileNameW (nullptr, self_path,
                                 sizeof (self_path) / sizeof (self_path[0]));
  if (n == 0 || n >= sizeof (self_path) / sizeof (self_path[0])) return false;
  const std::wstring self_norm= normalize (self_path, n);
  const std::wstring my_key   = install_key (self_norm);
  const std::wstring my_name  = exe_name_of (self_norm);

  HANDLE snap= CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE) return false;
  PROCESSENTRY32W entry;
  entry.dwSize= sizeof (entry);
  BOOL ok     = Process32FirstW (snap, &entry);
  while (ok) {
    // PROCESSENTRY32W 只有文件名（szExeFile），先按名预筛，再对同名进程
    // 查完整路径比对安装键
    if (entry.th32ProcessID != GetCurrentProcessId () &&
        normalize (entry.szExeFile, wcslen (entry.szExeFile)) == my_name) {
      HANDLE  proc= OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                 entry.th32ProcessID);
      wchar_t path[1024];
      DWORD   len= sizeof (path) / sizeof (path[0]);
      // 同名但读不到完整路径（权限不足，如另一实例以管理员运行）时按存在
      // 处理：漏检会损坏安装，误报只是推迟本次更新到下次单实例启动。
      if (!proc || !QueryFullProcessImageNameW (proc, 0, path, &len) ||
          install_key (normalize (path, len)) == my_key) {
        if (proc) CloseHandle (proc);
        CloseHandle (snap);
        return true;
      }
      CloseHandle (proc);
    }
    ok= Process32NextW (snap, &entry);
  }
  CloseHandle (snap);
  return false;
#else // OS_MACOS
  char self_path[PROC_PIDPATHINFO_MAXSIZE];
  int  n= proc_pidpath (getpid (), self_path, sizeof (self_path));
  if (n <= 0) return false;
  const std::string my_key= install_key (normalize (self_path, n));

  int count= proc_listallpids (nullptr, 0);
  if (count <= 0) return false;
  std::vector<pid_t> pids (count);
  count= proc_listallpids (pids.data (), static_cast<int> (pids.size ()));
  if (count <= 0) return false;
  for (int i= 0; i < count; ++i) {
    if (pids[i] == getpid ()) continue;
    char path[PROC_PIDPATHINFO_MAXSIZE];
    int  m= proc_pidpath (pids[i], path, sizeof (path));
    if (m <= 0) continue; // 系统进程/无权限，跳过不误判
    if (install_key (normalize (path, m)) == my_key) return true;
  }
  return false;
#endif
}

#else // 非 Windows/macOS：无 Velopack 更新，恒无其他实例

bool
has_other_mogan_instances () {
  return false;
}

#endif // defined(OS_WIN) || defined(OS_MACOS)
