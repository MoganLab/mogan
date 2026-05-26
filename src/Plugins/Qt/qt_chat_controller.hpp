
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

  void onSessionClicked (const string& sessionId);
  void onSendRequested (const string& sessionId);
  void onCancelRequested (const string& sessionId);
  void onDeleteRequested (const QList<string>& sessionIds);
  void onArchiveRequested (const QList<string>& sessionIds);
  void onRestoreRequested (const string& sessionId);
  void onNewChatRequested ();
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
  QTChatTabWidget*   view_= nullptr;
  ChatSessionManager sessionManager_;
  bool               firstOpen_= true;

  // 内部方法
  void                      activateSession (const string& sessionId);
  void                      loadSessionContent (ChatConversationPanel* panel);
  void                      saveOneSession (const string& sessionId);
  void                      ensureNewConversation ();
  ChatConversationPanel*    getOrCreatePanel (const string& sessionId);
  QList<SessionDisplayInfo> buildDisplayInfos ();
  string                    determineInitialActiveSession ();
  QMap<string, string>      getDisplayTitles ();
  string                    getSessionDisplayTitle (const string& sessionId);

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
