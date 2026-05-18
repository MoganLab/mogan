
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

class QHBoxLayout;
class QLabel;
class QPushButton;
class QSpacerItem;
class QStackedWidget;
class QString;
class QToolBar;
class QVBoxLayout;
class QEvent;

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
   * @brief 创建并激活一个以自动生成标题命名的新会话。
   */
  void create_new_conversation ();

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

public:
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

private:
  QWidget*        sidebarWidget_;                    ///< 左侧边栏容器。
  QWidget*        contentWidget_;                    ///< 右侧内容区容器。
  QLabel*         conversationCountLabel_;           ///< 显示会话数量的标签。
  QWidget*        conversationListWidget_;           ///< 承载会话列表的控件。
  QVBoxLayout*    conversationListLayout_;           ///< 会话按钮的布局。
  QPushButton*    collapseButton_;                   ///< 侧边栏内的收缩按钮。
  QPushButton*    newChatButton_;                    ///< 新建会话按钮。
  QWidget*        sidebarNormalContent_;             ///< 侧边栏展开时的内容容器。
  QWidget*        sidebarCollapsedBar_;              ///< 侧边栏收起时的窄条容器。
  QStackedWidget* conversationStack_;                ///< 会话页面的堆叠控件。
  QList<ChatConversationPanel*> conversations_;      ///< 所有会话面板的列表。
  ChatConversationPanel*        activeConversation_; ///< 当前激活的会话。
  int       nextConversationTitleId_; ///< 自动生成会话标题的 ID 计数器。
  bool      sidebarCollapsed_;        ///< 侧边栏当前是否处于收起状态。
  int       sidebarExpandedWidth_;    ///< 侧边栏展开时的宽度（像素）。
  QToolBar* chatMenuToolBar_;         ///< Chat Tab 的菜单工具栏。
  QToolBar* chatModeToolBar_;         ///< Chat Tab 的模式工具栏。
  QToolBar* chatFocusToolBar_;        ///< Chat Tab 的焦点工具栏。
};

#endif // QT_CHAT_TAB_WIDGET_HPP
