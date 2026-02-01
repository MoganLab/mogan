
/*****************************************************************************
 * MODULE     : font_test.cpp
 * DESCRIPTION: Tests on font
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "converter.hpp"
#include "data_cache.hpp"
#include "font.hpp"
#include "qtestcase.h"
#include "smart_font.hpp"
#include "sys_utils.hpp"
#include "tm_sys_utils.hpp"
#include "tree_helper.hpp"
#include <QtTest/QtTest>

class TestSmartFont : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
  }
  void test_resolve ();
  void test_resolve_first_attempt ();
  void test_resolve_chinese_puncts ();
  void test_get_right_slope ();
};

void
TestSmartFont::test_resolve () {
  // ((roman rm medium right $s $d) (ec ecrm $s $d))
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  qcompare (fn->res_name, "sys-chinese-rm-medium-right-10-600-smart");
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  // int             nr    = fn_rep->resolve ("1");
  // qcompare (fn_rep->fn[nr]->res_name, "ec:ecrm10@600");

  // int nr2= fn_rep->resolve (utf8_to_cork ("è"));
  // qcompare (fn_rep->fn[nr2]->res_name, "ec:ecrm10@600");
}

void
TestSmartFont::test_resolve_first_attempt () {
  // (roman rm bold small-caps $s $d) (ec ecxc $s $d)
  tree which= tree (TUPLE, "roman", "rm", "bold", "small-caps", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecxc", "$s", "$d");
  font_rule (which, by);
  // sys-chinese-rm-bold-small-caps-16-600-smart
  font   fn= smart_font ("sys-chinese", "rm", "bold", "small-caps", 16, 600);
  string c = utf8_to_cork ("中");
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  int fn_index= fn_rep->resolve (c, "cjk=" * default_chinese_font_name (), 1);
  QCOMPARE (fn_index, 2);
}

void
TestSmartFont::test_resolve_chinese_puncts () {
  // sys-chinese-rm-medium-right-10-600-smart
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  auto   puncts= array<string> ("<#2018>", "<#2019>", // Chinese: 单引号
                                "<#201C>", "<#201D>"  // Chinese: 双引号
    );
  string cjk_font_name;
#if defined(OS_WIN32) || defined(OS_WIN)
  cjk_font_name= "SimSun";
#else
  // Linux and other platforms
  cjk_font_name= "Noto CJK SC";
#endif

  for (int i= 0; i < N (puncts); i++) {
    int fn_index= fn_rep->resolve (puncts[i], "cjk=" * cjk_font_name, 1);
    QCOMPARE (fn_index, 2);
  }
}

void
TestSmartFont::test_get_right_slope () {
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  QCOMPARE (fn_rep->get_right_slope (utf8_to_cork ("典")), 0.0);

  fn    = smart_font ("sys-chinese", "rm", "bold", "right", 10, 600);
  fn_rep= (smart_font_rep*) fn.rep;
  QCOMPARE (fn_rep->get_right_slope (utf8_to_cork ("典")), 0.0);
}

QTEST_MAIN (TestSmartFont)
#include "smart_font_test.moc"
