
/******************************************************************************
 * MODULE     : image_files_test.cpp
 * COPYRIGHT  : (C) 2019  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "file.hpp"
#include "image_files.hpp"
#include "sys_utils.hpp"
#include "url.hpp"
#include <QtTest/QtTest>

class TestImageFiles : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void test_svg_image_size ();
  void test_image_size ();
  void test_ps_read_bbox ();
};

void
TestImageFiles::test_svg_image_size () {
  int w= 0, h= 0;
  url u1= url ("$TEXMACS_PATH/misc/images/fancy-c.svg");
  svg_image_size (u1, w, h);
  QCOMPARE (w, 24);
  QCOMPARE (h, 24);

  w= h  = 0;
  url u2= url_ramdisc (string_load (u1)) * url ("fancy-c.svg");
  svg_image_size (u2, w, h);
  QCOMPARE (w, 24);
  QCOMPARE (h, 24);
}

void
TestImageFiles::test_image_size () {
  int w= 0, h= 0;
  url u1= url ("$TEXMACS_PATH/misc/images/fancy-c.svg");
  image_size (u1, w, h);
  QCOMPARE (w, 24);
  QCOMPARE (h, 24);

  w= h  = 0;
  url u2= url_ramdisc (string_load (u1)) * url ("fancy-c.svg");
  image_size (u2, w, h);
  QCOMPARE (w, 24);
  QCOMPARE (h, 24);
}

void
TestImageFiles::test_ps_read_bbox () {
  int    x1= 0, y1= 0, x2= 0, y2= 0;
  string eps= "%!PS-Adobe-3.0 EPSF-3.0\n%%BoundingBox: 0 0 500 500\n%%EOF\n";
  bool   ok = ps_read_bbox (eps, x1, y1, x2, y2);
  QCOMPARE (ok, true);
  QCOMPARE (x1, 0);
  QCOMPARE (y1, 0);
  QCOMPARE (x2, 500);
  QCOMPARE (y2, 500);

  int    cx1= 0, cy1= 0, cx2= 0, cy2= 0;
  string eps_crlf=
      "%!PS-Adobe-3.1 EPSF-3.0\r\n%%BoundingBox: 77 57 709 583\r\n%%EOF\r\n";
  bool ok_crlf= ps_read_bbox (eps_crlf, cx1, cy1, cx2, cy2);
  QCOMPARE (ok_crlf, true);
  QCOMPARE (cx1, 77);
  QCOMPARE (cy1, 57);
  QCOMPARE (cx2, 709);
  QCOMPARE (cy2, 583);
}

QTEST_MAIN (TestImageFiles)
#include "image_files_test.moc"
