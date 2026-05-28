
/******************************************************************************
 * MODULE     : qt_chat_tab_widget_test.cpp
 * DESCRIPTION: Tests for QTChatTabWidget helper functions
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_chat_tab_widget.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest/QtTest>

using namespace moebius;

class TestChatTabWidget : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

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
};

QTEST_MAIN (TestChatTabWidget)
#include "qt_chat_tab_widget_test.moc"
