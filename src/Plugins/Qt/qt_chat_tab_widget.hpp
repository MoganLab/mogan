
/******************************************************************************
 * MODULE     : qt_chat_tab_widget.hpp
 * DESCRIPTION: Mogan STEM 的 LLM 聊天标签页控件（纯 View）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CHAT_TAB_WIDGET_HPP
#define QT_CHAT_TAB_WIDGET_HPP

#include "qt_chat_session.hpp"
#include <QList>
#include <QMap>
#include <QWidget>

#include "widget.hpp"

class QCheckBox;
class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSpacerItem;
class QStackedWidget;
class QTimer;
class QToolButton;
class QVBoxLayout;
class QEvent;
class QTMWidget;
class QTMStateToolButton;
class qt_tm_widget_rep;

/**
 * @brief 传递给侧边栏刷新的显示数据（值类型，由 Controller 准备）。
 */
struct SessionDisplayInfo {
  string sessionId;
  string displayTitle; ///< 去重后的标题（如 "hello (2)"）
  string model;
  bool   archived;
};

/**
 * @brief 单个会话内容页（QWidget 子类，只管右侧内容区）。
 *
 * 封装消息区、输入区、发送按钮，不持有任何 sidebar 相关字段。
 */
class ChatConversationPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief 构造会话内容面板。
   * @param sessionId 所属会话 ID
   * @param msgBufUrl 消息缓冲区 URL（外部注入）
   * @param inBufUrl  输入缓冲区 URL（外部注入）
   * @param parent    父控件
   */
  explicit ChatConversationPanel (const string& sessionId, const url& msgBufUrl,
                                  const url& inBufUrl, QWidget* parent);

  /**
   * @brief 进入对话模式（隐藏欢迎页，显示消息区域）。
   */
  void enterConversationMode ();

  /**
   * @brief 聚焦到输入区域。
   */
  void focusInput ();

  /**
   * @brief 读取输入区域的文档内容。
   * @return 输入内容的 tree 表示
   */
  tree readInputMessage () const;

  QToolButton*  sendButton () const { return sendButton_; }
  QToolButton*  thinkingButton () const { return thinkingButton_; }
  QToolButton*  searchButton () const { return searchButton_; }
  QLabel*       sessionTitle () const { return sessionTitle_; }
  const string& sessionId () const { return sessionId_; }
  bool          conversationMode () const { return conversationMode_; }

  /**
   * @brief 判断文档体是否为空（仅含一个空字符串的 DOCUMENT 节点）。
   * @param body 文档体 tree
   * @return 为空时返回 true
   */
  static bool is_empty_document_body (tree body);

  /**
   * @brief 计算输入文档的行数。
   * @param body 文档体 tree
   * @return 行数，非 DOCUMENT 类型返回 1
   */
  static int count_input_lines (tree body);

  /**
   * @brief 判断只读控件是否应拦截该事件。
   * @param watched 事件目标对象（需带有 chat_message_readonly 属性）
   * @param event   待判断的事件
   * @return 应拦截返回 true，放行返回 false
   */
  static bool should_block_readonly_event (QObject* watched, QEvent* event);

  /**
   * @brief 判断输入区当前按键是否应触发发送。
   * @param key 按键值
   * @param mods 修饰键状态
   * @param hasActiveCompletionPopup 是否存在待确认的补全/Tab cycle 弹窗
   * @return 应触发发送时返回 true
   */
  static bool should_send_on_keypress (int key, Qt::KeyboardModifiers mods,
                                       bool hasActiveCompletionPopup);

signals:
  void sendRequested (const string& sessionId);
  void thinkingToggled (const string& sessionId, bool enabled);
  void searchToggled (const string& sessionId, bool enabled);
  void inputHeightChanged ();
  void closeSidebarInDockModeRequested ();

protected:
  /// 事件过滤器：拦截 Enter 键触发发送
  bool eventFilter (QObject* watched, QEvent* event) override;
  /// 欢迎模式下按容器高度比例调整顶部偏移
  void resizeEvent (QResizeEvent* event) override;

public:
  /// 在当前事件处理完成后更新输入区高度，避免读取到旧排版结果
  void schedule_input_height_adjust ();

private:
  /// 构建面板 UI 布局
  void setup_ui ();
  /// 根据内容动态调整输入区高度
  void adjust_input_height ();

  string       sessionId_;                     ///< 所属会话 ID
  url          msgBufferUrl_;                  ///< 消息缓冲区 URL（外部注入）
  url          inputBufferUrl_;                ///< 输入缓冲区 URL（外部注入）
  bool         conversationMode_ = false;      ///< 是否已进入对话模式
  QLabel*      welcomeTitle_     = nullptr;    ///< 欢迎页标题
  QLabel*      sessionTitle_     = nullptr;    ///< 会话标题标签
  QWidget*     messageFrame_     = nullptr;    ///< 消息区域容器
  QWidget*     inputEditorWidget_= nullptr;    ///< 输入编辑器容器
  QTMWidget*   inputQTMWidget_   = nullptr;    ///< 输入区 QTMWidget
  QToolButton* sendButton_       = nullptr;    ///< 发送/停止按钮
  QToolButton* thinkingButton_   = nullptr;    ///< 推理模式开关
  QToolButton* searchButton_     = nullptr;    ///< 网络搜索开关
  QSpacerItem* topSpacer_        = nullptr;    ///< 欢迎页顶部弹性空间
  widget       messageWidget_;                 ///< 消息区 TeXmacs widget
  widget       inputWidget;                    ///< 输入区 TeXmacs widget
  int          fixedFrameExtra_           = 0; ///< 输入框额外高度（边框等）
  bool         inputHeightAdjustScheduled_= false; ///< 是否已有待执行的高度更新
};

/**
 * @brief 聊天侧边栏控件（纯 UI，自管理 items）。
 *
 * 根据 Controller 传入的 SessionDisplayInfo 数据，
 * 自行创建/更新/删除 sidebar item widgets。
 * 所有用户操作通过 signal 发出。
 */
class ChatSidebar : public QWidget {
  Q_OBJECT

public:
  /// 侧边栏项数据（内部使用）。
  struct SidebarItem {
    QWidget*     itemWidget    = nullptr;
    QPushButton* sidebarButton = nullptr;
    QPushButton* moreButton    = nullptr;
    QCheckBox*   selectCheckBox= nullptr;
    QLineEdit*   titleEdit     = nullptr;
    bool         isArchived    = false;
  };

  /**
   * @brief 构造侧边栏。
   * @param sessions        会话显示数据列表
   * @param activeSessionId 初始激活会话 ID（可为空）
   * @param parent          父控件
   */
  ChatSidebar (const QList<SessionDisplayInfo>& sessions,
               const string& activeSessionId, QWidget* parent= nullptr);

  // ---- 按场景调用的针对性方法（替代 refresh） ----

  /**
   * @brief 添加新的侧边栏项。
   * @param info 会话显示数据
   */
  void addItem (const SessionDisplayInfo& info);

  /**
   * @brief 更新指定会话的显示标题。
   * @param sessionId   目标会话 ID
   * @param displayTitle 新标题
   */
  void updateItemTitle (const string& sessionId, const string& displayTitle);

  /**
   * @brief 设置激活的侧边栏项（高亮显示）。
   * @param sessionId 要激活的会话 ID
   */
  void setActiveItem (const string& sessionId);

  /**
   * @brief 将会话项从活跃列表移到归档列表。
   * @param sessionId 目标会话 ID
   */
  void moveToArchive (const string& sessionId);

  /**
   * @brief 将会话项从归档列表移回活跃列表。
   * @param sessionId 目标会话 ID
   */
  void moveFromArchive (const string& sessionId);

  /**
   * @brief 根据搜索框文本过滤显示的会话项。
   */
  void applySearchFilter ();

  /**
   * @brief 开始内联编辑指定会话的标题。
   * @param sessionId 目标会话 ID
   */
  void beginEditTitle (const string& sessionId);

  // ---- 其他公共方法 ----

  /**
   * @brief 将指定会话项移到活跃列表顶部。
   * @param sessionId 目标会话 ID
   */
  void reorderItem (const string& sessionId);

  /**
   * @brief 移除指定的侧边栏项。
   * @param sessionId 要移除的会话 ID
   */
  void removeItem (const string& sessionId);

  /**
   * @brief 进入多选模式。
   * @param archived 是否在归档列表中多选
   */
  void enterMultiSelectMode (bool archived);

  /**
   * @brief 退出多选模式。
   */
  void exitMultiSelectMode ();

  /**
   * @brief 获取当前激活的会话 ID。
   * @return 激活的会话 ID，无激活项时返回空字符串
   */
  const string& activeSessionId () const;

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;
  void resizeEvent (QResizeEvent* event) override;

signals:
  void sessionClicked (const string& sessionId);
  void deleteRequested (const string& sessionId);
  void archiveRequested (const string& sessionId);
  void restoreRequested (const string& sessionId);
  void renameRequested (const string& sessionId, const string& newTitle);
  void newChatRequested ();
  void exportRequested (const string& sessionId);
  void multiDeleteRequested (const QList<string>& sessionIds);
  void multiArchiveRequested (const QList<string>& sessionIds);

private:
  QMap<string, SidebarItem> items_; ///< sessionId → SidebarItem 映射

  QLabel*      conversationCountLabel_= nullptr; ///< 活跃会话计数标签
  QWidget*     conversationListWidget_= nullptr; ///< 活跃会话列表容器
  QVBoxLayout* conversationListLayout_= nullptr; ///< 活跃会话列表布局
  QFrame*      archiveSeparator_      = nullptr; ///< 归档区分割线
  QPushButton* archiveHeaderButton_   = nullptr; ///< 归档区折叠按钮
  QScrollArea* archiveListWidget_     = nullptr; ///< 归档会话列表滚动容器
  QVBoxLayout* archiveListLayout_     = nullptr; ///< 归档会话列表布局
  bool         archiveCollapsed_      = true;    ///< 归档区是否折叠
  QWidget*     multiSelectBar_        = nullptr; ///< 多选操作栏
  QPushButton* batchArchiveBtn_       = nullptr; ///< 批量归档按钮
  QLineEdit*   searchEdit_            = nullptr; ///< 搜索框
  bool         multiSelectMode_       = false;   ///< 是否处于多选模式
  bool         archiveSelectMode_     = false;   ///< 是否在归档区多选
  string       activeSessionId_;                 ///< 当前激活的会话 ID

  SidebarItem createItem (const string& sessionId); ///< 创建单个侧边栏项 widget
  void destroyItem (const string& sessionId);       ///< 销毁单个侧边栏项 widget
  void updateCountLabels ();                        ///< 更新会话数/归档数标签
  void updateArchiveListVisibility ();       ///< 调整归档列表可见性与高度
  int  computeArchiveContentHeight () const; ///< 计算归档区内容总高度
  void endEditTitle (const string& sessionId, bool accept); ///< 结束内联编辑
  QList<string>
  getCheckedSessionIds () const; ///< 获取多选模式下已勾选的会话 ID 列表
};

/**
 * @brief Mogan STEM 的 LLM 聊天标签页控件（纯 View，整体协调）。
 *
 * 只负责 UI 展示和子组件的协调。
 * 所有用户操作通过 signal 发出，由 ChatController 连接处理。
 * View 不知道 Controller 的存在。
 */
class QTChatTabWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief 构造聊天标签页控件。
   * @param sessions        会话显示数据列表
   * @param activeSessionId 初始激活会话 ID
   * @param parent          父控件
   */
  QTChatTabWidget (const QList<SessionDisplayInfo>& sessions,
                   const string& activeSessionId, QWidget* parent= nullptr);
  ~QTChatTabWidget () override;

  // ---- 被 Controller 调用的方法（View 接口） ----

  /**
   * @brief 创建新的会话内容面板并加入堆栈。
   * @param sessionId 关联的会话 ID
   * @return 创建的面板指针
   */
  ChatConversationPanel* createPanel (const string& sessionId);

  /**
   * @brief 激活指定面板（切换堆栈当前页）。
   * @param panel 要激活的面板
   */
  void activatePanel (ChatConversationPanel* panel);

  /**
   * @brief 从堆栈中移除并销毁指定面板。
   * @param panel 要移除的面板
   */
  void removePanel (ChatConversationPanel* panel);

  // ---- 状态 ----
  /// 设置关联的 TeXmacs widget（用于 Scheme 交互）
  void setParentTmWidget (qt_tm_widget_rep* tm) { parentTmWidget_= tm; }
  /// 获取关联的 TeXmacs widget
  qt_tm_widget_rep* parentTmWidget () const { return parentTmWidget_; }

  // ---- 供 Controller 读取 ----
  ChatSidebar* sidebar () const { return sidebar_; }
  QPushButton* newChatButton () const { return newChatButton_; }
  QPushButton* floatingNewChatButton () const { return floatingNewChatBtn_; }
  QPushButton* closeSidebarButton () const { return closeSidebarBtn_; }
  QList<ChatConversationPanel*>& conversations () { return conversations_; }
  ChatConversationPanel*         activeConversation () const {
    return activeConversation_;
  }
  void setSidebarCollapsed (bool collapsed);
  bool isSidebarCollapsed () const { return sidebarCollapsed_; }

  /**
   * @brief 获取全局记忆的侧边栏折叠状态。
   */
  static bool globalSidebarCollapsed ();
  /**
   * @brief 设置全局记忆的侧边栏折叠状态。
   */
  static void setGlobalSidebarCollapsed (bool collapsed);
  bool        isSidebarWidgetVisible () const {
    return sidebarWidget_ != nullptr && sidebarWidget_->isVisible ();
  }
  bool isFloatingContainerVisible () const {
    return floatingBtnContainer_ != nullptr &&
           floatingBtnContainer_->isVisible ();
  }
  /**
   * @brief 直接设置内部侧边栏显隐（dock 模式使用，不触发浮动按钮）。
   */
  void setSidebarVisible (bool visible);
  void setCloseSidebarButtonVisible (bool visible);

  // ---- 供外部组件访问 ----
  QWidget* contentWidget () const { return contentWidget_; }

signals:
  void cancelRequested (const string& sessionId);
  void newChatRequested ();
  void closeSidebarRequested ();

protected:
  /// 键盘事件处理（Ctrl+N 新建会话等）
  void keyPressEvent (QKeyEvent* event) override;
  void keyReleaseEvent (QKeyEvent* event) override;
  /// 事件过滤器
  bool eventFilter (QObject* watched, QEvent* event) override;

private:
  /// 构建左侧侧边栏布局
  void setup_left_sidebar (QVBoxLayout*                     sidebarLayout,
                           const QList<SessionDisplayInfo>& sessions,
                           const string&                    activeSessionId);
  /// 构建右侧内容区布局
  void setup_right_content (QHBoxLayout* mainLayout);
  /// 切换侧边栏展开/折叠状态
  void toggle_sidebar ();

  // ---- 子组件 ----
  ChatSidebar*    sidebar_             = nullptr; ///< 侧边栏控件
  QWidget*        sidebarWidget_       = nullptr; ///< 侧边栏容器
  QWidget*        contentWidget_       = nullptr; ///< 右侧内容区容器
  QPushButton*    collapseButton_      = nullptr; ///< 折叠按钮
  QPushButton*    floatingExpandBtn_   = nullptr; ///< 浮动展开按钮
  QPushButton*    floatingNewChatBtn_  = nullptr; ///< 浮动新建按钮
  QWidget*        floatingBtnContainer_= nullptr; ///< 浮动按钮容器
  QPushButton*    newChatButton_       = nullptr; ///< 侧边栏新建按钮
  QPushButton*    newChatSidebarBtn_   = nullptr; ///< 新建按钮（dock 模式）
  QPushButton*    closeSidebarBtn_     = nullptr; ///< 对话区域关闭侧边栏按钮
  QWidget*        sidebarNormalContent_= nullptr; ///< 侧边栏常规内容区
  QStackedWidget* conversationStack_   = nullptr; ///< 会话面板堆栈

  QList<ChatConversationPanel*> conversations_;          ///< 所有会话面板
  ChatConversationPanel* activeConversation_  = nullptr; ///< 当前激活的面板
  bool                   sidebarCollapsed_    = false;   ///< 侧边栏是否折叠
  int                    sidebarExpandedWidth_= 0;       ///< 侧边栏展开时宽度
  qt_tm_widget_rep*      parentTmWidget_= nullptr; ///< 关联的 TeXmacs widget

  static bool globalSidebarCollapsed_; ///< 全局记忆的侧边栏折叠状态
};

#endif // QT_CHAT_TAB_WIDGET_HPP
