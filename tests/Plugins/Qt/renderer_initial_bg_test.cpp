/******************************************************************************
 * MODULE     : renderer_initial_bg_test.cpp
 * DESCRIPTION: Unit tests for initial background color of renderer and native picture
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "MuPDF/mupdf_picture.hpp"
#include "MuPDF/mupdf_renderer.hpp"
#include "base.hpp"
#include "gui.hpp"
#include "qt_utilities.hpp"

#include <QtTest/QtTest>

class TestRendererInitialBg : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_light_mode_native_picture_bg ();
  void test_dark_mode_native_picture_bg ();
  void test_dark_mode_mupdf_renderer_bg ();
  void test_dark_mode_shadow_renderer_bg ();
};

void
TestRendererInitialBg::test_light_mode_native_picture_bg () {
  string saved_theme= tm_style_sheet;
  tm_style_sheet    = "$TEXMACS_PATH/misc/themes/liii.css";
  tm_background     = rgb_color (160, 160, 160);

  picture p= native_picture (16, 16, 0, 0);
  QVERIFY (!is_nil (p));

  int r, g, b, a;
  get_rgb_color (p->get_pixel (0, 0), r, g, b, a);
  QCOMPARE (r, 255);
  QCOMPARE (g, 255);
  QCOMPARE (b, 255);

  tm_style_sheet= saved_theme;
}

void
TestRendererInitialBg::test_dark_mode_native_picture_bg () {
  string saved_theme= tm_style_sheet;
  tm_style_sheet    = "$TEXMACS_PATH/misc/themes/liii-night.css";
  tm_background     = rgb_color (32, 32, 32);

  picture p= native_picture (16, 16, 0, 0);
  QVERIFY (!is_nil (p));

  int r, g, b, a;
  get_rgb_color (p->get_pixel (0, 0), r, g, b, a);
  QCOMPARE (r, 32);
  QCOMPARE (g, 32);
  QCOMPARE (b, 32);

  tm_style_sheet= saved_theme;
}

void
TestRendererInitialBg::test_dark_mode_mupdf_renderer_bg () {
  string saved_theme= tm_style_sheet;
  tm_style_sheet    = "$TEXMACS_PATH/misc/themes/liii-night.css";
  tm_background     = rgb_color (32, 32, 32);

  mupdf_renderer_rep ren (100, 100);
  QCOMPARE (ren.get_background ()->get_color (), tm_background);

  tm_style_sheet= saved_theme;
}

void
TestRendererInitialBg::test_dark_mode_shadow_renderer_bg () {
  string saved_theme= tm_style_sheet;
  tm_style_sheet    = "$TEXMACS_PATH/misc/themes/liii-night.css";
  tm_background     = rgb_color (32, 32, 32);

  picture  p= native_picture (64, 64, 0, 0);
  renderer ren= picture_renderer (p, 1.0);
  renderer shadow= nullptr;
  ren->new_shadow (shadow);
  QVERIFY (shadow != nullptr);

  QCOMPARE (shadow->get_background ()->get_color (), tm_background);

  ren->delete_shadow (shadow);
  tm_delete (ren);

  tm_style_sheet= saved_theme;
}

#ifdef QTTEXMACS
QTEST_MAIN (TestRendererInitialBg)
#else
int
main () {
  return 0;
}
#endif
#include "renderer_initial_bg_test.moc"
