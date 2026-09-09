/******************************************************************************
 * MODULE     : gradient_selector_bridge_test.cpp
 * DESCRIPTION: 单元测试 GradientSelectorBridge 以及 glue 入口
 *              cpp_gradient_selector_dialog。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/GradientSelectorBridge.hpp"
#include "Qt/QTMQmlDialog.hpp"
#include "base.hpp"

#include "sys_utils.hpp"
#include "tree_helper.hpp"

#include <QtTest/QtTest>

class TestGradientSelectorBridge : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_cancel_hook_returns_empty ();
  void test_ok_hook_returns_pattern ();
  void test_bridge_properties_and_preview ();
};

void
TestGradientSelectorBridge::test_cancel_hook_returns_empty () {
  EnvHook hook ("MOGAN_TEST_GRADIENT_SELECTOR", "cancel");
  tree    r= cpp_gradient_selector_dialog (tree ());
  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 0);
}

void
TestGradientSelectorBridge::test_ok_hook_returns_pattern () {
  EnvHook hook ("MOGAN_TEST_GRADIENT_SELECTOR", "ok");
  tree    r= cpp_gradient_selector_dialog (tree ());
  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 1);
  QVERIFY (is_func (r[0], moebius::PATTERN, 4));
}

void
TestGradientSelectorBridge::test_bridge_properties_and_preview () {
  QDialog                host;
  GradientSelectorBridge bridge (&host, "vertical-white-black.png", "100%",
                                 "100%", "red", "blue");

  QCOMPARE (bridge.patternName (), QString ("vertical-white-black.png"));
  QCOMPARE (bridge.width (), QString ("100%"));
  QCOMPARE (bridge.height (), QString ("100%"));
  QCOMPARE (bridge.foregroundColor (), QString ("red"));
  QCOMPARE (bridge.backgroundColor (), QString ("blue"));

  // Preview URL must be a non-empty base64 PNG data URL
  QString pUrl= bridge.previewUrl ();
  QVERIFY (pUrl.startsWith ("data:image/png;base64,"));
  QVERIFY (pUrl.length () > 50);

  // Modifying properties must update the preview
  bridge.setForegroundColor ("green");
  QCOMPARE (bridge.foregroundColor (), QString ("green"));
  bridge.setBackgroundColor ("yellow");
  QCOMPARE (bridge.backgroundColor (), QString ("yellow"));
  bridge.setPatternName ("horizontal-white-black.png");
  QCOMPARE (bridge.patternName (), QString ("horizontal-white-black.png"));
  bridge.setWidth ("50%");
  QCOMPARE (bridge.width (), QString ("50%"));
  bridge.setHeight ("50%");
  QCOMPARE (bridge.height (), QString ("50%"));

  QString newPUrl= bridge.previewUrl ();
  QVERIFY (newPUrl.startsWith ("data:image/png;base64,"));
  QVERIFY (newPUrl != pUrl);

  // Labels and options lists must be non-empty
  QVERIFY (bridge.patternOptions ().size () > 0);
  QVERIFY (bridge.colorOptions ().size () > 0);
  QVERIFY (bridge.labels ().contains ("title"));
  QVERIFY (bridge.labels ().contains ("preview"));
}

#ifdef QTTEXMACS
QTEST_MAIN (TestGradientSelectorBridge)
#else
int
main () {
  return 0;
}
#endif
#include "gradient_selector_bridge_test.moc"
