/******************************************************************************
 * MODULE     : qt_tm_widget_focus_test.cpp
 * DESCRIPTION: Tests for AI sidebar focus restoration logic
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_tm_widget.hpp"
#include "base.hpp"
#include <QDockWidget>
#include <QMenu>
#include <QPushButton>
#include <QtTest/QtTest>

class TestQtTmWidgetFocus : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void
  test_should_restore_when_sidebar_visible_focus_inside_and_click_outside () {
    QVERIFY (qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        true, true, false));
  }

  // 真值表穷举 8 种组合，覆盖其余全部场景
  void test_full_truth_table () {
    for (int mask= 0; mask < 8; ++mask) {
      bool visible = (mask & 1) != 0;
      bool focusIn = (mask & 2) != 0;
      bool clickIn = (mask & 4) != 0;
      bool expected= visible && focusIn && !clickIn;
      QCOMPARE (qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
                    visible, focusIn, clickIn),
                expected);
    }
  }

  void test_widget_hierarchy_and_popup_protection () {
    QWidget     parent;
    QDockWidget dock ("AI Chat Sidebar", &parent);
    QPushButton childBtn ("Send", &dock);
    QPushButton outsideBtn ("ToolButton", &parent);
    QMenu       popupMenu (&childBtn);

    QVERIFY (qt_tm_widget_rep::isWidgetInSidebar (&dock, &childBtn));
    QVERIFY (!qt_tm_widget_rep::isWidgetInSidebar (&dock, &outsideBtn));
    QVERIFY (qt_tm_widget_rep::isWidgetInSidebar (&dock, &popupMenu));
    QVERIFY (!qt_tm_widget_rep::isWidgetInSidebar (&dock, &parent));
  }
};

QTEST_MAIN (TestQtTmWidgetFocus)
#include "qt_tm_widget_focus_test.moc"
