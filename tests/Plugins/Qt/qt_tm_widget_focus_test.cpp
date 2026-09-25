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

  void test_should_not_restore_when_click_inside_sidebar () {
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        true, true, true));
  }

  void test_should_not_restore_when_focus_not_in_sidebar () {
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        true, false, false));
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        true, false, true));
  }

  void test_should_not_restore_when_sidebar_not_visible () {
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        false, true, false));
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        false, true, true));
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        false, false, false));
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        false, false, true));
  }

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

    // 点击侧边栏外部控件时触发回切
    bool focusInSidebar= qt_tm_widget_rep::isWidgetInSidebar (&dock, &childBtn);
    bool clickOutside= qt_tm_widget_rep::isWidgetInSidebar (&dock, &outsideBtn);
    QVERIFY (qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        true, focusInSidebar, clickOutside));

    // 点击侧边栏内部子控件不触发回切
    bool clickChild= qt_tm_widget_rep::isWidgetInSidebar (&dock, &childBtn);
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        true, focusInSidebar, clickChild));

    // 点击具有侧边栏父级的弹出菜单不触发回切
    bool clickMenu= qt_tm_widget_rep::isWidgetInSidebar (&dock, &popupMenu);
    QVERIFY (!qt_tm_widget_rep::shouldRestoreDocumentFocusOnMousePress (
        true, focusInSidebar, clickMenu));
  }
};

QTEST_MAIN (TestQtTmWidgetFocus)
#include "qt_tm_widget_focus_test.moc"
