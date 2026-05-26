
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
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpacerItem;
class QStackedWidget;
class QTimer;
class QVBoxLayout;
class QEvent;
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
  explicit ChatConversationPanel (const string& sessionId, QWidget* parent);

  // 被 Controller/Widget 调用的接口
  void enterConversationMode ();
  void focusInput ();
  tree readInputMessage () const;

  // 被 Controller 访问
  QPushButton*  sendButton () const { return sendButton_; }
  QLabel*       modelLabel () const { return modelLabel_; }
  const string& sessionId () const { return sessionId_; }
  bool          conversationMode () const { return conversationMode_; }

  // 静态工具
  static bool is_empty_document_body (tree body);
  static int  count_input_lines (tree body);

signals:
  void sendRequested (const string& sessionId);
  void inputHeightChanged ();

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;

private:
  void setup_ui ();
  void adjust_input_height ();

  string       sessionId_;
  bool         conversationMode_ = false;
  QLabel*      welcomeTitle_     = nullptr;
  QLabel*      modelLabel_       = nullptr;
  QWidget*     messageFrame_     = nullptr;
  QWidget*     inputEditorWidget_= nullptr;
  QPushButton* sendButton_       = nullptr;
  QSpacerItem* topSpacer_        = nullptr;
  widget       messageWidget_;
  widget       inputWidget;
  int          fixedFrameExtra_  = 0;
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
    QCheckBox*   selectCheckBox= nullptr;
    bool         isArchived    = false;
  };

  ChatSidebar (const QList<SessionDisplayInfo>& sessions,
               const string& activeSessionId, QWidget* parent= nullptr);

  // ---- 按场景调用的针对性方法（替代 refresh） ----
  void addItem (const string& sessionId, const string& displayTitle);
  void updateItemTitle (const string& sessionId, const string& displayTitle);
  void setActiveItem (const string& sessionId);
  void moveToArchive (const string& sessionId);
  void moveFromArchive (const string& sessionId);
  void applySearchFilter ();

  // ---- 其他公共方法 ----
  void          removeItem (const string& sessionId);
  void          enterMultiSelectMode (bool archived);
  void          exitMultiSelectMode ();
  const string& activeSessionId () const;

signals:
  void sessionClicked (const string& sessionId);
  void deleteRequested (const string& sessionId);
  void archiveRequested (const string& sessionId);
  void restoreRequested (const string& sessionId);
  void renameRequested (const string& sessionId, const string& newTitle);
  void newChatRequested ();
  void multiDeleteRequested (const QList<string>& sessionIds);
  void multiArchiveRequested (const QList<string>& sessionIds);

private:
  QMap<string, SidebarItem> items_;

  QLabel*                   conversationCountLabel_= nullptr;
  QWidget*                  conversationListWidget_= nullptr;
  QVBoxLayout*              conversationListLayout_= nullptr;
  QPushButton*              archiveHeaderButton_   = nullptr;
  QWidget*                  archiveListWidget_     = nullptr;
  QVBoxLayout*              archiveListLayout_     = nullptr;
  bool                      archiveCollapsed_      = true;
  QWidget*                  multiSelectBar_        = nullptr;
  QPushButton*              batchArchiveBtn_       = nullptr;
  QLineEdit*                searchEdit_            = nullptr;
  bool                      multiSelectMode_       = false;
  bool                      archiveSelectMode_     = false;
  QList<SessionDisplayInfo> sessionCache_;
  string                    activeSessionId_;

  SidebarItem   createItem (const string& sessionId);
  void          destroyItem (const string& sessionId);
  void          updateCountLabels ();
  QList<string> getCheckedSessionIds () const;
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
  QTChatTabWidget (const QList<SessionDisplayInfo>& sessions,
                   const string& activeSessionId, QWidget* parent= nullptr);
  ~QTChatTabWidget () override;

  // ---- 被 Controller 调用的方法（View 接口） ----

  ChatConversationPanel* createPanel (const string& sessionId);
  void                   activatePanel (ChatConversationPanel* panel);
  void                   removePanel (ChatConversationPanel* panel);

  // ---- 状态 ----
  void setParentTmWidget (qt_tm_widget_rep* tm) { parentTmWidget_= tm; }
  qt_tm_widget_rep* parentTmWidget () const { return parentTmWidget_; }

  // ---- 供 Controller 读取 ----
  ChatSidebar* sidebar () const { return sidebar_; }
  QPushButton* newChatButton () const { return newChatButton_; }
  QPushButton* floatingNewChatButton () const { return floatingNewChatBtn_; }
  QList<ChatConversationPanel*>& conversations () { return conversations_; }
  ChatConversationPanel*         activeConversation () const {
    return activeConversation_;
  }

signals:
  void cancelRequested (const string& sessionId);
  void newChatRequested ();

protected:
  void keyPressEvent (QKeyEvent* event) override;
  void keyReleaseEvent (QKeyEvent* event) override;
  bool eventFilter (QObject* watched, QEvent* event) override;

private:
  void setup_left_sidebar (QVBoxLayout*                     sidebarLayout,
                           const QList<SessionDisplayInfo>& sessions,
                           const string&                    activeSessionId);
  void setup_right_content (QHBoxLayout* mainLayout);
  void toggle_sidebar ();

  // ---- 子组件 ----
  ChatSidebar*    sidebar_             = nullptr;
  QWidget*        sidebarWidget_       = nullptr;
  QWidget*        contentWidget_       = nullptr;
  QPushButton*    collapseButton_      = nullptr;
  QPushButton*    floatingExpandBtn_   = nullptr;
  QPushButton*    floatingNewChatBtn_  = nullptr;
  QWidget*        floatingBtnContainer_= nullptr;
  QPushButton*    newChatButton_       = nullptr;
  QWidget*        sidebarNormalContent_= nullptr;
  QStackedWidget* conversationStack_   = nullptr;

  QList<ChatConversationPanel*> conversations_;
  ChatConversationPanel*        activeConversation_  = nullptr;
  bool                          sidebarCollapsed_    = false;
  int                           sidebarExpandedWidth_= 0;
  qt_tm_widget_rep*             parentTmWidget_      = nullptr;
};

#endif // QT_CHAT_TAB_WIDGET_HPP
