/******************************************************************************
 * MODULE     : qt_math_completion_popup_test.cpp
 * DESCRIPTION: Tests for QTMMathCompletionPopup widget
 * COPYRIGHT  : (C) 2026 Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMMathCompletionPopup.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestQTMMathCompletionPopup : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_hide_on_parent_window_deactivate ();
};

void
TestQTMMathCompletionPopup::test_hide_on_parent_window_deactivate () {
  QWidget                mainWindow;
  QTMMathCompletionPopup popup (&mainWindow, nullptr);

  mainWindow.show ();
  popup.show ();
  QVERIFY (popup.isVisible ());

  // 模拟主窗口失活（如 Alt+Tab 切换窗口），popup 应当自动隐藏
  QEvent deactivateEvent (QEvent::WindowDeactivate);
  QApplication::sendEvent (&mainWindow, &deactivateEvent);

  QVERIFY (!popup.isVisible ());
}

QTEST_MAIN (TestQTMMathCompletionPopup)
#include "qt_math_completion_popup_test.moc"
