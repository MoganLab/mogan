/******************************************************************************
 * MODULE     : login_button_test.cpp
 * DESCRIPTION : LoginButton 悬浮信号的单元测试（hovered/unhovered）
 * COPYRIGHT  : (C) 2026 Liii
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"

#include <QApplication>
#include <QEnterEvent>
#include <QSignalSpy>
#include <QTest>

#include "QWindowKit/loginbutton.hpp"

class TestLoginButton : public QObject {
  Q_OBJECT
private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void enter_emits_hovered ();
  void leave_emits_unhovered ();
};

void
TestLoginButton::enter_emits_hovered () {
  QWK::LoginButton btn;
  QSignalSpy       spy (&btn, &QWK::LoginButton::hovered);
  QEnterEvent      enter (QPointF (1, 1), QPointF (1, 1), QPointF (1, 1));
  QApplication::sendEvent (&btn, &enter);
  QCOMPARE (spy.count (), 1);
}

void
TestLoginButton::leave_emits_unhovered () {
  QWK::LoginButton btn;
  QEnterEvent      enter (QPointF (1, 1), QPointF (1, 1), QPointF (1, 1));
  QApplication::sendEvent (&btn, &enter);
  QSignalSpy spy (&btn, &QWK::LoginButton::unhovered);
  QEvent     leave (QEvent::Leave);
  QApplication::sendEvent (&btn, &leave);
  QCOMPARE (spy.count (), 1);
}

#ifdef QTTEXMACS
QTEST_MAIN (TestLoginButton)
#else
int
main () {
  return 0;
}
#endif
#include "login_button_test.moc"
