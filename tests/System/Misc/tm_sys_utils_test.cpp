
/******************************************************************************
 * MODULE     : tm_sys_utils_test.cpp
 * DESCRIPTION: Unit tests for tm_sys_utils
 * COPYRIGHT  : (C) 2026  Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "base.hpp"
#include "sys_utils.hpp"
#include "tm_sys_utils.hpp"
#include "url.hpp"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestTmSysUtils : public QObject {
  Q_OBJECT

  string orig_home;
  string orig_docs;
  string orig_downloads;
  string orig_xdg_config;

  static void restore_env (const char* name, string val) {
    if (is_empty (val)) qunsetenv (name);
    else set_env (string (name), val);
  }

private slots:
  void init () {
    init_lolly ();
    orig_home      = get_env ("HOME");
    orig_docs      = get_env ("TEXMACS_DOCUMENTS_PATH");
    orig_downloads = get_env ("TEXMACS_DOWNLOADS_PATH");
    orig_xdg_config= get_env ("XDG_CONFIG_HOME");
  }
  void cleanup () {
    // set_env 对空值是 no-op，恢复未设置状态须走 qunsetenv
    restore_env ("HOME", orig_home);
    restore_env ("TEXMACS_DOCUMENTS_PATH", orig_docs);
    restore_env ("TEXMACS_DOWNLOADS_PATH", orig_downloads);
    restore_env ("XDG_CONFIG_HOME", orig_xdg_config);
  }
  void test_get_documents_path_home_root ();
  void test_get_downloads_path_env_override ();
  void test_get_downloads_path_xdg_user_dirs ();
  void test_get_downloads_path_fallback_home ();
};

void
TestTmSysUtils::test_get_documents_path_home_root () {
  // WASM 中 HOME=/，原来的字符串拼接会得到 "//Documents" 并被解析为 blank_url。
  // 这里验证修复后返回的是 default-rooted URL，不是 blank URL。
  set_env ("TEXMACS_DOCUMENTS_PATH", "");
  set_env ("HOME", "/");

  url docs= get_documents_path ();
  QVERIFY (!is_rooted_blank (docs));
  string docs_s= as_string (docs);
  QVERIFY2 (starts (docs_s, "/Documents"),
            as_charp ("expected /Documents but got " * docs_s));
  QVERIFY2 (!occurs ("//Documents", docs_s),
            as_charp ("path should not contain //Documents: " * docs_s));
}

void
TestTmSysUtils::test_get_downloads_path_env_override () {
  // 环境变量 TEXMACS_DOWNLOADS_PATH 优先于一切平台探测
  set_env ("TEXMACS_DOWNLOADS_PATH", "/tmp/mogan-test-downloads");
  url downloads= get_downloads_path ();
  QVERIFY2 (as_string (downloads) == "/tmp/mogan-test-downloads",
            as_charp ("env override broken: " * as_string (downloads)));
}

void
TestTmSysUtils::test_get_downloads_path_xdg_user_dirs () {
  if (os_win () || os_macos () || os_wasm ())
    QSKIP ("xdg-user-dirs only applies to Linux/BSD");
  // 中文发行版常见情形：XDG_DOWNLOAD_DIR 指向本地化的 ~/下载，
  // 且值里带 $HOME 变量引用，需展开
  QTemporaryDir config_dir;
  QVERIFY (config_dir.isValid ());
  QFile f (QDir (config_dir.path ()).filePath ("user-dirs.dirs"));
  QVERIFY (f.open (QIODevice::WriteOnly));
  f.write ("XDG_DESKTOP_DIR=\"$HOME/Desktop\"\n"
           "XDG_DOWNLOAD_DIR=\"$HOME/\xE4\xB8\x8B\xE8\xBD\xBD\"\n");
  f.close ();

  qunsetenv ("TEXMACS_DOWNLOADS_PATH");
  set_env ("XDG_CONFIG_HOME",
           string (config_dir.path ().toUtf8 ().constData ()));
  set_env ("HOME", "/tmp/mogan-test-home");

  url downloads= get_downloads_path ();
  QVERIFY2 (as_string (downloads) ==
                "/tmp/mogan-test-home/\xE4\xB8\x8B\xE8\xBD\xBD",
            as_charp ("xdg parse broken: " * as_string (downloads)));
}

void
TestTmSysUtils::test_get_downloads_path_fallback_home () {
  if (os_win () || os_macos () || os_wasm ())
    QSKIP ("xdg-user-dirs only applies to Linux/BSD");
  // 无 user-dirs.dirs 时兜底 $HOME/Downloads
  qunsetenv ("TEXMACS_DOWNLOADS_PATH");
  set_env ("XDG_CONFIG_HOME", "/tmp/mogan-test-nonexistent-config");
  set_env ("HOME", "/tmp/mogan-test-home");

  url downloads= get_downloads_path ();
  QVERIFY2 (as_string (downloads) == "/tmp/mogan-test-home/Downloads",
            as_charp ("fallback broken: " * as_string (downloads)));
}

QTEST_MAIN (TestTmSysUtils)
#include "tm_sys_utils_test.moc"