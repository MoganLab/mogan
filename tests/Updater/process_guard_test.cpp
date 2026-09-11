
/******************************************************************************
 * MODULE     : process_guard_test.cpp
 * DESCRIPTION: has_other_mogan_instances 的单元测试
 * COPYRIGHT  : (C) 2026 Mogan
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Updater/process_guard.hpp"

#include "base.hpp"

#include <QtTest/QtTest>

#include <functional>
#include <string>
#include <vector>

#if defined(OS_WIN) || defined(OS_MACOS)
#ifdef OS_WIN
#include <windows.h>
#else
#include <libproc.h>
#include <spawn.h>
#include <unistd.h>
extern "C" char** environ;
#endif
#endif

// 以 100ms 步进轮询谓词直至为真或超时。进程创建/退出在系统快照中可见有
// 抖动，直接断言会偶发失败。
static bool
wait_until (const std::function<bool ()>& pred, int timeout_ms) {
  for (int waited= 0; waited < timeout_ms; waited+= 100) {
    if (pred ()) return true;
    QTest::qWait (100);
  }
  return pred ();
}

class TestProcessGuard : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // 单实例（仅本测试进程）：不应报告其他实例。测试 exe 路径唯一，开发机上
  // 同时运行的 Mogan STEM 安装目录不同，不会互相误判。
  void test_single_instance () {
#if defined(OS_WIN) || defined(OS_MACOS)
    QVERIFY (!has_other_mogan_instances ());
#else
    QSKIP ("process guard 仅在 Windows/macOS 实现");
#endif
  }

  // 拉起自身副本（--process-guard-child 子模式驻留数秒）：应检测到其他
  // 实例（同 exe 路径 → install_key 兜底分支命中）；副本退出后恢复检测
  // 不到。真实 velopack 安装布局（/current/ 键）无法在单测中构造，其键
  // 提取为纯字符串逻辑，由安装版手动场景覆盖（见 devel/0970.md）。
  void test_spawned_self_detected () {
#if defined(OS_WIN) || defined(OS_MACOS)
#ifdef OS_WIN
    wchar_t exe[1024];
    DWORD n= GetModuleFileNameW (nullptr, exe, sizeof (exe) / sizeof (exe[0]));
    QVERIFY2 (n > 0 && n < sizeof (exe) / sizeof (exe[0]),
              "GetModuleFileNameW failed");
    std::wstring cmd=
        L"\"" + std::wstring (exe, n) + L"\" --process-guard-child";
    std::vector<wchar_t> cmd_buf (cmd.begin (), cmd.end ());
    cmd_buf.push_back (L'\0');
    STARTUPINFOW        si{};
    PROCESS_INFORMATION pi{};
    si.cb       = sizeof (si);
    BOOL spawned= CreateProcessW (nullptr, cmd_buf.data (), nullptr, nullptr,
                                  FALSE, 0, nullptr, nullptr, &si, &pi);
    QVERIFY2 (spawned, "CreateProcessW failed");
    QVERIFY (wait_until ([] { return has_other_mogan_instances (); }, 3000));
    WaitForSingleObject (pi.hProcess, 15000);
    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);
#else
    char exe[PROC_PIDPATHINFO_MAXSIZE];
    int  n= proc_pidpath (getpid (), exe, sizeof (exe));
    QVERIFY2 (n > 0, "proc_pidpath failed");
    char* argv[]= {exe, const_cast<char*> ("--process-guard-child"), nullptr};
    pid_t pid   = -1;
    int   rc    = posix_spawn (&pid, exe, nullptr, nullptr, argv, environ);
    QVERIFY2 (rc == 0, "posix_spawn failed");
    QVERIFY (wait_until ([] { return has_other_mogan_instances (); }, 3000));
    int status= 0;
    waitpid (pid, &status, 0);
#endif
    // 副本退出后可能短暂残留在进程快照中，轮询等待恢复
    QVERIFY (wait_until ([] { return !has_other_mogan_instances (); }, 3000));
#else
    QSKIP ("process guard 仅在 Windows/macOS 实现");
#endif
  }
};

// 自定义 main：先识别子进程模式（--process-guard-child，驻留数秒供父进程
// 检测后退出，不走 QtTest），再进入测试本体。
int
main (int argc, char* argv[]) {
  for (int i= 1; i < argc; ++i)
    if (std::string (argv[i]) == "--process-guard-child") {
#if defined(OS_WIN) || defined(OS_MACOS)
#ifdef OS_WIN
      Sleep (5000);
#else
      sleep (5);
#endif
#endif
      return 0;
    }
  TestProcessGuard t;
  return QTest::qExec (&t, argc, argv);
}

#include "process_guard_test.moc"
