/******************************************************************************
 * MODULE     : buffer_type_test.cpp
 * DESCRIPTION: is_startup_tab_buffer / is_chat_tab_buffer 单元测试
 * COPYRIGHT  : (C) 2026 Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "new_view.hpp"

class TestBufferType : public QObject {
  Q_OBJECT

private slots:
  void test_startup_tab_buffer ();
  void test_chat_tab_buffer ();
  void test_normal_buffer ();
  void test_chat_tab_variants ();
};

void
TestBufferType::test_startup_tab_buffer () {
  QVERIFY (is_startup_tab_buffer (url ("tmfs://startup-tab")));
  QVERIFY (!is_startup_tab_buffer (url ("tmfs://startup-tab/1")));
  QVERIFY (!is_startup_tab_buffer (url ("tmfs://chat-tab")));
  QVERIFY (!is_startup_tab_buffer (url ("tmfs://chat-tab/1")));
  QVERIFY (!is_startup_tab_buffer (url ("file:///test.tm")));
}

void
TestBufferType::test_chat_tab_buffer () {
  QVERIFY (is_chat_tab_buffer (url ("tmfs://chat-tab")));
  QVERIFY (is_chat_tab_buffer (url ("tmfs://chat-tab/1")));
  QVERIFY (is_chat_tab_buffer (url ("tmfs://chat-tab/session-abc")));
  QVERIFY (!is_chat_tab_buffer (url ("tmfs://startup-tab")));
  QVERIFY (!is_chat_tab_buffer (url ("file:///test.tm")));
}

void
TestBufferType::test_normal_buffer () {
  QVERIFY (!is_startup_tab_buffer (url ("file:///home/user/test.tm")));
  QVERIFY (!is_chat_tab_buffer (url ("file:///home/user/test.tm")));
  QVERIFY (!is_startup_tab_buffer (url ("tmfs://view/1/default/test.tm")));
  QVERIFY (!is_chat_tab_buffer (url ("tmfs://view/1/default/test.tm")));
}

void
TestBufferType::test_chat_tab_variants () {
  // 边界情况：空字符串、前缀匹配
  QVERIFY (!is_chat_tab_buffer (url ("")));
  QVERIFY (!is_chat_tab_buffer (url ("tmfs://chat")));
  // is_chat_tab_buffer 使用 starts 匹配前缀，tmfs://chat-taboola 会被视为匹配
  QVERIFY (is_chat_tab_buffer (url ("tmfs://chat-taboola")));
  QVERIFY (is_chat_tab_buffer (url ("tmfs://chat-tab-2")));
}

QTEST_MAIN (TestBufferType)
#include "buffer_type_test.moc"
