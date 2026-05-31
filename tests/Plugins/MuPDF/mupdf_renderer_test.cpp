
/******************************************************************************
 * MODULE     : mupdf_renderer_test.cpp
 * DESCRIPTION: Memory leak regression test for mupdf_renderer::render path
 * COPYRIGHT  : (C) 2026  Darcy Shen
 ******************************************************************************/

#include "MuPDF/mupdf_renderer.hpp"
#include "base.hpp"
#include "qt_ui_element.hpp"
#include "tm_memory.hpp"
#include <QtTest/QtTest>

class TestMupdfRenderer : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void test_render_does_not_leak ();
};

void
TestMupdfRenderer::test_render_does_not_leak () {
#ifndef __linux__
  QSKIP ("RSS-based leak test is Linux-only");
#endif

  // Create a minimal glue widget to exercise the render() path
  qt_glue_widget_rep* wid= new qt_glue_widget_rep (tree (""), false, false,
                                                   100 * PIXEL, 100 * PIXEL);

  // Warm up: first call may allocate caches
  {
    QTMPixmapOrImage pm= wid->render ();
    (void) pm;
  }

  long before= get_rss ();

  // Repeatedly render the same widget
  const int iterations= 2000;
  for (int i= 0; i < iterations; i++) {
    QTMPixmapOrImage pm= wid->render ();
    (void) pm;
  }

  long after= get_rss ();

  delete wid;

  long delta_kb= after - before;
  // Allow some RSS noise; a real leak of ~40 KB per iteration would
  // produce ~80 MB after 2000 iterations, so 5 MB is a safe ceiling.
  QVERIFY2 (delta_kb < 5120,
            qPrintable (QString ("RSS grew by %1 KB after %2 iterations, "
                                 "indicating a memory leak")
                            .arg (delta_kb)
                            .arg (iterations)));
}

QTEST_MAIN (TestMupdfRenderer)
#include "mupdf_renderer_test.moc"
