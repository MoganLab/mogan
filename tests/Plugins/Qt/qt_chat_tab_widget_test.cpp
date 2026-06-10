
/******************************************************************************
 * MODULE     : qt_chat_tab_widget_test.cpp
 * DESCRIPTION: Tests for QTChatTabWidget helper functions
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_chat_tab_widget.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
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

  void test_count_input_lines_empty_document () {
    tree empty_doc= tree (DOCUMENT, "");
    QCOMPARE (ChatConversationPanel::count_input_lines (empty_doc), 1);
  }

  void test_count_input_lines_single_paragraph () {
    tree doc= tree (DOCUMENT, "hello");
    QCOMPARE (ChatConversationPanel::count_input_lines (doc), 1);
  }

  void test_count_input_lines_multiple_paragraphs () {
    tree doc= tree (DOCUMENT, "para1", "para2", "para3");
    QCOMPARE (ChatConversationPanel::count_input_lines (doc), 3);
  }

  void test_count_input_lines_not_document () {
    tree not_doc= tree (WITH, "font", "roman", "hello");
    QCOMPARE (ChatConversationPanel::count_input_lines (not_doc), 1);
  }

  void test_count_input_lines_empty_string_only () {
    // DOCUMENT with only an empty string atom
    tree doc= tree (DOCUMENT, "");
    QCOMPARE (ChatConversationPanel::count_input_lines (doc), 1);
  }

  void test_count_input_lines_concat_formula_counts_as_one_paragraph () {
    tree doc= tree (DOCUMENT, tree (CONCAT, "x", "y", "z"));
    QCOMPARE (ChatConversationPanel::count_input_lines (doc), 1);
  }

  void test_count_input_lines_concat_formula_with_second_paragraph () {
    tree doc= tree (DOCUMENT, tree (CONCAT, "x", "y", "z"), "para2");
    QCOMPARE (ChatConversationPanel::count_input_lines (doc), 2);
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
    widget.setCloseSidebarButtonVisible (true);
    QVERIFY (widget.closeSidebarButton ()->isVisible ());
  }

  void test_closeSidebarButton_hidden () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    widget.setCloseSidebarButtonVisible (true);
    widget.setCloseSidebarButtonVisible (false);
    QVERIFY (!widget.closeSidebarButton ()->isVisible ());
  }

  void test_closeSidebarButton_emits_signal () {
    QList<SessionDisplayInfo> sessions;
    QTChatTabWidget           widget (sessions, "", nullptr);
    widget.show ();
    QSignalSpy spy (&widget, &QTChatTabWidget::closeSidebarRequested);
    widget.setCloseSidebarButtonVisible (true);
    QTest::mouseClick (widget.closeSidebarButton (), Qt::LeftButton);
    QCOMPARE (spy.count (), 1);
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
        Qt::Key_Return, Qt::NoModifier, true));
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
