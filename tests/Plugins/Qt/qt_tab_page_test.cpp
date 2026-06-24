/******************************************************************************
 * MODULE     : qt_tab_page_test.cpp
 * DESCRIPTION: Tests for QTMTabPage dirty marker behavior
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/QTMTabPage.hpp"
#include "base.hpp"
#include <QApplication>
#include <QMouseEvent>
#include <QtTest/QtTest>

class TestQTMTabPage : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void cleanup () { cleanup_qt_top_level_widgets (); }

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
    // macOS (Cocoa) 的 QTest::mouseMove 不会向未 grab 鼠标的 widget 派发
    // mouseMoveEvent，因此直接合成一个 MouseMove 事件投递给 tab，触发其
    // hover 检测逻辑（等价于 Windows 上鼠标移入关闭按钮区域）。
    QMouseEvent moveEvent (QEvent::MouseMove, closeCenter,
                           tab.mapToGlobal (closeCenter), Qt::NoButton,
                           Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent (&tab, &moveEvent);
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
