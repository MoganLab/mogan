/******************************************************************************
 * MODULE     : qt_picture_test.cpp
 * DESCRIPTION: USE_MUPDF_RENDERER 构建下，mupdf_picture_rep 与 qt_picture_rep
 *              的 get_type() 同为 picture_native，as_qt_picture 必须按实际
 *              rep 类型区分并完成像素转换，否则 qt_renderer_rep::draw_picture
 *              对 get_handle() 的强转产生非法 QImage 引用导致段错误
 *              （见 devel/1169.md）。
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "qt_picture.hpp"

#ifdef USE_MUPDF_RENDERER
#include "mupdf_picture.hpp"
#endif

#include <QtTest/QtTest>

class TestQtPicture : public QObject {
  Q_OBJECT
private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }
  void test_as_qt_picture_keeps_qt_picture ();
  void test_as_qt_picture_converts_mupdf_picture ();
};

void
TestQtPicture::test_as_qt_picture_keeps_qt_picture () {
  picture p= qt_picture (QImage (3, 2, QImage::Format_ARGB32), 1, 1);
  picture q= as_qt_picture (p);
  QVERIFY (dynamic_cast<qt_picture_rep*> (q.operator->()) != NULL);
  QCOMPARE (q.operator->(), p.operator->());
}

void
TestQtPicture::test_as_qt_picture_converts_mupdf_picture () {
#ifdef USE_MUPDF_RENDERER
  picture p= native_picture (3, 2, 1, 1);
  QVERIFY (dynamic_cast<mupdf_picture_rep*> (p.operator->()) != NULL);
  p->set_pixel (0, 0, 0xFFFF0000);

  picture q= as_qt_picture (p);
  // 转换结果必须是真正的 qt_picture_rep，drawImage 才不会踩野指针
  QVERIFY (dynamic_cast<qt_picture_rep*> (q.operator->()) != NULL);
  QCOMPARE (q->get_width (), 3);
  QCOMPARE (q->get_height (), 2);
  QCOMPARE (q->get_origin_x (), 1);
  QCOMPARE (q->get_origin_y (), 1);
  QCOMPARE (q->get_pixel (0, 0), p->get_pixel (0, 0));
  QCOMPARE (q->get_pixel (-1, -1), p->get_pixel (-1, -1));
#endif
}

#ifdef QTTEXMACS
QTEST_MAIN (TestQtPicture)
#else
int
main () {
  return 0;
}
#endif
#include "qt_picture_test.moc"
