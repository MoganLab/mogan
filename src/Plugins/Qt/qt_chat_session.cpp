
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
#include "qt_utilities.hpp"
#include "scheme.hpp"

#include <analyze.hpp>
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
  session.sessionId         = sessionId;
  session.state             = ChatState::Idle;
  session.archived          = false;
  std::time_t now           = std::time (nullptr);
  session.createdAt         = now;
  session.updateAt          = now;
  session.defaultExpandCount= 5;
  session.thinking          = false;
  session.search            = false;
  session.registered        = false;
  session.panel             = nullptr;
  sessions_.insert (std::make_pair (sessionId, session));
  timeIndex_.insert ({session.updateAt, sessionId});
  return sessionId;
}

void
ChatSessionManager::removeSession (const string& sessionId) {
  auto it= sessions_.find (sessionId);
  if (it != sessions_.end ()) {
    timeIndex_.erase ({it->second.updateAt, sessionId});
    sessions_.erase (it);
  }
}

void
ChatSessionManager::archiveSession (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (s) s->archived= true;
  // archive 不改变 updateAt，无需更新 timeIndex_
}

void
ChatSessionManager::restoreSession (const string& sessionId) {
  auto it= sessions_.find (sessionId);
  if (it == sessions_.end ()) return;

  auto& s   = it->second;
  s.archived= false;

  // 更新 updateAt 并重排：用户恢复会话是要继续对话，置顶到最前面
  timeIndex_.erase ({s.updateAt, sessionId});
  s.updateAt= std::time (nullptr);
  timeIndex_.insert ({s.updateAt, sessionId});
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
ChatSessionManager::getModel (const string& sessionId) const {
  auto it= sessions_.find (sessionId);
  if (it != sessions_.end ()) return it->second.model;
  return "";
}

void
ChatSessionManager::setThinking (const string& sessionId, bool thinking) {
  ChatSession* s= getSession (sessionId);
  if (s) s->thinking= thinking;
}

bool
ChatSessionManager::getThinking (const string& sessionId) const {
  auto it= sessions_.find (sessionId);
  if (it != sessions_.end ()) return it->second.thinking;
  return false;
}

void
ChatSessionManager::setSearch (const string& sessionId, bool search) {
  ChatSession* s= getSession (sessionId);
  if (s) s->search= search;
}

bool
ChatSessionManager::getSearch (const string& sessionId) const {
  auto it= sessions_.find (sessionId);
  if (it != sessions_.end ()) return it->second.search;
  return false;
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
  ids.reserve (timeIndex_.size ());
  for (const auto& ti : timeIndex_)
    ids.push_back (ti.sessionId);
  return ids;
}

size_t
ChatSessionManager::sessionCount () const {
  return sessions_.size ();
}

string
ChatSessionManager::firstActiveSessionId () const {
  for (const auto& ti : timeIndex_) {
    auto it= sessions_.find (ti.sessionId);
    if (it != sessions_.end () && !it->second.archived) return ti.sessionId;
  }
  return "";
}

string
ChatSessionManager::findReusableSession () const {
  for (const auto& ti : timeIndex_) {
    auto it= sessions_.find (ti.sessionId);
    if (it == sessions_.end () || it->second.archived) continue;
    const ChatSession& s= it->second;
    // 无标题 = 空白会话（未发过消息），无论是否有面板都可复用
    if (is_empty (s.title)) return ti.sessionId;
  }
  return "";
}

void
ChatSessionManager::touchSession (const string& sessionId) {
  auto it= sessions_.find (sessionId);
  if (it == sessions_.end ()) return;

  auto& s= it->second;

  // 从索引中删除旧 key（updateAt 是排序键，必须更新）
  timeIndex_.erase ({s.updateAt, sessionId});

  // 更新时间
  s.updateAt= std::time (nullptr);

  // 插入新 key
  timeIndex_.insert ({s.updateAt, sessionId});
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
  return url ("tmfs://chat/" * sessionId * "/message");
}

url
ChatSessionManager::inputBufferUrl (const string& sessionId) {
  return url ("tmfs://chat/" * sessionId * "/input");
}

void
ChatSessionManager::insertSession (const ChatSession& session) {
  auto it= sessions_.find (session.sessionId);
  if (it != sessions_.end ()) {
    // 已存在：清理旧 timeIndex_，直接赋值避免默认构造
    timeIndex_.erase ({it->second.updateAt, session.sessionId});
    it->second= session;
  }
  else {
    // 不存在：insert 新条目
    sessions_.insert ({session.sessionId, session});
  }
  timeIndex_.insert ({session.updateAt, session.sessionId});
}

string
ChatSession::formatTitle (const string& rawTitle) {
  QString qTitle= to_qstring (rawTitle);
  // 检测标题是否含 CJK 字符
  bool hasCJK= false;
  for (int i= 0; i < qTitle.length (); i++) {
    ushort code= qTitle[i].unicode ();
    if (code >= 0x4E00 && code <= 0x9FFF) {
      hasCJK= true;
      break;
    }
  }
  // 含 CJK: 截取前 10 个字符
  if (hasCJK) {
    if (qTitle.length () > 10) qTitle= qTitle.left (10) + "...";
  }
  // 纯英文: 截取前 5 个单词
  else {
    QStringList words= qTitle.split (' ', Qt::SkipEmptyParts);
    if (words.size () > 5) {
      words = words.mid (0, 5);
      qTitle= words.join (" ") + "...";
    }
  }
  return from_qstring_utf8 (qTitle);
}

void
ChatSessionManager::generateTitleFromContent (const string& sessionId) {
  ChatSession* s= getSession (sessionId);
  if (!s || !is_empty (s->title)) return;

  string extracted= as_string (call ("chat-persist-extract-title", sessionId));
  setTitle (sessionId, ChatSession::formatTitle (extracted));
}
