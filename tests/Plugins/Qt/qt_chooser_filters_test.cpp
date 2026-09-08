/******************************************************************************
 * MODULE     : qt_chooser_filters_test.cpp
 * DESCRIPTION: 另存为对话框纯函数决策逻辑测试（任务 1279：继任格式引导）
 * COPYRIGHT  : (C) 2026  MoganLab
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_chooser_widget.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestQtChooserFilters : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // .ts 另存为改写为 .stem（继任格式），目标后缀 stem
  void test_rewrite_ts () {
    string f= "foo.ts";
    QVERIFY (chooser_save_as_target (f) == "stem");
    QVERIFY (f == "foo.stem");
    string g= "a.dir/foo.ts";
    QVERIFY (chooser_save_as_target (g) == "stem");
    QVERIFY (g == "a.dir/foo.stem");
  }

  // .stem 本就是默认格式：保持不变但属于引导场景
  void test_rewrite_stem () {
    string f= "foo.stem";
    QVERIFY (chooser_save_as_target (f) == "stem");
    QVERIFY (f == "foo.stem");
  }

  // .tm 另存为改写为 .tmu；.stm/.tmu 不得误伤
  void test_rewrite_tm () {
    string f= "foo.tm";
    QVERIFY (chooser_save_as_target (f) == "tmu");
    QVERIFY (f == "foo.tmu");
    string g= "foo.stm";
    QVERIFY (chooser_save_as_target (g) == "");
    QVERIFY (g == "foo.stm");
    string h= "foo.tmu";
    QVERIFY (chooser_save_as_target (h) == "");
  }

  // 其余后缀与无后缀（草稿）不触发改写
  void test_rewrite_no_match () {
    string empty= "";
    QVERIFY (chooser_save_as_target (empty) == "");
    string bar= "bar";
    QVERIFY (chooser_save_as_target (bar) == "");
    string tex= "foo.tex";
    QVERIFY (chooser_save_as_target (tex) == "");
    string tp= "foo.tp";
    QVERIFY (chooser_save_as_target (tp) == "");
  }

  // 样式另存的成对过滤器：stem（默认）在前，ts 在后，无其他格式
  void test_style_filters () {
    QStringList filters= chooser_style_filters ();
    QCOMPARE (filters.size (), 2);
    QVERIFY (filters[0].contains ("*.stem"));
    QVERIFY (filters[0].contains ("STEM"));
    QVERIFY (!filters[0].contains ("*.tmu"));
    QVERIFY (filters[1].contains ("*.ts"));
  }

  // 从过滤器括号内容解析首个后缀；非法形状返回空串
  void test_filter_first_suffix () {
    QCOMPARE (chooser_filter_first_suffix ("STEM files (*.stem)"),
              QString ("stem"));
    QCOMPARE (chooser_filter_first_suffix ("TM files (*.tm)"), QString ("tm"));
    QCOMPARE (chooser_filter_first_suffix ("TMU files (*.tmu)"),
              QString ("tmu"));
    QCOMPARE (chooser_filter_first_suffix ("X (*.tmu *.stem)"),
              QString ("tmu"));
    QCOMPARE (chooser_filter_first_suffix ("All (*)"), QString (""));
    QCOMPARE (chooser_filter_first_suffix ("no parens"), QString (""));
    QCOMPARE (chooser_filter_first_suffix ("X (foo)"), QString (""));
  }

  // 最终文件名规范：去旧后缀再接所选后缀；非文件路径原样返回
  void test_normalize_suffix () {
    QCOMPARE (chooser_normalize_suffix ("/tmp/foo.ts", "stem"),
              QString ("/tmp/foo.stem"));
    QCOMPARE (chooser_normalize_suffix ("/tmp/foo.stem", "tm"),
              QString ("/tmp/foo.tm"));
    QCOMPARE (chooser_normalize_suffix ("/tmp/bar", "tmu"),
              QString ("/tmp/bar.tmu"));
    QCOMPARE (chooser_normalize_suffix ("/tmp/a.b/foo.md", "stem"),
              QString ("/tmp/a.b/foo.stem"));
    QCOMPARE (chooser_normalize_suffix ("/tmp/foo.ts", ""),
              QString ("/tmp/foo.ts"));
    QCOMPARE (chooser_normalize_suffix ("foo.ts", "stem"), QString ("foo.ts"));
    QCOMPARE (chooser_normalize_suffix ("/tmp/x/", "stem"),
              QString ("/tmp/x/"));
  }
};

#ifdef QTTEXMACS
QTEST_MAIN (TestQtChooserFilters)
#else
int
main () {
  return 0;
}
#endif
#include "qt_chooser_filters_test.moc"
