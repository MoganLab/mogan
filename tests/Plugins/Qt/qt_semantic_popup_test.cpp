/******************************************************************************
 * MODULE     : qt_semantic_popup_test.cpp
 * DESCRIPTION: Tests for QTMSemanticPopup widget
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMSemanticPopup.hpp"
#include "base.hpp"
#include "scheme.hpp"
#include <QtTest/QtTest>

class TestQTMSemanticPopup : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_semantic_popup_equation_star ();
  void test_semantic_popup_equation ();
  void test_semantic_popup_toc ();
};

void
TestQTMSemanticPopup::test_semantic_popup_equation_star () {
  QWidget          parent;
  QTMSemanticPopup popup (&parent, nullptr);

  tree t= compound ("equation*", compound ("document", "x+y=z"));
  popup.setSemanticNode ("equation*", t);

  QList<QToolButton*> btns= popup.findChildren<QToolButton*> ();
  QCOMPARE (btns.size (), 2);
  QVERIFY (btns[0]->text ().contains ("LaTeX") ||
           btns[0]->toolTip ().contains ("LaTeX"));
  QVERIFY (btns[1]->text ().contains ("Number") ||
           btns[1]->text ().contains ("编号") ||
           btns[1]->toolTip ().contains ("Number") ||
           btns[1]->toolTip ().contains ("编号"));
}

void
TestQTMSemanticPopup::test_semantic_popup_equation () {
  QWidget          parent;
  QTMSemanticPopup popup (&parent, nullptr);

  tree t= compound ("equation", compound ("document", "x+y=z"));
  popup.setSemanticNode ("equation", t);

  QList<QToolButton*> btns= popup.findChildren<QToolButton*> ();
  QCOMPARE (btns.size (), 2);
  QVERIFY (btns[1]->text ().contains ("Hide") ||
           btns[1]->text ().contains ("隐藏") ||
           btns[1]->toolTip ().contains ("Hide") ||
           btns[1]->toolTip ().contains ("隐藏"));
}

void
TestQTMSemanticPopup::test_semantic_popup_toc () {
  QWidget          parent;
  QTMSemanticPopup popup (&parent, nullptr);

  tree t= compound ("table-of-contents", "toc", compound ("document"));
  popup.setSemanticNode ("table-of-contents", t);

  QList<QToolButton*> btns= popup.findChildren<QToolButton*> ();
  QCOMPARE (btns.size (), 1);
  QVERIFY (btns[0]->text ().contains ("TOC") ||
           btns[0]->text ().contains ("目录") ||
           btns[0]->toolTip ().contains ("TOC") ||
           btns[0]->toolTip ().contains ("目录"));
}

QTEST_MAIN (TestQTMSemanticPopup)
#include "qt_semantic_popup_test.moc"
