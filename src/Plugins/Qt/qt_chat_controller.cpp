
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
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>

#include "qt_utilities.hpp"

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
  // 1. 先加载元数据到 sessionManager_（不需要 View）
  call ("chat-persist-load-all");
  cout << "[chat-persist] ChatController: restored "
       << sessionManager_.getAllSessionIds ().size () << " session metadatas"
       << LF;

  // 2. 构建显示数据 + 确定初始激活会话
  QList<SessionDisplayInfo> infos= buildDisplayInfos ();
  string                    initialId;
  if (firstOpen_) {
    // 首次打开：切换到新会话（触发 ensureNewConversation）
    initialId = "";
    firstOpen_= false;
  }
  else {
    initialId= determineInitialActiveSession ();
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

  // 5. 恢复 Scheme 层的全局当前模型（使用最新的会话）
  auto allIds= sessionManager_.getAllSessionIds ();
  if (!allIds.empty ()) {
    string       lastSid= allIds.front ();
    ChatSession* s      = sessionManager_.getSession (lastSid);
    if (s && !is_empty (s->model)) {
      call ("chat-tab-session-select-model", s->model);
    }
  }

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
    if (!is_empty (cur)) view_->sidebar ()->setActiveItem (cur);
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

  // 首次发送：通过 Scheme 自动提取标题
  if (is_empty (session->title)) {
    string extracted=
        as_string (call ("chat-persist-extract-title", sessionId));
    QString qTitle= to_qstring (extracted);
    // 检测标题是否含 CJK 字符
    bool hasCJK= false;
    for (int i= 0; i < qTitle.length (); i++) {
      ushort code= qTitle[i].unicode ();
      if (code >= 0x4E00 && code <= 0x9FFF) {
        hasCJK= true;
        break;
      }
    }
    // 含 CJK: 截取前 10 个字符
    if (hasCJK) {
      if (qTitle.length () > 10) qTitle= qTitle.left (10) + "...";
    }
    // 纯英文: 截取前 5 个单词
    else {
      QStringList words= qTitle.split (' ', Qt::SkipEmptyParts);
      if (words.size () > 5) {
        words = words.mid (0, 5);
        qTitle= words.join (" ") + "...";
      }
    }
    sessionManager_.setTitle (sessionId, from_qstring (qTitle));
    string displayTitle= getSessionDisplayTitle (sessionId);
    view_->sidebar ()->updateItemTitle (sessionId, displayTitle);
    view_->sidebar ()->setActiveItem (sessionId);
    // 更新会话标题标签
    if (session->panel) {
      ChatConversationPanel* p=
          static_cast<ChatConversationPanel*> (session->panel);
      if (p->sessionTitle ()) {
        p->sessionTitle ()->setText (qTitle);
        p->sessionTitle ()->show ();
      }
    }
  }

  if (!as_bool (call ("chat-tab-send", sessionId))) return;

  sessionManager_.setState (sessionId, ChatState::Generating);
  panel->enterConversationMode ();

  panel->focusInput ();
  exportBuffer (sessionId);
  updateManifest (sessionId);
}

void
ChatController::onCancelRequested (const string& sessionId) {
  call ("chat-tab-cancel", sessionId);
}

void
ChatController::onThinkingToggled (const string& sessionId, bool enabled) {
  sessionManager_.setThinking (sessionId, enabled);
  call ("chat-tab-session-set-thinking", sessionId,
        enabled ? string ("enabled") : string ("disabled"));
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

    call ("chat-persist-delete-one", sid);
    call ("chat-tab-session-destroy", sid);
    view_->sidebar ()->removeItem (sid);
    sessionManager_.removeSession (sid);

    if (panel) view_->removePanel (panel);
  }

  // 查找第一个非归档会话作为下一个激活项
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
  else {
    ensureNewConversation ();
  }

  // 将被删除的 buffer 标记为已保存，避免关闭时弹窗
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
  for (const string& sid : sessionIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s || is_empty (s->title)) continue; // 空白会话跳过归档
    sessionManager_.archiveSession (sid);
    updateManifest (sid);
    view_->sidebar ()->moveToArchive (sid);
  }

  // 检查是否归档了当前激活会话
  string cur           = view_->sidebar ()->activeSessionId ();
  bool   archivedActive= false;
  for (const string& sid : sessionIds) {
    if (sid == cur) {
      archivedActive= true;
      break;
    }
  }

  string nextSid;

  if (archivedActive) {
    auto allIds= sessionManager_.getAllSessionIds ();
    for (const string& sid : allIds) {
      ChatSession* s= sessionManager_.getSession (sid);
      if (s && !s->archived) {
        nextSid= sid;
        break;
      }
    }
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

void
ChatController::onExportRequested (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s) return;

  QString defaultName= is_empty (s->title) ? QString ("export.tmu")
                                           : to_qstring (s->title) + ".tmu";
  QString targetPath = QFileDialog::getSaveFileName (
      nullptr, qt_translate ("Export Conversation"), defaultName,
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
  sessionManager_.setTitle (sessionId, newTitle);
  string displayTitle= getSessionDisplayTitle (sessionId);
  view_->sidebar ()->updateItemTitle (sessionId, displayTitle);
  view_->sidebar ()->setActiveItem (sessionId);
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
  QPushButton* btn= panel->sendButton ();
  if (!btn) return;

  // Generating 状态：切换按钮为 Stop
  if (newState == ChatState::Generating) {
    btn->setToolTip ("Stop");
    btn->setIcon (QIcon (":llm-chat/cancel.svg"));
    disconnect (session->sendBtnConnection);
    session->sendBtnConnection=
        connect (btn, &QPushButton::clicked, this,
                 [this, sessionId] () { onCancelRequested (sessionId); });
  }
  // Idle 状态：切换按钮为 Send，并保存会话
  else {
    btn->setToolTip ("Send");
    btn->setIcon (QIcon (":llm-chat/send.svg"));
    disconnect (session->sendBtnConnection);
    session->sendBtnConnection=
        connect (btn, &QPushButton::clicked, this,
                 [this, sessionId] () { onSendRequested (sessionId); });
    exportBuffer (sessionId);
    panel->focusInput ();
  }
}

void
ChatController::restoreSessionMeta (const string& sessionId,
                                    const string& title, const string& model,
                                    bool archived, const string& createdAt,
                                    int defaultExpandCount, bool thinking) {
  // 注册 Scheme 侧会话状态
  call ("chat-persist-register-session", sessionId, model,
        thinking ? string ("enabled") : string ("disabled"));

  // 插入元数据，不创建面板
  ChatSession session;
  session.sessionId         = sessionId;
  session.title             = title;
  session.model             = model;
  session.state             = ChatState::Idle;
  session.archived          = archived;
  session.createdAt         = createdAt;
  session.defaultExpandCount= defaultExpandCount;
  session.thinking          = thinking;
  session.panel             = nullptr;
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
      call ("chat-scroll-message-to-end", sid);
    });
  }

  // 同步会话标题标签
  if (panel->sessionTitle () && s) {
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
  call ("chat-persist-export-buffer", sessionId);
}

void
ChatController::updateManifest (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s) return;
  array<object> args;
  args << object (sessionId) << object (s->title) << object (s->model)
       << object (s->archived ? string ("true") : string ("false"))
       << object (s->createdAt)
       << object (s->thinking ? string ("enabled") : string ("disabled"));
  call ("chat-persist-update-manifest", args);
}

void
ChatController::ensureNewConversation () {
  if (!view_) return;
  string currentModel=
      as_string (call ("chat-tab-session-select-model", string ("")));

  // 尝试复用已有的空白会话（无标题、未发送过的）
  auto allIds= sessionManager_.getAllSessionIds ();
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s || s->archived) continue;
    // 空白 session 无面板：复用并更新模型
    if (!s->panel && is_empty (s->title)) {
      sessionManager_.setModel (sid, currentModel);
      activateSession (sid);
      return;
    }
    // 有面板但未进入会话模式：复用并更新模型
    if (s->panel) {
      ChatConversationPanel* p= static_cast<ChatConversationPanel*> (s->panel);
      if (p && !p->conversationMode ()) {
        sessionManager_.setModel (sid, currentModel);
        if (p->sessionTitle ()) {
          p->sessionTitle ()->hide ();
        }
        view_->sidebar ()->setActiveItem (sid);
        view_->activatePanel (p);
        return;
      }
    }
  }

  // 无可复用会话，创建新会话
  string                 sid  = sessionManager_.createSession ();
  ChatConversationPanel* panel= view_->createPanel (sid);
  if (!panel) return;

  sessionManager_.setPanel (sid, panel);
  sessionManager_.setModel (sid, currentModel);

  call ("chat-tab-sync-dark-style!", sid);

  if (panel->sessionTitle ()) {
    panel->sessionTitle ()->hide ();
  }

  // 连接 Panel 的信号
  connect (panel, &ChatConversationPanel::sendRequested, this,
           &ChatController::onSendRequested);
  connect (panel, &ChatConversationPanel::thinkingToggled, this,
           &ChatController::onThinkingToggled);

  // 添加 sidebar item
  view_->sidebar ()->addItem (sid, "新会话");

  view_->activatePanel (panel);
  view_->sidebar ()->setActiveItem (sid);

  // 标记 buffer 为已保存
  call ("buffer-pretend-saved", ChatSessionManager::messageBufferUrl (sid));
  call ("buffer-pretend-saved", ChatSessionManager::inputBufferUrl (sid));
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

  // 连接 Panel 的信号
  connect (panel, &ChatConversationPanel::sendRequested, this,
           &ChatController::onSendRequested);
  connect (panel, &ChatConversationPanel::thinkingToggled, this,
           &ChatController::onThinkingToggled);

  // 恢复推理模式按钮状态
  if (panel->thinkingButton () && s->thinking) {
    panel->thinkingButton ()->setChecked (true);
  }

  return panel;
}

/******************************************************************************
 * ChatController 辅助方法
 ******************************************************************************/

QMap<string, string>
ChatController::getDisplayTitles () {
  QMap<string, string> result;

  auto allIds= sessionManager_.getAllSessionIds ();
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s) continue;
    result[sid]= is_empty (s->title) ? string ("新会话") : s->title;
  }

  return result;
}

QList<SessionDisplayInfo>
ChatController::buildDisplayInfos () {
  QList<SessionDisplayInfo> infos;
  QMap<string, string>      titles= getDisplayTitles ();
  auto                      allIds= sessionManager_.getAllSessionIds ();

  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (!s) continue;

    SessionDisplayInfo info;
    info.sessionId   = s->sessionId;
    info.model       = s->model;
    info.archived    = s->archived;
    info.displayTitle= titles.value (sid, "新会话");

    infos.append (info);
  }

  return infos;
}

string
ChatController::determineInitialActiveSession () {
  auto allIds= sessionManager_.getAllSessionIds ();
  for (const string& sid : allIds) {
    ChatSession* s= sessionManager_.getSession (sid);
    if (s && !s->archived) return sid;
  }
  return "";
}

string
ChatController::getSessionDisplayTitle (const string& sessionId) {
  QMap<string, string> titles= getDisplayTitles ();
  return titles.value (sessionId, "新会话");
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
                             string archived, string createdAt,
                             int defaultExpandCount, string thinking) {
  bool isArchived = (archived == "true");
  int  expandCount= (defaultExpandCount > 0) ? defaultExpandCount : 5;
  bool isThinking = (thinking == "enabled");
  get_chat_controller ()->restoreSessionMeta (
      sessionId, title, model, isArchived, createdAt, expandCount, isThinking);
}
