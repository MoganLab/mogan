/******************************************************************************
 * MODULE     : parsetex_test.cpp
 * DESCRIPTION: Tests for latex parser
 * COPYRIGHT  : (C) 2026 Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "Tex/tex.hpp"
#include "base.hpp"
#include "boot.hpp"
#include "file.hpp"
#include "server.hpp"
#include "tm_sys_utils.hpp"
#include <s7_tm.hpp>

extern s7_pointer user_env;

class TestParseTex : public QObject {
  Q_OBJECT

private:
  server* sv;

private slots:
  void initTestCase ();
  void cleanupTestCase ();
  void test_crash_case_249 ();
  void test_crash_case_289 ();
};

void
TestParseTex::initTestCase () {
  init_lolly ();
  init_texmacs_home_path ();
  init_texmacs_front ();
  int   argc  = 1;
  char* argv[]= {(char*) "parsetex_test", nullptr};
  gui_open (argc, argv);
  if (!tm_s7) {
    tm_s7   = s7_init ();
    user_env= s7_inlet (tm_s7, s7_nil (tm_s7));
    s7_gc_protect (tm_s7, user_env);
  }
  sv= new server (app_type::RESEARCH);
}

void
TestParseTex::cleanupTestCase () {
  // server rep is managed
}

void
TestParseTex::test_crash_case_249 () {
  // 自递归宏定义用例，GUI「粘贴自 LaTeX」触发进程崩溃，
  // 复现代码与手动测试共用 TeXmacs/tests/tex/1294_1.tex
  string s;
  QVERIFY2 (
      !load_string (url_system ("$TEXMACS_PATH/tests/tex/1294_1.tex"), s, true),
      "cannot load 1294_1.tex");
  // latex_document_to_tree 是粘贴路径的完整转换入口
  tree doc= latex_document_to_tree (s);
  QVERIFY (is_func (doc, moebius::DOCUMENT));
}

void
TestParseTex::test_crash_case_289 () {
  // 带参递归宏定义用例，复现代码 TeXmacs/tests/tex/1294_2.tex
  string s;
  QVERIFY2 (
      !load_string (url_system ("$TEXMACS_PATH/tests/tex/1294_2.tex"), s, true),
      "cannot load 1294_2.tex");
  tree doc= latex_document_to_tree (s);
  QVERIFY (is_func (doc, moebius::DOCUMENT));
}

QTEST_MAIN (TestParseTex)
#include "parsetex_test.moc"
