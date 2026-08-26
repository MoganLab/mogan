
/******************************************************************************
 * MODULE     : qt_utilities_test.cpp
 * COPYRIGHT  : (C) 2019  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include "sys_utils.hpp"
#include <Qt>
#include <QtTest/QtTest>

class TestQtUtilities : public QObject {
  Q_OBJECT

private slots:
  void test_qt_supports ();
  void test_from_modifiers ();
  void test_from_key_press_event ();
  void test_to_qstring_utf8 ();
  void test_from_qstring_utf8_roundtrip ();
  void test_title_encoding_roundtrip ();
};

void
TestQtUtilities::test_qt_supports () {
#ifdef QTTEXMACS
  QVERIFY (qt_supports (url ("x.svg")));
  QVERIFY (qt_supports (url ("x.png")));
  QVERIFY (!qt_supports (url ("x.eps")));
  QVERIFY (!qt_supports (url ("x.ps")));
  QVERIFY (!qt_supports (url ("x.pdf")));
#endif
}

void
TestQtUtilities::test_from_modifiers () {
  qcompare (from_modifiers (Qt::NoModifier), "");
  qcompare (from_modifiers (Qt::ShiftModifier), "S-");
  qcompare (from_modifiers (Qt::AltModifier), "A-");
  qcompare (from_modifiers (Qt::AltModifier | Qt::ShiftModifier), "A-S-");
  if (os_macos ()) {
    qcompare (from_modifiers (Qt::MetaModifier), "C-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::AltModifier), "C-A-");
    qcompare (
        from_modifiers (Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier),
        "C-A-S-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::ShiftModifier), "C-S-");
    qcompare (from_modifiers (Qt::ControlModifier), "M-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier), "M-A-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier |
                              Qt::ShiftModifier),
              "M-A-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::ShiftModifier), "M-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::MetaModifier), "M-C-");
  }
  else {
    qcompare (from_modifiers (Qt::MetaModifier), "M-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::AltModifier), "M-A-");
    qcompare (
        from_modifiers (Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier),
        "M-A-S-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::ShiftModifier), "M-S-");
    qcompare (from_modifiers (Qt::ControlModifier), "C-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier), "C-A-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier |
                              Qt::ShiftModifier),
              "C-A-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::ShiftModifier), "C-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::MetaModifier), "M-C-");
  }
}

void
TestQtUtilities::test_from_key_press_event () {
  if (os_macos ()) {
    auto ctrl_plus= QKeyEvent (QEvent::KeyPress, (int) '=',
                               Qt::ControlModifier | Qt::ShiftModifier, "=");
    qcompare (from_key_press_event (&ctrl_plus), "M-S-=");

    // A-<number>
    auto alt_1= QKeyEvent (QEvent::KeyPress, (int) '1', Qt::AltModifier, "¡");
    qcompare (from_key_press_event (&alt_1), "A-1");
    // A-<alpha>
    auto alt_v= QKeyEvent (QEvent::KeyPress, (int) 'V', Qt::AltModifier, "√");
    qcompare (from_key_press_event (&alt_v), "A-v");
    // A-<not alpha and not number>
    auto alt_dot= QKeyEvent (QEvent::KeyPress, (int) '.', Qt::AltModifier, "≥");
    qcompare (from_key_press_event (&alt_dot), "≥");
  }
}

QTEST_MAIN (TestQtUtilities)
#include "qt_utilities_test.moc"

/*
 * [0250] Encoding tests for chat tab title storage.
 *
 * session->title stores UTF-8. to_qstring auto-detects Cork vs UTF-8.
 * These tests verify the encoding round-trip for UTF-8 inputs (CJK, Latin).
 * Note: Cork encoding tests require TeXmacs runtime dictionaries and
 * cannot run in a standalone test.
 */

// Helper: check that a Mogan string equals expected byte sequence
static bool
bytes_equal (string s, const char* expected) {
  string e (expected);
  if (N (s) != N (e)) return false;
  for (int i= 0; i < N (s); i++)
    if (s[i] != e[i]) return false;
  return true;
}

void
TestQtUtilities::test_to_qstring_utf8 () {
  // UTF-8: ö = 0xC3 0xB6 (two bytes)
  string  utf8_title= "Erwin Schr"
                      "\xC3\xB6"
                      "dinger";
  QString q         = to_qstring (utf8_title);
  QCOMPARE (q, QString::fromUtf8 ("Erwin Schrödinger"));

  // UTF-8: 你好 = E4 BD A0 E5 A5 BD
  string  cjk_title= "\xE4\xBD\xA0\xE5\xA5\xBD";
  QString q2       = to_qstring (cjk_title);
  QCOMPARE (q2, QString::fromUtf8 ("\xE4\xBD\xA0\xE5\xA5\xBD"));
}

void
TestQtUtilities::test_from_qstring_utf8_roundtrip () {
  // Latin: QString → from_qstring_utf8 → UTF-8 bytes
  QString qLatin= QString::fromUtf8 ("Erwin Schrödinger");
  string  utf8  = from_qstring_utf8 (qLatin);
  QVERIFY (bytes_equal (utf8, "Erwin Schr"
                              "\xC3\xB6"
                              "dinger"));

  // CJK: QString → from_qstring_utf8 → UTF-8 bytes
  QString qCJK    = QString::fromUtf8 ("\xE4\xBD\xA0\xE5\xA5\xBD");
  string  utf8_cjk= from_qstring_utf8 (qCJK);
  QVERIFY (bytes_equal (utf8_cjk, "\xE4\xBD\xA0\xE5\xA5\xBD"));
}

void
TestQtUtilities::test_title_encoding_roundtrip () {
  // Simulate the full title storage cycle:
  // to_qstring(mixed_input) → from_qstring_utf8 → store UTF-8
  // → to_qstring(stored_utf8) → display

  // CJK round-trip: UTF-8 verbatim → store UTF-8 → display
  {
    string  utf8_input= "\xE4\xBD\xA0\xE5\xA5\xBD";
    QString qTitle    = to_qstring (utf8_input);
    string  stored    = from_qstring_utf8 (qTitle);
    QVERIFY (bytes_equal (stored, "\xE4\xBD\xA0\xE5\xA5\xBD"));
    QString displayed= to_qstring (stored);
    QCOMPARE (displayed, QString::fromUtf8 ("\xE4\xBD\xA0\xE5\xA5\xBD"));
  }

  // Latin UTF-8 round-trip
  {
    string  utf8_input= "Schr"
                        "\xC3\xB6"
                        "dinger";
    QString qTitle    = to_qstring (utf8_input);
    string  stored    = from_qstring_utf8 (qTitle);
    QVERIFY (bytes_equal (stored, "Schr"
                                  "\xC3\xB6"
                                  "dinger"));
    QString displayed= to_qstring (stored);
    QCOMPARE (displayed, QString::fromUtf8 ("Schrödinger"));
  }

  // ASCII round-trip (no special encoding)
  {
    string  ascii_input= "Hello World";
    QString qTitle     = to_qstring (ascii_input);
    string  stored     = from_qstring_utf8 (qTitle);
    QVERIFY (bytes_equal (stored, "Hello World"));
    QString displayed= to_qstring (stored);
    QCOMPARE (displayed, QString ("Hello World"));
  }
}
