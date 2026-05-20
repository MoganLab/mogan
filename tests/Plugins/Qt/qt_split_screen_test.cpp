/******************************************************************************
 * MODULE     : qt_split_screen_test.cpp
 * DESCRIPTION: Tests for split screen functionality in qt_tm_widget_rep
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#include "Qt/qt_tm_widget.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QApplication>
#include <QtTest/QtTest>

static QtMessageHandler defaultMessageHandler= nullptr;

static void
filterTestWarnings (QtMsgType type, const QMessageLogContext& context,
                    const QString& msg) {
  if (type == QtWarningMsg) {
    if (msg.contains ("cached device pixel ratio") ||
        msg.contains ("wayland.textinput")) {
      return;
    }
  }
  defaultMessageHandler (type, context, msg);
}

class TestSplitScreen : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    defaultMessageHandler= qInstallMessageHandler (filterTestWarnings);
  }

  void init () { init_lolly (); }

  void test_default_not_split () {
    qt_tm_widget_rep* w= new qt_tm_widget_rep (0, command ());
    QVERIFY (!w->is_split_mode ());
    QCOMPARE (w->active_pane (), 0);
    delete w;
  }

  void test_split_and_unsplit () {
    qt_tm_widget_rep* w= new qt_tm_widget_rep (0, command ());
    QVERIFY (!w->is_split_mode ());

    w->split_window_horizontally ();
    QVERIFY (w->is_split_mode ());

    w->unsplit_window ();
    QVERIFY (!w->is_split_mode ());
    delete w;
  }

  void test_active_pane_switching () {
    qt_tm_widget_rep* w= new qt_tm_widget_rep (0, command ());
    QCOMPARE (w->active_pane (), 0);

    w->split_window_horizontally ();
    w->set_active_pane (1);
    QCOMPARE (w->active_pane (), 1);

    w->set_active_pane (0);
    QCOMPARE (w->active_pane (), 0);
    delete w;
  }

  void test_unsplit_restores_single_pane () {
    qt_tm_widget_rep* w= new qt_tm_widget_rep (0, command ());
    w->split_window_horizontally ();
    QVERIFY (w->is_split_mode ());
    w->unsplit_window ();
    QVERIFY (!w->is_split_mode ());
    QCOMPARE (w->active_pane (), 0);
    delete w;
  }
};

QTEST_MAIN (TestSplitScreen)
#include "qt_split_screen_test.moc"
