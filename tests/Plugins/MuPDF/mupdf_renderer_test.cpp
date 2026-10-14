
/******************************************************************************
 * MODULE     : mupdf_renderer_test.cpp
 * DESCRIPTION: Memory leak regression test for mupdf_renderer::register_pattern
 * COPYRIGHT  : (C) 2026  Darcy Shen
 ******************************************************************************/

#include "mupdf_renderer.hpp"
#include "base.hpp"
#include "tm_memory.hpp"
#include <QDir>
#include <QImage>
#include <QtTest/QtTest>

extern void del_obj_mupdf_renderer (void);

class test_mupdf_renderer_rep : public mupdf_renderer_rep {
public:
  void test_register_pattern (brush br, SI pixel) {
    register_pattern (br, pixel);
  }
};

class TestMupdfRenderer : public QObject {
  Q_OBJECT

private slots:
  void   init () { init_lolly (); }
  void   test_register_pattern_does_not_leak ();
};

void
TestMupdfRenderer::test_register_pattern_does_not_leak () {
#ifndef __linux__
  QSKIP ("RSS-based leak test is Linux-only");
#endif

  // Create a minimal temporary image to use as pattern
  QString temp_path= QDir::temp ().filePath ("mupdf_test_pattern.png");
  QImage  img (10, 10, QImage::Format_RGB32);
  img.fill (Qt::white);
  QVERIFY (img.save (temp_path));

  QByteArray path_bytes= temp_path.toUtf8 ();
  string     path_str  = string (path_bytes.constData ());
  url        test_url  = url_system (path_str);

  qDebug () << "temp_path:" << temp_path;
  c_string _path_str (path_str);
  qDebug () << "path_str:" << QString::fromUtf8 ((char*) _path_str);
  c_string _test_url_str (as_string (test_url));
  qDebug () << "test_url as_string:" << QString::fromUtf8 ((char*) _test_url_str);
  qDebug () << "is_none:" << is_none (test_url);
  qDebug () << "is_atomic test_url:" << is_atomic (test_url);

  // Prepare renderer with a dummy pixmap
  test_mupdf_renderer_rep* ren= tm_new<test_mupdf_renderer_rep> ();
  fz_context*              ctx= mupdf_context ();
  fz_pixmap* pix= fz_new_pixmap (ctx, fz_device_rgb (ctx), 100, 100, NULL, 1);
  ren->begin (pix);

  // Build a pattern brush that references the temp image
  tree  pattern= tree (moebius::PATTERN, as_string (test_url), "10", "10", "white");
  c_string _pattern_str (as_string (pattern));
  qDebug () << "pattern tree:" << QString::fromUtf8 ((char*) _pattern_str);
  brush br= brush (pattern);
  qDebug () << "brush type:" << br->get_type ();
  c_string _brush_url_str (as_string (br->get_pattern_url ()));
  qDebug () << "brush pattern url:" << QString::fromUtf8 ((char*) _brush_url_str);

  // Warm up: first call loads image into caches
  ren->test_register_pattern (br, PIXEL);
  del_obj_mupdf_renderer ();

  long before= get_rss ();

  // Repeatedly register the same pattern and clear caches so that
  // register_pattern runs its full allocation path every iteration.
  const int iterations= 2000;
  for (int i= 0; i < iterations; i++) {
    ren->test_register_pattern (br, PIXEL);
    del_obj_mupdf_renderer ();
  }

  long after= get_rss ();

  // Cleanup
  ren->end ();
  fz_drop_pixmap (ctx, pix);
  tm_delete (ren);

  long delta_kb= after - before;
  // Allow some RSS noise; a real leak of ~3-5 KB per iteration would
  // produce tens of MB after 2000 iterations, so 5 MB is a safe ceiling.
  QVERIFY2 (delta_kb < 5120,
            qPrintable (QString ("RSS grew by %1 KB after %2 iterations, "
                                 "indicating a memory leak")
                            .arg (delta_kb)
                            .arg (iterations)));
}

QTEST_MAIN (TestMupdfRenderer)
#include "mupdf_renderer_test.moc"
