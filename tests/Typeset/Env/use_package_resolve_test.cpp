/******************************************************************************
 * MODULE     : use_package_resolve_test.cpp
 * DESCRIPTION: 带路径包名 <use-package|a/b/c> 的解析回归测试
 * COPYRIGHT  : (C) 2026  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "file.hpp"
#include "sys_utils.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>
#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius; // USE_PACKAGE
using moebius::drd::init_std_drd;
using moebius::drd::std_drd;

// edit_env 按引用持有 drd 和各 hashmap，必须与 env 同生命周期
struct test_env {
  drd_info              drd;
  hashmap<string, tree> lref, gref, laux, gaux, latt, gatt;
  edit_env              env;

  test_env (url base_file_name)
      : drd ("test", std_drd),
        env (drd, base_file_name, lref, gref, laux, gaux, latt, gatt) {}
};

static url the_home= url_none ();
static url the_doc = url_none ();

static void
write_pack (url file, string body) {
  make_dir (head (file));
  string doc= string ("<TeXmacs|2.1.5>\n\n<style|source>\n\n<\\body>\n  ") *
              body * string ("\n</body>\n");
  QVERIFY (!save_string (file, doc, false));
}

static void
set_roots (url package_root, url style_root) {
  set_env ("TEXMACS_PACKAGE_ROOT", as_string (package_root));
  set_env ("TEXMACS_STYLE_ROOT", as_string (style_root));
}

class TestUsePackageResolve : public QObject {
  Q_OBJECT

private slots:
  void initTestCase ();
  void init ();
  void cleanupTestCase ();

  void nested_user_package ();
  void document_relative_package ();
  void home_plugin_package ();
  void style_root_package ();
  void package_root_wins_over_style_root ();
  void bare_package_name_still_works ();
  void missing_package_is_silent ();
};

void
TestUsePackageResolve::initTestCase () {
  // edit_env 的构造会走到 update_font，字体栈没起来会抛异常，
  // 所以先初始化 TeX/字体环境，之后再改写 TEXMACS_HOME_PATH
  init_lolly ();
  init_texmacs_home_path ();
  cache_initialize ();
  init_tex ();
  init_std_drd ();

  the_home= url_temp ();
  the_doc = url_temp ();

  // issue #4453 复现用的用户包：入口包在 TST-core/，两个子包在
  // TST-core/unicode/
  url packages= the_home * "packages";
  write_pack (packages * "TST-core" * "unicode" * "unicode-figures.ts",
              "<assign|unicode-figures-marker|<macro|figures>>");
  write_pack (packages * "TST-core" * "unicode" * "unicode-environments.ts",
              "<assign|unicode-environments-marker|<macro|environments>>");
  write_pack (packages * "TST-core" * "unicode-core.ts",
              "<use-package|TST-core/unicode/unicode-figures>\n"
              "  <use-package|TST-core/unicode/unicode-environments>\n"
              "  <assign|unicode-core-marker|<macro|core>>");

  // 不带路径的包名，走未改动的 $TEXMACS_STYLE_PATH 分支
  write_pack (packages * "solo.ts", "<assign|solo-marker|<macro|solo>>");

  // package 根与 style 根同名同路径，用于验证根的先后顺序
  write_pack (packages * "over" * "pack.ts",
              "<assign|over-from-packages|<macro|packages>>");
  write_pack (the_home * "styles" * "over" * "pack.ts",
              "<assign|over-from-styles|<macro|styles>>");
  write_pack (the_home * "styles" * "TST-style" * "style-pack.ts",
              "<assign|style-pack-marker|<macro|style>>");

  // $TEXMACS_HOME_PATH 下的插件包
  write_pack (the_home * "plugins" * "tstplug" * "packages" * "tstplug" *
                  "deep" * "plug-pack.ts",
              "<assign|plug-pack-marker|<macro|plug>>");

  // 文档旁边的包，文档本身放在下一层，同时覆盖祖先目录上溯
  write_pack (the_doc * "relative" * "rel-pack.ts",
              "<assign|rel-pack-marker|<macro|rel>>");

  set_env ("TEXMACS_HOME_PATH", as_string (the_home));
}

void
TestUsePackageResolve::init () {
  // 每个用例自己声明搜索根，避免相互影响
  set_env ("TEXMACS_PACKAGE_ROOT", "");
  set_env ("TEXMACS_STYLE_ROOT", "");
  set_env ("TEXMACS_STYLE_PATH", "");
}

void
TestUsePackageResolve::cleanupTestCase () {
  if (!is_none (the_home)) rmdir (the_home);
  if (!is_none (the_doc)) rmdir (the_doc);
}

// issue #4453：只加 unicode-core，两个子包必须级联加载
void
TestUsePackageResolve::nested_user_package () {
  set_roots (url ("$TEXMACS_HOME_PATH/packages"),
             url ("$TEXMACS_HOME_PATH/styles"));

  test_env te (url ("$PWD/none"));
  te.env->exec (tree (USE_PACKAGE, string ("TST-core/unicode-core")));

  QVERIFY (te.env->provides ("unicode-core-marker"));
  QVERIFY (te.env->provides ("unicode-figures-marker"));
  QVERIFY (te.env->provides ("unicode-environments-marker"));
}

// 文档在 <doc>/sub/，包在 <doc>/relative/，靠祖先目录上溯找到
void
TestUsePackageResolve::document_relative_package () {
  set_roots (url ("$TEXMACS_HOME_PATH/packages"),
             url ("$TEXMACS_HOME_PATH/styles"));

  test_env te (the_doc * "sub" * "doc.tm");
  te.env->exec (tree (USE_PACKAGE, string ("relative/rel-pack")));

  QVERIFY (te.env->provides ("rel-pack-marker"));
}

// $TEXMACS_HOME_PATH/plugins/<插件>/packages 下的包
void
TestUsePackageResolve::home_plugin_package () {
  set_roots (url ("$TEXMACS_HOME_PATH/packages") |
                 url ("$TEXMACS_HOME_PATH/plugins/tstplug/packages"),
             url ("$TEXMACS_HOME_PATH/styles"));

  test_env te (url ("$PWD/none"));
  te.env->exec (tree (USE_PACKAGE, string ("tstplug/deep/plug-pack")));

  QVERIFY (te.env->provides ("plug-pack-marker"));
}

// 带路径包名也要能落到 style 根上
void
TestUsePackageResolve::style_root_package () {
  set_roots (url ("$TEXMACS_HOME_PATH/packages"),
             url ("$TEXMACS_HOME_PATH/styles"));

  test_env te (url ("$PWD/none"));
  te.env->exec (tree (USE_PACKAGE, string ("TST-style/style-pack")));

  QVERIFY (te.env->provides ("style-pack-marker"));
}

// 同名包同时在两个根里时，靠前的根胜出
void
TestUsePackageResolve::package_root_wins_over_style_root () {
  set_roots (url ("$TEXMACS_HOME_PATH/packages"),
             url ("$TEXMACS_HOME_PATH/styles"));

  test_env te (url ("$PWD/none"));
  te.env->exec (tree (USE_PACKAGE, string ("over/pack")));

  QVERIFY (te.env->provides ("over-from-packages"));
  QVERIFY (!te.env->provides ("over-from-styles"));
}

// 不含 / 的包名仍走 $TEXMACS_STYLE_PATH，防回归
void
TestUsePackageResolve::bare_package_name_still_works () {
  set_env ("TEXMACS_STYLE_PATH", as_string (the_home * "packages"));

  test_env te (url ("$PWD/none"));
  te.env->exec (tree (USE_PACKAGE, string ("solo")));

  QVERIFY (te.env->provides ("solo-marker"));
}

// 找不到的包不崩、不定义任何东西
void
TestUsePackageResolve::missing_package_is_silent () {
  set_roots (url ("$TEXMACS_HOME_PATH/packages"),
             url ("$TEXMACS_HOME_PATH/styles"));

  test_env te (url ("$PWD/none"));
  te.env->exec (tree (USE_PACKAGE, string ("no/such/package")));

  QVERIFY (!te.env->provides ("unicode-figures-marker"));
}

QTEST_MAIN (TestUsePackageResolve)
#include "use_package_resolve_test.moc"
