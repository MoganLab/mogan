
/******************************************************************************
 * MODULE     : qt_chat_session.cpp
 * DESCRIPTION: 聊天会话数据模型与会话管理器
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_chat_session.hpp"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <lolly/hash/uuid.hpp>

/******************************************************************************
 * ChatSessionManager 实现
 ******************************************************************************/

string
ChatSessionManager::createSession () {
  string      sessionId= lolly::hash::uuid_make ();
  ChatSession session;
  session.sessionId= sessionId;
  session.state    = ChatState::Idle;
  session.archived = false;
  // 生成 Unix 时间戳字符串
  std::time_t now= std::time (nullptr);
  char        buf[32];
  std::snprintf (buf, sizeof (buf), "%ld", (long) now);
  session.createdAt         = string (buf);
  session.defaultExpandCount= 5;
  session.panel             = nullptr;
  sessions_.insert (std::make_pair (sessionId, session));
  return sessionId;
}

void
ChatSessionManager::removeSession (const string& sessionId) {
  sessions_.erase (sessionId);
}

void
ChatSessionManager::archiveSession (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (s) s->archived= true;
}

void
ChatSessionManager::restoreSession (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (s) s->archived= false;
}

void
ChatSessionManager::setTitle (const string& sessionId, const string& title) {
  ChatSession* s= getSession (sessionId);
  if (s) s->title= title;
}

void
ChatSessionManager::setState (const string& sessionId, ChatState state) {
  ChatSession* s= getSession (sessionId);
  if (s) s->state= state;
}

void
ChatSessionManager::setModel (const string& sessionId, const string& model) {
  ChatSession* s= getSession (sessionId);
  if (s) s->model= model;
}

string
ChatSessionManager::getModel (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (s) return s->model;
  return "";
}

ChatSession*
ChatSessionManager::getSession (const string& sessionId) {
  auto it= sessions_.find (sessionId);
  if (it != sessions_.end ()) return &(it->second);
  return nullptr;
}

std::vector<string>
ChatSessionManager::getAllSessionIds () const {
  std::vector<string> ids;
  for (const auto& kv : sessions_)
    ids.push_back (kv.first);
  // 按 createdAt 降序排列（新在前），空 createdAt 排最后
  std::sort (ids.begin (), ids.end (),
             [this] (const string& a, const string& b) {
               auto it_a= sessions_.find (a);
               auto it_b= sessions_.find (b);
               if (it_a == sessions_.end () || it_b == sessions_.end ())
                 return false;
               // a > b 等价于 !(a <= b)
               return !(it_a->second.createdAt <= it_b->second.createdAt);
             });
  return ids;
}

ChatSession*
ChatSessionManager::findSessionByPanel (ChatConversationPanel* panel) {
  for (auto& kv : sessions_) {
    if (kv.second.panel == panel) return &kv.second;
  }
  return nullptr;
}

void
ChatSessionManager::setPanel (const string&          sessionId,
                              ChatConversationPanel* panel) {
  ChatSession* s= getSession (sessionId);
  if (s) s->panel= panel;
}

url
ChatSessionManager::messageBufferUrl (const string& sessionId) {
  return url ("tmfs://chat-message-" * sessionId);
}

url
ChatSessionManager::inputBufferUrl (const string& sessionId) {
  return url ("tmfs://chat-input-" * sessionId);
}

void
ChatSessionManager::insertSession (const ChatSession& session) {
  sessions_.insert (std::make_pair (session.sessionId, session));
}
