
/******************************************************************************
 * MODULE     : qt_chat_tab_widget_test.cpp
 * DESCRIPTION: Tests for QTChatTabWidget helper functions
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_chat_tab_widget.hpp"
#include "base.hpp"
#include "qt_utilities.hpp"
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

  void test_estimate_lines_from_height_zero () {
    QCOMPARE (ChatConversationPanel::estimate_lines_from_height (0), 0);
  }

  void test_estimate_lines_from_height_negative () {
    QCOMPARE (ChatConversationPanel::estimate_lines_from_height (-100), 0);
  }

  void test_estimate_lines_from_height_less_than_one_line () {
    // kInputLineHeight == 22
    // 高度不足一行时应返回 1（向上取整）
    SI h= (22 / 2) * PIXEL;
    QCOMPARE (ChatConversationPanel::estimate_lines_from_height (h), 1);
  }

  void test_estimate_lines_from_height_exact_one_line () {
    SI h= 22 * PIXEL;
    QCOMPARE (ChatConversationPanel::estimate_lines_from_height (h), 1);
  }

  void test_estimate_lines_from_height_three_lines () {
    SI h= (22 * 3) * PIXEL;
    QCOMPARE (ChatConversationPanel::estimate_lines_from_height (h), 3);
  }

  void test_estimate_lines_from_height_with_rounding () {
    // 高度为 2.5 行时应向上取整为 3
    SI h= ((22 * 5) / 2) * PIXEL;
    QCOMPARE (ChatConversationPanel::estimate_lines_from_height (h), 3);
  }
};

QTEST_MAIN (TestChatTabWidget)
#include "qt_chat_tab_widget_test.moc"
