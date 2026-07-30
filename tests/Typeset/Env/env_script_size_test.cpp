
/******************************************************************************
 * MODULE     : env_script_size_test.cpp
 * DESCRIPTION: behavior-locking tests for edit_env_rep::get_script_size
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "sys_utils.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>
#include <cmath>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using moebius::drd::std_drd;

class TestEnvScriptSize : public QObject {
  Q_OBJECT

private:
  drd_info*             drd;
  hashmap<string, tree> h1, h2, h3, h4, h5, h6;

  bool     close (double a, double b) { return fabs (a - b) < 1e-9; }
  edit_env make_env ();
  edit_env make_env (tree sizes);

private slots:
  void initTestCase ();
  void default_table ();
  void normalize_half_multiple ();
  void tuple_all_table ();
  void tuple_exact_size_table ();
  void cache_growth_and_reuse ();
};

void
TestEnvScriptSize::initTestCase () {
  init_lolly ();
  init_texmacs_home_path ();
  cache_initialize ();
  init_tex ();
  moebius::drd::init_std_drd ();
  drd= tm_new<drd_info> ("none", std_drd);
}

edit_env
TestEnvScriptSize::make_env () {
  return edit_env (*drd, "none", h1, h2, h3, h4, h5, h6);
}

edit_env
TestEnvScriptSize::make_env (tree sizes) {
  edit_env env        = make_env ();
  env->math_font_sizes= sizes;
  env->size_cache     = array<array<double>> ();
  return env;
}

void
TestEnvScriptSize::default_table () {
  edit_env env= make_env ("default");
  double   s1 = (10.0 * 2.0 + 2.0) / 3.0;
  double   s2 = (s1 * 2.0 + 2.0) / 3.0;
  QVERIFY (close (env->get_script_size (10.0, 0), 10.0));
  QVERIFY (close (env->get_script_size (10.0, 1), s1));
  QVERIFY (close (env->get_script_size (10.0, 2), s2));
  // level 超出缓存级别数时回退到最后一级
  QVERIFY (close (env->get_script_size (10.0, 3), s2));
  QVERIFY (close (env->get_script_size (10.0, 100), s2));
}

void
TestEnvScriptSize::normalize_half_multiple () {
  edit_env env= make_env ("default");
  QVERIFY (close (env->get_script_size (10.3, 0), 10.5));
  QVERIFY (close (env->get_script_size (10.24, 0), 10.0));
  QVERIFY (close (env->get_script_size (10.26, 0), 10.5));
}

void
TestEnvScriptSize::tuple_all_table () {
  edit_env env= make_env (tree (TUPLE, tree (TUPLE, tree ("all"), tree ("*0.8"),
                                             tree ("*0.6"), tree ("7"))));
  QVERIFY (close (env->get_script_size (10.0, 0), 10.0));
  QVERIFY (close (env->get_script_size (10.0, 1), 8.0));
  QVERIFY (close (env->get_script_size (10.0, 2), 6.0));
  QVERIFY (close (env->get_script_size (10.0, 3), 7.0));
  QVERIFY (close (env->get_script_size (10.0, 4), 7.0));
  QVERIFY (close (env->get_script_size (12.0, 1), 10.0));
}

void
TestEnvScriptSize::tuple_exact_size_table () {
  edit_env env= make_env (tree (TUPLE, tree (TUPLE, tree ("12"), tree ("*0.5")),
                                tree (TUPLE, tree ("all"), tree ("*0.7"))));
  // sz=12 命中精确尺寸条目；sz=10 落到 all 条目
  QVERIFY (close (env->get_script_size (12.0, 1), 6.0));
  QVERIFY (close (env->get_script_size (10.0, 1), 7.0));
}

void
TestEnvScriptSize::cache_growth_and_reuse () {
  edit_env env= make_env ("default");
  // isz=20：填充索引 0..20
  QVERIFY (close (env->get_script_size (10.0, 0), 10.0));
  QCOMPARE (N (env->size_cache), 21);
  // 已缓存的尺寸不再增长
  double s1= (5.0 * 2.0 + 2.0) / 3.0;
  QVERIFY (close (env->get_script_size (5.0, 1), s1));
  QCOMPARE (N (env->size_cache), 21);
  // 半尺寸 10.5 -> isz=21，缓存继续增长
  QVERIFY (close (env->get_script_size (10.5, 0), 10.5));
  QCOMPARE (N (env->size_cache), 22);
  // 重复调用结果一致
  QVERIFY (
      close (env->get_script_size (10.0, 1), env->get_script_size (10.0, 1)));
}

QTEST_MAIN (TestEnvScriptSize)
#include "env_script_size_test.moc"
