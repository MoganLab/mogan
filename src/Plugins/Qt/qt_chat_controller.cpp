
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
#include "qt_floating_search_bar.hpp"
#include "qt_floating_toast.hpp"
#include "qt_utilities.hpp"

#include "new_buffer.hpp"
#include "s7_tm.hpp"
#include "scheme.hpp"

#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <cinttypes>
#include <cstdio>

using namespace moebius;

/******************************************************************************
 * ChatController 实现
 ******************************************************************************/

static ChatController* g_chat_controller= nullptr;

ChatController::ChatController (QObject* parent) : QObject (parent) {}

ChatController::~ChatController () {
  view_            = nullptr;
  g_chat_controller= nullptr;
}

void
ChatController::destroyView () {
  view_= nullptr;
}

QWidget*
ChatController::createView (QWidget* parent, qt_tm_widget_rep* tm) {
  // 1. Load session metadata
  call ("chat-persist-load-all");
  cout << "[chat-persist] ChatController: restored "
       << sessionManager_.sessionCount () << " session metadatas" << LF;

  // 2. 构建显示数据 + 确定初始激活会话
  QList<SessionDisplayInfo> infos= buildDisplayInfos ();
  string                    initialId;
  if (firstOpen_) {
    // 首次打开：切换到新会话（触发 ensureNewConversation）
    initialId = "";
    firstOpen_= false;
  }
  else {
    initialId= sessionManager_.firstActiveSessionId ();
  }

  // 3. 创建 View，Sidebar 构造时就有数据
  view_= new QTChatTabWidget (infos, initialId, parent);
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
    connect (sb, &ChatSidebar::exportRequested, this,
             &ChatController::onExportRequested);
    connect (sb, &ChatSidebar::newChatRequested, this,
             &ChatController::onNewChatRequested);
    connect (sb, &ChatSidebar::renameRequested, this,
             [this, sb] (const string& sid, const string& newTitle) {
               if (is_empty (newTitle)) sb->beginEditTitle (sid);
               else onRenameRequested (sid, newTitle);
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

  // 4. 激活初始会话（按需创建 Panel）
  if (!is_empty (initialId)) {
    activateSession (initialId);
  }
  else {
    ensureNewConversation ();
  }

  // 5. 恢复当前模型（使用激活的会话）
  if (!is_empty (initialId)) {
    ChatSession* s= sessionManager_.getSession (initialId);
    if (s && !is_empty (s->model)) {
      currentModel_= s->model;
    }
  }

  // 6. 注册浮动搜索栏的 parent provider
  qt_floating_search_set_parent_provider ([this] () -> QWidget* {
    if (!view_) return nullptr;
    return view_->contentWidget ();
  });

  return view_;
}

ChatSessionManager&
ChatController::sessionManager () {
  return sessionManager_;
}

void
ChatController::onSessionClicked (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (s && !s->archived) {
    activateSession (sessionId);
  }
  else {
    // 归档会话不可激活，刷新当前激活项以恢复视觉状态
    string cur= view_->sidebar ()->activeSessionId ();
    view_->sidebar ()->setActiveItem (cur);
  }
}

void
ChatController::onSendRequested (const string& sessionId) {
  if (!view_) return;
  ChatSession* session= sessionManager_.getSession (sessionId);
  if (!session || !session->panel) return;
  if (session->state == ChatState::Generating) return;

  ChatConversationPanel* panel=
      static_cast<ChatConversationPanel*> (session->panel);
  tree inputBody= panel->readInputMessage ();
  if (ChatConversationPanel::is_empty_document_body (inputBody)) return;

  // 包含图片时提示不支持，不发送
  if (as_bool (call ("chat-tab-tree-has-image?", inputBody))) {
    QtFloatingToast::showToast (
        view_, qt_translate ("Images are not supported in AI chat"), 3000,
        QtFloatingToast::Warning);
    return;
  }

  // 首次发送时注册 session 到持久化层 + 加入 sidebar
  registerSession (sessionId);

  // 首次发送：生成标题
  if (is_empty (session->title)) {
    sessionManager_.generateTitleFromContent (sessionId);

    string displayTitle= getSessionDisplayTitle (sessionId);
    view_->sidebar ()->updateItemTitle (sessionId, displayTitle);
    view_->sidebar ()->setActiveItem (sessionId);
    // 更新会话标题标签
    if (session->panel) {
      ChatConversationPanel* p=
          static_cast<ChatConversationPanel*> (session->panel);
      if (p->sessionTitle ()) {
        p->sessionTitle ()->setText (to_qstring (session->title));
        p->sessionTitle ()->show ();
      }
    }
  }

  if (!as_bool (
          call ("chat-tab-send", sessionId, session->model,
                session->thinking ? string ("enabled") : string ("disabled"),
                session->search ? string ("enabled") : string ("disabled"))))
    return;

  sessionManager_.setState (sessionId, ChatState::Generating);
  sessionManager_.touchSession (sessionId);
  panel->enterConversationMode ();

  panel->focusInput ();
  exportBuffer (sessionId);
  updateManifest (sessionId);
  view_->sidebar ()->reorderItem (sessionId);
}

void
ChatController::onCancelRequested (const string& sessionId) {
  call ("chat-tab-cancel", sessionId);
}

void
ChatController::onThinkingToggled (const string& sessionId, bool enabled) {
  sessionManager_.setThinking (sessionId, enabled);
  updateManifest (sessionId);
}

void
ChatController::onSearchToggled (const string& sessionId, bool enabled) {
  sessionManager_.setSearch (sessionId, enabled);
  updateManifest (sessionId);
}

void
ChatController::onDeleteRequested (const QList<string>& sessionIds) {
  if (!view_) return;

  for (const string& sid : sessionIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s) continue;

    ChatConversationPanel* panel=
        static_cast<ChatConversationPanel*> (s->panel);

    call ("chat-tab-cancel", sid);
    call ("chat-persist-delete-one", sid);
    sessionManager_.removeSession (sid);

    if (panel) {
      view_->removePanel (panel);
    }
    else if (view_->sidebar ()) {
      // 无 panel 的 session（延迟加载，从未激活），仍需清理侧边栏项
      view_->sidebar ()->removeItem (sid);
    }
  }

  // 查找第一个非归档会话作为下一个激活项
  string nextSid= sessionManager_.firstActiveSessionId ();

  if (!is_empty (nextSid)) {
    activateSession (nextSid);
  }
  else {
    ensureNewConversation ();
  }

  // 确保所有剩余 buffer 标记为已保存，避免关闭时弹窗
  auto allIds= sessionManager_.getAllSessionIds ();
  for (const string& sid : allIds) {
    call ("buffer-pretend-saved", ChatSessionManager::messageBufferUrl (sid));
    call ("buffer-pretend-saved", ChatSessionManager::inputBufferUrl (sid));
  }
  call ("buffer-pretend-saved", url ("tmfs://chat-tab"));

  // 多选删除后退出多选模式
  view_->sidebar ()->exitMultiSelectMode ();
}

void
ChatController::onArchiveRequested (const QList<string>& sessionIds) {
  // 在 moveToArchive 前保存当前激活会话 ID（moveToArchive 会清空它）
  string cur           = view_->sidebar ()->activeSessionId ();
  bool   archivedActive= false;
  for (const string& sid : sessionIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s || is_empty (s->title)) continue; // 空白会话跳过归档
    if (sid == cur) archivedActive= true;
    sessionManager_.archiveSession (sid);
    updateManifest (sid);
    view_->sidebar ()->moveToArchive (sid);
  }

  string nextSid;

  if (archivedActive) {
    nextSid= sessionManager_.firstActiveSessionId ();
  }

  if (!is_empty (nextSid)) {
    activateSession (nextSid);
  }
  else {
    ensureNewConversation ();
  }

  // 多选归档后退出多选模式
  view_->sidebar ()->exitMultiSelectMode ();
}

void
ChatController::onRestoreRequested (const string& sessionId) {
  sessionManager_.restoreSession (sessionId);
  updateManifest (sessionId);
  view_->sidebar ()->moveFromArchive (sessionId);
  activateSession (sessionId);
}

QString
ChatController::sanitizeExportFileName (const QString& rawName) {
  QString sanitized;
  for (int i= 0; i < rawName.size (); ++i) {
    QChar c= rawName[i];
    if (c == ' ') sanitized+= '_';
    else if (c != '\\' && c != '/' && c != ':' && c != '*' && c != '?' &&
             c != '"' && c != '<' && c != '>' && c != '|')
      sanitized+= c;
  }
  if (sanitized.isEmpty ()) sanitized= "export";
  return sanitized;
}

void
ChatController::onExportRequested (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s) return;

  QString docsDir=
      QStandardPaths::writableLocation (QStandardPaths::DocumentsLocation);
  if (docsDir.isEmpty ()) {
    docsDir= QStandardPaths::writableLocation (QStandardPaths::HomeLocation);
  }
  docsDir= QDir (docsDir).filePath ("LiiiSTEM");
  if (!QDir (docsDir).exists ()) QDir ().mkpath (docsDir);

  QString rawName=
      is_empty (s->title) ? QString ("export") : to_qstring (s->title);
  QString sanitized  = sanitizeExportFileName (rawName);
  QString defaultName= sanitized + ".tmu";
  QString defaultPath= QDir (docsDir).filePath (defaultName);
  QString targetPath = QFileDialog::getSaveFileName (
      nullptr, qt_translate ("Export Conversation"), defaultPath,
      qt_translate ("TMU Files (*.tmu)"));
  if (targetPath.isEmpty ()) return;

  call ("chat-persist-export-session-to", sessionId,
        from_qstring_utf8 (targetPath));
}

void
ChatController::onNewChatRequested () {
  ensureNewConversation ();
}

void
ChatController::onRenameRequested (const string& sessionId,
                                   const string& newTitle) {
  string curActiveId= view_->sidebar ()->activeSessionId ();
  sessionManager_.setTitle (sessionId, newTitle);
  string displayTitle= getSessionDisplayTitle (sessionId);
  view_->sidebar ()->updateItemTitle (sessionId, displayTitle);
  view_->sidebar ()->setActiveItem (curActiveId);

  ChatSession* s= sessionManager_.getSession (sessionId);
  if (s && s->panel) {
    ChatConversationPanel* panel=
        static_cast<ChatConversationPanel*> (s->panel);
    if (panel->sessionTitle ()) {
      panel->sessionTitle ()->setText (to_qstring (newTitle));
      panel->sessionTitle ()->show ();
    }
  }

  updateManifest (sessionId);
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
  QToolButton* btn= panel->sendButton ();
  if (!btn) return;

  // Generating 状态：切换按钮为 Stop
  if (newState == ChatState::Generating) {
    btn->setProperty ("generating", true);
    btn->style ()->unpolish (btn);
    btn->style ()->polish (btn);
    btn->setToolTip ("Stop");
    disconnect (session->sendBtnConnection);
    session->sendBtnConnection=
        connect (btn, &QToolButton::clicked, this,
                 [this, sessionId] () { onCancelRequested (sessionId); });
  }
  // Idle 状态：切换按钮为 Send，并保存会话
  else {
    btn->setProperty ("generating", false);
    btn->style ()->unpolish (btn);
    btn->style ()->polish (btn);
    btn->setToolTip ("Send");
    disconnect (session->sendBtnConnection);
    session->sendBtnConnection=
        connect (btn, &QToolButton::clicked, this,
                 [this, sessionId] () { onSendRequested (sessionId); });
    exportBuffer (sessionId);
    updateManifest (sessionId);
    panel->focusInput ();
  }
}

void
ChatController::restoreSessionMeta (const ChatSession& session) {
  sessionManager_.insertSession (session);
}

/**
 * @brief 激活指定会话：按需创建面板，按需加载内容。
 */
void
ChatController::activateSession (const string& sessionId) {
  if (!view_) return;

  // 切换 session 时隐藏悬浮搜索栏
  qt_floating_search_bar_show (view_->contentWidget (), false);

  ChatConversationPanel* panel= getOrCreatePanel (sessionId);
  if (!panel) return;

  // 按需加载消息内容
  if (!panel->conversationMode ()) {
    loadSessionContent (panel);
  }
  else {
    // 已加载过内容，滚动消息区域到底部
    call ("chat-scroll-message-to-end", sessionId);
  }

  view_->activatePanel (panel);
  view_->sidebar ()->setActiveItem (sessionId);
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

  call ("chat-persist-load-session-content", panel->sessionId (),
        object (s->defaultExpandCount));

  // 检查消息 buffer 是否非空，若非空则进入会话模式并滚动到底部
  tree msgBody= get_buffer_body (
      ChatSessionManager::messageBufferUrl (panel->sessionId ()));
  if (!ChatConversationPanel::is_empty_document_body (msgBody)) {
    panel->enterConversationMode ();
    QTimer::singleShot (3000, this, [this, sid= panel->sessionId ()] () {
      if (!sessionManager_.getSession (sid)) return;
      call ("chat-scroll-message-to-end", sid);
    });
  }

  // 同步会话标题标签
  if (panel->sessionTitle ()) {
    if (is_empty (s->title)) {
      panel->sessionTitle ()->hide ();
    }
    else {
      panel->sessionTitle ()->setText (to_qstring (s->title));
      panel->sessionTitle ()->show ();
    }
  }
}

void
ChatController::exportBuffer (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s || !s->registered) return;
  call ("chat-persist-export-buffer", sessionId);
}

void
ChatController::updateManifest (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s || !s->registered) return;
  char createdAtBuf[32];
  std::snprintf (createdAtBuf, sizeof (createdAtBuf), "%" PRId64,
                 (int64_t) s->createdAt);
  char updateAtBuf[32];
  std::snprintf (updateAtBuf, sizeof (updateAtBuf), "%" PRId64,
                 (int64_t) s->updateAt);
  array<object> args;
  args << object (sessionId) << object (s->title) << object (s->model)
       << object (s->archived ? string ("true") : string ("false"))
       << object (string (createdAtBuf))
       << object (s->thinking ? string ("enabled") : string ("disabled"))
       << object (s->search ? string ("enabled") : string ("disabled"))
       << object (string (updateAtBuf));
  call ("chat-persist-update-manifest", args);
}

void
ChatController::registerSession (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s || s->registered) return;

  call ("buffer-pretend-saved",
        ChatSessionManager::messageBufferUrl (sessionId));
  call ("buffer-pretend-saved", ChatSessionManager::inputBufferUrl (sessionId));

  string             displayTitle= getSessionDisplayTitle (sessionId);
  SessionDisplayInfo info;
  info.sessionId   = sessionId;
  info.displayTitle= displayTitle;
  info.model       = s->model;
  info.archived    = false;
  view_->sidebar ()->addItem (info);

  s->registered= true;
}

void
ChatController::connectPanelSignals (ChatConversationPanel* panel) {
  connect (panel, &ChatConversationPanel::sendRequested, this,
           &ChatController::onSendRequested);
  connect (panel, &ChatConversationPanel::thinkingToggled, this,
           &ChatController::onThinkingToggled);
  connect (panel, &ChatConversationPanel::searchToggled, this,
           &ChatController::onSearchToggled);
  connect (panel, &ChatConversationPanel::closeSidebarInDockModeRequested, this,
           [this] () {
             if (!view_) return;
             QWidget* gp= view_->parentWidget ();
             if (gp && qobject_cast<QDockWidget*> (gp))
               emit view_->closeSidebarRequested ();
           });
}

void
ChatController::ensureNewConversation () {
  if (!view_) return;

  // 复用无标题的空白会话（面板和输入内容保持不变）
  string reusable= sessionManager_.findReusableSession ();
  if (!is_empty (reusable)) {
    sessionManager_.setModel (reusable, currentModel_);
    ChatSession* s= sessionManager_.getSession (reusable);
    if (s && s->panel) {
      ChatConversationPanel* p= static_cast<ChatConversationPanel*> (s->panel);
      if (p->sessionTitle ()) p->sessionTitle ()->hide ();
    }
    activateSession (reusable);
    view_->sidebar ()->setActiveItem (""); // 未注册会话不在 sidebar，清除高亮
    return;
  }

  // 创建新会话
  string                 sid  = sessionManager_.createSession ();
  ChatConversationPanel* panel= view_->createPanel (sid);
  if (!panel) return;

  sessionManager_.setPanel (sid, panel);
  sessionManager_.setModel (sid, currentModel_);

  call ("chat-tab-sync-dark-style!", sid);
  call ("chat-tab-load-input-styles!", sid);

  if (panel->sessionTitle ()) panel->sessionTitle ()->hide ();

  // 连接 Panel 的信号
  connectPanelSignals (panel);

  view_->activatePanel (panel);
  view_->sidebar ()->setActiveItem ("");
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

  call ("chat-tab-sync-dark-style!", sessionId);
  call ("chat-tab-init-session!", sessionId, s->model);

  // 连接 Panel 的信号
  connectPanelSignals (panel);

  // 恢复推理模式按钮状态
  if (panel->thinkingButton () && s->thinking) {
    panel->thinkingButton ()->setChecked (true);
  }

  // 恢复网络搜索按钮状态
  if (panel->searchButton () && s->search) {
    panel->searchButton ()->setChecked (true);
  }

  return panel;
}

/******************************************************************************
 * ChatController 辅助方法
 ******************************************************************************/

QList<SessionDisplayInfo>
ChatController::buildDisplayInfos () {
  QList<SessionDisplayInfo> infos;
  auto                      allIds= sessionManager_.getAllSessionIds ();

  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s) continue;

    SessionDisplayInfo info;
    info.sessionId   = s->sessionId;
    info.model       = s->model;
    info.archived    = s->archived;
    info.displayTitle= is_empty (s->title) ? string ("新会话") : s->title;

    infos.append (info);
  }

  return infos;
}

string
ChatController::getSessionDisplayTitle (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (s && !is_empty (s->title)) return s->title;
  return "新会话";
}

/******************************************************************************
 * 自由函数回调（Scheme→C++）
 ******************************************************************************/

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
                             string archived, string createdAtStr,
                             string updatedAtStr, int defaultExpandCount,
                             string thinking, string search) {
  time_t      createdAt= (time_t) std::atol (c_string (createdAtStr));
  time_t      updateAt = is_empty (updatedAtStr)
                             ? createdAt
                             : (time_t) std::atol (c_string (updatedAtStr));
  ChatSession session;
  session.sessionId         = sessionId;
  session.title             = title;
  session.model             = model;
  session.state             = ChatState::Idle;
  session.archived          = (archived == "true");
  session.createdAt         = createdAt;
  session.updateAt          = updateAt;
  session.defaultExpandCount= (defaultExpandCount > 0) ? defaultExpandCount : 5;
  session.thinking          = (thinking == "enabled");
  session.search            = (search == "enabled");
  session.panel             = nullptr;
  get_chat_controller ()->restoreSessionMeta (session);
}

string
ChatController::activeSessionMessageBufferUrl () const {
  if (!view_) return "";
  ChatSidebar* sidebar= view_->sidebar ();
  if (!sidebar) return "";
  string activeId= sidebar->activeSessionId ();
  if (is_empty (activeId)) return "";
  url msgBufUrl= ChatSessionManager::messageBufferUrl (activeId);
  return as_string (msgBufUrl);
}

string
qt_chat_tab_active_message_buffer_url () {
  return get_chat_controller ()->activeSessionMessageBufferUrl ();
}

void
qt_chat_notify_input_height () {
  ChatController* ctrl= get_chat_controller ();
  if (!ctrl || !ctrl->view_) return;

  ChatConversationPanel* panel= ctrl->view_->activeConversation ();
  if (!panel) return;

  panel->schedule_input_height_adjust ();
}
