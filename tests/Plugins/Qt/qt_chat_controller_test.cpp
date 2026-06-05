/******************************************************************************
 * MODULE     : qt_chat_controller_test.cpp
 * DESCRIPTION: Tests for ChatController helper functions
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_chat_controller.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestChatController : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // === sanitizeExportFileName ===
  void test_sanitize_plain_title () {
    QCOMPARE (ChatController::sanitizeExportFileName ("HelloWorld"),
              QString ("HelloWorld"));
  }

  void test_sanitize_spaces_to_underscores () {
    QCOMPARE (ChatController::sanitizeExportFileName ("Hello World"),
              QString ("Hello_World"));
  }

  void test_sanitize_multiple_spaces () {
    QCOMPARE (ChatController::sanitizeExportFileName ("A  B   C"),
              QString ("A__B___C"));
  }

  void test_sanitize_removes_asterisk () {
    QCOMPARE (ChatController::sanitizeExportFileName ("Title*With*Stars"),
              QString ("TitleWithStars"));
  }

  void test_sanitize_removes_slash () {
    QCOMPARE (ChatController::sanitizeExportFileName ("A/B/C"),
              QString ("ABC"));
  }

  void test_sanitize_removes_backslash () {
    QCOMPARE (ChatController::sanitizeExportFileName ("A\\B\\C"),
              QString ("ABC"));
  }

  void test_sanitize_removes_colon () {
    QCOMPARE (ChatController::sanitizeExportFileName ("A:B:C"),
              QString ("ABC"));
  }

  void test_sanitize_removes_question_mark () {
    QCOMPARE (ChatController::sanitizeExportFileName ("What?"),
              QString ("What"));
  }

  void test_sanitize_removes_quotes () {
    QCOMPARE (ChatController::sanitizeExportFileName ("Say \"Hello\""),
              QString ("Say_Hello"));
  }

  void test_sanitize_removes_angle_brackets () {
    QCOMPARE (ChatController::sanitizeExportFileName ("A <B> C"),
              QString ("A_B_C"));
  }

  void test_sanitize_removes_pipe () {
    QCOMPARE (ChatController::sanitizeExportFileName ("A|B|C"),
              QString ("ABC"));
  }

  void test_sanitize_all_invalid_chars () {
    QCOMPARE (ChatController::sanitizeExportFileName ("\\/:*?\"<>|"),
              QString ("export"));
  }

  void test_sanitize_empty_returns_export () {
    QCOMPARE (ChatController::sanitizeExportFileName (""), QString ("export"));
  }

  void test_sanitize_only_invalid_returns_export () {
    QCOMPARE (ChatController::sanitizeExportFileName ("***///"),
              QString ("export"));
  }

  void test_sanitize_only_spaces_becomes_underscores () {
    QCOMPARE (ChatController::sanitizeExportFileName ("   "), QString ("___"));
  }

  void test_sanitize_cjk_preserved () {
    QCOMPARE (ChatController::sanitizeExportFileName ("你好 世界"),
              QString ("你好_世界"));
  }

  void test_sanitize_mixed_valid_and_invalid () {
    QCOMPARE (
        ChatController::sanitizeExportFileName ("My *cool* chat / session?"),
        QString ("My_cool_chat__session"));
  }

  void test_sanitize_leading_trailing_spaces () {
    QCOMPARE (ChatController::sanitizeExportFileName ("  hello  "),
              QString ("__hello__"));
  }
};

QTEST_MAIN (TestChatController)
#include "qt_chat_controller_test.moc"
