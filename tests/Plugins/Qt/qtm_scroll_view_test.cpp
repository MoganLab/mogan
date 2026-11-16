
/******************************************************************************
 * MODULE     : qtm_scroll_view_test.cpp
 * DESCRIPTION: Tests for QTMScrollView transparent overlay scrollbars
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMScrollView.hpp"
#include "base.hpp"

#include <QScrollBar>
#include <QtTest/QtTest>

class TestQTMScrollView : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_overlay_scrollbars_keep_full_viewport ();
  void test_scrollbar_containers_are_transparent ();
};

void
TestQTMScrollView::test_overlay_scrollbars_keep_full_viewport () {
  QTMScrollView view;
  view.resize (400, 300);
  view.setExtents (QRect (0, 0, 1000, 800));
  QCoreApplication::processEvents ();

  QCOMPARE (view.viewport ()->geometry (), view.contentsRect ());

  QScrollBar* hbar= view.horizontalScrollBar ();
  QScrollBar* vbar= view.verticalScrollBar ();
  QVERIFY (hbar != nullptr);
  QVERIFY (vbar != nullptr);
  QCOMPARE (hbar->maximum (), 1000 - view.contentsRect ().width ());
  QCOMPARE (vbar->maximum (), 800 - view.contentsRect ().height ());

  QWidget* hTarget= (hbar->parentWidget () && hbar->parentWidget () != &view)
                        ? hbar->parentWidget ()
                        : static_cast<QWidget*> (hbar);
  QWidget* vTarget= (vbar->parentWidget () && vbar->parentWidget () != &view)
                        ? vbar->parentWidget ()
                        : static_cast<QWidget*> (vbar);

  QCOMPARE (hTarget->geometry ().bottom (), view.contentsRect ().bottom ());
  QCOMPARE (vTarget->geometry ().right (), view.contentsRect ().right ());
}

void
TestQTMScrollView::test_scrollbar_containers_are_transparent () {
  QTMScrollView view;
  view.resize (400, 300);

  QScrollBar* hbar= view.horizontalScrollBar ();
  QScrollBar* vbar= view.verticalScrollBar ();
  QVERIFY (hbar != nullptr);
  QVERIFY (vbar != nullptr);

  QVERIFY (hbar->testAttribute (Qt::WA_NoSystemBackground));
  QVERIFY (!hbar->autoFillBackground ());
  QVERIFY (vbar->testAttribute (Qt::WA_NoSystemBackground));
  QVERIFY (!vbar->autoFillBackground ());

  if (hbar->parentWidget () && hbar->parentWidget () != &view) {
    QVERIFY (hbar->parentWidget ()->testAttribute (Qt::WA_NoSystemBackground));
    QVERIFY (!hbar->parentWidget ()->autoFillBackground ());
  }
  if (vbar->parentWidget () && vbar->parentWidget () != &view) {
    QVERIFY (vbar->parentWidget ()->testAttribute (Qt::WA_NoSystemBackground));
    QVERIFY (!vbar->parentWidget ()->autoFillBackground ());
  }
}

#ifdef QTTEXMACS
QTEST_MAIN (TestQTMScrollView)
#else
int
main () {
  return 0;
}
#endif
#include "qtm_scroll_view_test.moc"
