
/******************************************************************************
 * MODULE     : qt_chat_controller.cpp
 * DESCRIPTION: Chat Tab 的核心管理类（逻辑 + Scheme 交互）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_chat_controller.hpp"
#include "qt_chat_tab_widget.hpp"

#include "new_buffer.hpp"
#include "s7_tm.hpp"
#include "scheme.hpp"

#include <QApplication>
#include <QLabel>
#include <QPushButton>

#include "qt_utilities.hpp"

using namespace moebius;

/******************************************************************************
 * ChatController 实现
 ******************************************************************************/

ChatController::ChatController (QObject* parent) : QObject (parent) {}

ChatController::~ChatController () {}

QWidget*
ChatController::createView (QWidget* parent, qt_tm_widget_rep* tm) {
  view_= new QTChatTabWidget (parent);
  view_->setParentTmWidget (tm);

  // 连接 Sidebar 信号
  ChatSidebar* sb= view_->sidebar ();
  if (sb) {
    connect (sb, &ChatSidebar::sessionClicked, this,
             &ChatController::onSessionClicked);
    connect (sb, &ChatSidebar::deleteRequested, this,
             [this] (const string& sid) {
               QList<string> ids;
               ids.append (sid);
               onDeleteRequested (ids);
             });
    connect (sb, &ChatSidebar::archiveRequested, this,
             [this] (const string& sid) {
               QList<string> ids;
               ids.append (sid);
               onArchiveRequested (ids);
             });
    connect (sb, &ChatSidebar::restoreRequested, this,
             &ChatController::onRestoreRequested);
    connect (sb, &ChatSidebar::newChatRequested, this,
             &ChatController::onNewChatRequested);
    connect (sb, &ChatSidebar::renameRequested, this,
             [this] (const string& sid, const string&) {
               // TODO: 重命名弹框
             });
    connect (sb, &ChatSidebar::multiDeleteRequested, this,
             &ChatController::onDeleteRequested);
    connect (sb, &ChatSidebar::multiArchiveRequested, this,
             &ChatController::onArchiveRequested);
  }

  // 连接 View 自身信号（不再有 sendRequested）
  connect (view_, &QTChatTabWidget::cancelRequested, this,
           &ChatController::onCancelRequested);
  connect (view_, &QTChatTabWidget::newChatRequested, this,
           &ChatController::onNewChatRequested);

  // 连接新建按钮
  if (view_->newChatButton ()) {
    connect (view_->newChatButton (), &QPushButton::clicked, this,
             &ChatController::onNewChatRequested);
  }
  if (view_->floatingNewChatButton ()) {
    connect (view_->floatingNewChatButton (), &QPushButton::clicked, this,
             &ChatController::onNewChatRequested);
  }

  // 延迟加载：初始化完成后加载会话
  loadAllSessions ();

  return view_;
}

ChatSessionManager&
ChatController::sessionManager () {
  return sessionManager_;
}

/**
 * @brief 启动时加载所有持久化会话。
 *
 * Scheme 端 chat-persist-load-all 会逐个调用 qt-chat-tab-restore-session
 * 来恢复元数据（不创建面板）。之后再激活第一个非归档会话。
 */
void
ChatController::loadAllSessions () {
  cout << "[chat-persist] ChatController::loadAllSessions started" << LF;
  call ("chat-persist-load-all");
  cout << "[chat-persist] ChatController::loadAllSessions: restored "
       << sessionManager_.getAllSessionIds ().size () << " session metadatas"
       << LF;

  // 激活第一个非归档会话
  auto allIds= sessionManager_.getAllSessionIds ();
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (s && !s->archived) {
      activateSession (sid);
      return;
    }
  }
  // 全部归档或无会话，确保有一个空白新对话
  ensureNewConversation ();

  // 恢复 Scheme 层的全局当前模型
  if (!allIds.empty ()) {
    string       lastSid= allIds.back ();
    ChatSession* s      = sessionManager_.getSession (lastSid);
    if (s && !is_empty (s->model)) {
      call ("chat-tab-session-select-model", s->model);
    }
  }
}

void
ChatController::onSessionClicked (const string& sessionId) {
  activateSession (sessionId);
}

void
ChatController::onSendRequested (const string& sessionId) {
  if (!view_) return;
  ChatSession* session= sessionManager_.getSession (sessionId);
  if (!session || !session->panel) return;

  ChatConversationPanel* panel=
      static_cast<ChatConversationPanel*> (session->panel);
  tree inputBody= panel->readInputMessage ();
  if (ChatConversationPanel::is_empty_document_body (inputBody)) return;

  // 首次发送：通过 Scheme 自动提取标题
  if (is_empty (session->title)) {
    string extracted=
        as_string (call ("chat-persist-extract-title", sessionId, "20"));
    sessionManager_.setTitle (sessionId, extracted);
  }

  if (!as_bool (call ("chat-tab-send", sessionId))) return;

  sessionManager_.setState (sessionId, ChatState::Generating);
  panel->enterConversationMode ();
  refreshSidebar (sessionId);
  panel->focusInput ();
  saveOneSession (sessionId);
}

void
ChatController::onCancelRequested (const string& sessionId) {
  call ("chat-tab-cancel", sessionId);
  sessionManager_.setState (sessionId, ChatState::Idle);
}

void
ChatController::onDeleteRequested (const QList<string>& sessionIds) {
  if (!view_) return;

  for (const string& sid : sessionIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s || !s->panel) continue;

    ChatConversationPanel* panel=
        static_cast<ChatConversationPanel*> (s->panel);

    // 1. 从持久化数据删除
    call ("chat-persist-delete-one", sid);
    // 2. 清理 Scheme 会话状态
    call ("chat-tab-session-destroy", sid);
    // 3. 从 SessionManager 移除
    sessionManager_.removeSession (sid);
    // 4. 从 View 移除面板
    view_->removePanel (panel);
  }

  // 激活下一个可用会话
  auto   allIds= sessionManager_.getAllSessionIds ();
  string nextSid;
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (s && !s->archived) {
      nextSid= sid;
      break;
    }
  }

  if (!is_empty (nextSid)) {
    activateSession (nextSid);
  }
  else if (!allIds.empty ()) {
    activateSession (allIds.front ());
  }
  else {
    ensureNewConversation ();
  }

  // 确保所有 buffer 标记为已保存
  for (const string& sid : allIds) {
    call ("buffer-pretend-saved", ChatSessionManager::messageBufferUrl (sid));
    call ("buffer-pretend-saved", ChatSessionManager::inputBufferUrl (sid));
  }
  call ("buffer-pretend-saved", url ("tmfs://chat-tab"));
}

void
ChatController::onArchiveRequested (const QList<string>& sessionIds) {
  for (const string& sid : sessionIds) {
    sessionManager_.archiveSession (sid);
    saveOneSession (sid);
  }
  refreshSidebar ("");
}

void
ChatController::onRestoreRequested (const string& sessionId) {
  sessionManager_.restoreSession (sessionId);
  saveOneSession (sessionId);
  refreshSidebar (sessionId);
}

void
ChatController::onNewChatRequested () {
  ensureNewConversation ();
}

void
ChatController::onRenameRequested (const string& sessionId,
                                   const string& newTitle) {
  sessionManager_.setTitle (sessionId, newTitle);
  refreshSidebar (sessionId);
}

void
ChatController::notifyStateChanged (const string& sessionId,
                                    const string& stateStr) {
  ChatSession* session= sessionManager_.getSession (sessionId);
  if (!session) return;

  ChatState newState=
      (stateStr == "generating") ? ChatState::Generating : ChatState::Idle;
  sessionManager_.setState (sessionId, newState);

  if (!session->panel || !view_) return;
  ChatConversationPanel* panel=
      static_cast<ChatConversationPanel*> (session->panel);
  QPushButton* btn= panel->sendButton ();
  if (!btn) return;

  if (newState == ChatState::Generating) {
    btn->setToolTip ("Stop");
    disconnect (btn, nullptr, this, nullptr);
    connect (btn, &QPushButton::clicked, this,
             [this, sessionId] () { onCancelRequested (sessionId); });
  }
  else {
    btn->setToolTip ("Send");
    disconnect (btn, nullptr, this, nullptr);
    connect (btn, &QPushButton::clicked, this,
             [this, sessionId] () { onSendRequested (sessionId); });
    saveOneSession (sessionId);
  }
}

void
ChatController::restoreSessionMeta (const string& sessionId,
                                    const string& title, const string& model,
                                    bool archived) {
  // 注册 Scheme 侧会话状态
  call ("chat-persist-register-session", sessionId, model);

  // 插入元数据，不创建面板
  ChatSession session;
  session.sessionId= sessionId;
  session.title    = title;
  session.model    = model;
  session.state    = ChatState::Idle;
  session.archived = archived;
  session.panel    = nullptr;
  sessionManager_.insertSession (session);
}

/**
 * @brief 激活指定会话：按需创建面板，按需加载内容。
 */
void
ChatController::activateSession (const string& sessionId) {
  if (!view_) return;

  ChatConversationPanel* panel= getOrCreatePanel (sessionId);
  if (!panel) return;

  // 按需加载消息内容
  if (!panel->conversationMode ()) {
    loadSessionContent (panel);
  }

  view_->activatePanel (panel);
}

/**
 * @brief 按需加载会话的消息内容到面板。
 *
 * 调用 Scheme 的 chat-persist-load-session-content 加载 message.tmu。
 */
void
ChatController::loadSessionContent (ChatConversationPanel* panel) {
  if (!panel) return;

  ChatSession* s= sessionManager_.getSession (panel->sessionId ());
  if (!s) return;

  // 只在非归档会话且内容未加载时才加载
  if (s->archived) return;

  call ("chat-persist-load-session-content", panel->sessionId ());

  // 检查消息 buffer 是否非空，若非空则进入会话模式
  tree msgBody= get_buffer_body (
      ChatSessionManager::messageBufferUrl (panel->sessionId ()));
  if (!ChatConversationPanel::is_empty_document_body (msgBody)) {
    panel->enterConversationMode ();
  }

  // 同步模型标签
  if (panel->modelLabel () && s) {
    panel->modelLabel ()->setText (to_qstring (s->model));
  }
}

void
ChatController::saveOneSession (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s) return;
  call ("chat-persist-save-one", sessionId, s->title, s->model,
        s->archived ? string ("true") : string ("false"));
}

void
ChatController::ensureNewConversation () {
  if (!view_) return;
  string currentModel=
      as_string (call ("chat-tab-session-select-model", string ("")));

  // 检查是否已有空白的新会话且模型一致
  auto allIds= sessionManager_.getAllSessionIds ();
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (s && !s->archived && !s->panel && is_empty (s->title)) {
      // 空白 session 无面板
      if (s->model == currentModel) {
        activateSession (sid);
        return;
      }
    }
    // 有面板但未进入会话模式
    if (s && !s->archived && s->panel) {
      ChatConversationPanel* p= static_cast<ChatConversationPanel*> (s->panel);
      if (p && !p->conversationMode () && s->model == currentModel) {
        view_->activatePanel (p);
        return;
      }
    }
  }

  // 新建会话
  string                 sid  = sessionManager_.createSession ();
  ChatConversationPanel* panel= view_->createPanel (sid);
  if (!panel) return;

  sessionManager_.setPanel (sid, panel);
  sessionManager_.setModel (sid, currentModel);

  if (panel->modelLabel ()) {
    panel->modelLabel ()->setText (to_qstring (currentModel));
  }

  // 连接 Panel 的信号
  connect (panel, &ChatConversationPanel::sendRequested, this,
           &ChatController::onSendRequested);

  view_->activatePanel (panel);

  // 标记 buffer 为已保存
  call ("buffer-pretend-saved", ChatSessionManager::messageBufferUrl (sid));
  call ("buffer-pretend-saved", ChatSessionManager::inputBufferUrl (sid));

  saveOneSession (sid);
}

/**
 * @brief 获取或按需创建面板。
 *
 * 如果会话无面板（延迟加载场景），则调用 view_->createPanel 创建。
 */
ChatConversationPanel*
ChatController::getOrCreatePanel (const string& sessionId) {
  if (!view_) return nullptr;

  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s) return nullptr;

  if (s->panel) return static_cast<ChatConversationPanel*> (s->panel);

  // 按需创建面板
  ChatConversationPanel* panel= view_->createPanel (sessionId);
  if (!panel) return nullptr;

  sessionManager_.setPanel (sessionId, panel);

  // 连接 Panel 的信号
  connect (panel, &ChatConversationPanel::sendRequested, this,
           &ChatController::onSendRequested);

  return panel;
}

/******************************************************************************
 * ChatController::refreshSidebar — 构建显示数据并刷新侧边栏
 ******************************************************************************/

void
ChatController::refreshSidebar (const string& activeSessionId) {
  if (!view_ || !view_->sidebar ()) return;

  QList<SessionDisplayInfo> infos;

  // 标题去重计数
  QMap<QString, int> titleCounts;
  auto               allIds= sessionManager_.getAllSessionIds ();
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (s && !is_empty (s->title)) titleCounts[to_qstring (s->title)]++;
  }

  QMap<QString, int> titleSeq;
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s) continue;

    SessionDisplayInfo info;
    info.sessionId= s->sessionId;
    info.model    = s->model;
    info.archived = s->archived;

    if (!is_empty (s->title)) {
      QString qtTitle= to_qstring (s->title);
      if (titleCounts[qtTitle] > 1) {
        int seq          = ++titleSeq[qtTitle];
        info.displayTitle= from_qstring (qtTitle + QString (" (%1)").arg (seq));
      }
      else {
        info.displayTitle= s->title;
      }
    }
    else {
      info.displayTitle= "新会话";
    }

    infos.append (info);
  }

  view_->sidebar ()->refresh (infos, activeSessionId);
}

/******************************************************************************
 * 自由函数回调（Scheme→C++）
 ******************************************************************************/

static ChatController* g_chat_controller= nullptr;

ChatController*
get_chat_controller () {
  if (!g_chat_controller) {
    g_chat_controller= new ChatController ();
  }
  return g_chat_controller;
}

void
qt_chat_tab_set_state (string sessionId, string stateStr) {
  get_chat_controller ()->notifyStateChanged (sessionId, stateStr);
}

void
qt_chat_tab_restore_session (string sessionId, string title, string model,
                             string archived) {
  bool isArchived= (archived == "true");
  get_chat_controller ()->restoreSessionMeta (sessionId, title, model,
                                              isArchived);
}

void
qt_chat_tab_load_sessions () {
  cout << "[chat-persist] qt_chat_tab_load_sessions called" << LF;
  get_chat_controller ()->loadAllSessions ();
}
