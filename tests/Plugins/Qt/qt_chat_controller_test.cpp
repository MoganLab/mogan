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
#include <QPushButton>
#include <QtTest/QtTest>

#include "converter.hpp"
#include "locale.hpp"
#include "preferences.hpp"

using namespace moebius;

class TestChatController : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

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

  void test_compose_explain_prompt_language () {
    // 释义：引用1 = 上下文（document 树，公式保留为子树），引用2 = 选区，
    // 编号标签与说明句都在提示词内。
    // 提示词语言 english（from==to，translate 剥掉 :: 后缀返回英文键）与
    // chinese（命中 zh_CN 词典，词条值 Cork 编码）两种组合
    tree ctx= tree (DOCUMENT, tree ("hello world"));
    set_ai_lang ("english", "english");
    tree body=
        ChatController::composeAiInputBody (tree ("world"), "explain", ctx);
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
    body= ChatController::composeAiInputBody (tree ("world"), "explain", ctx);
    QCOMPARE (int (N (body)), 5);
    QVERIFY (body[0] == utf8_to_cork ("引用1"));
    QVERIFY (body[2] == utf8_to_cork ("引用2"));
    QVERIFY (body[4] ==
             utf8_to_cork ("引文2是引文1的一部分，解释一下引文2的含义（含义的"
                           "范围包括字典、专业术语等）"));

    reset_ai_lang ();
  }

  void test_compose_explain_context_preserves_formula () {
    // 引文1 的上下文以 document 树传入（Scheme 侧 ai-selection-context 已把
    // 公式哨兵换回子树）：引用块直接包装该树，公式子树原样保留。
    // 公式节点用枚举标签构造：compound("with",...) 在未初始化标准标签表的
    // 单测环境会 intern 出扩展标签，与 WITH 枚举不等
    tree ctx= tree (DOCUMENT,
                    tree (CONCAT, tree ("see "),
                          tree (WITH, tree ("mode"), tree ("math"), tree ("x")),
                          tree (" here")));
    tree body=
        ChatController::composeAiInputBody (tree ("sel"), "explain", ctx);
    QVERIFY (body[1] == compound ("quote-env", ctx));
  }

  // === syncDockActionButtons ===
  void test_syncDockActionButtons () {
    ChatController            ctrl;
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    ctrl.view_= &widget;

    string sid1= ctrl.sessionManager_.createSession ();
    ctrl.sessionManager_.setType (sid1, "translate");

    ctrl.syncDockActionButtons (sid1);
    QVERIFY (widget.translateButton ()->isChecked ());
    QVERIFY (!widget.explainButton ()->isChecked ());

    string sid2= ctrl.sessionManager_.createSession ();
    ctrl.sessionManager_.setType (sid2, "explain");

    ctrl.syncDockActionButtons (sid2);
    QVERIFY (!widget.translateButton ()->isChecked ());
    QVERIFY (widget.explainButton ()->isChecked ());

    string sid3= ctrl.sessionManager_.createSession ();
    ctrl.sessionManager_.setType (sid3, "");

    ctrl.syncDockActionButtons (sid3);
    QVERIFY (!widget.translateButton ()->isChecked ());
    QVERIFY (!widget.explainButton ()->isChecked ());

    ctrl.view_= nullptr;
  }

  // === 附件本地预检（纯函数） ===

  static AttachmentItem make_attachment (const QString& path,
                                         const QString& name, qint64 size) {
    AttachmentItem it;
    it.path    = path;
    it.filename= name;
    it.size    = size;
    return it;
  }

  void test_attachment_extension_allowed_documents () {
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.pdf"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.docx"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.xls"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.pptx"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.md"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.txt"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.csv"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.epub"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.html"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.json"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.log"));
    // 大小写不敏感
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.PDF"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.Md"));
  }

  void test_attachment_extension_allowed_source_code () {
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.c"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.cpp"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.h"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.java"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.py"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.js"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.ts"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.go"));
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/a.rs"));
    // 路径含空格
    QVERIFY (ChatController::attachment_extension_allowed ("/tmp/my dir/a.py"));
  }

  void test_attachment_extension_rejected () {
    QVERIFY (!ChatController::attachment_extension_allowed ("/tmp/a.exe"));
    QVERIFY (!ChatController::attachment_extension_allowed ("/tmp/a.zip"));
    QVERIFY (!ChatController::attachment_extension_allowed ("/tmp/a.mp4"));
    // 无扩展名与隐藏文件（QFileInfo 视 .hidden 无后缀）均拒绝
    QVERIFY (!ChatController::attachment_extension_allowed ("/tmp/noext"));
    QVERIFY (!ChatController::attachment_extension_allowed ("/tmp/.hidden"));
  }

  void test_attachment_precheck_total_size_boundary () {
    // 每轮总大小上限 80 MiB（含已选累计）：恰好通过，+1 拒绝
    qint64         maxBytes= ChatController::kAttachmentMaxTotalSize;
    AttachmentItem big= make_attachment ("/tmp/a.pdf", "a.pdf", maxBytes - 100);
    QCOMPARE (ChatController::attachment_precheck_error (big, 0, 100),
              QString (""));
    QVERIFY (
        !ChatController::attachment_precheck_error (big, 0, 101).isEmpty ());
    // 单文件超过总上限：由总大小约束拒绝（不再有单文件独立上限）
    QVERIFY (!ChatController::attachment_precheck_error (
                  make_attachment ("/tmp/a.pdf", "a.pdf", maxBytes + 1), 0, 0)
                  .isEmpty ());
    QCOMPARE (ChatController::attachment_precheck_error (
                  make_attachment ("/tmp/a.pdf", "a.pdf", 0), 0, 0),
              QString (""));
  }

  void test_attachment_precheck_count_limit () {
    AttachmentItem ok= make_attachment ("/tmp/a.pdf", "a.pdf", 10);
    // 已选 9 个可加第 10 个；已选 10 个（上限）拒绝
    QCOMPARE (ChatController::attachment_precheck_error (ok, 9, 0),
              QString (""));
    QVERIFY (!ChatController::attachment_precheck_error (ok, 10, 0).isEmpty ());
  }

  void test_attachment_precheck_extension_first () {
    // 扩展名白名单最先判定：不过白名单时其余条件不再参与
    QVERIFY (!ChatController::attachment_precheck_error (
                  make_attachment ("/tmp/a.exe", "a.exe", 1), 99, 0)
                  .isEmpty ());
    // 拒绝原因包含文件名（%1 已替换）
    QString err= ChatController::attachment_precheck_error (
        make_attachment ("/tmp/a.exe", "a.exe", 1), 0, 0);
    QVERIFY (err.contains ("a.exe"));
  }
};

QTEST_MAIN (TestChatController)
#include "qt_chat_controller_test.moc"
