/******************************************************************************
 * MODULE     : matrix_bracket_rendering_test.cpp
 * DESCRIPTION: Tests for continuous rendering of matrix brackets / delimiters
 ******************************************************************************/

#include "Metafont/load_tex.hpp"
#include "Qt/qt_renderer.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "font.hpp"
#include "smart_font.hpp"
#include "tm_sys_utils.hpp"

#include <QImage>
#include <QPainter>
#include <QtTest/QtTest>

class TestMatrixBracketRendering : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
  }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_cmex_bracket_continuity ();
  void test_cmex_paren_continuity ();
  void test_unicode_bracket_glyph_continuity ();
  void test_unicode_paren_glyph_continuity ();
  void test_unicode_canvas_continuity ();
};

static int
count_column_gaps (const QImage& canvas) {
  int img_w= canvas.width ();
  int img_h= canvas.height ();

  int best_col= -1, max_pixels= 0;
  for (int x= 0; x < img_w; x++) {
    int cnt= 0;
    for (int y= 0; y < img_h; y++) {
      if (qAlpha (canvas.pixel (x, y)) > 0) cnt++;
    }
    if (cnt > max_pixels) {
      max_pixels= cnt;
      best_col  = x;
    }
  }
  if (best_col < 0) return -1; // nothing drawn

  int first_y= -1, last_y= -1;
  for (int y= 0; y < img_h; y++) {
    if (qAlpha (canvas.pixel (best_col, y)) > 0) {
      if (first_y == -1) first_y= y;
      last_y= y;
    }
  }

  int gap_count= 0;
  for (int y= first_y; y <= last_y; y++) {
    if (qAlpha (canvas.pixel (best_col, y)) == 0) {
      gap_count++;
    }
  }
  return gap_count;
}

void
TestMatrixBracketRendering::test_cmex_bracket_continuity () {
  font trfn= tex_rubber_font ("rubber-cmex", "cmex", 10, 600, 10);
  QVERIFY (!is_nil (trfn));

  int    img_w= 100, img_h= 300;
  QImage canvas (img_w, img_h, QImage::Format_ARGB32_Premultiplied);
  canvas.fill (Qt::transparent);

  QPainter        painter (&canvas);
  qt_renderer_rep ren (&painter, img_w, img_h);
  ren.set_zoom_factor (1.0);
  ren.set_origin (0, 0);
  ren.set_clipping (0, -300 * 256 * 5, 100 * 256 * 5, 0);
  ren.set_pencil (pencil (rgb_color (0, 0, 0, 255)));

  trfn->draw_fixed (&ren, "<left-[-6>", 20 * 1280, -30 * 1280);
  painter.end ();

  int gaps= count_column_gaps (canvas);
  QCOMPARE (gaps, 0);
}

void
TestMatrixBracketRendering::test_cmex_paren_continuity () {
  font trfn= tex_rubber_font ("rubber-cmex", "cmex", 10, 600, 10);
  QVERIFY (!is_nil (trfn));

  int    img_w= 100, img_h= 300;
  QImage canvas (img_w, img_h, QImage::Format_ARGB32_Premultiplied);
  canvas.fill (Qt::transparent);

  QPainter        painter (&canvas);
  qt_renderer_rep ren (&painter, img_w, img_h);
  ren.set_zoom_factor (1.0);
  ren.set_origin (0, 0);
  ren.set_clipping (0, -300 * 256 * 5, 100 * 256 * 5, 0);
  ren.set_pencil (pencil (rgb_color (0, 0, 0, 255)));

  trfn->draw_fixed (&ren, "<left-(-6>", 20 * 1280, -30 * 1280);
  painter.end ();

  int gaps= count_column_gaps (canvas);
  QCOMPARE (gaps, 0);
}

void
TestMatrixBracketRendering::test_unicode_bracket_glyph_continuity () {
  font lm=
      smart_font ("Latin Modern Math", "rm", "medium", "mathitalic", 10, 600);
  QVERIFY (!is_nil (lm));

  glyph gl= lm->get_glyph ("<left-[-11>");
  QVERIFY (!is_nil (gl));
  QVERIFY (gl->height > 0);

  int empty_rows= 0;
  for (int j= 0; j < gl->height; j++) {
    bool has_pixel= false;
    for (int i= 0; i < gl->width; i++) {
      if (gl->get_x (i, j) > 0) {
        has_pixel= true;
        break;
      }
    }
    if (!has_pixel) empty_rows++;
  }
  QCOMPARE (empty_rows, 0);
}

void
TestMatrixBracketRendering::test_unicode_paren_glyph_continuity () {
  font lm=
      smart_font ("Latin Modern Math", "rm", "medium", "mathitalic", 10, 600);
  QVERIFY (!is_nil (lm));

  glyph gl= lm->get_glyph ("<left-(-11>");
  QVERIFY (!is_nil (gl));
  QVERIFY (gl->height > 0);

  int empty_rows= 0;
  for (int j= 0; j < gl->height; j++) {
    bool has_pixel= false;
    for (int i= 0; i < gl->width; i++) {
      if (gl->get_x (i, j) > 0) {
        has_pixel= true;
        break;
      }
    }
    if (!has_pixel) empty_rows++;
  }
  QCOMPARE (empty_rows, 0);
}

void
TestMatrixBracketRendering::test_unicode_canvas_continuity () {
  font lm=
      smart_font ("Latin Modern Math", "rm", "medium", "mathitalic", 10, 600);
  QVERIFY (!is_nil (lm));

  int    img_w= 100, img_h= 300;
  QImage canvas (img_w, img_h, QImage::Format_ARGB32_Premultiplied);
  canvas.fill (Qt::transparent);

  QPainter        painter (&canvas);
  qt_renderer_rep ren (&painter, img_w, img_h);
  ren.set_zoom_factor (1.0);
  ren.set_origin (0, 0);
  ren.set_clipping (0, -300 * 256 * 5, 100 * 256 * 5, 0);
  ren.set_pencil (pencil (rgb_color (0, 0, 0, 255)));

  lm->draw_fixed (&ren, "<left-(-11>", 20 * 1280, -30 * 1280);
  painter.end ();

  int gaps= count_column_gaps (canvas);
  QCOMPARE (gaps, 0);
}

#ifdef QTTEXMACS
QTEST_MAIN (TestMatrixBracketRendering)
#else
int
main () {
  return 0;
}
#endif
#include "matrix_bracket_rendering_test.moc"
