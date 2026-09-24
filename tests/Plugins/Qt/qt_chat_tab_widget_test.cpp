
/******************************************************************************
 * MODULE     : qt_chat_tab_widget_test.cpp
 * DESCRIPTION: Tests for QTChatTabWidget helper functions
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_chat_tab_widget.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QActionGroup>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QWheelEvent>
#include <QWidget>
#include <QtTest/QtTest>

using namespace moebius;

class TestChatTabWidget : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    // 重置全局侧边栏折叠状态，避免测试间互相影响
    QTChatTabWidget::setGlobalSidebarCollapsed (false);
  }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  // === chat_effort_menu_populate ===
  void test_effort_menu_structure () {
    QMenu  menu;
    QMenu* sub= chat_effort_menu_populate (&menu, "medium");
    QVERIFY (sub != nullptr);
    // 子菜单挂在顶层菜单尾部，且前面有分隔符
    QList<QAction*> topActions= menu.actions ();
    QCOMPARE (topActions.size (), 2);
    QVERIFY (topActions.at (0)->isSeparator ());
    QCOMPARE (topActions.at (1)->menu (), sub);
    // 三档互斥、均可勾选、data 为强度值
    QList<QAction*> actions= sub->actions ();
    QCOMPARE (actions.size (), 3);
    QStringList expected= {"low", "medium", "high"};
    for (int i= 0; i < 3; i++) {
      QVERIFY (actions.at (i)->isCheckable ());
      QCOMPARE (actions.at (i)->data ().toString (), expected.at (i));
    }
    QActionGroup* group= actions.first ()->actionGroup ();
    QVERIFY (group != nullptr);
    QVERIFY (group->isExclusive ());
    QCOMPARE (group->actions ().size (), 3);
  }

  void test_effort_menu_checked_state () {
    QMenu           menu;
    QMenu*          sub    = chat_effort_menu_populate (&menu, "high");
    QList<QAction*> actions= sub->actions ();
    QVERIFY (!actions.at (0)->isChecked ());
    QVERIFY (!actions.at (1)->isChecked ());
    QVERIFY (actions.at (2)->isChecked ());
  }

  void test_effort_menu_invalid_effort_none_checked () {
    QMenu  menu;
    QMenu* sub= chat_effort_menu_populate (&menu, "bogus");
    for (QAction* a : sub->actions ())
      QVERIFY (!a->isChecked ());
  }

  // === input_lines_for_extent：画布高度 → 内容行数（通用判定） ===
  void test_input_lines_for_extent_exact_multiples () {
    // 整倍数：正好 N 行
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (22, 22), 1);
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (44, 22), 2);
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (66, 22), 3);
  }

  void test_input_lines_for_extent_rounds_up () {
    // 不足一行按一行计（向上取整）
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (23, 22), 2);
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (45, 22), 3);
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (67, 22), 4);
  }

  void test_input_lines_for_extent_minimum_one_line () {
    // 空内容 / 非正值至少计 1 行
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (0, 22), 1);
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (-5, 22), 1);
  }

  void test_input_lines_for_extent_invalid_line_px () {
    // 行高非正（未标定且常量异常）时按 1 行兜底，不除零
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (100, 0), 1);
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (100, -3), 1);
  }

  void test_input_lines_for_extent_large_content () {
    // 大量内容（长引用、整段粘贴）按高度如实折算
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (1000, 22), 46);
    QCOMPARE (ChatConversationPanel::input_lines_for_extent (2200, 22), 100);
  }

  // === input_height_step_lines 固定档位（基准 3 行的 1~5 倍） ===
  void test_input_height_step_lines_within_default () {
    // 1~3 行：维持现有大小（1 倍基准）
    QCOMPARE (ChatConversationPanel::input_height_step_lines (0), 3);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (1), 3);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (2), 3);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (3), 3);
  }

  void test_input_height_step_lines_double () {
    // 4~6 行：固定扩为现有大小的 2 倍
    QCOMPARE (ChatConversationPanel::input_height_step_lines (4), 6);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (5), 6);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (6), 6);
  }

  void test_input_height_step_lines_triple_to_quintuple_capped () {
    // 7 行起逐档放大：3 倍（7~9 行）、4 倍（10~12 行）、5 倍封顶
    QCOMPARE (ChatConversationPanel::input_height_step_lines (7), 9);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (9), 9);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (10), 12);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (12), 12);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (13), 15);
    QCOMPARE (ChatConversationPanel::input_height_step_lines (20), 15);
  }

  void test_is_empty_document_body_truly_empty () {
    // tree(DOCUMENT) 在 TeXmacs 中实际创建的是带有一个空子节点的 DOCUMENT
    // 空文档的标准表示是 tree(DOCUMENT, "")
    tree empty_doc= tree (DOCUMENT, "");
    QVERIFY (ChatConversationPanel::is_empty_document_body (empty_doc));
  }

  void test_is_empty_document_body_with_empty_string () {
    tree doc= tree (DOCUMENT, "");
    QVERIFY (ChatConversationPanel::is_empty_document_body (doc));
  }

  void test_is_empty_document_body_not_empty () {
    tree doc= tree (DOCUMENT, "hello");
    QVERIFY (!ChatConversationPanel::is_empty_document_body (doc));
  }

  void test_is_empty_document_body_not_document () {
    tree not_doc= tree (WITH, "font", "roman", "hello");
    QVERIFY (!ChatConversationPanel::is_empty_document_body (not_doc));
  }

  void test_is_empty_document_body_multiple_paragraphs () {
    tree doc= tree (DOCUMENT, "para1", "para2");
    QVERIFY (!ChatConversationPanel::is_empty_document_body (doc));
  }

  void test_is_empty_document_body_missing_buffer () {
    // get_buffer_body 对不存在的 buffer 返回原子空串，必须视为空文档，
    // 否则复用空白会话会被误判有消息而进入对话模式（消息区误显示）
    tree missing= tree ("");
    QVERIFY (ChatConversationPanel::is_empty_document_body (missing));
  }

  void test_is_empty_document_body_atomic_non_empty () {
    tree atom= tree ("hello");
    QVERIFY (!ChatConversationPanel::is_empty_document_body (atom));
  }

  // === setSidebarCollapsed / isSidebarCollapsed ===
  void test_setSidebarCollapsed_expand () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.setSidebarCollapsed (false);
    QVERIFY (!widget.isSidebarCollapsed ());
  }

  void test_setSidebarCollapsed_collapse () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.setSidebarCollapsed (true);
    QVERIFY (widget.isSidebarCollapsed ());
  }

  void test_setSidebarCollapsed_toggle () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    bool                      initial= widget.isSidebarCollapsed ();
    widget.setSidebarCollapsed (!initial);
    QCOMPARE (widget.isSidebarCollapsed (), !initial);
    widget.setSidebarCollapsed (initial);
    QCOMPARE (widget.isSidebarCollapsed (), initial);
  }

  void test_setSidebarCollapsed_idempotent () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.setSidebarCollapsed (true);
    widget.setSidebarCollapsed (true);
    QVERIFY (widget.isSidebarCollapsed ());
    widget.setSidebarCollapsed (false);
    widget.setSidebarCollapsed (false);
    QVERIFY (!widget.isSidebarCollapsed ());
  }

  void test_setSidebarCollapsed_affects_widget_visibility () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show (); // 必须 show 才能检查实际 Qt 可见性
    // 默认侧边栏可见，浮动按钮隐藏
    QVERIFY (widget.isSidebarWidgetVisible ());
    QVERIFY (!widget.isFloatingContainerVisible ());

    widget.setSidebarCollapsed (true);
    QVERIFY (!widget.isSidebarWidgetVisible ());
    QVERIFY (widget.isFloatingContainerVisible ());

    widget.setSidebarCollapsed (false);
    QVERIFY (widget.isSidebarWidgetVisible ());
    QVERIFY (!widget.isFloatingContainerVisible ());
  }

  // === globalSidebarCollapsed 全局状态记忆 ===
  void test_globalSidebarCollapsed_default () {
    QCOMPARE (QTChatTabWidget::globalSidebarCollapsed (), false);
  }

  void test_globalSidebarCollapsed_set_and_get () {
    QTChatTabWidget::setGlobalSidebarCollapsed (true);
    QVERIFY (QTChatTabWidget::globalSidebarCollapsed ());

    QTChatTabWidget::setGlobalSidebarCollapsed (false);
    QVERIFY (!QTChatTabWidget::globalSidebarCollapsed ());
  }

  void test_constructor_respects_global_collapsed () {
    QTChatTabWidget::setGlobalSidebarCollapsed (true);
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();

    QVERIFY (widget.isSidebarCollapsed ());
    QVERIFY (!widget.isSidebarWidgetVisible ());
    QVERIFY (widget.isFloatingContainerVisible ());
  }

  void test_constructor_respects_global_expanded () {
    QTChatTabWidget::setGlobalSidebarCollapsed (false);
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();

    QVERIFY (!widget.isSidebarCollapsed ());
    QVERIFY (widget.isSidebarWidgetVisible ());
    QVERIFY (!widget.isFloatingContainerVisible ());
  }

  void test_setSidebarCollapsed_updates_global () {
    QTChatTabWidget::setGlobalSidebarCollapsed (false);
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);

    widget.setSidebarCollapsed (true);
    QVERIFY (QTChatTabWidget::globalSidebarCollapsed ());

    widget.setSidebarCollapsed (false);
    QVERIFY (!QTChatTabWidget::globalSidebarCollapsed ());
  }

  void test_global_state_persists_across_instances () {
    QTChatTabWidget::setGlobalSidebarCollapsed (false);
    QList<SessionDisplayInfo> sessions;

    {
      QTChatTabWidget widget1 (sessions, "", nullptr);
      widget1.setSidebarCollapsed (true);
      QVERIFY (QTChatTabWidget::globalSidebarCollapsed ());
    }

    {
      QTChatTabWidget widget2 (sessions, "", nullptr);
      widget2.show ();
      QVERIFY (widget2.isSidebarCollapsed ());
      QVERIFY (!widget2.isSidebarWidgetVisible ());
      QVERIFY (widget2.isFloatingContainerVisible ());
    }
  }

  // === setSidebarVisible (dock 模式专用) ===
  void test_setSidebarVisible_hide () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    widget.setSidebarVisible (false);
    QVERIFY (!widget.isSidebarWidgetVisible ());
    QVERIFY (!widget.isFloatingContainerVisible ());
  }

  void test_setSidebarVisible_show () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    widget.setSidebarVisible (false);
    widget.setSidebarVisible (true);
    QVERIFY (widget.isSidebarWidgetVisible ());
    QVERIFY (!widget.isFloatingContainerVisible ());
  }

  void test_setSidebarVisible_does_not_show_floating_buttons () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    widget.setSidebarCollapsed (true); // 正常折叠会显示浮动按钮
    QVERIFY (widget.isFloatingContainerVisible ());

    widget.setSidebarVisible (false); // dock 模式隐藏，不显示浮动按钮
    QVERIFY (!widget.isSidebarWidgetVisible ());
    QVERIFY (!widget.isFloatingContainerVisible ());
  }

  // === close sidebar 按钮 ===
  void test_closeSidebarButton_visible () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    QVERIFY (widget.closeSidebarButton () != nullptr);
    widget.setDockButtonsVisible (true);
    QVERIFY (widget.closeSidebarButton ()->isVisible ());
  }

  void test_closeSidebarButton_hidden () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    widget.setDockButtonsVisible (true);
    widget.setDockButtonsVisible (false);
    QVERIFY (!widget.closeSidebarButton ()->isVisible ());
  }

  void test_closeSidebarButton_emits_signal () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    QSignalSpy spy (&widget, &QTChatTabWidget::closeSidebarRequested);
    widget.setDockButtonsVisible (true);
    QTest::mouseClick (widget.closeSidebarButton (), Qt::LeftButton);
    QCOMPARE (spy.count (), 1);
  }

  // === 最大化按钮（dock 模式） ===
  void test_maximizeButton_default_hidden () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    QVERIFY (widget.maximizeButton () != nullptr);
    QVERIFY (!widget.maximizeButton ()->isVisible ());
  }

  void test_maximizeButton_follows_dock_buttons_visibility () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    widget.setDockButtonsVisible (true);
    QVERIFY (widget.maximizeButton ()->isVisible ());
    widget.setDockButtonsVisible (false);
    QVERIFY (!widget.maximizeButton ()->isVisible ());
  }

  void test_maximizeButton_emits_signal () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    QSignalSpy spy (&widget, &QTChatTabWidget::maximizeRequested);
    widget.setDockButtonsVisible (true);
    QTest::mouseClick (widget.maximizeButton (), Qt::LeftButton);
    QCOMPARE (spy.count (), 1);
  }

  /// 三个 dock 辅助按钮在对话区左上角依次横排，互不重叠
  void test_maximizeButton_sits_right_of_new_chat_button () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    widget.setDockButtonsVisible (true);
    QPushButton* closeBtn  = widget.closeSidebarButton ();
    QPushButton* newChatBtn= widget.newChatSidebarButton ();
    QPushButton* maxBtn    = widget.maximizeButton ();
    QVERIFY (closeBtn != nullptr);
    QVERIFY (newChatBtn != nullptr);
    QVERIFY (maxBtn != nullptr);
    QVERIFY (newChatBtn->x () >= closeBtn->x () + closeBtn->width ());
    QVERIFY (maxBtn->x () >= newChatBtn->x () + newChatBtn->width ());
  }

  // === ChatSidebar title rename ===
  void test_beginEditTitle_shows_editor () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();
    sidebar.beginEditTitle ("s1");

    auto item= sidebar.findChild<QLineEdit*> ("chat-tab-title-edit");
    QVERIFY (item != nullptr);
    QVERIFY (item->isVisible ());
  }

  void test_beginEditTitle_hides_button () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();
    auto button= sidebar.findChild<QPushButton*> ("chat-tab-conversation-btn");
    QVERIFY (button != nullptr);
    QVERIFY (button->isVisible ());

    sidebar.beginEditTitle ("s1");
    QVERIFY (!button->isVisible ());
  }

  void test_endEditTitle_accept_emits_signal () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QString capturedSessionId;
    QString capturedNewTitle;
    connect (&sidebar, &ChatSidebar::renameRequested,
             [&capturedSessionId, &capturedNewTitle] (const string& sessionId,
                                                      const string& newTitle) {
               capturedSessionId= to_qstring (sessionId);
               capturedNewTitle = to_qstring (newTitle);
             });

    sidebar.beginEditTitle ("s1");
    auto edit= sidebar.findChild<QLineEdit*> ("chat-tab-title-edit");
    QVERIFY (edit != nullptr);
    edit->setText ("world");
    emit edit->returnPressed ();

    QCOMPARE (capturedSessionId, QString ("s1"));
    QCOMPARE (capturedNewTitle, QString ("world"));
  }

  void test_endEditTitle_empty_title_no_signal () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    bool signalEmitted= false;
    connect (&sidebar, &ChatSidebar::renameRequested,
             [&signalEmitted] (const string&, const string&) {
               signalEmitted= true;
             });

    sidebar.beginEditTitle ("s1");
    auto edit= sidebar.findChild<QLineEdit*> ("chat-tab-title-edit");
    QVERIFY (edit != nullptr);
    edit->setText ("");
    emit edit->returnPressed ();

    QVERIFY (!signalEmitted);
  }

  void test_endEditTitle_same_title_no_signal () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    bool signalEmitted= false;
    connect (&sidebar, &ChatSidebar::renameRequested,
             [&signalEmitted] (const string&, const string&) {
               signalEmitted= true;
             });

    sidebar.beginEditTitle ("s1");
    auto edit= sidebar.findChild<QLineEdit*> ("chat-tab-title-edit");
    QVERIFY (edit != nullptr);
    edit->setText ("hello");
    emit edit->returnPressed ();

    QVERIFY (!signalEmitted);
  }

  void test_endEditTitle_trims_whitespace () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QString capturedNewTitle;
    connect (&sidebar, &ChatSidebar::renameRequested,
             [&capturedNewTitle] (const string&, const string& newTitle) {
               capturedNewTitle= to_qstring (newTitle);
             });

    sidebar.beginEditTitle ("s1");
    auto edit= sidebar.findChild<QLineEdit*> ("chat-tab-title-edit");
    QVERIFY (edit != nullptr);
    edit->setText ("  world  ");
    emit edit->returnPressed ();

    QCOMPARE (capturedNewTitle, QString ("world"));
  }

  void test_updateItemTitle_updates_titleEdit () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.updateItemTitle ("s1", "world");
    auto edit= sidebar.findChild<QLineEdit*> ("chat-tab-title-edit");
    QVERIFY (edit != nullptr);
    QCOMPARE (edit->text (), QString ("world"));
  }

  // === ChatSidebar exportRequested signal ===
  void test_exportRequested_emitted () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QSignalSpy spy (&sidebar, &ChatSidebar::exportRequested);
    QVERIFY (spy.isValid ());

    // 点击 "..." 按钮会弹出菜单，我们需要模拟菜单中的 Export 动作
    // 直接触发 exportRequested 信号来验证连接
    emit sidebar.exportRequested ("s1");
    QCOMPARE (spy.count (), 1);
    QCOMPARE (to_qstring (spy.at (0).at (0).value<string> ()), QString ("s1"));
  }

  void test_exportRequested_different_session () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false}
             << SessionDisplayInfo{"s2", "world", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QSignalSpy spy (&sidebar, &ChatSidebar::exportRequested);
    QVERIFY (spy.isValid ());

    emit sidebar.exportRequested ("s2");
    QCOMPARE (spy.count (), 1);
    QCOMPARE (to_qstring (spy.at (0).at (0).value<string> ()), QString ("s2"));
  }

  void test_exportRequested_multiple_emissions () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QSignalSpy spy (&sidebar, &ChatSidebar::exportRequested);
    QVERIFY (spy.isValid ());

    emit sidebar.exportRequested ("s1");
    emit sidebar.exportRequested ("s1");
    QCOMPARE (spy.count (), 2);
    // ---- ChatConversationPanel 输入事件判定测试 ----
  }

  // === ChatSidebar addItem (SessionDisplayInfo) ===

  void test_addItem_creates_visible_item () {
    QList<SessionDisplayInfo> sessions;
    ChatSidebar               sidebar (sessions, "", nullptr);
    sidebar.show ();

    SessionDisplayInfo info{"s1", "hello", "", false};
    sidebar.addItem (info);

    auto buttons=
        sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn");
    QCOMPARE (buttons.size (), 1);
    QCOMPARE (buttons[0]->text (), QString ("hello"));
  }

  void test_addItem_sets_archived_state () {
    QList<SessionDisplayInfo> sessions;
    ChatSidebar               sidebar (sessions, "", nullptr);
    sidebar.show ();

    SessionDisplayInfo info{"s1", "archived session", "", true};
    sidebar.addItem (info);

    // 归档项不应出现在活跃列表的按钮中（在归档区）
    auto buttons=
        sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn");
    QCOMPARE (buttons.size (), 1);
    QCOMPARE (buttons[0]->text (), QString ("archived session"));
  }

  void test_addItem_duplicate_ignored () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    SessionDisplayInfo info{"s1", "world", "", false};
    sidebar.addItem (info);

    auto buttons=
        sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn");
    QCOMPARE (buttons.size (), 1);
    QCOMPARE (buttons[0]->text (), QString ("hello"));
  }

  void test_addItem_sets_active () {
    QList<SessionDisplayInfo> sessions;
    ChatSidebar               sidebar (sessions, "", nullptr);
    sidebar.show ();

    SessionDisplayInfo info{"s1", "hello", "", false};
    sidebar.addItem (info);

    QCOMPARE (to_qstring (sidebar.activeSessionId ()), QString ("s1"));
  }

  // === ChatSidebar removeItem ===

  void test_removeItem_destroys_widget () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.removeItem ("s1");

    auto buttons=
        sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn");
    QCOMPARE (buttons.size (), 0);
  }

  void test_removeItem_clears_active () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.removeItem ("s1");
    QCOMPARE (to_qstring (sidebar.activeSessionId ()), QString (""));
  }

  void test_removeItem_nonexistent_noop () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.removeItem ("nonexistent");

    auto buttons=
        sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn");
    QCOMPARE (buttons.size (), 1);
  }

  // === ChatSidebar moveToArchive ===

  void test_moveToArchive_moves_item () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.moveToArchive ("s1");

    // 归档后 activeSessionId 应被清除
    QCOMPARE (to_qstring (sidebar.activeSessionId ()), QString (""));

    // 归档 header 应显示
    auto header= sidebar.findChild<QPushButton*> ("chat-tab-archive-header");
    QVERIFY (header != nullptr);
    QVERIFY (header->isVisible ());
  }

  void test_moveToArchive_already_archived_noop () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", true};
    ChatSidebar sidebar (sessions, "", nullptr);
    sidebar.show ();

    // s1 已归档，再次 moveToArchive 不应崩溃或重复
    sidebar.moveToArchive ("s1");
    auto buttons=
        sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn");
    QCOMPARE (buttons.size (), 1);
  }

  // === ChatSidebar moveFromArchive ===

  void test_moveFromArchive_restores_item () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", true};
    ChatSidebar sidebar (sessions, "", nullptr);
    sidebar.show ();

    sidebar.moveFromArchive ("s1");

    // s1 应回到活跃列表顶部
    auto buttons=
        sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn");
    QCOMPARE (buttons.size (), 1);
  }

  void test_moveFromArchive_already_active_noop () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "hello", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.moveFromArchive ("s1");
    QCOMPARE (to_qstring (sidebar.activeSessionId ()), QString ("s1"));
  }

  // === ChatSidebar updateCountLabels ===

  void test_updateCountLabels_active_count () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "a", "", false}
             << SessionDisplayInfo{"s2", "b", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    auto label= sidebar.findChild<QLabel*> ("chat-tab-conversation-count");
    QVERIFY (label != nullptr);
    QVERIFY (label->text ().contains ("2"));
  }

  void test_updateCountLabels_archived_shows_header () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "a", "", false}
             << SessionDisplayInfo{"s2", "b", "", true};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    auto header= sidebar.findChild<QPushButton*> ("chat-tab-archive-header");
    QVERIFY (header != nullptr);
    QVERIFY (header->isVisible ());
    QVERIFY (header->text ().contains ("1"));
  }

  void test_updateCountLabels_after_remove () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "a", "", false}
             << SessionDisplayInfo{"s2", "b", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.removeItem ("s1");

    auto label= sidebar.findChild<QLabel*> ("chat-tab-conversation-count");
    QVERIFY (label != nullptr);
    QVERIFY (label->text ().contains ("1"));
  }

  // === ChatSidebar exitMultiSelectMode ===

  void test_exitMultiSelectMode_clears_flags () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "a", "", false};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    sidebar.enterMultiSelectMode (false);
    sidebar.exitMultiSelectMode ();

    auto bar= sidebar.findChild<QWidget*> ("chat-tab-multi-select-bar");
    QVERIFY (bar != nullptr);
    QVERIFY (!bar->isVisible ());
  }

  // === 6203: ChatSidebar 会话按 type 分类标签测试 ===

  void test_type_tabs_initial_state () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "chat1", "", false, ""}
             << SessionDisplayInfo{"s2", "trans1", "", false, "translate"}
             << SessionDisplayInfo{"s3", "expl1", "", false, "explain"};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QPushButton* chatBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-chat");
    QPushButton* transBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-translate");
    QPushButton* explBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-explain");

    QCOMPARE (sidebar.currentTypeTab (), ChatSidebar::SessionTypeTab::Chat);
    QVERIFY (chatBtn != nullptr);
    QVERIFY (transBtn != nullptr);
    QVERIFY (explBtn != nullptr);

    QVERIFY (chatBtn->isChecked ());
    QVERIFY (!transBtn->isChecked ());
    QVERIFY (!explBtn->isChecked ());

    QVERIFY (chatBtn->text ().contains ("1"));
    QVERIFY (transBtn->text ().contains ("1"));
    QVERIFY (explBtn->text ().contains ("1"));
  }

  void test_type_tabs_filtering () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "chat_item", "", false, ""}
             << SessionDisplayInfo{"s2", "trans_item", "", false, "translate"}
             << SessionDisplayInfo{"s3", "expl_item", "", false, "explain"};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QPushButton* btnS1= nullptr;
    QPushButton* btnS2= nullptr;
    QPushButton* btnS3= nullptr;
    for (auto* b :
         sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn")) {
      if (b->text () == "chat_item") btnS1= b;
      else if (b->text () == "trans_item") btnS2= b;
      else if (b->text () == "expl_item") btnS3= b;
    }
    QVERIFY (btnS1 != nullptr);
    QVERIFY (btnS2 != nullptr);
    QVERIFY (btnS3 != nullptr);

    // 默认选中「对话」：仅普通会话可见
    QVERIFY (btnS1->parentWidget ()->isVisible ());
    QVERIFY (!btnS2->parentWidget ()->isVisible ());
    QVERIFY (!btnS3->parentWidget ()->isVisible ());

    // 切换到「翻译」
    QPushButton* transBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-translate");
    QPushButton* explBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-explain");
    QVERIFY (transBtn != nullptr);
    QVERIFY (explBtn != nullptr);

    transBtn->click ();
    QCOMPARE (sidebar.currentTypeTab (),
              ChatSidebar::SessionTypeTab::Translate);
    QVERIFY (!btnS1->parentWidget ()->isVisible ());
    QVERIFY (btnS2->parentWidget ()->isVisible ());
    QVERIFY (!btnS3->parentWidget ()->isVisible ());

    // 切换到「释义」
    explBtn->click ();
    QCOMPARE (sidebar.currentTypeTab (), ChatSidebar::SessionTypeTab::Explain);
    QVERIFY (!btnS1->parentWidget ()->isVisible ());
    QVERIFY (!btnS2->parentWidget ()->isVisible ());
    QVERIFY (btnS3->parentWidget ()->isVisible ());
  }

  void test_type_tabs_search_cross_categories () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "Common Alpha", "", false, ""}
             << SessionDisplayInfo{"s2", "Common Beta", "", false, "translate"}
             << SessionDisplayInfo{"s3", "Unique Gamma", "", false, "explain"};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QPushButton* btnS1= nullptr;
    QPushButton* btnS2= nullptr;
    QPushButton* btnS3= nullptr;
    for (auto* b :
         sidebar.findChildren<QPushButton*> ("chat-tab-conversation-btn")) {
      if (b->text () == "Common Alpha") btnS1= b;
      else if (b->text () == "Common Beta") btnS2= b;
      else if (b->text () == "Unique Gamma") btnS3= b;
    }
    QVERIFY (btnS1 != nullptr);
    QVERIFY (btnS2 != nullptr);
    QVERIFY (btnS3 != nullptr);

    auto searchEdit= sidebar.findChild<QLineEdit*> ("chat-tab-search-edit");
    QVERIFY (searchEdit != nullptr);

    // 搜索跨全部类别过滤：搜索 "Common" 命中 s1 和 s2
    searchEdit->setText ("Common");
    QVERIFY (btnS1->parentWidget ()->isVisible ());
    QVERIFY (btnS2->parentWidget ()->isVisible ());
    QVERIFY (!btnS3->parentWidget ()->isVisible ());

    // 清空搜索后恢复当前类别（对话）：仅 s1 可见
    searchEdit->clear ();
    QVERIFY (btnS1->parentWidget ()->isVisible ());
    QVERIFY (!btnS2->parentWidget ()->isVisible ());
    QVERIFY (!btnS3->parentWidget ()->isVisible ());
  }

  void test_type_tabs_click_clears_search () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "Common Alpha", "", false, ""}
             << SessionDisplayInfo{"s2", "Common Beta", "", false, "translate"}
             << SessionDisplayInfo{"s3", "Unique Gamma", "", false, "explain"};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    auto searchEdit= sidebar.findChild<QLineEdit*> ("chat-tab-search-edit");
    QVERIFY (searchEdit != nullptr);

    searchEdit->setText ("Unique");
    QCOMPARE (searchEdit->text (), QString ("Unique"));

    // 点击类别按钮应清除搜索框并切换到目标类别
    QPushButton* transBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-translate");
    QVERIFY (transBtn != nullptr);
    transBtn->click ();
    QCOMPARE (sidebar.currentTypeTab (),
              ChatSidebar::SessionTypeTab::Translate);
    QVERIFY (searchEdit->text ().isEmpty ());
  }

  void test_type_tabs_count_updates () {
    QList<SessionDisplayInfo> sessions;
    ChatSidebar               sidebar (sessions, "", nullptr);
    sidebar.show ();

    QPushButton* chatBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-chat");
    QPushButton* transBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-translate");
    QPushButton* explBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-explain");
    QVERIFY (chatBtn != nullptr);
    QVERIFY (transBtn != nullptr);
    QVERIFY (explBtn != nullptr);

    QVERIFY (transBtn->text ().contains ("0"));
    QVERIFY (chatBtn->text ().contains ("0"));
    QVERIFY (explBtn->text ().contains ("0"));

    sidebar.addItem (SessionDisplayInfo{"s1", "c1", "", false, ""});
    QVERIFY (chatBtn->text ().contains ("1"));

    sidebar.addItem (SessionDisplayInfo{"s2", "t1", "", false, "translate"});
    QVERIFY (transBtn->text ().contains ("1"));

    sidebar.addItem (SessionDisplayInfo{"s3", "e1", "", false, "explain"});
    QVERIFY (explBtn->text ().contains ("1"));

    sidebar.addItem (SessionDisplayInfo{"s4", "t2", "", false, "translate"});
    QVERIFY (transBtn->text ().contains ("2"));

    // 归档不计入活跃类别数量
    sidebar.moveToArchive ("s4");
    QVERIFY (transBtn->text ().contains ("1"));

    sidebar.moveFromArchive ("s4");
    QVERIFY (transBtn->text ().contains ("2"));

    sidebar.removeItem ("s4");
    QVERIFY (transBtn->text ().contains ("1"));
  }

  void test_setActiveItem_switches_type_tab () {
    QList<SessionDisplayInfo> sessions;
    sessions << SessionDisplayInfo{"s1", "c1", "", false, ""}
             << SessionDisplayInfo{"s2", "t1", "", false, "translate"};
    ChatSidebar sidebar (sessions, "s1", nullptr);
    sidebar.show ();

    QCOMPARE (sidebar.currentTypeTab (), ChatSidebar::SessionTypeTab::Chat);

    // 激活非当前类别的会话应自动切换标签页
    sidebar.setActiveItem ("s2");
    QCOMPARE (sidebar.currentTypeTab (),
              ChatSidebar::SessionTypeTab::Translate);
    QPushButton* transBtn=
        sidebar.findChild<QPushButton*> ("chat-tab-type-tab-translate");
    QVERIFY (transBtn != nullptr);
    QVERIFY (transBtn->isChecked ());
  }

  void test_send_on_plain_enter_without_completion_popup () {
    QVERIFY (ChatConversationPanel::should_send_on_keypress (
        Qt::Key_Return, Qt::NoModifier, false));
  }

  void test_send_on_ctrl_enter_without_completion_popup () {
    QVERIFY (ChatConversationPanel::should_send_on_keypress (
        Qt::Key_Return, Qt::ControlModifier, false));
  }

  void test_not_send_on_shift_enter () {
    QVERIFY (!ChatConversationPanel::should_send_on_keypress (
        Qt::Key_Return, Qt::ShiftModifier, false));
  }

  void test_not_send_on_plain_enter_with_completion_popup () {
    QVERIFY (!ChatConversationPanel::should_send_on_keypress (
        Qt::Key_Return, Qt::NoModifier, true, false));
  }

  void test_not_send_on_plain_enter_when_in_hybrid () {
    QVERIFY (!ChatConversationPanel::should_send_on_keypress (
        Qt::Key_Return, Qt::NoModifier, false, true));
  }

  void test_not_send_on_non_enter_key () {
    QVERIFY (!ChatConversationPanel::should_send_on_keypress (
        Qt::Key_A, Qt::NoModifier, false));
    // ---- should_block_readonly_event 测试 ----
  }

  void test_readonly_no_property () {
    // 无 chat_message_readonly 属性的对象 → 不拦截
    QObject   obj;
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_property_false () {
    // 属性显式为 false → 不拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", false);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_plain_keypress () {
    // 无修饰键的 KeyPress → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_enter_keypress () {
    // Enter 键无修饰 → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_c () {
    // Ctrl+C 复制 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_meta_c () {
    // Meta+C（macOS ⌘+C）→ 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_C, Qt::MetaModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_a () {
    // Ctrl+A 全选 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_f () {
    // Ctrl+F 搜索 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_F, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_ctrl_v () {
    // Ctrl+V 粘贴 → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_ctrl_x () {
    // Ctrl+X 剪切 → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_ctrl_z () {
    // Ctrl+Z 撤销 → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_plain_keyrelease () {
    // 无修饰键的 KeyRelease → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_f_keyrelease () {
    // Ctrl+F 的 KeyRelease → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyRelease, Qt::Key_F, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_ctrl_v_keyrelease () {
    // Ctrl+V 的 KeyRelease → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_input_method () {
    // InputMethod 事件 → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QInputMethodEvent ime{QString (), QList<QInputMethodEvent::Attribute>{}};
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ime));
  }

  void test_readonly_allows_mouse_events () {
    // 鼠标事件 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QMouseEvent me (QEvent::MouseButtonPress, QPointF (0, 0), QPointF (0, 0),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &me));
  }

  void test_readonly_allows_wheel_event () {
    // 滚轮事件 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QWheelEvent we (QPointF (0, 0), QPointF (0, 0), QPoint (0, 120),
                    QPoint (0, 120), Qt::NoButton, Qt::NoModifier,
                    Qt::NoScrollPhase, false);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &we));
  }

  // === shift+方向键 选中内容 ===
  void test_readonly_allows_shift_left () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Left, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_right () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Right, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_up () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Up, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_down () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Down, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_home () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Home, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_end () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_End, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_pageup () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_PageUp, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_pagedown () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_PageDown, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_left () {
    // 单独方向键 → 放行（移动光标）
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_right () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_up () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_down () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_home () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_end () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_pageup () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_PageUp, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_plain_pagedown () {
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_plain_shift () {
    // 单独按 Shift 键 → 拦截
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_shift_left () {
    // Ctrl+Shift+Left 选中单词 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Left,
                  Qt::ControlModifier | Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_shift_left_keyrelease () {
    // Shift+Left 的 KeyRelease → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyRelease, Qt::Key_Left, Qt::ShiftModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  // === cmd+/- 缩放 ===
  void test_readonly_allows_ctrl_plus () {
    // Ctrl+Plus 放大 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Plus, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_equal () {
    // Ctrl+Equal（主键盘 + 号）放大 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Equal, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_minus () {
    // Ctrl+Minus 缩小 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Minus, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_meta_plus () {
    // Meta+Plus（macOS Cmd+Plus）放大 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_Plus, Qt::MetaModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_plus_keyrelease () {
    // Ctrl+Plus 的 KeyRelease → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyRelease, Qt::Key_Plus, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  // === Ctrl/Cmd+J AI 侧边栏快捷键 ===
  void test_readonly_allows_ctrl_j () {
    // Ctrl+J 切换 AI 侧边栏 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_J, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_meta_j () {
    // Meta+J（macOS ⌘+J）切换 AI 侧边栏 → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_J, Qt::MetaModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_ctrl_j_keyrelease () {
    // Ctrl+J 的 KeyRelease → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyRelease, Qt::Key_J, Qt::ControlModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_allows_meta_j_keyrelease () {
    // Meta+J 的 KeyRelease → 放行
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyRelease, Qt::Key_J, Qt::MetaModifier);
    QVERIFY (!ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  void test_readonly_blocks_plain_j () {
    // 无修饰键的 J → 拦截（readonly 区域禁止输入）
    QObject obj;
    obj.setProperty ("chat_message_readonly", true);
    QKeyEvent ke (QEvent::KeyPress, Qt::Key_J, Qt::NoModifier);
    QVERIFY (ChatConversationPanel::should_block_readonly_event (&obj, &ke));
  }

  // === enterConversationMode 布局行为 ===
  // 模拟 contentLayout + topPanel 的布局结构，验证 enterConversationMode
  // 的布局逻辑：将 topPanel 从 Preferred/AlignTop 切换到 Expanding/无对齐
  void test_enterConversationMode_topPanel_expands () {
    // 模拟 contentLayout 的结构：topSpacer + topPanel(AlignTop, stretch=1)
    QWidget     container;
    QVBoxLayout contentLayout (&container);
    contentLayout.setContentsMargins (0, 0, 0, 0);

    QSpacerItem* topSpacer=
        new QSpacerItem (0, 100, QSizePolicy::Minimum, QSizePolicy::Fixed);
    contentLayout.addSpacerItem (topSpacer);

    QWidget* topPanel= new QWidget (&container);
    topPanel->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
    contentLayout.addWidget (topPanel, 1, Qt::AlignTop);

    container.resize (400, 600);
    QTest::qWait (0);

    // 验证初始状态
    QCOMPARE (topPanel->sizePolicy ().verticalPolicy (),
              QSizePolicy::Preferred);
    QLayoutItem* topPanelItem= contentLayout.itemAt (1);
    QVERIFY (topPanelItem != nullptr);
    QVERIFY (topPanelItem->alignment () & Qt::AlignTop);

    // 模拟 enterConversationMode 的布局逻辑
    topSpacer->changeSize (0, 30, QSizePolicy::Minimum, QSizePolicy::Fixed);
    topPanel->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);
    contentLayout.setAlignment (topPanel, Qt::Alignment ());
    contentLayout.invalidate ();
    contentLayout.activate ();
    container.updateGeometry ();
    QTest::qWait (0);

    // 验证：sizePolicy 变为 Expanding
    QCOMPARE (topPanel->sizePolicy ().verticalPolicy (),
              QSizePolicy::Expanding);
    // 验证：AlignTop 已被移除
    QCOMPARE (topPanelItem->alignment (), Qt::Alignment ());
    // 验证：topPanel 填满剩余空间（600 - 30 spacer = ~570）
    QVERIFY (topPanel->height () > 500);
  }

  void test_enterConversationMode_topPanel_before_expand () {
    // 验证 AlignTop + Preferred 下 topPanel 不扩展
    QWidget     container;
    QVBoxLayout contentLayout (&container);
    contentLayout.setContentsMargins (0, 0, 0, 0);

    QSpacerItem* topSpacer=
        new QSpacerItem (0, 100, QSizePolicy::Minimum, QSizePolicy::Fixed);
    contentLayout.addSpacerItem (topSpacer);

    QWidget* topPanel= new QWidget (&container);
    topPanel->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
    contentLayout.addWidget (topPanel, 1, Qt::AlignTop);

    container.resize (400, 600);
    QTest::qWait (0);

    // Preferred + AlignTop：topPanel 只取 sizeHint（很小），不填满空间
    QVERIFY (topPanel->height () < 100);
  }

  // 模拟真实的 QStackedWidget + Ignored panel 场景
  void test_enterConversationMode_ignored_panel_expands () {
    // 外层 QStackedWidget 模拟 conversationStack_
    QStackedWidget stack;
    stack.resize (400, 600);

    // 内层 panel 模拟 ChatConversationPanel（Ignored 垂直策略）
    QWidget* panel= new QWidget (&stack);
    panel->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Ignored);
    stack.addWidget (panel);

    // panel 内部的 contentLayout
    QVBoxLayout* contentLayout= new QVBoxLayout (panel);
    contentLayout->setContentsMargins (0, 0, 0, 0);

    QSpacerItem* topSpacer=
        new QSpacerItem (0, 100, QSizePolicy::Minimum, QSizePolicy::Fixed);
    contentLayout->addSpacerItem (topSpacer);

    QWidget* topPanel= new QWidget (panel);
    topPanel->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
    contentLayout->addWidget (topPanel, 1, Qt::AlignTop);

    stack.show ();
    QTest::qWaitFor ([&stack] { return stack.isVisible (); });

    // 初始状态：Ignored panel 在 QStackedWidget 中，
    // topPanel(AlignTop+Preferred) 不应扩展
    QVERIFY (topPanel->height () < 100);

    // 模拟 enterConversationMode 的布局操作
    topSpacer->changeSize (0, 30, QSizePolicy::Minimum, QSizePolicy::Fixed);
    topPanel->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);
    contentLayout->setAlignment (topPanel, Qt::Alignment ());
    contentLayout->invalidate ();
    contentLayout->activate ();
    panel->updateGeometry ();
    QTest::qWait (0);

    // 验证：即使 panel 自身是 Ignored，topPanel 仍能扩展填满空间
    QVERIFY2 (topPanel->height () > 400,
              qPrintable (QString ("topPanel height = %1, expected > 400")
                              .arg (topPanel->height ())));
  }
};

QTEST_MAIN (TestChatTabWidget)
#include "qt_chat_tab_widget_test.moc"
