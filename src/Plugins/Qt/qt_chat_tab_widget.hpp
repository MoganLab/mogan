
/******************************************************************************
 * MODULE     : qt_chat_tab_widget.hpp
 * DESCRIPTION: Mogan STEM 的 LLM 聊天标签页控件
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CHAT_TAB_WIDGET_HPP
#define QT_CHAT_TAB_WIDGET_HPP

#include "url.hpp"
#include <QList>
#include <QWidget>

#include "widget.hpp"

#include <map>

class QCheckBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpacerItem;
class QStackedWidget;
class QString;
class QTimer;
class QToolBar;
class QVBoxLayout;
class QEvent;

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
  string    sessionId; ///< UUID，创建时生成
  string    title;     ///< 会话标题，初始为空字符串
  string    model;     ///< 绑定的模型名称
  ChatState state;     ///< 当前生成状态
  bool      archived;  ///< 是否归档
  void*     panel;     ///< 关联的 ChatConversationPanel 指针
};

/**
 * @brief 聊天会话管理器，负责会话的创建、销毁和元数据管理。
 */
class ChatSessionManager {
public:
  /**
   * @brief 创建新会话，分配 UUID，返回 sessionId。
   */
  string createSession ();

  /**
   * @brief 销毁指定会话。
   */
  void removeSession (const string& sessionId);

  /**
   * @brief 将会话移入归档区。
   */
  void archiveSession (const string& sessionId);

  /**
   * @brief 将会话从归档区恢复。
   */
  void restoreSession (const string& sessionId);

  /**
   * @brief 设置会话标题。
   */
  void setTitle (const string& sessionId, const string& title);

  /**
   * @brief 设置会话生成状态。
   */
  void setState (const string& sessionId, ChatState state);

  /**
   * @brief 设置会话绑定的模型。
   */
  void setModel (const string& sessionId, const string& model);

  /**
   * @brief 获取会话绑定的模型。
   */
  string getModel (const string& sessionId);

  /**
   * @brief 获取所有会话 ID 列表。
   */
  std::vector<string> getAllSessionIds () const;

  /**
   * @brief 获取会话数据，不存在则返回 nullptr。
   */
  ChatSession* getSession (const string& sessionId);

  /**
   * @brief 通过面板指针反查会话。
   */
  ChatSession* findSessionByPanel (void* panel);

  /**
   * @brief 设置会话关联的面板指针。
   */
  void setPanel (const string& sessionId, void* panel);

  /**
   * @brief 手动插入已有会话（用于恢复持久化会话）。
   */
  void insertSession (const ChatSession& session);

  /**
   * @brief 根据 sessionId 推导消息 buffer URL。
   */
  static url messageBufferUrl (const string& sessionId);

  /**
   * @brief 根据 sessionId 推导输入 buffer URL。
   */
  static url inputBufferUrl (const string& sessionId);

private:
  std::map<string, ChatSession> sessions_; ///< sessionId → ChatSession 映射。
};

/**
 * @brief Mogan STEM 的 LLM 聊天标签页控件。
 *
 * 提供基于侧边栏的聊天界面，支持多会话切换。
 * 每个会话拥有独立的输入区和消息展示区，
 * 底层由嵌入的 TeXmacs 控件承载。
 */
class QTChatTabWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief 构造聊天标签页控件。
   * @param parent 父控件。
   */
  explicit QTChatTabWidget (QWidget* parent= nullptr);

  /**
   * @brief 销毁控件及其所有会话面板。
   */
  ~QTChatTabWidget () override;

protected:
  /**
   * @brief 将按键按下事件转发到 Scheme 层。
   * @param event 按键事件。
   */
  void keyPressEvent (QKeyEvent* event) override;

  /**
   * @brief 将按键释放事件转发到 Scheme 层。
   * @param event 按键事件。
   */
  void keyReleaseEvent (QKeyEvent* event) override;
  bool eventFilter (QObject* watched, QEvent* event) override;

private:
  /**
   * @brief 单个会话面板的内部数据。
   *
   * 保存与会话轮次关联的所有 Qt 控件和 TeXmacs buffer。
   */
  struct ChatConversationPanel;

  /**
   * @brief 构建左侧边栏（标题、新建聊天按钮、会话列表）。
   * @param sidebarLayout 待填充的布局。
   */
  void setup_left_sidebar (QVBoxLayout* sidebarLayout);

  /**
   * @brief 构建右侧内容区（堆叠的会话页面）。
   * @param mainLayout 主水平布局，用于插入内容区。
   */
  void setup_right_content (QHBoxLayout* mainLayout);

  /**
   * @brief 创建新的会话面板，包含控件和 buffer。
   * @param title 会话的显示标题。
   * @return 新建会话面板的指针。
   */
  ChatConversationPanel* create_conversation (const QString& title);

  /**
   * @brief 使用指定模型创建并激活一个新会话。
   * @param model 模型名称。
   */
  void create_new_conversation_with_model (const string& model);

  /**
   * @brief 将可见页面切换到指定会话。
   * @param panel 待激活的会话面板。
   */
  void activate_conversation (ChatConversationPanel* panel);

  /**
   * @brief 更新侧边栏标签及选中状态。
   */
  void refresh_sidebar ();

  /**
   * @brief 将指定面板从欢迎态切换到会话态。
   *
   * 播放淡入淡出及顶部间距动画。
   * @param panel 目标会话面板。
   */
  void enter_conversation_mode (ChatConversationPanel* panel);

  /**
   * @brief 读取输入内容，委托给 Scheme 层处理，并触发模式切换。
   * @param panel 发送消息的会话面板。
   */
  void handle_send (ChatConversationPanel* panel);

  /**
   * @brief 取消当前会话的 LLM 生成。
   * @param panel 待取消的会话面板。
   */
  void handle_cancel (ChatConversationPanel* panel);

  /**
   * @brief 从输入 buffer 中获取文档树。
   * @param panel 待读取输入的会话面板。
   * @return 输入内容对应的 TeXmacs 树。
   */
  tree read_input_message (const ChatConversationPanel* panel) const;

  /**
   * @brief 将键盘焦点设置到指定面板的输入编辑器。
   * @param panel 目标会话面板。
   */
  void focus_input_editor (ChatConversationPanel* panel);

  /**
   * @brief 切换侧边栏的收起/展开状态。
   */
  void toggle_sidebar ();

  /**
   * @brief 根据输入内容自适应调整输入框高度。
   * @param panel 目标会话面板。
   */
  void adjust_input_height (ChatConversationPanel* panel);

  /**
   * @brief 删除指定的会话面板列表。
   * @param panels 待删除的会话面板列表。
   */
  void delete_sessions (const QList<ChatConversationPanel*>& panels);

  /**
   * @brief 获取所有 checkbox 被勾选的会话面板。
   * @return 被勾选的面板列表。
   */
  QList<ChatConversationPanel*> get_checked_panels () const;

  /**
   * @brief 进入多选模式，显示 checkbox 和批量操作栏。
   * @param archived 是否从归档区进入（决定显示哪些操作按钮）。
   */
  void enter_multi_select_mode (bool archived);

  /**
   * @brief 退出多选模式，隐藏 checkbox 和批量操作栏。
   */
  void exit_multi_select_mode ();

public:
  /**
   * @brief 计算输入文档的段落（行）数。
   * @param body TeXmacs 文档树。
   * @return 段落数量。
   */
  static int count_input_lines (tree body);

  /**
   * @brief 根据排版后的实际高度估算等效行数。
   * @param contentHeight 排版后的内容高度（SI 单位）。
   * @return 等效行数，若高度无效则返回 0。
   */
  static int estimate_lines_from_height (SI contentHeight);

  /**
   * @brief 判断文档主体是否实际为空。
   * @param body TeXmacs 文档树。
   * @return 若主体不含可见内容则返回 true。
   */
  static bool is_empty_document_body (tree body);

  /**
   * @brief 确保至少存在一个空白新对话，若没有则创建。
   */
  void ensure_new_conversation ();

  /**
   * @brief 为 Chat Tab 安装主菜单栏内容。
   * @param menuWidget 菜单 widget。
   */
  void install_chat_menu_bar (widget menuWidget);

  /**
   * @brief 设置 Chat Tab 的模式工具栏内容。
   * @param modeWidget 模式图标 widget。
   */
  void set_chat_mode_icons (widget modeWidget);

  /**
   * @brief 设置 Chat Tab 的焦点工具栏内容。
   * @param focusWidget 焦点图标 widget。
   */
  void set_chat_focus_icons (widget focusWidget);

  /**
   * @brief 被通知 Scheme 侧生成状态变更。
   * @param sessionId 会话 ID。
   * @param stateStr 状态字符串 ("idle" 或 "generating")。
   */
  void notifyStateChanged (const string& sessionId, const string& stateStr);

  /**
   * @brief 保存单个会话到磁盘（增量）。
   */
  void saveOneSession (const string& sessionId);

  /**
   * @brief 从磁盘加载会话。
   */
  void loadSessions ();

  /**
   * @brief 将恢复的面板添加到会话列表。
   */
  void addConversation (ChatConversationPanel* panel);

  /**
   * @brief 恢复单个会话面板（用于加载持久化会话）。
   * @param sessionId 会话 UUID。
   * @param title 会话标题。
   * @param model 模型名称。
   * @param archived 是否归档。
   * @return 恢复的会话面板指针。
   */
  ChatConversationPanel* restore_conversation (const string& sessionId,
                                               const string& title,
                                               const string& model,
                                               bool          archived);

private:
  QWidget*        sidebarWidget_;          ///< 左侧边栏容器。
  QWidget*        contentWidget_;          ///< 右侧内容区容器。
  QLabel*         conversationCountLabel_; ///< 显示会话数量的标签。
  QWidget*        conversationListWidget_; ///< 承载会话列表的控件。
  QVBoxLayout*    conversationListLayout_; ///< 会话按钮的布局。
  QPushButton*    archiveHeaderButton_;  ///< 归档区标题按钮（点击展开/折叠）。
  QWidget*        archiveListWidget_;    ///< 承载归档会话列表的控件。
  QVBoxLayout*    archiveListLayout_;    ///< 归档会话按钮的布局。
  bool            archiveCollapsed_;     ///< 归档区当前是否折叠。
  QPushButton*    collapseButton_;       ///< 侧边栏内的收缩按钮。
  QPushButton*    floatingExpandBtn_;    ///< 内容区左上角的浮球展开按钮。
  QPushButton*    newChatButton_;        ///< 新建会话按钮。
  QWidget*        sidebarNormalContent_; ///< 侧边栏展开时的内容容器。
  QStackedWidget* conversationStack_;    ///< 会话页面的堆叠控件。
  QList<ChatConversationPanel*> conversations_;      ///< 所有会话面板的列表。
  ChatConversationPanel*        activeConversation_; ///< 当前激活的会话。
  ChatSessionManager            sessionManager_;     ///< 会话管理器。
  bool         sidebarCollapsed_;     ///< 侧边栏当前是否处于收起状态。
  int          sidebarExpandedWidth_; ///< 侧边栏展开时的宽度（像素）。
  QToolBar*    chatMenuToolBar_;      ///< Chat Tab 的菜单工具栏。
  QToolBar*    chatModeToolBar_;      ///< Chat Tab 的模式工具栏。
  QToolBar*    chatFocusToolBar_;     ///< Chat Tab 的焦点工具栏。
  bool         multiSelectMode_;      ///< 是否处于多选模式（活跃会话）。
  bool         archiveSelectMode_;    ///< 是否处于多选模式（归档会话）。
  QWidget*     multiSelectBar_;       ///< 多选模式下的批量操作栏。
  QPushButton* batchArchiveBtn_;      ///< 批量归档按钮（归档区多选时隐藏）。
  QLineEdit*   searchEdit_;           ///< 会话搜索输入框。
  QList<ChatConversationPanel*>
      zombiePanels_; ///< 已删除的会话面板（隐藏但未释放）。
};

/**
 * @brief Scheme→C++ 回调：通知 Chat Tab 的会话状态变更。
 * @param sessionId 会话 UUID。
 * @param stateStr 状态字符串 ("idle" 或 "generating")。
 */
void qt_chat_tab_set_state (string sessionId, string stateStr);

/**
 * @brief Scheme→C++ 回调：加载所有聊天会话。
 */
void qt_chat_tab_load_sessions ();

/**
 * @brief Scheme→C++ 回调：恢复单个聊天会话。
 * @param sessionId 会话 UUID。
 * @param title 会话标题。
 * @param model 模型名称。
 * @param archived 是否归档（"true"/"false"）。
 */
void qt_chat_tab_restore_session (string sessionId, string title, string model,
                                  string archived);

#endif // QT_CHAT_TAB_WIDGET_HPP
