
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
#include <QString>
#include <QStringList>
#include <ctime>
#include <map>
#include <set>
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
  string                 sessionId;          ///< UUID，创建时生成
  string                 title;              ///< 会话标题，初始为空字符串
  string                 model;              ///< 绑定的模型名称
  ChatState              state;              ///< 当前生成状态
  bool                   archived;           ///< 是否归档
  time_t                 createdAt;          ///< 创建时间（Unix 时间戳）
  time_t                 updateAt;           ///< 最近活跃时间（用于排序索引）
  int                    defaultExpandCount; ///< 默认展开对话条数，固定为 5
  bool                   thinking;           ///< 是否启用推理模式，默认 false
  bool                   search;             ///< 是否启用网络搜索，默认 false
  bool                   registered; ///< 是否已注册到持久化层（已加入 sidebar）
  ChatConversationPanel* panel;      ///< 关联的面板指针
  QMetaObject::Connection sendBtnConnection; ///< send/stop 按钮信号连接句柄

  /**
   * @brief 格式化原始标题文本，CJK 截前 10 字符，英文截前 5 个单词。
   * @param rawTitle 从用户输入提取的原始标题
   * @return 格式化后的标题
   */
  static string formatTitle (const string& rawTitle);
};

/**
 * @brief 聊天会话管理器，负责会话的创建、销毁和元数据管理。
 *
 * 使用双容器索引：sessions_（主存储，按 sessionId 查找）+
 * timeIndex_（有序索引，按 updateAt 降序排列）。所有增删操作同步维护两个容器，
 * 消除运行时排序开销。
 *
 * @note 仅在 GUI 线程调用。ChatSessionManager 不保证线程安全。
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
   * @brief 将已归档的会话恢复为活跃状态，并更新 updateAt 置顶。
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
  string getModel (const string& sessionId) const;

  /**
   * @brief 设置会话的推理模式开关。
   * @param sessionId 目标会话 ID
   * @param thinking  是否启用推理模式
   */
  void setThinking (const string& sessionId, bool thinking);

  /**
   * @brief 获取会话的推理模式开关状态。
   * @param sessionId 目标会话 ID
   * @return 是否启用推理模式，会话不存在时返回 false
   */
  bool getThinking (const string& sessionId) const;

  /**
   * @brief 设置会话的网络搜索开关。
   * @param sessionId 目标会话 ID
   * @param search    是否启用网络搜索
   */
  void setSearch (const string& sessionId, bool search);

  /**
   * @brief 获取会话的网络搜索开关状态。
   * @param sessionId 目标会话 ID
   * @return 是否启用网络搜索，会话不存在时返回 false
   */
  bool getSearch (const string& sessionId) const;

  /**
   * @brief 获取所有会话 ID，按 updateAt 降序排列（最近活跃在前）。
   * 内部使用有序索引，无需排序，直接遍历。
   * @return 会话 ID 列表
   */
  std::vector<string> getAllSessionIds () const;

  /**
   * @brief 获取会话总数。
   */
  size_t sessionCount () const;

  /**
   * @brief 获取第一个非归档会话 ID（按 updateAt 降序）。
   *
   * 按 updateAt 降序遍历 timeIndex_，跳过 archived 会话，返回第一个非归档。
   * 复杂度：O(k)，k 为跳过的归档数。典型场景下接近 O(1)。
   *
   * @return 会话 ID，不存在则返回空字符串
   */
  string firstActiveSessionId () const;

  /**
   * @brief 查找可复用的空白会话（按 updateAt 降序优先）。
   *
   * 条件：非归档、无标题（未发送过消息的空白会话）。
   * 无论是否有面板，都可复用。
   *
   * @return 可复用的会话 ID，无则返回空字符串
   */
  string findReusableSession () const;

  /**
   * @brief 更新会话活跃时间并重排索引。
   * 在消息发送/接收完成时调用，更新 updateAt 并重排 timeIndex_。
   * @param sessionId 目标会话 ID
   */
  void touchSession (const string& sessionId);

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
   * @brief 从 Scheme 提取内容并生成标题，设置到 session。
   *
   * 调用 chat-persist-extract-title 获取原始文本，通过 formatTitle() 格式化后
   * 写入 session->title。仅当 session 无标题时执行，已有标题不覆盖。
   * @param sessionId 目标会话 ID
   */
  void generateTitleFromContent (const string& sessionId);

  /**
   * @brief 获取会话消息缓冲区的 tmfs URL。
   * @param sessionId 会话 ID
   * @return 格式为 "tmfs://chat/{sessionId}/message" 的 URL
   */
  static url messageBufferUrl (const string& sessionId);

  /**
   * @brief 获取会话输入缓冲区的 tmfs URL。
   * @param sessionId 会话 ID
   * @return 格式为 "tmfs://chat/{sessionId}/input" 的 URL
   */
  static url inputBufferUrl (const string& sessionId);

private:
  /// 时间索引结构，用于 set 排序
  struct TimeIndex {
    time_t updateAt;  ///< 最近活跃时间
    string sessionId; ///< 关联的会话 ID

    /// 降序排列：最近活跃在前；时间相同时按 sessionId 字典序
    bool operator< (const TimeIndex& o) const {
      if (updateAt != o.updateAt) return updateAt > o.updateAt;
      return sessionId < o.sessionId;
    }
  };

  std::map<string, ChatSession> sessions_;  ///< 主存储：sessionId → ChatSession
  std::set<TimeIndex>           timeIndex_; ///< 时间索引：始终按 updateAt 降序
};

#endif // QT_CHAT_SESSION_HPP
