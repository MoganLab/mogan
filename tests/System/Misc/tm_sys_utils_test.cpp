
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

#include <QtTest/QtTest>

class TestTmSysUtils : public QObject {
  Q_OBJECT

  string orig_home;
  string orig_docs;

private slots:
  void init () {
    init_lolly ();
    orig_home= get_env ("HOME");
    orig_docs= get_env ("TEXMACS_DOCUMENTS_PATH");
  }
  void cleanup () {
    set_env ("HOME", orig_home);
    set_env ("TEXMACS_DOCUMENTS_PATH", orig_docs);
  }
  void test_get_documents_path_home_root ();
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

QTEST_MAIN (TestTmSysUtils)
#include "tm_sys_utils_test.moc"
