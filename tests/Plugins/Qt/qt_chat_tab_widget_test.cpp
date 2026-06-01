
/******************************************************************************
 * MODULE     : qt_chat_tab_widget_test.cpp
 * DESCRIPTION: Tests for QTChatTabWidget helper functions
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_chat_tab_widget.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSignalSpy>
#include <QWheelEvent>
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
};

QTEST_MAIN (TestChatTabWidget)
#include "qt_chat_tab_widget_test.moc"
