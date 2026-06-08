
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
#include <chrono>

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

  // === sessionCount ===
  void test_sessionCount_empty ();
  void test_sessionCount_after_create ();
  void test_sessionCount_after_remove ();

  // === firstActiveSessionId ===
  void test_firstActiveSessionId_empty ();
  void test_firstActiveSessionId_returns_newest ();
  void test_firstActiveSessionId_skips_archived ();

  // === touchSession ===
  void test_touchSession_updates_order ();
  void test_touchSession_nonexistent ();

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

  // === defaultExpandCount ===
  void test_createSession_defaultExpandCount ();
  void test_insertSession_defaultExpandCount ();

  // === 会话标题可见性判断 ===
  void test_title_empty_for_new_session ();
  void test_title_nonempty_after_set ();
  void test_title_preserved_across_archive_restore ();

  // === thinking ===
  void test_createSession_thinking_default_false ();
  void test_setThinking_and_getThinking ();
  void test_getThinking_nonexistent ();
  void test_setThinking_nonexistent ();

  // === 双容器一致性 ===
  void test_dual_container_consistency_after_create ();
  void test_dual_container_consistency_after_remove ();
  void test_dual_container_consistency_after_insert ();

  // === 性能 benchmark ===
  void test_benchmark_getAllSessionIds_linear_scaling ();
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
  QVERIFY (s->createdAt > 0);
  QVERIFY (s->updateAt > 0);
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

  // 用 insertSession 注入已知 updateAt 的会话，验证降序排列
  ChatSession s1;
  s1.sessionId= "old-session";
  s1.state    = ChatState::Idle;
  s1.createdAt= 1000;
  s1.updateAt = 1000;

  ChatSession s2;
  s2.sessionId= "new-session";
  s2.state    = ChatState::Idle;
  s2.createdAt= 2000;
  s2.updateAt = 2000;

  ChatSession s3;
  s3.sessionId= "mid-session";
  s3.state    = ChatState::Idle;
  s3.createdAt= 1500;
  s3.updateAt = 1500;

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
 * sessionCount
 ******************************************************************************/

void
TestChatSession::test_sessionCount_empty () {
  ChatSessionManager mgr;
  QCOMPARE ((int) mgr.sessionCount (), 0);
}

void
TestChatSession::test_sessionCount_after_create () {
  ChatSessionManager mgr;
  mgr.createSession ();
  mgr.createSession ();
  QCOMPARE ((int) mgr.sessionCount (), 2);
}

void
TestChatSession::test_sessionCount_after_remove () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.createSession ();
  QCOMPARE ((int) mgr.sessionCount (), 2);
  mgr.removeSession (sid);
  QCOMPARE ((int) mgr.sessionCount (), 1);
}

/******************************************************************************
 * firstActiveSessionId
 ******************************************************************************/

void
TestChatSession::test_firstActiveSessionId_empty () {
  ChatSessionManager mgr;
  QVERIFY (is_empty (mgr.firstActiveSessionId ()));
}

void
TestChatSession::test_firstActiveSessionId_returns_newest () {
  ChatSessionManager mgr;

  ChatSession s1;
  s1.sessionId= "older";
  s1.state    = ChatState::Idle;
  s1.createdAt= 1000;
  s1.updateAt = 1000;
  s1.archived = false;
  s1.panel    = nullptr;

  ChatSession s2;
  s2.sessionId= "newer";
  s2.state    = ChatState::Idle;
  s2.createdAt= 2000;
  s2.updateAt = 2000;
  s2.archived = false;
  s2.panel    = nullptr;

  mgr.insertSession (s1);
  mgr.insertSession (s2);

  QVERIFY (mgr.firstActiveSessionId () == string ("newer"));
}

void
TestChatSession::test_firstActiveSessionId_skips_archived () {
  ChatSessionManager mgr;

  ChatSession s1;
  s1.sessionId= "archived-one";
  s1.state    = ChatState::Idle;
  s1.createdAt= 3000;
  s1.updateAt = 3000;
  s1.archived = true;
  s1.panel    = nullptr;

  ChatSession s2;
  s2.sessionId= "active-one";
  s2.state    = ChatState::Idle;
  s2.createdAt= 2000;
  s2.updateAt = 2000;
  s2.archived = false;
  s2.panel    = nullptr;

  mgr.insertSession (s1);
  mgr.insertSession (s2);

  QVERIFY (mgr.firstActiveSessionId () == string ("active-one"));
}

/******************************************************************************
 * touchSession
 ******************************************************************************/

void
TestChatSession::test_touchSession_updates_order () {
  ChatSessionManager mgr;

  ChatSession s1;
  s1.sessionId= "old-session";
  s1.state    = ChatState::Idle;
  s1.createdAt= 1000;
  s1.updateAt = 1000;
  s1.archived = false;
  s1.panel    = nullptr;

  ChatSession s2;
  s2.sessionId= "new-session";
  s2.state    = ChatState::Idle;
  s2.createdAt= 2000;
  s2.updateAt = 2000;
  s2.archived = false;
  s2.panel    = nullptr;

  mgr.insertSession (s1);
  mgr.insertSession (s2);

  // 初始顺序：new-session (2000) > old-session (1000)
  auto ids= mgr.getAllSessionIds ();
  QVERIFY (ids[0] == string ("new-session"));

  // touch old-session，将其 updateAt 设为当前时间（> 2000）
  mgr.touchSession ("old-session");

  // 现在 old-session 应该在最前面
  ids= mgr.getAllSessionIds ();
  QVERIFY (ids[0] == string ("old-session"));
  QVERIFY (ids[1] == string ("new-session"));
}

void
TestChatSession::test_touchSession_nonexistent () {
  ChatSessionManager mgr;
  // 不应崩溃
  mgr.touchSession ("nonexistent-id");
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
  s.createdAt= 1234567890;
  s.updateAt = 1234567890;
  s.panel    = nullptr;

  mgr.insertSession (s);

  ChatSession* found= mgr.getSession ("test-id");
  QVERIFY (found != nullptr);
  QVERIFY (found->sessionId == string ("test-id"));
  QVERIFY (found->title == string ("Test Title"));
  QVERIFY (found->model == string ("gpt-4"));
  QCOMPARE ((int) found->state, (int) ChatState::Idle);
  QVERIFY (found->archived);
  QCOMPARE ((long) found->createdAt, (long) 1234567890);
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
  s.createdAt= 1000;
  s.updateAt = 1000;
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

/******************************************************************************
 * defaultExpandCount
 ******************************************************************************/

void
TestChatSession::test_createSession_defaultExpandCount () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  ChatSession*       s  = mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QCOMPARE (s->defaultExpandCount, 5);
}

void
TestChatSession::test_insertSession_defaultExpandCount () {
  ChatSessionManager mgr;
  ChatSession        s;
  s.sessionId         = "test-expand";
  s.title             = "Test";
  s.model             = "gpt-4";
  s.state             = ChatState::Idle;
  s.archived          = false;
  s.createdAt         = 1234567890;
  s.updateAt          = 1234567890;
  s.defaultExpandCount= 5;
  s.panel             = nullptr;
  mgr.insertSession (s);

  ChatSession* found= mgr.getSession ("test-expand");
  QVERIFY (found != nullptr);
  QCOMPARE (found->defaultExpandCount, 5);
}

/******************************************************************************
 * thinking
 ******************************************************************************/

void
TestChatSession::test_createSession_thinking_default_false () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  ChatSession*       s  = mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (!s->thinking);
  QVERIFY (!mgr.getThinking (sid));
}

void
TestChatSession::test_setThinking_and_getThinking () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setThinking (sid, true);
  QVERIFY (mgr.getThinking (sid));
  QCOMPARE (mgr.getSession (sid)->thinking, true);

  mgr.setThinking (sid, false);
  QVERIFY (!mgr.getThinking (sid));
  QCOMPARE (mgr.getSession (sid)->thinking, false);
}

void
TestChatSession::test_getThinking_nonexistent () {
  ChatSessionManager mgr;
  QVERIFY (!mgr.getThinking ("nonexistent-id"));
}

void
TestChatSession::test_setThinking_nonexistent () {
  ChatSessionManager mgr;
  mgr.setThinking ("nonexistent-id", true);
  // 不应崩溃
}

QTEST_MAIN (TestChatSession)
#include "qt_chat_session_test.moc"

/******************************************************************************
 * 会话标题可见性判断
 *
 * 验证 Controller 用来决定 sessionTitle 标签显隐的 is_empty(s->title) 判断：
 * - 新会话 title 为空 → 标签应隐藏
 * - 设置 title 后非空 → 标签应显示
 * - 归档/恢复后 title 保持 → 标签应保持显示
 ******************************************************************************/

void
TestChatSession::test_title_empty_for_new_session () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  ChatSession*       s  = mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (is_empty (s->title));
}

void
TestChatSession::test_title_nonempty_after_set () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setTitle (sid, "测试标题");
  ChatSession* s= mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (!is_empty (s->title));
  QVERIFY (s->title == string ("测试标题"));
}

void
TestChatSession::test_title_preserved_across_archive_restore () {
  ChatSessionManager mgr;
  string             sid= mgr.createSession ();
  mgr.setTitle (sid, "My Session");
  mgr.archiveSession (sid);
  mgr.restoreSession (sid);
  ChatSession* s= mgr.getSession (sid);
  QVERIFY (s != nullptr);
  QVERIFY (!s->archived);
  QVERIFY (!is_empty (s->title));
  QVERIFY (s->title == string ("My Session"));
}

/******************************************************************************
 * 双容器一致性
 ******************************************************************************/

void
TestChatSession::test_dual_container_consistency_after_create () {
  ChatSessionManager mgr;
  mgr.createSession ();
  mgr.createSession ();
  mgr.createSession ();
  QCOMPARE ((int) mgr.sessionCount (), 3);
  QCOMPARE ((int) mgr.getAllSessionIds ().size (), 3);
}

void
TestChatSession::test_dual_container_consistency_after_remove () {
  ChatSessionManager mgr;
  string             sid1= mgr.createSession ();
  string             sid2= mgr.createSession ();
  mgr.removeSession (sid1);
  QCOMPARE ((int) mgr.sessionCount (), 1);
  QCOMPARE ((int) mgr.getAllSessionIds ().size (), 1);
  QVERIFY (mgr.getAllSessionIds ()[0] == sid2);
}

void
TestChatSession::test_dual_container_consistency_after_insert () {
  ChatSessionManager mgr;

  ChatSession s;
  s.sessionId= "inserted";
  s.state    = ChatState::Idle;
  s.createdAt= 1000;
  s.updateAt = 1000;
  s.panel    = nullptr;
  mgr.insertSession (s);

  QCOMPARE ((int) mgr.sessionCount (), 1);
  QCOMPARE ((int) mgr.getAllSessionIds ().size (), 1);
}

/******************************************************************************
 * 性能 benchmark：验证 getAllSessionIds 是 O(n)
 *
 * 方法：测量 3 组数据（N, 2N, 4N）的单次调用均摊耗时（per-session us）。
 * O(n) 时均摊耗时应为常数（三组接近）；O(n log² n) 时均摊耗时会随 N 增长。
 ******************************************************************************/

/// 向 manager 插入 N 个 session
static void
insertNSessions (ChatSessionManager& mgr, int n, time_t baseTime) {
  for (int i= 0; i < n; i++) {
    ChatSession s;
    char        buf[32];
    std::snprintf (buf, sizeof (buf), "bench-%d", i);
    s.sessionId= string (buf);
    s.state    = ChatState::Idle;
    s.archived = false;
    s.createdAt= baseTime + i;
    s.updateAt = baseTime + i;
    s.panel    = nullptr;
    mgr.insertSession (s);
  }
}

/// 测量 getAllSessionIds 单次调用平均耗时（微秒）
static double
measureAvgUs (ChatSessionManager& mgr, int repeats) {
  auto t1= std::chrono::high_resolution_clock::now ();
  for (int i= 0; i < repeats; i++)
    mgr.getAllSessionIds ();
  auto t2= std::chrono::high_resolution_clock::now ();
  return std::chrono::duration<double, std::micro> (t2 - t1).count () / repeats;
}

void
TestChatSession::test_benchmark_getAllSessionIds_linear_scaling () {
  const int    N1= 200, N2= 400, N3= 800;
  const int    REPEAT= 100;
  const time_t BASE = 1000000000;

  ChatSessionManager mgr1, mgr2, mgr3;
  insertNSessions (mgr1, N1, BASE);
  insertNSessions (mgr2, N2, BASE);
  insertNSessions (mgr3, N3, BASE);

  double us1= measureAvgUs (mgr1, REPEAT);
  double us2= measureAvgUs (mgr2, REPEAT);
  double us3= measureAvgUs (mgr3, REPEAT);

  // per-session 均摊耗时：O(n) 时应为常数
  double per1= us1 / N1;
  double per2= us2 / N2;
  double per3= us3 / N3;

  // 三组的 per-session 耗时变化不应超过 2x（排除 cache 效应等噪声）
  // O(n log² n) 时，per3/per1 会随 N 显著增长
  double maxPer= std::max ({per1, per2, per3});
  double minPer= std::min ({per1, per2, per3});
  double variation= maxPer / minPer;

  QVERIFY2 (variation < 2.0,
            QString ("per-session variation %1x (expected ~1.0 for O(n)), "
                     "N=%2: %3, N=%4: %5, N=%6: %7 us/session")
                .arg (variation, 0, 'f', 2)
                .arg (N1).arg (per1, 0, 'f', 4)
                .arg (N2).arg (per2, 0, 'f', 4)
                .arg (N3).arg (per3, 0, 'f', 4)
                .toUtf8 ()
                .constData ());

  qDebug () << "=== getAllSessionIds O(n) verification ===";
  qDebug () << "  N=" << N1 << ":" << per1 << "us/session (" << us1 << "us total)";
  qDebug () << "  N=" << N2 << ":" << per2 << "us/session (" << us2 << "us total)";
  qDebug () << "  N=" << N3 << ":" << per3 << "us/session (" << us3 << "us total)";
  qDebug () << "  variation:" << variation << "x";
}
