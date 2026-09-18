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
#include "locale.hpp"
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

  // composeAiInputBody 的提示词测试都显式指定两个语言偏好：默认值 system
  // 解析为系统语言（get_locale_language），随机器而变（Linux 无 LANG 时回落
  // english），不写死就无法断言。
  void set_ai_lang (string prompt, string target) {
    set_user_preference ("ai:prompt language", prompt);
    set_user_preference ("ai:translate target language", target);
  }
  void reset_ai_lang () {
    reset_user_preference ("ai:prompt language");
    reset_user_preference ("ai:translate target language");
  }

  void test_compose_atomic_selection_translate_appends_prompt () {
    // 单行选区（selection_get 的原子串形态）：包成引用块并追加提示词。
    // 提示词语言 chinese 命中仓库 zh_CN 词典（$TEXMACS_PATH 可用）的
    // "please translate the above text into" 词条，得到中文提示词。
    set_ai_lang ("chinese", "chinese");
    tree body= ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (is_func (body, DOCUMENT));
    QCOMPARE (int (N (body)), 2);
    QVERIFY (body[0] ==
             compound ("quote-env", tree (DOCUMENT, tree ("hello"))));
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为中文"));
    reset_ai_lang ();
  }

  void test_compose_translate_target_language_from_preference () {
    // 显式设置目标语言偏好：提示词里的目标语言名跟随偏好（按提示词语言
    // 本地化，此处提示词语言为中文）
    set_ai_lang ("chinese", "english");
    tree body= ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为英语"));
    reset_ai_lang ();
  }

  void test_compose_translate_system_follows_locale () {
    // system（默认）：跟随系统语言（get_locale_language），与界面语言偏好
    // 无关。系统语言随机器而变，故不写死期望串，改与「显式指定系统语言」
    // 的结果比对——同时钉住「改界面语言不影响 system」
    set_user_preference ("language", "french");
    set_ai_lang ("system", "system");
    tree viaSystem=
        ChatController::composeAiInputBody (tree ("hello"), "translate");
    set_ai_lang (get_locale_language (), get_locale_language ());
    tree viaExplicit=
        ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (viaSystem[1] == viaExplicit[1]);

    reset_ai_lang ();
    reset_user_preference ("language");
  }

  void test_compose_translate_prompt_language_preference () {
    // AI 提示词语言（0995）：与翻译目标语言相互独立——同一目标语言
    // （japanese）下，提示词语言 english 出英文整句 + 空格拼接，chinese
    // 出 zh_CN 词条整句且 CJK 词间无空格（目标语言名随提示词语言本地化）
    set_ai_lang ("english", "japanese");
    tree body= ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (body[1] ==
             string ("Please translate the above text into Japanese"));

    set_ai_lang ("chinese", "japanese");
    body= ChatController::composeAiInputBody (tree ("hello"), "translate");
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为日语"));

    reset_ai_lang ();
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
    set_ai_lang ("chinese", "chinese");
    tree sel = tree (DOCUMENT, tree ("para 1"), tree ("para 2"));
    tree body= ChatController::composeAiInputBody (sel, "translate");
    QVERIFY (is_func (body, DOCUMENT));
    QCOMPARE (int (N (body)), 2);
    QVERIFY (body[0] == compound ("quote-env", tree (DOCUMENT, tree ("para 1"),
                                                     tree ("para 2"))));
    QVERIFY (body[1] == utf8_to_cork ("请翻译上述文字为中文"));
    reset_ai_lang ();
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

  void test_compose_gloss_prompt_language () {
    // 释义：引用1 = 上下文（document 树，公式保留为子树），引用2 = 选区，
    // 编号标签与说明句都在提示词内。
    // 提示词语言 english（from==to，translate 剥掉 :: 后缀返回英文键）与
    // chinese（命中 zh_CN 词典，词条值 Cork 编码）两种组合
    tree ctx= tree (DOCUMENT, tree ("hello world"));
    set_ai_lang ("english", "english");
    tree body=
        ChatController::composeAiInputBody (tree ("world"), "gloss", ctx);
    QVERIFY (is_func (body, DOCUMENT));
    QCOMPARE (int (N (body)), 5);
    QVERIFY (body[0] == tree ("reference 1"));
    QVERIFY (body[1] == compound ("quote-env", ctx));
    QVERIFY (body[2] == tree ("reference 2"));
    QVERIFY (body[3] ==
             compound ("quote-env", tree (DOCUMENT, tree ("world"))));
    QVERIFY (body[4] ==
             tree ("reference 2 is part of reference 1, explain the meaning of "
                   "reference 2 (including dictionary and technical terms)"));

    set_ai_lang ("chinese", "chinese");
    body= ChatController::composeAiInputBody (tree ("world"), "gloss", ctx);
    QCOMPARE (int (N (body)), 5);
    QVERIFY (body[0] == utf8_to_cork ("引用1"));
    QVERIFY (body[2] == utf8_to_cork ("引用2"));
    QVERIFY (body[4] ==
             utf8_to_cork ("引文2是引文1的一部分，解释一下引文2的含义（含义的"
                           "范围包括字典、专业术语等）"));

    reset_ai_lang ();
  }

  void test_compose_gloss_context_preserves_formula () {
    // 引文1 的上下文以 document 树传入（Scheme 侧 ai-selection-context 已把
    // 公式哨兵换回子树）：引用块直接包装该树，公式子树原样保留。
    // 公式节点用枚举标签构造：compound("with",...) 在未初始化标准标签表的
    // 单测环境会 intern 出扩展标签，与 WITH 枚举不等
    tree ctx = tree (DOCUMENT,
                     tree (CONCAT, tree ("see "),
                           tree (WITH, tree ("mode"), tree ("math"), tree ("x")),
                           tree (" here")));
    tree body= ChatController::composeAiInputBody (tree ("sel"), "gloss", ctx);
    QVERIFY (body[1] == compound ("quote-env", ctx));
  }
};

QTEST_MAIN (TestChatController)
#include "qt_chat_controller_test.moc"
