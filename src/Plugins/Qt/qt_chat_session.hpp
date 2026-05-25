
/******************************************************************************
 * MODULE     : qt_chat_session.hpp
 * DESCRIPTION: 聊天会话数据模型与会话管理器
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CHAT_SESSION_HPP
#define QT_CHAT_SESSION_HPP

#include "url.hpp"
#include <QMetaObject>
#include <map>
#include <vector>

class ChatConversationPanel;

/**
 * @brief 聊天会话的生成状态。
 */
enum class ChatState {
  Idle,       ///< 空闲，可发送
  Generating, ///< LLM 正在生成，可取消
};

/**
 * @brief 单个聊天会话的数据。
 */
struct ChatSession {
  string                  sessionId;         ///< UUID，创建时生成
  string                  title;             ///< 会话标题，初始为空字符串
  string                  model;             ///< 绑定的模型名称
  ChatState               state;             ///< 当前生成状态
  bool                    archived;          ///< 是否归档
  ChatConversationPanel*  panel;             ///< 关联的面板指针
  QMetaObject::Connection sendBtnConnection; ///< send/stop 按钮信号连接句柄
};

/**
 * @brief 聊天会话管理器，负责会话的创建、销毁和元数据管理。
 */
class ChatSessionManager {
public:
  string              createSession ();
  void                removeSession (const string& sessionId);
  void                archiveSession (const string& sessionId);
  void                restoreSession (const string& sessionId);
  void                setTitle (const string& sessionId, const string& title);
  void                setState (const string& sessionId, ChatState state);
  void                setModel (const string& sessionId, const string& model);
  string              getModel (const string& sessionId);
  std::vector<string> getAllSessionIds () const;
  ChatSession*        getSession (const string& sessionId);
  ChatSession*        findSessionByPanel (ChatConversationPanel* panel);
  void       setPanel (const string& sessionId, ChatConversationPanel* panel);
  void       insertSession (const ChatSession& session);
  static url messageBufferUrl (const string& sessionId);
  static url inputBufferUrl (const string& sessionId);

private:
  std::map<string, ChatSession> sessions_;
};

#endif // QT_CHAT_SESSION_HPP
