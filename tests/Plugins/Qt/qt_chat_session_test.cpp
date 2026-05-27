
/******************************************************************************
 * MODULE     : qt_chat_session_test.cpp
 * DESCRIPTION: Tests for ChatSessionManager
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_chat_session.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestChatSession : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // === createSession ===
  void test_createSession ();
  void test_createSession_multiple ();

  // === removeSession ===
  void test_removeSession ();
  void test_removeSession_nonexistent ();

  // === archiveSession / restoreSession ===
  void test_archiveSession ();
  void test_restoreSession ();
  void test_archiveSession_nonexistent ();

  // === setTitle / setState / setModel / getModel ===
  void test_setTitle ();
  void test_getModel_default_empty ();
  void test_setModel_and_getModel ();
  void test_setState ();

  // === getAllSessionIds ===
  void test_getAllSessionIds_ordering ();

  // === getSession / findSessionByPanel ===
  void test_getSession ();
  void test_getSession_nonexistent ();
  void test_findSessionByPanel ();
  void test_findSessionByPanel_not_found ();

  // === insertSession ===
  void test_insertSession ();

  // === 空白会话（title 为空）相关场景 ===
  void test_createSession_empty_title ();
  void test_archiveSession_preserves_empty_title ();
  void test_restoreSession_preserves_title ();
  void test_insertSession_empty_title ();
  void test_archiveSession_with_title ();

  // === messageBufferUrl / inputBufferUrl ===
  void test_messageBufferUrl ();
  void test_inputBufferUrl ();
};

/******************************************************************************
 * createSession
 ******************************************************************************/

void
TestChatSession::test_createSession () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();

  QVERIFY (!is_empty (sid));

  ChatSession* s= mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (s->sessionId == sid);
  QCOMPARE ((int) s->state, (int) ChatState::Idle);
  QVERIFY (!s->archived);
  QVERIFY (is_empty (s->title));
  QVERIFY (is_empty (s->model));
  QVERIFY (!is_empty (s->createdAt));
  QVERIFY (s->panel == nullptr);
}

void
TestChatSession::test_createSession_multiple () {
  ChatSessionManager mgr;
  string             sid1= mgr.createSession ();
  string             sid2= mgr.createSession ();
  QVERIFY (sid1 != sid2);
}

/******************************************************************************
 * removeSession
 ******************************************************************************/

void
TestChatSession::test_removeSession () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.removeSession (sid);
  QVERIFY (mgr.getSession (sid) == nullptr);
}

void
TestChatSession::test_removeSession_nonexistent () {
  ChatSessionManager mgr;
  // 删除不存在的 ID 不应崩溃
  mgr.removeSession ("nonexistent-id");
}

/******************************************************************************
 * archiveSession / restoreSession
 ******************************************************************************/

void
TestChatSession::test_archiveSession () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.archiveSession (sid);
  QVERIFY (mgr.getSession (sid)->archived);
}

void
TestChatSession::test_restoreSession () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.archiveSession (sid);
  mgr.restoreSession (sid);
  QVERIFY (!mgr.getSession (sid)->archived);
}

void
TestChatSession::test_archiveSession_nonexistent () {
  ChatSessionManager mgr;
  // 归档不存在的 ID 不应崩溃
  mgr.archiveSession ("nonexistent-id");
}

/******************************************************************************
 * setTitle / setState / setModel / getModel
 ******************************************************************************/

void
TestChatSession::test_setTitle () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setTitle (sid, "Hello");
  QVERIFY (mgr.getSession (sid)->title == string ("Hello"));
}

void
TestChatSession::test_getModel_default_empty () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  QVERIFY (mgr.getModel (sid) == string (""));
}

void
TestChatSession::test_setModel_and_getModel () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setModel (sid, "gpt-4");
  QVERIFY (mgr.getModel (sid) == string ("gpt-4"));
}

void
TestChatSession::test_setState () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setState (sid, ChatState::Generating);
  QCOMPARE ((int) mgr.getSession (sid)->state, (int) ChatState::Generating);
}

/******************************************************************************
 * getAllSessionIds
 ******************************************************************************/

void
TestChatSession::test_getAllSessionIds_ordering () {
  ChatSessionManager mgr;

  // 用 insertSession 注入已知时间戳的会话，验证降序排列
  ChatSession s1;
  s1.sessionId= "old-session";
  s1.state    = ChatState::Idle;
  s1.createdAt= "1000";

  ChatSession s2;
  s2.sessionId= "new-session";
  s2.state    = ChatState::Idle;
  s2.createdAt= "2000";

  ChatSession s3;
  s3.sessionId= "mid-session";
  s3.state    = ChatState::Idle;
  s3.createdAt= "1500";

  mgr.insertSession (s1);
  mgr.insertSession (s2);
  mgr.insertSession (s3);

  auto ids= mgr.getAllSessionIds ();
  QCOMPARE ((int) ids.size (), 3);
  QVERIFY (ids[0] == string ("new-session"));
  QVERIFY (ids[1] == string ("mid-session"));
  QVERIFY (ids[2] == string ("old-session"));
}

/******************************************************************************
 * getSession / findSessionByPanel
 ******************************************************************************/

void
TestChatSession::test_getSession () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  ChatSession*       s  = mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (s->sessionId == sid);
}

void
TestChatSession::test_getSession_nonexistent () {
  ChatSessionManager mgr;
  QVERIFY (mgr.getSession ("nonexistent-id") == nullptr);
}

void
TestChatSession::test_findSessionByPanel () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();

  // 使用假指针（仅存储比较，不解引用）
  auto* fakePanel=
      reinterpret_cast<ChatConversationPanel*> (uintptr_t (0x1234));
  mgr.setPanel (sid, fakePanel);

  ChatSession* found= mgr.findSessionByPanel (fakePanel);
  QVERIFY (found != nullptr);
  QVERIFY (found->sessionId == sid);
}

void
TestChatSession::test_findSessionByPanel_not_found () {
  ChatSessionManager mgr;
  auto*              fakePanel=
      reinterpret_cast<ChatConversationPanel*> (uintptr_t (0x5678));
  QVERIFY (mgr.findSessionByPanel (fakePanel) == nullptr);
}

/******************************************************************************
 * insertSession
 ******************************************************************************/

void
TestChatSession::test_insertSession () {
  ChatSessionManager mgr;

  ChatSession s;
  s.sessionId= "test-id";
  s.title    = "Test Title";
  s.model    = "gpt-4";
  s.state    = ChatState::Idle;
  s.archived = true;
  s.createdAt= "1234567890";
  s.panel    = nullptr;

  mgr.insertSession (s);

  ChatSession* found= mgr.getSession ("test-id");
  QVERIFY (found != nullptr);
  QVERIFY (found->sessionId == string ("test-id"));
  QVERIFY (found->title == string ("Test Title"));
  QVERIFY (found->model == string ("gpt-4"));
  QCOMPARE ((int) found->state, (int) ChatState::Idle);
  QVERIFY (found->archived);
  QVERIFY (found->createdAt == string ("1234567890"));
  QVERIFY (found->panel == nullptr);
}

/******************************************************************************
 * 空白会话（title 为空）相关场景
 *
 * 验证 [0229] 中"空白会话不归档"的判断依据：
 * ChatController 通过 is_empty(s->title) 判断空白会话，
 * 因此 ChatSessionManager 必须保证 title 在各种操作后保持一致。
 ******************************************************************************/

void
TestChatSession::test_createSession_empty_title () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  ChatSession*       s  = mgr.getSession (sid);
  QVERIFY (s != nullptr);
  // 新创建的会话 title 必须为空（空白会话判断依据）
  QVERIFY (is_empty (s->title));
}

void
TestChatSession::test_archiveSession_preserves_empty_title () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  // 空白会话归档后，title 仍为空（Manager 不自动填充 title）
  mgr.archiveSession (sid);
  ChatSession* s= mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (s->archived);
  QVERIFY (is_empty (s->title));
}

void
TestChatSession::test_restoreSession_preserves_title () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setTitle (sid, "My Chat");
  mgr.archiveSession (sid);
  mgr.restoreSession (sid);
  // 恢复后 title 保持不变
  ChatSession* s= mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (!s->archived);
  QVERIFY (s->title == string ("My Chat"));
}

void
TestChatSession::test_insertSession_empty_title () {
  ChatSessionManager mgr;
  ChatSession        s;
  s.sessionId= "blank-id";
  s.title    = ""; // 空白会话
  s.model    = "gpt-4";
  s.state    = ChatState::Idle;
  s.archived = false;
  s.createdAt= "1000";
  s.panel    = nullptr;
  mgr.insertSession (s);

  ChatSession* found= mgr.getSession ("blank-id");
  QVERIFY (found != nullptr);
  QVERIFY (is_empty (found->title));
}

void
TestChatSession::test_archiveSession_with_title () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setTitle (sid, "Has Title");
  mgr.archiveSession (sid);
  // 有标题的会话归档后 title 保持
  ChatSession* s= mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (s->archived);
  QVERIFY (s->title == string ("Has Title"));
}

/******************************************************************************
 * messageBufferUrl / inputBufferUrl
 ******************************************************************************/

void
TestChatSession::test_messageBufferUrl () {
  url result  = ChatSessionManager::messageBufferUrl ("abc-123");
  url expected= url ("tmfs://chat-message-abc-123");
  QVERIFY (result == expected);
}

void
TestChatSession::test_inputBufferUrl () {
  url result  = ChatSessionManager::inputBufferUrl ("abc-123");
  url expected= url ("tmfs://chat-input-abc-123");
  QVERIFY (result == expected);
}

QTEST_MAIN (TestChatSession)
#include "qt_chat_session_test.moc"
