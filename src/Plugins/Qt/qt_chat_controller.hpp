
/******************************************************************************
 * MODULE     : qt_chat_controller.hpp
 * DESCRIPTION: Chat Tab 的核心管理类（逻辑 + Scheme 交互）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CHAT_CONTROLLER_HPP
#define QT_CHAT_CONTROLLER_HPP

#include "qt_chat_tab_widget.hpp"
#include <QMap>
#include <QObject>

/**
 * @brief Chat Tab 的核心管理类。
 *
 * 持有 ChatSessionManager 和 View 指针，
 * 集中管理所有 Scheme 交互、状态变更、持久化逻辑。
 * 依赖方向：Controller → View（单向）。
 */
class ChatController : public QObject {
  Q_OBJECT

public:
  explicit ChatController (QObject* parent= nullptr);
  ~ChatController () override;

  /**
   * @brief 创建 View 控件并完成初始化（连接信号、加载会话）。
   *
   * 返回 QWidget*，调用方无需知道 QTChatTabWidget 的存在。
   */
  QWidget* createView (QWidget* parent, qt_tm_widget_rep* tm);

  /**
   * @brief 获取会话管理器引用。
   */
  ChatSessionManager& sessionManager ();

  // ---- 用户交互（由 View 的 signal 触发） ----

  /**
   * @brief 侧边栏点击会话项时触发。
   *
   * 活跃会话执行 activateSession，已归档会话忽略并恢复视觉状态。
   * @param sessionId 被点击的会话 ID
   */
  void onSessionClicked (const string& sessionId);

  /**
   * @brief 用户点击发送按钮时触发。
   *
   * 执行空输入检查、自动提取标题（首次发送时）、调用 Scheme 发送、
   * 更新状态为 Generating。
   * @param sessionId 目标会话 ID
   */
  void onSendRequested (const string& sessionId);

  /**
   * @brief 用户点击取消/停止按钮时触发。
   * @param sessionId 目标会话 ID
   */
  void onCancelRequested (const string& sessionId);

  /**
   * @brief 删除指定会话列表。
   *
   * 逐个调用 Scheme 销毁、移除 sidebar item、移除面板，
   * 然后激活下一个可用会话或创建新会话。
   * @param sessionIds 要删除的会话 ID 列表
   */
  void onDeleteRequested (const QList<string>& sessionIds);

  /**
   * @brief 归档指定会话列表。
   *
   * 如果归档了当前激活会话，自动激活下一个非归档会话。
   * @param sessionIds 要归档的会话 ID 列表
   */
  void onArchiveRequested (const QList<string>& sessionIds);

  /**
   * @brief 恢复已归档的会话并激活。
   * @param sessionId 要恢复的会话 ID
   */
  void onRestoreRequested (const string& sessionId);

  /**
   * @brief 创建新会话或复用空白会话。
   */
  void onNewChatRequested ();

  /**
   * @brief 重命名会话标题并更新侧边栏显示。
   * @param sessionId 目标会话 ID
   * @param newTitle  新标题
   */
  void onRenameRequested (const string& sessionId, const string& newTitle);

  /**
   * @brief Scheme→C++ 回调：通知状态变更。
   */
  void notifyStateChanged (const string& sessionId, const string& stateStr);

  /**
   * @brief Scheme→C++ 回调：恢复单个会话元数据。
   */
  void restoreSessionMeta (const string& sessionId, const string& title,
                           const string& model, bool archived,
                           const string& createdAt);

  /**
   * @brief 销毁 View 引用，防止悬垂指针。
   */
  void destroyView ();

private:
  QTChatTabWidget*   view_= nullptr;   ///< View 指针，由 createView 创建
  ChatSessionManager sessionManager_;  ///< 会话管理器
  bool               firstOpen_= true; ///< 是否首次打开（首次时切换到新会话）

  /**
   * @brief 激活指定会话：按需创建面板，按需加载内容。
   * @param sessionId 要激活的会话 ID
   */
  void activateSession (const string& sessionId);

  /**
   * @brief 按需加载会话的消息内容到面板。
   *
   * 调用 Scheme 的 chat-persist-load-session-content 加载消息 buffer。
   * @param panel 目标面板
   */
  void loadSessionContent (ChatConversationPanel* panel);

  /**
   * @brief 将单个会话元数据持久化到 Scheme 层。
   * @param sessionId 要保存的会话 ID
   */
  void saveOneSession (const string& sessionId);

  /**
   * @brief 确保存在一个可用的空白会话。
   *
   * 优先复用已有的空白会话，否则创建新会话。
   */
  void ensureNewConversation ();

  /**
   * @brief 获取或按需创建面板（延迟加载场景）。
   * @param sessionId 目标会话 ID
   * @return 面板指针，失败时返回 nullptr
   */
  ChatConversationPanel* getOrCreatePanel (const string& sessionId);

  /**
   * @brief 构建所有会话的显示信息列表，供 Sidebar 初始化使用。
   * @return 显示信息列表
   */
  QList<SessionDisplayInfo> buildDisplayInfos ();

  /**
   * @brief 确定初始激活会话（第一个非归档会话）。
   * @return 会话 ID，无可用会话时返回空字符串
   */
  string determineInitialActiveSession ();

  /**
   * @brief 获取所有会话的显示标题映射。
   * @return sessionId → displayTitle 映射
   */
  QMap<string, string> getDisplayTitles ();

  /**
   * @brief 获取单个会话的显示标题。
   * @param sessionId 目标会话 ID
   * @return 显示标题，无标题时返回 "新会话"
   */
  string getSessionDisplayTitle (const string& sessionId);

  friend void qt_chat_tab_set_state (string sessionId, string stateStr);
  friend void qt_chat_tab_restore_session (string sessionId, string title,
                                           string model, string archived,
                                           string createdAt);
};

/**
 * @brief 获取全局 ChatController 实例。
 */
ChatController* get_chat_controller ();

/**
 * @brief Scheme→C++ 回调：通知 Chat Tab 的会话状态变更。
 */
void qt_chat_tab_set_state (string sessionId, string stateStr);

/**
 * @brief Scheme→C++ 回调：恢复单个聊天会话。
 */
void qt_chat_tab_restore_session (string sessionId, string title, string model,
                                  string archived, string createdAt);

#endif // QT_CHAT_CONTROLLER_HPP
