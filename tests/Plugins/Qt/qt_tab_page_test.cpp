/******************************************************************************
 * MODULE     : qt_tab_page_test.cpp
 * DESCRIPTION: Tests for QTMTabPage dirty marker behavior
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/QTMTabPage.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestQTMTabPage : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_dirty_title_moves_star_to_close_slot () {
    QAction    titleAction (QString::fromUtf8 ("very-long-file-name.tm *"),
                            nullptr);
    QAction    closeAction ("Close", nullptr);
    QTMTabPage tab (url ("file:///tmp/test.tm"), &titleAction, &closeAction,
                    false);
    tab.resize (220, 32);
    tab.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&tab));

    QCOMPARE (tab.text (), QString::fromUtf8 ("very-long-file-name.tm"));
    QVERIFY (tab.isDirty ());

    auto* closeBtn= tab.findChild<QWK::WindowButton*> ("tabpage-close-button");
    QVERIFY (closeBtn != nullptr);
    QVERIFY (!closeBtn->isVisible ());

    QPoint closeCenter= closeBtn->geometry ().center ();
    QTest::mouseMove (&tab, closeCenter);
    QTRY_VERIFY (closeBtn->isVisible ());
  }

  void test_clean_title_keeps_close_button_hidden_without_hover () {
    QAction    titleAction ("clean-file.tm", nullptr);
    QAction    closeAction ("Close", nullptr);
    QTMTabPage tab (url ("file:///tmp/test.tm"), &titleAction, &closeAction,
                    false);
    tab.resize (220, 32);
    tab.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&tab));

    QCOMPARE (tab.text (), QString::fromUtf8 ("clean-file.tm"));
    QVERIFY (!tab.isDirty ());
  }
};

QTEST_MAIN (TestQTMTabPage)
#include "qt_tab_page_test.moc"
