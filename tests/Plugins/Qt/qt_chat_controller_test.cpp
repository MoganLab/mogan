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

#include "converter.hpp"
#include "preferences.hpp"

using namespace moebius;

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

  // === composeAiInputBody ===

  void test_compose_atomic_selection_translate_appends_prompt () {
    // 单行选区（selection_get 的原子串形态）：包成引用块并追加提示词；
    // 提示词须为 cork 编码，不能是 UTF-8 原始字节。
    // 目标语言默认 interface（按界面语言）；测试环境无 scheme 偏好与词典，
    // 界面语言回退 "chinese"、translate 原样返回（out_lan 默认 english）。
    tree body= ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (is_func (body, DOCUMENT));
    QCOMPARE (int (N (body)), 2);
    QVERIFY (body[0] ==
             compound ("quote-env", tree (DOCUMENT, tree ("hello"))));
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为") * "Chinese");
    QVERIFY (!(body[1] == tree ("请翻译上述文字为中文")));
  }

  void test_compose_translate_target_language_from_preference () {
    // 显式设置目标语言偏好：提示词语言名跟随偏好
    set_user_preference ("ai:translate target language", "english");
    tree body= ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为") * "English");
    reset_user_preference ("ai:translate target language");
  }

  void test_compose_translate_interface_follows_ui_language () {
    // interface（默认）：目标语言跟随界面语言偏好
    set_user_preference ("ai:translate target language", "interface");
    set_user_preference ("language", "french");
    tree body= ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为") * "French");
    reset_user_preference ("ai:translate target language");
    reset_user_preference ("language");
  }

  void test_compose_atomic_selection_chat_keeps_selection_only () {
    // 对话：引用块后留空段，光标落在引用块的下一行
    tree body= ChatController::composeAiInputBody (tree ("hello"), "chat");
    QVERIFY (is_func (body, DOCUMENT));
    QCOMPARE (int (N (body)), 2);
    QVERIFY (body[0] ==
             compound ("quote-env", tree (DOCUMENT, tree ("hello"))));
    QVERIFY (body[1] == tree (""));
  }

  void test_compose_document_selection_spreads_children () {
    // 多段选区：document 子节点在引用块内依次展开，提示词追加为末段
    tree sel = tree (DOCUMENT, tree ("para 1"), tree ("para 2"));
    tree body= ChatController::composeAiInputBody (sel, "translate");
    QVERIFY (is_func (body, DOCUMENT));
    QCOMPARE (int (N (body)), 2);
    QVERIFY (body[0] == compound ("quote-env", tree (DOCUMENT, tree ("para 1"),
                                                     tree ("para 2"))));
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为") * "Chinese");
  }

  void test_compose_document_selection_chat_no_prompt () {
    tree sel = tree (DOCUMENT, tree ("para 1"), tree ("para 2"));
    tree body= ChatController::composeAiInputBody (sel, "chat");
    QVERIFY (is_func (body, DOCUMENT));
    QCOMPARE (int (N (body)), 2);
    QVERIFY (body[0] == compound ("quote-env", tree (DOCUMENT, tree ("para 1"),
                                                     tree ("para 2"))));
    QVERIFY (body[1] == tree (""));
  }
};

QTEST_MAIN (TestChatController)
#include "qt_chat_controller_test.moc"
