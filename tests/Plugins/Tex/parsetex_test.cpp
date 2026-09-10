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
  // latex_document_to_tree 是粘贴路径的完整转换入口；
  // 复现代码与手动测试共用 TeXmacs/tests/tex/ 下的样本
  void check_crash_case (const char* file);

private slots:
  void initTestCase ();
  void test_crash_case_249 ();
  void test_crash_case_289 ();
  void test_crash_case_1294_3 ();
  void test_has_macro_cycle ();
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
TestParseTex::check_crash_case (const char* file) {
  string s;
  QVERIFY2 (!load_string (url_system (string ("$TEXMACS_PATH/tests/tex/") *
                                      string (file)),
                          s, true),
            "cannot load crash case tex file");
  tree doc= latex_document_to_tree (s);
  QVERIFY (is_func (doc, moebius::DOCUMENT));
}

void
TestParseTex::test_crash_case_249 () {
  // 无参自递归宏定义
  check_crash_case ("1294_1.tex");
}

void
TestParseTex::test_crash_case_289 () {
  // 带参自递归宏定义
  check_crash_case ("1294_2.tex");
}

void
TestParseTex::test_crash_case_1294_3 () {
  // 相互递归宏定义 (crash_pattern_c)
  check_crash_case ("1294_3.tex");
}

void
TestParseTex::test_has_macro_cycle () {
  // 1. 无参自递归: \def\foo{\foo} -> <assign|foo|<macro|<foo>>>
  tree self_rec=
      tuple (compound ("assign", "foo", compound ("macro", compound ("foo"))));
  QVERIFY (has_macro_cycle (self_rec));

  // 2. 带参自递归: \def\foo#1{\foo{#1}} -> <assign|foo|<macro|x|<foo|<arg|x>>>>
  tree param_rec= tuple (compound (
      "assign", "foo",
      compound ("macro", "x", compound ("foo", compound ("arg", "x")))));
  QVERIFY (has_macro_cycle (param_rec));

  // 3. 相互递归: foo 调 bar, bar 调 foo
  tree mutual_rec=
      tuple (compound ("assign", "foo", compound ("macro", compound ("bar"))),
             compound ("assign", "bar", compound ("macro", compound ("foo"))));
  QVERIFY (has_macro_cycle (mutual_rec));

  // 4. 非递归正常宏: foo 调内置命令，无环
  tree normal_macro= tuple (compound (
      "assign", "foo",
      compound ("macro", "x", compound ("bold", compound ("arg", "x")))));
  QVERIFY (!has_macro_cycle (normal_macro));

  // 5. 普通文档无宏定义
  tree no_macro= compound ("document", "Hello world");
  QVERIFY (!has_macro_cycle (no_macro));
}

QTEST_MAIN (TestParseTex)
#include "parsetex_test.moc"
