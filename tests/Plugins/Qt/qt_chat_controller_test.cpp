/******************************************************************************
 * MODULE     : qt_chat_controller_test.cpp
 * DESCRIPTION: Tests for ChatController helper functions
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/AttachMenuBridge.hpp"
#include "Qt/qt_chat_controller.hpp"
#include "base.hpp"
#include <QPushButton>
#include <QQuickItem>
#include <QQuickWidget>
#include <QToolButton>
#include <QtTest/QtTest>

#include "converter.hpp"
#include "locale.hpp"
#include "preferences.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"

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

  // === 附件下发序列化（预检在附件插件侧，见 chat-protocol-test） ===

  static AttachmentItem make_attachment (const QString& path,
                                         const QString& name, qint64 size) {
    AttachmentItem it;
    it.path    = path;
    it.filename= name;
    it.size    = size;
    return it;
  }

  void test_attachment_items_to_scheme () {
    // object API 依赖 tm_s7 与 object-stack：最小启动 s7 解释器
    // （不加载 TeXmacs boot），初始化序列仿 tm_server.cpp
    static bool schemeStarted= false;
    if (!schemeStarted) {
      schemeStarted= true;
      start_scheme (0, nullptr, [] (int, char**) {});
      // eval 会把结果包成 object（构造函数依赖 object-stack），定义
      // object-stack 须走返回裸 tmscm 的 eval_scheme_root
      eval_scheme_root ("(define object-stack '(()))");
      initialize_scheme ();
    }
    // 空表 → null_object（scheme 侧 '()）
    QVERIFY (is_null (
        ChatController::attachment_items_to_scheme (QList<AttachmentItem> ())));
    // 处理中条目不下传（防御）
    AttachmentItem pending= make_attachment ("/tmp/p.pdf", "p.pdf", 10);
    QVERIFY (is_null (ChatController::attachment_items_to_scheme (
        QList<AttachmentItem>{pending})));
    // ready 条目序列化为 sha256/filename 两键，不含本地路径与大小
    AttachmentItem ok= make_attachment ("/tmp/secret/dir/a.pdf", "a.pdf", 42);
    ok.ready         = true;
    ok.sha256        = "deadbeef";
    object scm       = ChatController::attachment_items_to_scheme (
        QList<AttachmentItem>{pending, ok});
    QVERIFY (!is_null (scm));
    array<object> arr= as_array_object (scm);
    QCOMPARE ((int) N (arr), 1);
    // alist 形态：(("sha256" . s) ("filename" . f))，仅两键
    object item= arr[0];
    string k1  = as_string (car (car (item)));
    string v1  = as_string (cdr (car (item)));
    string k2  = as_string (car (car (cdr (item))));
    string v2  = as_string (cdr (car (cdr (item))));
    QVERIFY (k1 == "sha256");
    QVERIFY (v1 == "deadbeef");
    QVERIFY (k2 == "filename");
    QVERIFY (v2 == "a.pdf");
    // 恰好两键：无 size/path 等第三键
    QCOMPARE ((int) N (as_array_object (item)), 2);
    // 非 ASCII 文件名须为 herk 内部形态（中文成 <#XXXX> 实体）：裸 UTF-8
    // 会被 connection_write 的 tree_herk_to_utf8 逐字节二次映射打乱，
    // llm 插件收到乱码文件名
    AttachmentItem cjk= make_attachment ("/tmp/论文/", "北大论文.pdf", 42);
    cjk.ready         = true;
    cjk.sha256        = "deadbeef";
    object scmCjk=
        ChatController::attachment_items_to_scheme (QList<AttachmentItem>{cjk});
    object itemCjk= as_array_object (scmCjk)[0];
    string vCjk   = as_string (cdr (car (cdr (itemCjk))));
    QVERIFY (vCjk == "<#5317><#5927><#8BBA><#6587>.pdf");
  }

  void test_attachment_error_routing_and_send_gate () {
    ChatController            ctrl;
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    ctrl.view_= &widget;

    string                 sid  = ctrl.sessionManager_.createSession ();
    ChatConversationPanel* panel= widget.createPanel (sid);
    QVERIFY (panel != nullptr);
    ChatSession* session= ctrl.sessionManager_.getSession (sid);
    session->panel      = panel;

    // 初始状态：无附件，发送按钮可用
    ctrl.updateSendGate (sid);
    QVERIFY (panel->sendButton ()->isEnabled ());

    // 添加处理中附件：发送按钮被发送栅栏禁用
    AttachmentItem it= make_attachment ("/tmp/noperm.pdf", "noperm.pdf", 10);
    it.taskId        = "t-noperm";
    it.ready         = false;
    panel->setAttachments (QList<AttachmentItem>{it});
    ctrl.updateSendGate (sid);
    QVERIFY (panel->hasProcessingAttachments ());
    QVERIFY (!panel->sendButton ()->isEnabled ());

    // 注册该 task 所属 session
    ctrl.attachTaskSession_.insert ("t-noperm", to_qstring (sid));

    // 路由 onAttachmentError：panel 处理中条目被移除，发送栅栏重新使能发送按钮
    ctrl.onAttachmentError ("t-noperm", "附件文件不可读取： noperm.pdf");
    ctrl.updateSendGate (sid);

    QVERIFY (!panel->hasProcessingAttachments ());
    QVERIFY (panel->attachments ().isEmpty ());
    QVERIFY (panel->sendButton ()->isEnabled ());
    QVERIFY (!ctrl.attachTaskSession_.contains ("t-noperm"));

    // 二次调用：已 take，安全无操作
    ctrl.onAttachmentError ("t-noperm", "附件文件不可读取： noperm.pdf");
    QVERIFY (!ctrl.attachTaskSession_.contains ("t-noperm"));

    ctrl.view_= nullptr;
  }

  void test_attach_menu_meta_lists_current_attachments () {
    ChatController            ctrl;
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    ctrl.view_= &widget;

    string                 sid  = ctrl.sessionManager_.createSession ();
    ChatConversationPanel* panel= widget.createPanel (sid);
    QVERIFY (panel != nullptr);
    ctrl.sessionManager_.getSession (sid)->panel= panel;

    AttachmentItem regular=
        make_attachment ("/tmp/regular.pdf", "regular.pdf", 10);
    regular.ready= true;
    AttachmentItem converted=
        make_attachment ("/tmp/convert/doc.pdf", "doc.pdf", 20);
    converted.sourcePath= "/tmp/doc.tmu";
    converted.ready     = true;
    panel->setAttachments (QList<AttachmentItem>{regular, converted});

    AttachMenuBridge bridge (nullptr, sid, &ctrl);
    QVariantList     attached= bridge.meta ()["attached"].toList ();
    QCOMPARE (attached.size (), 2);
    QCOMPARE (attached[0].toMap ()["url"].toString (),
              QString ("/tmp/regular.pdf"));
    QCOMPARE (attached[0].toMap ()["title"].toString (),
              QString ("regular.pdf"));
    QCOMPARE (attached[1].toMap ()["url"].toString (),
              QString ("/tmp/doc.tmu"));

    ctrl.view_= nullptr;
  }

  void test_attach_menu_popup_resizes_when_attached_expanded () {
    ChatController            ctrl;
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    ctrl.view_= &widget;

    string                 sid  = ctrl.sessionManager_.createSession ();
    ChatConversationPanel* panel= widget.createPanel (sid);
    QVERIFY (panel != nullptr);
    ChatSession* session= ctrl.sessionManager_.getSession (sid);
    session->panel      = panel;
    session->title      = "chat";

    QList<AttachmentItem> items;
    for (int i= 0; i < 4; i++) {
      AttachmentItem item= make_attachment (QString ("/tmp/a%1.pdf").arg (i),
                                            QString ("a%1.pdf").arg (i), 10);
      item.ready         = true;
      items.append (item);
    }
    panel->setAttachments (items);

    QRect  avail= widget.screen ()->availableGeometry ();
    QPoint anchor (avail.center ().x (), avail.bottom () - 50);
    ctrl.onAttachRequested (sid, anchor);
    QVERIFY (ctrl.attachMenuPopup_);
    QWidget* popup= ctrl.attachMenuPopup_;
    auto*    quick= popup->findChild<QQuickWidget*> ();
    QVERIFY (quick && quick->rootObject ());
    QCOMPARE (quick->status (), QQuickWidget::Ready);

    int collapsedHeight= popup->height ();
    int quickHeight    = quick->height ();
    int rootHeight= quick->rootObject ()->property ("implicitHeight").toInt ();
    int anchorBottom= popup->y () + collapsedHeight;
    auto* bridge    = popup->findChild<AttachMenuBridge*> ();
    QVERIFY (bridge);
    QSignalSpy resizeSpy (bridge, &AttachMenuBridge::resizeRequested);
    quick->rootObject ()->setProperty ("attachedExpanded", true);
    QTRY_VERIFY (resizeSpy.count () > 0);
    QTRY_VERIFY (quick->rootObject ()->property ("implicitHeight").toInt () >
                 rootHeight);
    QTRY_VERIFY (quick->height () > quickHeight);
    QTRY_VERIFY (popup->height () > collapsedHeight);
    // 向上弹出的菜单展开时底边不动，新增高度向上增长而不是被裁掉
    QCOMPARE (popup->y () + popup->height (), anchorBottom);

    quick->rootObject ()->setProperty ("attachedExpanded", false);
    QTRY_COMPARE (popup->height (), collapsedHeight);
    QCOMPARE (popup->y () + popup->height (), anchorBottom);

    popup->close ();
    ctrl.view_= nullptr;
  }

  // === 提取后累计上限（totalReadyExtractedChars + kMaxTotalExtractedChars）===

  // 构造一个已就绪条目：累计只统计 ready，构造时须置 ready=true
  static AttachmentItem readyItem (const QString& name, qint64 chars) {
    AttachmentItem it;
    it.filename      = name;
    it.taskId        = name;
    it.ready         = true;
    it.extractedChars= chars;
    return it;
  }

  void test_total_chars_empty_list () {
    QList<AttachmentItem> items;
    QCOMPARE (ChatController::totalReadyExtractedChars (items), (qint64) 0);
    // 空列表恒不超限
    QVERIFY (ChatController::totalReadyExtractedChars (items) <=
             ChatController::kMaxTotalExtractedChars);
  }

  void test_total_chars_single_at_threshold () {
    QList<AttachmentItem> items;
    items.append (
        readyItem ("exact.tmu", ChatController::kMaxTotalExtractedChars));
    // 恰好等于阈值：通过（判定为「大于」才拒绝）
    QCOMPARE (ChatController::totalReadyExtractedChars (items),
              ChatController::kMaxTotalExtractedChars);
    QVERIFY (ChatController::totalReadyExtractedChars (items) <=
             ChatController::kMaxTotalExtractedChars);
  }

  void test_total_chars_single_over_threshold () {
    QList<AttachmentItem> items;
    items.append (
        readyItem ("over.tmu", ChatController::kMaxTotalExtractedChars + 1));
    // 超一即拒
    QVERIFY (ChatController::totalReadyExtractedChars (items) >
             ChatController::kMaxTotalExtractedChars);
  }

  void test_total_chars_accumulates_multiple () {
    QList<AttachmentItem> items;
    const qint64          half= ChatController::kMaxTotalExtractedChars / 2;
    items.append (readyItem ("a.txt", half));
    items.append (readyItem ("b.txt", half));
    // 单条均在阈内，合计正好触阈：仍通过
    QCOMPARE (ChatController::totalReadyExtractedChars (items),
              ChatController::kMaxTotalExtractedChars);
    QVERIFY (ChatController::totalReadyExtractedChars (items) <=
             ChatController::kMaxTotalExtractedChars);

    // 再加一条即超限
    items.append (readyItem ("c.txt", 1));
    QVERIFY (ChatController::totalReadyExtractedChars (items) >
             ChatController::kMaxTotalExtractedChars);
  }

  void test_total_chars_ignores_processing () {
    QList<AttachmentItem> items;
    items.append (readyItem ("done.txt", 100));
    // 处理中条目 extractedChars 尚未回填（0），即便字段有值也不该计入，
    // 否则会在 ready 之前用陈旧值判定
    AttachmentItem pending;
    pending.taskId        = "busy.txt";
    pending.ready         = false;
    pending.extractedChars= ChatController::kMaxTotalExtractedChars;
    items.append (pending);
    QCOMPARE (ChatController::totalReadyExtractedChars (items), (qint64) 100);
  }

  void test_threshold_value_is_one_mib () {
    QCOMPARE (ChatController::kMaxTotalExtractedChars, (qint64) 1048576);
  }
};

QTEST_MAIN (TestChatController)
#include "qt_chat_controller_test.moc"
