/******************************************************************************
 * MODULE     : cjk_spacing_test.cpp
 * DESCRIPTION: Unit tests for CJK line printing and spacing optimization
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Boxes/construct.hpp"
#include "Format/line_item.hpp"
#include "Metafont/load_tex.hpp"
#include "Stack/stacker.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using moebius::drd::std_drd;

static edit_env
create_test_cjk_env () {
  drd_info              drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  edit_env              env (drd, "none", h1, h2, h3, h4, h5, h6);
  env->lan= text_language ("chinese");
  return env;
}

class TestCjkSpacing : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
    moebius::drd::init_std_drd ();
  }
  void test_pure_cjk_paragraph_typeset ();
  void test_mixed_cjk_western_typeset ();
};

void
TestCjkSpacing::test_pure_cjk_paragraph_typeset () {
  edit_env     env= create_test_cjk_env ();
  stack_border sb;
  // 纯中文段落树
  tree t (CONCAT, "方源口中叹息，一切都结束了。");

  array<page_item> lines= typeset_stack (env, t, path (0), 500000,
                                         array<line_item> (),
                                         array<line_item> (), sb);

  // 验证排版成功生成行，未发生越界或崩溃
  QVERIFY (N (lines) > 0);
  for (int i= 0; i < N (lines); i++) {
    QVERIFY (lines[i]->type == PAGE_LINE_ITEM ||
             lines[i]->type == PAGE_CONTROL_ITEM);
  }
}

void
TestCjkSpacing::test_mixed_cjk_western_typeset () {
  edit_env     env= create_test_cjk_env ();
  stack_border sb;
  // 中西文混排段落
  tree t (CONCAT, "方源 says hello world 结束。");

  array<page_item> lines= typeset_stack (env, t, path (0), 500000,
                                         array<line_item> (),
                                         array<line_item> (), sb);

  QVERIFY (N (lines) > 0);
  for (int i= 0; i < N (lines); i++) {
    QVERIFY (lines[i]->type == PAGE_LINE_ITEM ||
             lines[i]->type == PAGE_CONTROL_ITEM);
  }
}

QTEST_MAIN (TestCjkSpacing)
#include "cjk_spacing_test.moc"
