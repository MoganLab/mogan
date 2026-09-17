
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

#include "qt_chat_model.hpp"
#include "qt_chat_tab_widget.hpp"
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
   * @brief 导出指定会话的消息 buffer 到用户选择的路径（TMU 格式）。
   * @param sessionId 要导出的会话 ID
   */
  void onExportRequested (const string& sessionId);

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
   * @brief 推理模式开关切换时触发。
   * @param sessionId 目标会话 ID
   * @param enabled   是否启用推理模式
   */
  void onThinkingToggled (const string& sessionId, bool enabled);

  /**
   * @brief 网络搜索开关切换时触发。
   * @param sessionId 目标会话 ID
   * @param enabled   是否启用网络搜索
   */
  void onSearchToggled (const string& sessionId, bool enabled);

  /**
   * @brief Model 按钮点击时触发：弹出模型选择菜单（按清单构建）。
   *
   * 菜单每次打开重建并按当前会话模型刷新选中态，尾部追加「思考强度」
   * 子菜单（低/中/高）；模型项点击转 onModelSelected，强度项点击转
   * onThinkingEffortSelected。
   * @param sessionId 目标会话 ID
   * @param globalPos 菜单弹出位置（全局坐标）
   */
  void onModelMenuRequested (const string& sessionId, const QPoint& globalPos);

  /**
   * @brief 模型菜单项被选择时触发。
   *
   * key 不在清单内忽略；写回 ChatSession.model 并落 manifest。
   * 切换到不允许推理/搜索的模型时，对应开关重置为关（会话状态 +
   * manifest + 按钮取消勾选），避免隐藏的开泄漏到下一轮请求。
   * @param sessionId 目标会话 ID
   * @param key       模型 key（清单条目的 model 字段）
   */
  void onModelSelected (const string& sessionId, const string& key);

  /**
   * @brief 思考强度子菜单项被选择时触发。
   *
   * 非法取值按 medium 处理；写回 ChatSession.thinkingEffort 并落 manifest。
   * @param sessionId 目标会话 ID
   * @param effort    思考强度（low/medium/high）
   */
  void onThinkingEffortSelected (const string& sessionId, const string& effort);

  /**
   * @brief Scheme→C++ 回调：通知状态变更。
   */
  void notifyStateChanged (const string& sessionId, const string& stateStr);

  /**
   * @brief Scheme→C++ 回调：恢复单个会话元数据。
   */
  void restoreSessionMeta (const ChatSession& session);

  /**
   * @brief 销毁 View 引用，防止悬垂指针。
   */
  void destroyView ();

  string activeSessionMessageBufferUrl () const;

  /**
   * @brief 清理导出文件名：空格替换为下划线，过滤非法字符。
   * @param rawName 原始文件名（不含后缀）
   * @return 清理后的文件名（不含后缀）
   */
  static QString sanitizeExportFileName (const QString& rawName);

  /**
   * @brief 把文档选区组成 AI 聊天输入体。
   *
   * 选区整体包成「引用」外观块（插入 → 外观块 → 引用，quote-env）；
   * translate 动作在其后追加固定提示词（cork 编码）；chat 追加空段，
   * 使光标落在引用块的下一行。
   * @param sel    文档选区树
   * @param action AI 动作：translate 或 chat
   * @return document 形态的输入体
   */
  static tree composeAiInputBody (tree sel, string action);

private:
  QTChatTabWidget*   view_= nullptr;  ///< View 指针，由 createView 创建
  ChatSessionManager sessionManager_; ///< 会话管理器
  ChatModelStore     modelStore_; ///< 模型清单（构造即加载，早于 createView）
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
   * @brief 导出会话的 message buffer 到磁盘（TMU 格式）。
   *
   * 调用 Scheme 的 chat-persist-export-buffer，仅写 buffer 文件，不更新
   * manifest。 适用于 buffer 内容确实发生变更的场景（发送消息、LLM 生成完成）。
   * @param sessionId 目标会话 ID
   */
  void exportBuffer (const string& sessionId);

  /**
   * @brief 更新 manifest 中指定会话的元数据条目。
   *
   * 调用 Scheme 的 chat-persist-update-manifest，仅写 manifest JSON，不导出
   * buffer。 适用于归档、恢复等仅元数据变更的场景，避免用空 buffer
   * 覆盖磁盘文件。
   * @param sessionId 目标会话 ID
   */
  void updateManifest (const string& sessionId);

  /**
   * @brief 确保存在一个可用的空白会话。
   *
   * 优先复用已有的空白会话，否则创建新会话。
   */
  void ensureNewConversation ();

  /**
   * @brief 创建全新会话并激活，绝不复用已有空白会话。
   *
   * 与 ensureNewConversation 的差别：空白会话里可能有未发送的输入草稿，
   * AI 翻译等一次性动作不应覆盖它，故每次都创建全新会话。
   * @param modelKey 指定初始模型 key（须在清单内，否则回退清单默认模型），
   *                 默认为空（清单默认模型）
   * @return 新会话的面板指针，创建失败时返回 nullptr
   */
  ChatConversationPanel* createNewConversation (const string& modelKey= "");

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
   * @brief 获取单个会话的显示标题。
   * @param sessionId 目标会话 ID
   * @return 显示标题，无标题时返回 "新会话"
   */
  string getSessionDisplayTitle (const string& sessionId);

  /**
   * @brief 注册未注册的 session 到持久化层并加入 sidebar。
   *
   * 首次发送消息时调用，完成延迟注册。
   * @param sessionId 目标会话 ID
   */
  void registerSession (const string& sessionId);

  /**
   * @brief 连接 Panel 的所有 signal 到 Controller。
   * @param panel 目标面板
   */
  void connectPanelSignals (ChatConversationPanel* panel);

  /**
   * @brief 刷新指定会话面板的 Model 按钮显示（logo+名称+箭头）。
   *
   * 会话模型不在清单内时按默认模型显示（与 activateSession 的内存
   * 回退一致）；会话无面板时忽略。
   * @param sessionId 目标会话 ID
   * @param menuOpen  模型菜单是否打开（箭头朝上/下）
   */
  void updateModelButtonDisplay (const string& sessionId, bool menuOpen= false);

  /**
   * @brief 新会话的初始思考强度。
   *
   * 有历史记录（已发送过消息、即有标题的会话）时取最近活跃记录的强度；
   * 首次对话无记录时取清单中 modelKey 对应条目的 thinking_effort。
   * @param modelKey 新会话使用的模型 key（须在清单内）
   */
  string initialThinkingEffort (const string& modelKey);

  /**
   * @brief 按当前会话模型的能力（allowThinking/allowSearch）刷新深度
   * 思考、联网搜索按钮的显隐。
   *
   * 模型不允许的能力对应按钮隐藏；模型不在清单内时按 find 的 key 兜底
   * （允许全部，按钮保持可见）。面板未创建时忽略。
   * @param sessionId 目标会话 ID
   */
  void applyModelCapabilities (const string& sessionId);

  friend void qt_chat_tab_set_state (string sessionId, string stateStr);
  friend void qt_chat_tab_restore_session (
      string sessionId, string title, string model, string archived,
      string createdAtStr, string updatedAtStr, int defaultExpandCount,
      string thinking, string search, string thinkingEffort);
  friend void qt_chat_notify_input_height ();
  friend void qt_chat_ai_send_selection (tree sel, string action);
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
 * @brief 引用文档选区到 AI 聊天输入区；翻译动作追加提示词并自动发送。
 *
 * AI 操作栏（翻译/对话）的共用入口：打开 AI 侧边栏，把选区内容写入输入区。
 * translate 按来源文档的 stem-doc-id 共享一个翻译会话：已有未归档的翻译
 * 会话则激活并继续发送，否则新建（不复用空白会话，避免覆盖未发送的草稿），
 * 在引用内容下方追加固定提示词并走与发送按钮相同的 onSendRequested 管线
 * 自动发送，会话标题为词典本地化的「翻译: 文件名」；chat 只填入当前会话
 * 输入区（当前会话是绑定了文档的翻译会话时先切到空白会话），留给用户
 * 补写后手动发送。
 * @param sel    文档选区树（调用方须在焦点/视图切换前捕获）
 * @param action AI 动作：translate 或 chat
 */
void qt_chat_ai_send_selection (tree sel, string action);

/**
 * @brief Scheme→C++ 回调：恢复单个聊天会话。
 */
void qt_chat_tab_restore_session (string sessionId, string title, string model,
                                  string archived, string createdAtStr,
                                  string updatedAtStr, int defaultExpandCount,
                                  string thinking, string search,
                                  string thinkingEffort);

/**
 * @brief Scheme→C++ 回调：设置会话来源文档的 stem-doc-id。
 *
 * glue 单函数参数上限为 10，恢复会话时 sourceDocId 经此函数单独设置。
 */
void qt_chat_tab_set_source_doc_id (string sessionId, string docId);

string qt_chat_tab_active_message_buffer_url ();

/**
 * @brief Scheme→C++ 回调：通知 Chat 输入区重新计算高度。
 */
void qt_chat_notify_input_height ();

#endif // QT_CHAT_CONTROLLER_HPP
