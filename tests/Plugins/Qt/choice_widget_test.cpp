/******************************************************************************
 * MODULE     : choice_widget_test.cpp
 * DESCRIPTION: Tests for QTMListView and QTMPlainWindow focus & double-click
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/QTMMenuHelper.hpp"
#include "Qt/QTMWindow.hpp"
#include "base.hpp"

#include <QPushButton>
#include <QVBoxLayout>
#include <QtTest/QtTest>

class TestChoiceWidget : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_initial_current_index ();
  void test_window_focus_and_navigation ();
  void test_double_click_triggers_ok ();
  void test_enter_key_triggers_ok ();
};

void
TestChoiceWidget::test_initial_current_index () {
  QStringList items;
  items << "Markdown" << "LaTeX" << "HTML" << "Plain text";
  QStringList sel;
  sel << "Markdown";

  QTMListView lv (command (), items, sel, false);

  QVERIFY (lv.currentIndex ().isValid ());
  QCOMPARE (lv.currentIndex ().row (), 0);
  QCOMPARE (lv.model ()->data (lv.currentIndex ()).toString (),
            QString ("Markdown"));
}

void
TestChoiceWidget::test_window_focus_and_navigation () {
  QTMPlainWindow win (nullptr);
  QVBoxLayout*   layout= new QVBoxLayout (&win);

  QStringList items;
  items << "Markdown" << "LaTeX" << "HTML" << "Plain text";
  QStringList sel;
  sel << "Markdown";

  QTMListView* lv   = new QTMListView (command (), items, sel, false);
  QPushButton* okBtn= new QPushButton ("Ok", &win);
  layout->addWidget (lv);
  layout->addWidget (okBtn);

  win.show ();
  win.activateWindow ();
  QApplication::processEvents ();

  // 窗口弹出后，QTMListView 应该获取焦点
  QCOMPARE (win.focusWidget (), (QWidget*) lv);
  if (win.isActiveWindow ()) {
    QVERIFY (lv->hasFocus ());
  }
  QCOMPARE (lv->currentIndex ().row (), 0);

  // 下方向键切换选项到 row 1 ("LaTeX")
  QTest::keyClick (lv, Qt::Key_Down);
  QApplication::processEvents ();
  QCOMPARE (lv->currentIndex ().row (), 1);
  QCOMPARE (lv->model ()->data (lv->currentIndex ()).toString (),
            QString ("LaTeX"));

  // 上方向键切换选项回 row 0 ("Markdown")
  QTest::keyClick (lv, Qt::Key_Up);
  QApplication::processEvents ();
  QCOMPARE (lv->currentIndex ().row (), 0);
  QCOMPARE (lv->model ()->data (lv->currentIndex ()).toString (),
            QString ("Markdown"));

  win.close ();
}

void
TestChoiceWidget::test_double_click_triggers_ok () {
  QTMPlainWindow win (nullptr);
  QVBoxLayout*   layout= new QVBoxLayout (&win);

  QStringList items;
  items << "Markdown" << "LaTeX" << "HTML" << "Plain text";
  QStringList sel;
  sel << "Markdown";

  QTMListView* lv   = new QTMListView (command (), items, sel, false);
  QPushButton* okBtn= new QPushButton ("Ok", &win);
  layout->addWidget (lv);
  layout->addWidget (okBtn);

  bool okClicked= false;
  connect (okBtn, &QPushButton::clicked, [&okClicked] () { okClicked= true; });

  win.show ();
  QApplication::processEvents ();

  // 双击第 2 个选项 ("LaTeX", row 1)
  QModelIndex target= lv->model ()->index (1, 0);
  QMetaObject::invokeMethod (lv, "onDoubleClicked",
                             Q_ARG (QModelIndex, target));
  QApplication::processEvents ();

  // 验证 double click 直接触发了 Ok 按钮，且当前选中项变为 row 1
  QVERIFY (okClicked);
  QCOMPARE (lv->currentIndex ().row (), 1);
  QCOMPARE (lv->model ()->data (lv->currentIndex ()).toString (),
            QString ("LaTeX"));

  win.close ();
}

void
TestChoiceWidget::test_enter_key_triggers_ok () {
  QTMPlainWindow win (nullptr);
  QVBoxLayout*   layout= new QVBoxLayout (&win);

  QStringList items;
  items << "Markdown" << "LaTeX" << "HTML" << "Plain text";
  QStringList sel;
  sel << "Markdown";

  QTMListView* lv   = new QTMListView (command (), items, sel, false);
  QPushButton* okBtn= new QPushButton ("Ok", &win);
  layout->addWidget (lv);
  layout->addWidget (okBtn);

  bool okClicked= false;
  connect (okBtn, &QPushButton::clicked, [&okClicked] () { okClicked= true; });

  win.show ();
  QApplication::processEvents ();

  // 焦点在 lv，按 Enter 键触发确定
  QTest::keyClick (lv, Qt::Key_Return);
  QApplication::processEvents ();

  QVERIFY (okClicked);

  win.close ();
}

#ifdef QTTEXMACS
QTEST_MAIN (TestChoiceWidget)
#else
int
main () {
  return 0;
}
#endif
#include "choice_widget_test.moc"
