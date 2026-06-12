
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
  void test_resolve_200B ();
  void test_get_right_slope ();
  void test_latin_modern_math_italic_greek ();
  void test_cursor_position_iii ();
  void test_performance ();
  void test_math_performance ();
  void test_in_unicode_range_cyrillic ();
  void test_roman_cyrillic_fallback ();
  void test_sys_chinese_cyrillic ();
  void test_dingbats_font_support ();
  void test_dingbats_emoji_fallback ();
  void test_noto_sans_symbols_font_support ();
  void test_noto_sans_symbols2_font_support ();
  void test_misc_symbols_fallback ();
  void test_misc_symbols_fallback_to_symbols2 ();
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

  int nr_0x17= fn_rep->resolve (string ((char) 0x17));
  qcompare (fn_rep->fn[nr_0x17]->res_name, "ec:ecrm10@600");
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
  auto   puncts       = array<string> ("<#2018>", "<#2019>", // Chinese: 单引号
                                       "<#201C>", "<#201D>"  // Chinese: 双引号
           );
  string cjk_font_name= "Noto CJK SC";

  for (int i= 0; i < N (puncts); i++) {
    int fn_index= fn_rep->resolve (puncts[i], "cjk=" * cjk_font_name, 1);
    QCOMPARE (fn_index, 2);
  }
}

void
TestSmartFont::test_resolve_200B () {
  // sys-chinese-rm-medium-right-10-600-smart
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep       = (smart_font_rep*) fn.rep;
  string          cjk_font_name= "Noto CJK SC";

  // U+200B 零宽空格应该被解析到 CJK 字体
  int fn_index= fn_rep->resolve ("<#200B>", "cjk=" * cjk_font_name, 1);
  QCOMPARE (fn_index, 2);
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

void
TestSmartFont::test_latin_modern_math_italic_greek () {
  font fn=
      smart_font ("Latin Modern Math", "rm", "medium", "mathitalic", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;

  int    pos;
  string r;
  int    nr;

  // Test alpha in mathitalic mode should map to math italic alpha (U+1D6FC)
  pos= 0;
  fn_rep->advance ("<alpha>", pos, r, nr);
  QCOMPARE (pos, N (string ("<alpha>")));
  qcompare (r, "<#1D6FC>");

  // Test beta in mathitalic mode should map to math italic beta (U+1D6FD)
  pos= 0;
  fn_rep->advance ("<beta>", pos, r, nr);
  QCOMPARE (pos, N (string ("<beta>")));
  qcompare (r, "<#1D6FD>");

  // Test gamma in mathitalic mode should map to math italic gamma (U+1D6FE)
  pos= 0;
  fn_rep->advance ("<gamma>", pos, r, nr);
  QCOMPARE (pos, N (string ("<gamma>")));
  qcompare (r, "<#1D6FE>");
}

void
TestSmartFont::test_cursor_position_iii () {
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);

  string s= "IIIIIIIIII";
  metric ex;
  fn->get_extents (s, ex);

  STACK_NEW_ARRAY (xpos, SI, N (s) + 1);
  fn->get_xpositions (s, xpos);

  // Check consistency: get_extents of prefix should match xpos
  for (int l= 1; l <= N (s); l++) {
    metric ex2;
    fn->get_extents (s (0, l), ex2);
    QCOMPARE (ex2->x2, xpos[l]);
  }
  STACK_DELETE_ARRAY (xpos);
}

void
TestSmartFont::test_performance () {
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);

  // Trigger a lot of character resolutions with repeated characters
  string long_text= "The quick brown fox jumps over the lazy dog. "
                    "The quick brown fox jumps over the lazy dog. "
                    "The quick brown fox jumps over the lazy dog.";
  metric ex;
  fn->get_extents (long_text, ex);
}

void
TestSmartFont::test_math_performance () {
  font fn=
      smart_font ("Latin Modern Math", "rm", "medium", "mathitalic", 10, 600);

  // Trigger math character resolutions
  string math_text= "<alpha><beta><gamma><delta><epsilon><zeta><eta><theta>"
                    "<iota><kappa><lambda><mu><nu><xi><omicron><pi><rho><sigma>"
                    "<tau><upsilon><phi><chi><psi><omega>";
  metric ex;
  fn->get_extents (math_text, ex);
}

void
TestSmartFont::test_in_unicode_range_cyrillic () {
  // Cyrillic 字符不应该被当作 CJK
  QVERIFY (!in_unicode_range ("<#400>", "cjk"));
  QVERIFY (!in_unicode_range ("<#4FF>", "cjk"));
  // Cyrillic 字符应该属于 cyrillic range
  QVERIFY (in_unicode_range ("<#400>", "cyrillic"));
  QVERIFY (in_unicode_range ("<#4FF>", "cyrillic"));
  // Latin 字符不应该属于 cyrillic range
  QVERIFY (!in_unicode_range ("a", "cyrillic"));
}

void
TestSmartFont::test_roman_cyrillic_fallback () {
  // roman 文档中的 Cyrillic 字符应该 fallback 到 default_chinese_font_name
  font            fn= smart_font ("roman", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  string          c     = utf8_to_cork ("А");
  int             nr    = fn_rep->resolve (c);
  QVERIFY (nr >= 0);
  string chinese_name= default_chinese_font_name ();
  if (chinese_name != "roman") {
    // 确认没有 fallback 到 roman 的 ecrm 字体
    QVERIFY (!occurs ("ecrm", fn_rep->fn[nr]->res_name));
  }
}

void
TestSmartFont::test_sys_chinese_cyrillic () {
  // sys-chinese 文档中的 Cyrillic 字符应该正确路由
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  string          c     = utf8_to_cork ("А");
  int             nr    = fn_rep->resolve (c);
  QVERIFY (nr >= 0);
  string chinese_name= default_chinese_font_name ();
  if (chinese_name != "roman") {
    // 确认没有 fallback 到 roman 的 ecrm 字体
    QVERIFY (!occurs ("ecrm", fn_rep->fn[nr]->res_name));
  }
}

void
TestSmartFont::test_dingbats_font_support () {
  // U+2700-U+27BF (Dingbats) 被归类为 emoji range
  QVERIFY (in_unicode_range ("<#2700>", "emoji"));
  QVERIFY (in_unicode_range ("<#27BF>", "emoji"));

  // closest_font("DejaVu Sans") 不应被替换成 DejaVu Serif
  font dejavu_fn=
      closest_font ("DejaVu Sans", "rm", "medium", "right", 10, 600, 1);
  QVERIFY (!is_nil (dejavu_fn));
  QVERIFY (!occurs ("Serif", dejavu_fn->res_name));
  QVERIFY (dejavu_fn->supports ("<#2702>"));
  QVERIFY (dejavu_fn->supports ("<#2717>"));
}

void
TestSmartFont::test_dingbats_emoji_fallback () {
  // U+2717 (✗) 在 Noto Color Emoji 中不存在，应 fallback 到 NotoSansSymbols2
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  int             nr    = fn_rep->resolve ("<#2717>");
  QVERIFY (nr >= 0);
  // 不应路由到 error 字体
  QVERIFY (!occurs ("error", fn_rep->fn[nr]->res_name));
  // 应路由到 NotoSansSymbols2
  QVERIFY (occurs ("NotoSansSymbols2", fn_rep->fn[nr]->res_name));
}

void
TestSmartFont::test_noto_sans_symbols_font_support () {
  // NotoSansSymbols 应该可以正确加载
  font noto_fn=
      closest_font ("Noto Sans Symbols", "rm", "medium", "right", 10, 600, 1);
  QVERIFY (!is_nil (noto_fn));
  // U+269D (⚝) 由 NotoSansSymbols 支持
  QVERIFY (noto_fn->supports ("<#269D>"));
}

void
TestSmartFont::test_noto_sans_symbols2_font_support () {
  // NotoSansSymbols2 应该可以正确加载
  font noto2_fn=
      closest_font ("Noto Sans Symbols2", "rm", "medium", "right", 10, 600, 1);
  QVERIFY (!is_nil (noto2_fn));
  // U+26BF (≛) 由 NotoSansSymbols2 支持
  QVERIFY (noto2_fn->supports ("<#26BF>"));
}

void
TestSmartFont::test_misc_symbols_fallback () {
  // U+269D (⚝) 应 fallback 到 NotoSansSymbols（第一层）
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  int             nr    = fn_rep->resolve ("<#269D>");
  QVERIFY (nr >= 0);
  // 不应路由到 error 字体
  QVERIFY (!occurs ("error", fn_rep->fn[nr]->res_name));
  // 应路由到 NotoSansSymbols
  QVERIFY (occurs ("NotoSansSymbols", fn_rep->fn[nr]->res_name));
}

void
TestSmartFont::test_misc_symbols_fallback_to_symbols2 () {
  // U+26BF (≛) 不在 NotoSansSymbols 中，应 fallback 到
  // NotoSansSymbols2（第二层）
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;
  int             nr    = fn_rep->resolve ("<#26BF>");
  QVERIFY (nr >= 0);
  // 不应路由到 error 字体
  QVERIFY (!occurs ("error", fn_rep->fn[nr]->res_name));
  // 应路由到 NotoSansSymbols2
  QVERIFY (occurs ("NotoSansSymbols2", fn_rep->fn[nr]->res_name));
}

QTEST_MAIN (TestSmartFont)
#include "smart_font_test.moc"
