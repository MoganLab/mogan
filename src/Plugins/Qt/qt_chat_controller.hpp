
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
#include <QHash>
#include <QObject>
#include <QPointer>

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

  /**
   * @brief 设置会话类型并同步侧边栏显示。
   * @param sessionId 目标会话 ID
   * @param type      会话类型（translate/explain，空串为普通对话）
   */
  void setSessionType (const string& sessionId, const string& type);

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
   * @brief 翻译/释义切换按钮点击时触发。
   *
   * 点击时根据当前文档身份切换到对应专属会话（不存在则创建并激活，
   * 不自动发送）；若点击已处于选中态的按钮，则退出并回到此前的会话。
   * @param action 动作类型（"translate" 或 "explain"）
   */
  void onActionToggleClicked (const string& action);

  /**
   * @brief 同步 dock 模式顶部翻译/释义切换按钮的选中态。
   *
   * 当前激活会话类型与动作匹配时对应按钮选中，两者互斥；非专属会话两按钮均不选中。
   * @param activeSessionId 当前激活的会话 ID
   */
  void syncDockActionButtons (const string& activeSessionId);

  /**
   * @brief dock 侧边栏打开时调用。
   *
   * 若当前活跃会话是翻译/释义专属会话，切到默认新会话（复用空白会话），
   * 使打开时顶部切换按钮不处于选中态。
   */
  void onDockSidebarShown ();

  /**
   * @brief 主文档切换时调用：刷新聊天 buffer 的 master 绑定。
   *
   * 聊天 buffer 的 master 在面板创建时一次性绑定，用户切换文档后会过期，
   * 导致面板内解析来源文档拿到旧文档；主文档切换为非 tmfs 文件时重指。
   * @param doc 新的主文档 buffer URL
   */
  void onMainDocumentChanged (const url& doc);

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

  // ---- 附件 ----

  /**
   * @brief 附件列表 → scheme alist 列表（每项 sha256/filename 两键，
   *        协议 v2 不暴露本地路径）；空表以 null_object 表示 '()。
   *
   * 仅序列化已 ready 的条目；处理中条目不应出现（发送栅栏保证），
   * 防御性跳过。纯函数，便于单元测试。
   */
  static object attachment_items_to_scheme (const QList<AttachmentItem>& items);

  /**
   * @brief 附件按钮点击时触发：弹出 AttachMenu 悬浮窗（最近文档 +
   * 选择文件入口）。
   *
   * 悬浮窗在附件按钮上方弹出并做屏幕边界钳制；重复点击附件按钮 toggle 关闭。
   * @param sessionId 目标会话 ID
   * @param globalPos Attach 按钮左上角全局坐标
   */
  void onAttachRequested (const string& sessionId, const QPoint& globalPos);

  /**
   * @brief 附件插件 ready 事件：清理转换 PDF、回填 sha256 与
   *        extractedChars、弹出 chip，累计超限时移除该附件并 toast。
   *
   * 假插件在写入假缓存后直接 ready；真插件完成实际抽取与缓存后 ready。
   * 两种实现均在 ready 后不再需要转换 PDF。常规附件原文件不删除。
   * chip 展示用的 filename/size 在选择时已填入 AttachmentItem，
   * 事件只需 taskId 定位、sha256 与 extractedChars 回填。
   *
   * @param extractedChars 插件提取出的文本字符数，参与本轮已就绪附件的
   *        累计上限判定（见 kAttachmentMaxTotalExtractedChars）。
   */
  void onAttachmentReady (const string& taskId, const string& sha256,
                          qint64 extractedChars);
  /**
   * @brief 附件插件 error 事件：清理转换 PDF、移除条目并 toast 原因。
   */
  void onAttachmentError (const string& taskId, const string& reason);

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
   * @brief 本轮已就绪附件的提取文本字符数累计。
   *
   * 只累加 ready 条目：处理中的附件 extractedChars 尚未回填（为 0），计入会
   * 得到偏小值而误放行。已移除的条目不在列表中，自然不参与累计。
   *
   * @param items 本轮附件列表
   * @return 累计字符数
   */
  static qint64 totalReadyExtractedChars (const QList<AttachmentItem>& items);

  /**
   * @brief 提取后累计上限（字符数）。
   *
   * 标定依据：1M 上下文模型在留出系统提示与对话历史后，正文可安全容纳的
   * 量级约 1 MiB；超过则请求必然被服务端以 413 拒绝，故在 ready 回填处
   * （上传后、发送前）就地拦截。
   */
  static const qint64 kMaxTotalExtractedChars;

  /**
   * @brief 把文档选区组成 AI 聊天输入体。
   *
   * 选区整体包成「引用」外观块（插入 → 外观块 → 引用，quote-env）；
   * translate 动作在其后追加固定提示词（cork 编码）；chat 追加空段，
   * 使光标落在引用块的下一行；explain 出两个引用块（上下文 + 选区），
   * 编号标签与说明句作为提示词夹在块间。
   * @param sel     文档选区树
   * @param action  AI 动作：translate / chat / explain
   * @param context 选区上下文（explain 的引文1，document 树，公式保留为
   *                子树），其余动作传空 document
   * @return document 形态的输入体
   */
  static tree composeAiInputBody (tree sel, string action,
                                  tree context= tree (moebius::DOCUMENT));

private:
  QTChatTabWidget*   view_= nullptr;  ///< View 指针，由 createView 创建
  ChatSessionManager sessionManager_; ///< 会话管理器
  ChatModelStore     modelStore_; ///< 模型清单（构造即加载，早于 createView）
  bool               firstOpen_= true; ///< 是否首次打开（首次时切换到新会话）
  string previousSessionId_; ///< 切入翻译/释义专属会话前的上一个活跃会话 ID
  QHash<QString, QString>
      attachTaskSession_; ///< ingest taskId → sessionId（事件路由）
  QHash<QString, QString> attachConvertedPdf_; ///< taskId → 待终态清理的 PDF
  int               attachTaskSeq_= 0; ///< ingest 任务号自增序号（不透明）
  QPointer<QWidget> attachMenuPopup_;  ///< 附件菜单悬浮窗

  /**
   * @brief 批量附加文件到指定会话：去重、派发 ingest 并入列 chips。
   *
   * 文档类附件（.tmu/.tm/.stem）先经宿主无头转换出 PDF，再以 PDF 走同一条
   * ingest；其余格式直接派发。
   *
   * @param sessionId 目标会话 ID
   * @param files     文件系统路径列表
   */
  void attachFiles (const string& sessionId, const QStringList& files);

  /**
   * @brief 派发 %attachment ingest 并登记 taskId 路由。
   *
   * 文档类附件与常规附件共用的插件侧入口。item.taskId 由调用方生成且条目
   * 已入列，本函数不改动它，只在派发成功后登记路由。
   *
   * @param sessionId  目标会话 ID（路由用）
   * @param item       待派发条目（taskId 须已生成）
   * @param count      本轮已选附件数（宿主预检的数量上限用）
   * @param bytes      本轮已选附件总字节（宿主预检的总量上限用）
   * @param sourcePath 宿主转换过格式时的原文件路径（插件据它算 sha256 与
   *        抽取缓存键，实际读取仍走 item.path）；常规附件传空串
   * @return 派发成功返回 true；插件不可用（已 toast）返回 false
   */
  bool dispatch_attachment_ingest (const string&   sessionId,
                                   AttachmentItem& item, int count,
                                   qint64 bytes, const QString& sourcePath);

  /**
   * @brief 文档类附件的转换编排。
   *
   * 条目入列由调用方（attachFiles）随本轮 items 统一完成，本函数不碰列表；
   * 转换成功后改写该条目指向临时 PDF 并派发 ingest，失败则移除并 toast。
   *
   * @param sessionId 目标会话 ID
   * @param panel     目标面板
   * @param item      已并入列表的条目（taskId 已生成，path 指向原文档）
   */
  void start_attachment_conversion (const string&          sessionId,
                                    ChatConversationPanel* panel,
                                    const AttachmentItem&  item);

  /**
   * @brief 按 ingest taskId 取出路由会话的面板（事件路由统一入口）。
   *
   * 命中即从路由表移除该任务（每个任务只投递一次事件）。
   * @return 目标面板；任务未知、无视图或无面板时返回 nullptr
   */
  ChatConversationPanel* takeAttachmentPanel (const QString& taskId);

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
   * @brief 记录当前活跃会话为切回目标。
   *
   * 仅当活跃会话不是翻译/释义专属会话时记录；专属会话之间互切时保留
   * 原切回目标。
   */
  void rememberReturnSession ();

  /**
   * @brief 当前活跃会话（无视图或无激活面板时为空）。
   */
  ChatSession* activeSession ();

  /**
   * @brief 查找或创建当前文档的翻译/释义专属会话并激活。
   *
   * 按 docId + 会话类型找未归档会话，命中即复用；未命中（含 docId 为空）
   * 才新建。
   * @param action  会话类型（"translate" 或 "explain"）
   * @param docId   来源文档 ID
   * @param docName 来源文档名（用于标题）
   * @return 会话 ID；创建失败返回空串
   */
  string getOrCreateDocSession (const string& action, const string& docId,
                                const string& docName);

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
   * @brief 发送按钮安全栅栏：附件处理中（未 ready）且非 Generating 时禁用。
   *
   * Generating 时按钮承担 Stop 职责必须可用；附件处理变化与会话状态
   * 变化都经本函数重算。
   * @param sessionId 目标会话 ID
   */
  void updateSendGate (const string& sessionId);

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
  friend class AttachMenuBridge;
  friend class TestChatController;
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
 * @brief Scheme→C++ 回调：附件插件 ready 事件（%attachment）。
 *
 * @param extractedChars 插件提取出的文本字符数（ready 事件六键契约之一），
 *        供累计上限判定。
 */
void qt_chat_attachment_ready (string taskId, string sha256,
                               int extractedChars);

/**
 * @brief Scheme→C++ 回调：附件插件 error 事件（%attachment）。
 */
void qt_chat_attachment_error (string taskId, string reason);

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

/**
 * @brief Scheme→C++ 回调：设置会话类型（translate/explain）。
 *
 * 与 sourceDocId 同理，恢复会话时 type 经此函数单独设置。
 */
void qt_chat_tab_set_session_type (string sessionId, string type);

string qt_chat_tab_active_message_buffer_url ();

/**
 * @brief Scheme→C++ 回调：通知 Chat 输入区重新计算高度。
 */
void qt_chat_notify_input_height ();

/**
 * @brief 主文档切换通知（qt_tm_widget SLOT_FILE 调用）。
 *
 * tmfs 标签页（如 Chat 页）在函数内过滤，不作为主文档；聊天控制器尚未创建
 * （从未打开过聊天）时静默无效，避免为此提前实例化。
 * @param name 新的主文档 buffer 名（SLOT_FILE 的 file 字符串）
 */
void qt_chat_notify_main_document_changed (const string& name);

#endif // QT_CHAT_CONTROLLER_HPP
