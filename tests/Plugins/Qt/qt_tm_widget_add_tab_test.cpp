/******************************************************************************
 * MODULE     : qt_tm_widget_add_tab_test.cpp
 * DESCRIPTION: Tests for new-tab current view normalization
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_tm_widget.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestQtTmWidgetAddTab : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_should_reset_when_current_view_missing () {
    QVERIFY (qt_tm_widget_rep::shouldResetCurrentViewForNewTab (
        url_none (), url_none (), url ("window-1")));
  }

  void test_should_reset_when_current_view_has_no_window () {
    QVERIFY (qt_tm_widget_rep::shouldResetCurrentViewForNewTab (
        url ("tmfs://view/12/default/Users/test/chat.tm"), url_none (),
        url ("window-1")));
  }

  void test_should_reset_when_current_view_belongs_to_other_window () {
    QVERIFY (qt_tm_widget_rep::shouldResetCurrentViewForNewTab (
        url ("tmfs://view/12/default/Users/test/chat.tm"), url ("window-2"),
        url ("window-1")));
  }

  void test_should_reset_when_current_view_is_not_default () {
    QVERIFY (qt_tm_widget_rep::shouldResetCurrentViewForNewTab (
        url ("tmfs://view/12/tmfs/aux/tmfs://chat/session-1/input"),
        url ("window-1"), url ("window-1")));
  }

  void test_should_not_reset_for_default_view_in_owner_window () {
    QVERIFY (!qt_tm_widget_rep::shouldResetCurrentViewForNewTab (
        url ("tmfs://view/12/default/Users/test/chat.tm"), url ("window-1"),
        url ("window-1")));
  }

  void test_should_not_reset_without_owner_window () {
    QVERIFY (!qt_tm_widget_rep::shouldResetCurrentViewForNewTab (
        url ("tmfs://view/12/default/Users/test/chat.tm"), url ("window-1"),
        url_none ()));
  }
};

QTEST_MAIN (TestQtTmWidgetAddTab)
#include "qt_tm_widget_add_tab_test.moc"
