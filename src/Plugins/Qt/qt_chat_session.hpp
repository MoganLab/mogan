
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
  string                  sessionId; ///< UUID，创建时生成
  string                  title;     ///< 会话标题，初始为空字符串
  string                  model;     ///< 绑定的模型名称
  ChatState               state;     ///< 当前生成状态
  bool                    archived;  ///< 是否归档
  string                  createdAt; ///< Unix 时间戳字符串，如 "1748266800"
  ChatConversationPanel*  panel;     ///< 关联的面板指针
  QMetaObject::Connection sendBtnConnection; ///< send/stop 按钮信号连接句柄
};

/**
 * @brief 聊天会话管理器，负责会话的创建、销毁和元数据管理。
 */
class ChatSessionManager {
public:
  /**
   * @brief 创建新的聊天会话。
   *
   * 生成 UUID 作为 sessionId，初始化状态为 Idle、archived 为 false，
   * 记录当前 Unix 时间戳。
   *
   * @return 新创建会话的 sessionId
   */
  string createSession ();

  /**
   * @brief 删除指定会话。
   * @param sessionId 要删除的会话 ID
   */
  void removeSession (const string& sessionId);

  /**
   * @brief 将指定会话标记为已归档。
   * @param sessionId 要归档的会话 ID
   */
  void archiveSession (const string& sessionId);

  /**
   * @brief 将已归档的会话恢复为活跃状态。
   * @param sessionId 要恢复的会话 ID
   */
  void restoreSession (const string& sessionId);

  /**
   * @brief 设置会话标题。
   * @param sessionId 目标会话 ID
   * @param title     新标题
   */
  void setTitle (const string& sessionId, const string& title);

  /**
   * @brief 设置会话生成状态。
   * @param sessionId 目标会话 ID
   * @param state     新状态（Idle 或 Generating）
   */
  void setState (const string& sessionId, ChatState state);

  /**
   * @brief 设置会话绑定的模型名称。
   * @param sessionId 目标会话 ID
   * @param model     模型名称
   */
  void setModel (const string& sessionId, const string& model);

  /**
   * @brief 获取会话绑定的模型名称。
   * @param sessionId 目标会话 ID
   * @return 模型名称，会话不存在时返回空字符串
   */
  string getModel (const string& sessionId);

  /**
   * @brief 获取所有会话 ID，按 createdAt 降序排列（最新的在前）。
   * @return 会话 ID 列表
   */
  std::vector<string> getAllSessionIds () const;

  /**
   * @brief 根据 ID 获取会话指针。
   * @param sessionId 目标会话 ID
   * @return 会话指针，不存在时返回 nullptr
   */
  ChatSession* getSession (const string& sessionId);

  /**
   * @brief 根据面板指针反查所属会话。
   * @param panel 面板指针
   * @return 关联的会话指针，未找到时返回 nullptr
   */
  ChatSession* findSessionByPanel (ChatConversationPanel* panel);

  /**
   * @brief 设置会话关联的面板指针。
   * @param sessionId 目标会话 ID
   * @param panel     面板指针
   */
  void setPanel (const string& sessionId, ChatConversationPanel* panel);

  /**
   * @brief 插入预构造的会话（用于从持久化数据恢复）。
   * @param session 要插入的会话数据
   */
  void insertSession (const ChatSession& session);

  /**
   * @brief 获取会话消息缓冲区的 tmfs URL。
   * @param sessionId 会话 ID
   * @return 格式为 "tmfs://chat-message-{sessionId}" 的 URL
   */
  static url messageBufferUrl (const string& sessionId);

  /**
   * @brief 获取会话输入缓冲区的 tmfs URL。
   * @param sessionId 会话 ID
   * @return 格式为 "tmfs://chat-input-{sessionId}" 的 URL
   */
  static url inputBufferUrl (const string& sessionId);

private:
  std::map<string, ChatSession> sessions_; ///< sessionId → ChatSession 映射
};

#endif // QT_CHAT_SESSION_HPP
