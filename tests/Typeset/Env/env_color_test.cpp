/******************************************************************************
 * MODULE     : env_color_test.cpp
 * DESCRIPTION: Unit tests for edit_env text background color caching
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Concat/concater.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "font.hpp"
#include "sys_utils.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using moebius::drd::std_drd;

static edit_env
create_test_env () {
  drd_info              drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  edit_env              env= edit_env (drd, "none", h1, h2, h3, h4, h5, h6);
  env->write (COLOR, "none");
  env->write (FILL_COLOR, "none");
  env->write (OPACITY, "1.0");
  return env;
}

class TestEnvColor : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();

    tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
    tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
    font_rule (which, by);
  }
  void test_default_text_bg_color ();
  void test_valid_text_bg_color ();
  void test_white_text_bg_color ();
  void test_empty_text_bg_color ();
  void test_text_bg_color_with_alpha ();
};

void
TestEnvColor::test_default_text_bg_color () {
  edit_env env= create_test_env ();
  env->update_color ();

  QCOMPARE (env->has_text_bg_color, false);
  color c= get_env_bg_color (env);
  QCOMPARE (c, rgb_color (0, 0, 0, 0));
}

void
TestEnvColor::test_valid_text_bg_color () {
  edit_env env= create_test_env ();

  // 设置黄色文本背景色
  env->write (TEXT_BG_COLOR, tree ("yellow"));
  env->update_color ();

  QCOMPARE (env->has_text_bg_color, true);
  color expected= named_color ("yellow", env->alpha);
  QCOMPARE (env->text_bg_color, expected);
  QCOMPARE (get_env_bg_color (env), expected);

  // 设置十六进制颜色 #FF0000 (纯红)
  env->write (TEXT_BG_COLOR, tree ("#FF0000"));
  env->update_color ();

  QCOMPARE (env->has_text_bg_color, true);
  expected= named_color ("#FF0000", env->alpha);
  QCOMPARE (env->text_bg_color, expected);
  QCOMPARE (get_env_bg_color (env), expected);
}

void
TestEnvColor::test_white_text_bg_color () {
  edit_env env= create_test_env ();

  // "white" 背景色视为无文字背景（透明）
  env->write (TEXT_BG_COLOR, tree ("white"));
  env->update_color ();

  QCOMPARE (env->has_text_bg_color, false);
  QCOMPARE (get_env_bg_color (env), rgb_color (0, 0, 0, 0));
}

void
TestEnvColor::test_empty_text_bg_color () {
  edit_env env= create_test_env ();

  // 先设置一个有效颜色
  env->write (TEXT_BG_COLOR, tree ("green"));
  env->update_color ();
  QCOMPARE (env->has_text_bg_color, true);

  // 随后清空
  env->write (TEXT_BG_COLOR, tree (""));
  env->update_color ();

  QCOMPARE (env->has_text_bg_color, false);
  QCOMPARE (get_env_bg_color (env), rgb_color (0, 0, 0, 0));
}

void
TestEnvColor::test_text_bg_color_with_alpha () {
  edit_env env= create_test_env ();

  env->write (OPACITY, tree ("50%"));
  env->write (TEXT_BG_COLOR, tree ("blue"));
  env->update_color ();

  QCOMPARE (env->has_text_bg_color, true);
  color expected= named_color ("blue", env->alpha);
  QCOMPARE (env->text_bg_color, expected);
  QCOMPARE (get_env_bg_color (env), expected);
}

QTEST_MAIN (TestEnvColor)
#include "env_color_test.moc"
