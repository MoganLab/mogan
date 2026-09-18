
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
#include "preferences.hpp"
#include "s7_tm.hpp"
#include "scheme.hpp"
#include "tm_debug.hpp"

#include "analyze.hpp"
#include "converter.hpp"
#include "dictionary.hpp"
#include "locale.hpp"

#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
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
  // llm 插件按 idle 延迟初始化，新建 Chat 标签页时其 scheme 模块可能尚未加载
  bool benching= QTChatTabWidget::isInitBenchPending ();
  if (benching) bench_start ("chat_init: load chat modules");
  eval ("(use-modules (llm chat-list))");
  call ("chat-persist-load-all");
  if (benching) bench_end ("chat_init: load chat modules");
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
  if (benching) bench_start ("chat_init: activate session");
  if (!is_empty (initialId)) {
    activateSession (initialId);
  }
  else {
    ensureNewConversation ();
  }
  if (benching) bench_end ("chat_init: activate session");

  // 5. 注册浮动搜索栏的 parent provider
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

  // 首次发送且无标题：从内容生成标题（翻译会话创建时已带标题，不进此分支）
  if (is_empty (session->title)) {
    sessionManager_.generateTitleFromContent (sessionId);

    string displayTitle= getSessionDisplayTitle (sessionId);
    view_->sidebar ()->updateItemTitle (sessionId, displayTitle);
  }
  view_->sidebar ()->setActiveItem (sessionId);
  // 面板标题标签随会话标题同步（翻译会话的标题在创建时已确定，同样要显示）
  if (session->panel && !is_empty (session->title)) {
    ChatConversationPanel* p=
        static_cast<ChatConversationPanel*> (session->panel);
    if (p->sessionTitle ()) {
      p->sessionTitle ()->setText (to_qstring (session->title));
      p->sessionTitle ()->show ();
    }
  }

  // 必须在 scheme 发送之前创建消息编辑器：texmacs_input_widget 对已存在
  // buffer 会 set_buffer_tree 整体覆盖，若放在 chat-tab-send 之后，scheme
  // 侧记录的输出节点指针会指向被替换的旧树，导致首轮 %chat 回显漏出到
  // session 文档末尾（devel/1230.md）
  panel->ensureMessageWidget ();

  // 协议下发参数取自模型清单：baseUrl 透传清单原值，相对路径由 scheme 侧
  // 拼接当前 stem site
  ChatModelInfo info= modelStore_.find (session->model);
  array<object> args;
  args << object (sessionId) << object (info.key) << object (info.baseUrl)
       << object (session->thinking ? string ("enabled") : string ("disabled"))
       << object (session->search ? string ("enabled") : string ("disabled"))
       << object (session->thinkingEffort);
  if (!as_bool (call ("chat-tab-send", args))) return;

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
ChatController::onModelMenuRequested (const string& sessionId,
                                      const QPoint& globalPos) {
  string current= sessionManager_.getModel (sessionId);
  if (!modelStore_.contains (current)) current= modelStore_.defaultKey ();

  // 菜单每次打开重建，选中态按当前会话模型刷新
  QMenu menu;
  chat_model_menu_populate (&menu, modelStore_.models (), current);
  // 思考强度子菜单追加在模型项之后，选中态按当前会话强度刷新
  chat_effort_menu_populate (&menu,
                             sessionManager_.getThinkingEffort (sessionId));

  // 菜单在 Model 按钮上方完整弹出（按钮底边贴菜单顶边），不遮挡按钮
  QPoint pos (globalPos.x (), globalPos.y () - menu.sizeHint ().height ());

  updateModelButtonDisplay (sessionId, true); // 打开：箭头朝上
  // QWidgetAction 内控件经 released 手动 trigger 时，Qt 只发 triggered
  // 信号并关菜单、不写 exec 的 syncAction（exec 会返回 null），故选择
  // 结果从 triggered 信号捕获，不依赖 exec 返回值；子菜单 action 的触发
  // 会沿父子链传播到这里，强度项以 actionGroup 归属构造性区分
  string chosenKey, chosenEffort;
  connect (&menu, &QMenu::triggered, &menu,
           [&chosenKey, &chosenEffort] (QAction* a) {
             string d= from_qstring_utf8 (a->data ().toString ());
             if (a->actionGroup ()) chosenEffort= d;
             else chosenKey= d;
           });
  menu.exec (pos);
  updateModelButtonDisplay (sessionId); // 关闭：箭头朝下（选择后模型已变）
  if (!is_empty (chosenKey)) onModelSelected (sessionId, chosenKey);
  if (!is_empty (chosenEffort))
    onThinkingEffortSelected (sessionId, chosenEffort);
}

void
ChatController::onThinkingEffortSelected (const string& sessionId,
                                          const string& effort) {
  sessionManager_.setThinkingEffort (sessionId, effort);
  updateManifest (sessionId);
}

void
ChatController::onModelSelected (const string& sessionId, const string& key) {
  if (!modelStore_.contains (key)) return;
  sessionManager_.setModel (sessionId, key);

  // 切到不允许某能力的模型时重置对应开关为关：隐藏的开会泄漏到下一轮请求
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (s) {
    ChatModelInfo          info= modelStore_.find (key);
    ChatConversationPanel* panel=
        static_cast<ChatConversationPanel*> (s->panel);
    if (!info.allowThinking && s->thinking) {
      sessionManager_.setThinking (sessionId, false);
      if (panel && panel->thinkingButton ())
        panel->thinkingButton ()->setChecked (false);
    }
    if (!info.allowSearch && s->search) {
      sessionManager_.setSearch (sessionId, false);
      if (panel && panel->searchButton ())
        panel->searchButton ()->setChecked (false);
    }
  }

  updateManifest (sessionId); // 仅元数据变更，不导出 buffer（含重置后的开关）
  updateModelButtonDisplay (sessionId);
  applyModelCapabilities (sessionId);
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

  // 模型不在清单内（manifest 旧值或空值）时内存回退默认模型，不回写 manifest
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (s && !modelStore_.contains (s->model)) {
    sessionManager_.setModel (sessionId, modelStore_.defaultKey ());
  }

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

  updateModelButtonDisplay (sessionId);
}

void
ChatController::updateModelButtonDisplay (const string& sessionId,
                                          bool          menuOpen) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s || !s->panel) return;
  string key= s->model;
  if (!modelStore_.contains (key)) key= modelStore_.defaultKey ();
  ChatModelInfo info= modelStore_.find (key);
  static_cast<ChatConversationPanel*> (s->panel)->setModelDisplay (
      info.name, info.icon, menuOpen);
}

void
ChatController::applyModelCapabilities (const string& sessionId) {
  ChatSession* s= sessionManager_.getSession (sessionId);
  if (!s || !s->panel) return;
  ChatModelInfo info= modelStore_.find (s->model); // find 自带 key 兜底
  ChatConversationPanel* panel= static_cast<ChatConversationPanel*> (s->panel);
  if (panel->thinkingButton ())
    panel->thinkingButton ()->setVisible (info.allowThinking);
  if (panel->searchButton ())
    panel->searchButton ()->setVisible (info.allowSearch);
}

string
ChatController::initialThinkingEffort (const string& modelKey) {
  // 「上一条记录」= updateAt 降序中第一个有标题（发送过消息）的会话；
  // 恢复的历史会话带持久化强度，重启后规则依然成立
  string latest= sessionManager_.firstTitledSessionId ();
  if (!is_empty (latest)) return sessionManager_.getThinkingEffort (latest);
  // 首次对话无记录：取清单中该模型的默认强度
  return modelStore_.find (modelKey).thinkingEffort;
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
    // 历史会话恢复时面板尚未上屏，无需欢迎页→消息区的渐隐过渡，
    // 直接切换避免欢迎页先闪现（1275）
    panel->enterConversationMode (false);
    // ensureMessageWidget 里 texmacs_input_widget 会 set_buffer_tree 整体
    // 覆盖消息 buffer 样式，而此前的 scheme 样式操作发生在无视图阶段、
    // 已被 with-buffer 静默跳过；须在视图就绪后补齐默认样式包，
    // 历史会话恢复才能带上 llm 等插件包
    call ("chat-tab-sync-session-styles!", panel->sessionId ());
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
       << object (string (updateAtBuf)) << object (s->thinkingEffort)
       << object (s->sourceDocId) << object (s->type);
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
  connect (panel, &ChatConversationPanel::modelMenuRequested, this,
           &ChatController::onModelMenuRequested);
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
    // 新会话（含复用）固定为清单默认模型，不继承最近激活会话；思考强度
    // 按「首次取清单默认、之后取最近一条记录」规则初始化
    sessionManager_.setModel (reusable, modelStore_.defaultKey ());
    sessionManager_.setThinkingEffort (
        reusable, initialThinkingEffort (modelStore_.defaultKey ()));
    ChatSession* s= sessionManager_.getSession (reusable);
    if (s && s->panel) {
      ChatConversationPanel* p= static_cast<ChatConversationPanel*> (s->panel);
      if (p->sessionTitle ()) p->sessionTitle ()->hide ();
      p->showWelcomePage ();
    }
    activateSession (reusable);
    view_->sidebar ()->setActiveItem (""); // 未注册会话不在 sidebar，清除高亮
    return;
  }

  createNewConversation ();
}

ChatConversationPanel*
ChatController::createNewConversation (const string& modelKey) {
  if (!view_) return nullptr;
  // 创建新会话（与 ensureNewConversation 的复用分支共用语义：默认模型、
  // 欢迎页、不继承最近激活会话）
  string                 sid  = sessionManager_.createSession ();
  ChatConversationPanel* panel= view_->createPanel (sid);
  if (!panel) return nullptr;

  // 指定模型须在清单内，否则回退清单默认模型（AI 翻译指定 v4-pro 走此分支）
  string initialModel= (!is_empty (modelKey) && modelStore_.contains (modelKey))
                           ? modelKey
                           : modelStore_.defaultKey ();
  sessionManager_.setPanel (sid, panel);
  sessionManager_.setModel (sid, initialModel);
  sessionManager_.setThinkingEffort (sid, initialThinkingEffort (initialModel));

  eval ("(use-modules (llm chat-style))");
  call ("chat-tab-sync-session-styles!", sid);
  call ("chat-tab-load-input-styles!", sid);

  if (panel->sessionTitle ()) panel->sessionTitle ()->hide ();
  panel->showWelcomePage ();

  // 连接 Panel 的信号
  connectPanelSignals (panel);

  view_->activatePanel (panel);
  view_->sidebar ()->setActiveItem ("");

  updateModelButtonDisplay (sid);
  // 此路径不经 getOrCreatePanel，默认模型可能不允许某能力，需单独应用
  applyModelCapabilities (sid);

  return panel;
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

  eval ("(use-modules (llm chat-style) (llm chat-protocol))");
  call ("chat-tab-sync-session-styles!", sessionId);
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

  // 恢复的会话模型可能不允许推理/搜索，按能力隐藏对应按钮
  applyModelCapabilities (sessionId);

  return panel;
}

/******************************************************************************
 * ChatController 辅助方法
 ******************************************************************************/

// AI 语言首选项（翻译目标语言/提示词语言）：system 哨兵表示跟随系统语言
// （get_locale_language；非界面语言偏好，用户改界面语言不影响）
static string
get_ai_language_preference (string key) {
  string lang= get_preference (key, "system");
  return lang == "system" ? get_locale_language () : lang;
}

// 引用块（插入 → 外观块 → 引用即 quote-env，generic 样式链的 std-markup
// 提供）：document 子节点在块内依次展开，其余整体入块
static tree
aiQuoteBlock (tree content) {
  tree quoted= is_func (content, DOCUMENT) ? content : tree (DOCUMENT, content);
  return compound ("quote-env", quoted);
}

// 上下文纯文本按行拆成段落节点：换行符留在树节点里不会被排版（空行跳过）
static tree
aiTextTree (string text) {
  tree          doc (DOCUMENT);
  array<string> lines= tokenize (text, "\n");
  for (int i= 0; i < N (lines); i++)
    if (N (lines[i]) > 0) doc << lines[i];
  return doc;
}

tree
ChatController::composeAiInputBody (tree sel, string action, string context) {
  // 组装聊天输入体：gloss 分支的提示词节点夹在两个引用块之间，提前返回；
  // 其余动作引用块在前、提示词（或空段）追加为末段
  tree body (DOCUMENT);
  // 释义：上下文（引文1）与选区（引文2）各成一块，编号标签与说明句都算
  // 提示词，提示词按 0995 提示词语言首选项本地化
  if (action == "gloss") {
    string prompt_lang= get_ai_language_preference ("ai:prompt language");
    string label=
        translate (string ("reference %1::ai"), "english", prompt_lang);
    body << replace (label, "%1", "1");
    body << aiQuoteBlock (aiTextTree (context));
    body << replace (label, "%1", "2");
    body << aiQuoteBlock (sel);
    body << translate (
        string ("reference 2 is part of reference 1, explain the "
                "meaning of reference 2 (including dictionary "
                "and technical terms)"),
        "english", prompt_lang);
    return body;
  }
  body << aiQuoteBlock (sel);
  // 未知动作不追加尾段（调用方白名单 translate/chat/gloss）
  if (action == "translate") {
    // 两个独立首选项（AI 标签页）：翻译目标语言决定翻成哪种语言，提示词
    // 语言（0995）决定提示词本身用什么语言书写。目标语言名也按提示词语言
    // 本地化（translate 返回 Cork，与提示词编码一致；词典缺词条时回落为
    // 英文原句）
    string target= get_ai_language_preference ("ai:translate target language");
    string prompt_lang= get_ai_language_preference ("ai:prompt language");
    string prompt= translate (string ("Please translate the above text into"),
                              "english", prompt_lang);
    string target_name=
        translate (upcase_first (target), "english", prompt_lang);
    // 空格按提示词尾字符判定而非按语言：拉丁字母结尾（英文句，含词典缺
    // 词条时的回落）补空格，CJK 词尾（"请翻译上述文字为"）直接接目标语言名
    bool space= N (prompt) > 0 && is_iso_alpha (prompt[N (prompt) - 1]);
    body << prompt * (space ? string (" ") : string ("")) * target_name;
  }
  else if (action == "chat") body << ""; // 空段使 go-end 光标落在引用块下一行
  return body;
}

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
qt_chat_ai_send_selection (tree sel, string action) {
  ChatController* ctrl= get_chat_controller ();
  // 翻译/释义绑定来源文档，须在打开侧边栏（焦点/视图切换）前捕获文档身份
  // （与释义的上下文，引文1）：此时 current-buffer 仍是文档本身，切到聊天
  // 输入缓冲后选区已不在文档上。llm 模块按 idle 延迟初始化，首次动作可能
  // 尚未加载，确保模块就绪（幂等，只执行一次）
  const bool docBound= (action == "translate" || action == "gloss");
  string     docId, docName, context;
  if (docBound) {
    static bool treeOpsLoaded= false;
    if (!treeOpsLoaded) {
      eval ("(use-modules (llm chat-tree-ops))");
      treeOpsLoaded= true;
    }
    object info= call ("chat-tab-source-doc-info");
    docId      = as_string (car (info));
    docName    = as_string (cdr (info));
    if (action == "gloss") context= as_string (call ("ai-selection-context"));
  }
  // 打开 AI 侧边栏：同步创建聊天部件并确保活动会话。已打开时跳过，避免
  // sync_chat_sidebar_mode 重复 dock 重排；社区版无聊天部件，调用静默无效
  if (!ctrl->view_ || !ctrl->view_->isVisible ())
    call ("show-chat-sidebar", object (true));
  if (!ctrl->view_) return;
  // 写入前无需加载检查：面板存在即保证 llm 模块已加载（会话创建路径
  // eval 过 use-modules，chat-loader 亦在启动 idle 阶段整体加载）
  if (docBound) {
    // 同一文档的翻译/释义各自共享一个会话：按 stem-doc-id + 会话类型找
    // 未归档会话（最近活跃优先），命中即复用；未命中（含文档未绑定
    // doc-id、会话已归档/删除）才新建
    string sid;
    if (!is_empty (docId))
      sid= ctrl->sessionManager_.findSessionBySourceDoc (docId, action);
    if (!is_empty (sid)) {
      ChatSession* s= ctrl->sessionManager_.getSession (sid);
      if (s && s->state == ChatState::Generating) {
        // 生成中不覆盖输入，仅激活展示
        ctrl->activateSession (sid);
        return;
      }
      ctrl->activateSession (sid);
    }
    else {
      // 标题 = 词典动作名 + ": " + 文件名。「翻译」与操作栏按钮同一词典键
      // （"Translate" 首字符折叠命中 "translate"），「释义」与释义按钮同用
      // "Gloss::ai" 消歧键（裸 "explain" 词条是「解释」），均随界面语言本地
      // 化；标题为 UTF-8，经 from_qstring_utf8 归一编码后拼接。质量优先，
      // 默认模型取清单 translate_model 字段（清单未配置时 translateKey 回退
      // 清单默认模型）
      string title= from_qstring_utf8 (qt_translate (
                        action == "gloss" ? "Gloss::ai" : "Translate")) *
                    ": " * docName;
      ChatConversationPanel* panel=
          ctrl->createNewConversation (ctrl->modelStore_.translateKey ());
      if (!panel) return;
      sid= panel->sessionId ();
      ctrl->sessionManager_.setTitle (sid, title);
      ctrl->sessionManager_.setSourceDocId (sid, docId);
      ctrl->sessionManager_.setType (sid, action);
    }
    ChatSession*           s= ctrl->sessionManager_.getSession (sid);
    ChatConversationPanel* panel=
        s ? static_cast<ChatConversationPanel*> (s->panel) : nullptr;
    if (!panel) return;
    call ("chat-tab-set-input-body!", ChatSessionManager::inputBufferUrl (sid),
          ChatController::composeAiInputBody (sel, action, context));
    ctrl->onSendRequested (sid);
    return;
  }
  ChatConversationPanel* panel= ctrl->view_->activeConversation ();
  if (!panel) return;
  // 翻译/释义会话专属于来源文档，对话不写进去：激活会话绑定了文档时先切到
  // 空白会话（ensureNewConversation 复用或新建）再填输入
  if (ChatSession* active= ctrl->sessionManager_.findSessionByPanel (panel)) {
    if (!is_empty (active->sourceDocId)) {
      ctrl->ensureNewConversation ();
      panel= ctrl->view_->activeConversation ();
      if (!panel) return;
    }
  }
  call ("chat-tab-set-input-body!",
        ChatSessionManager::inputBufferUrl (panel->sessionId ()),
        ChatController::composeAiInputBody (sel, action, context));
  // 对话只填入输入区，聚焦并滚动到光标（引用块下方）留给用户补写后手动
  // 发送；未知动作同样只填入不发送
  if (action == "chat") panel->revealInputCursor ();
}

void
qt_chat_tab_restore_session (string sessionId, string title, string model,
                             string archived, string createdAtStr,
                             string updatedAtStr, int defaultExpandCount,
                             string thinking, string search,
                             string thinkingEffort) {
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
  session.thinkingEffort    = chat_normalize_thinking_effort (thinkingEffort);
  // 恢复的会话本就在 manifest 与侧边栏中；POD 成员无缺省初始化，显式赋值
  // 避免 registered 脏值导致首次发送时重复注册
  session.registered= true;
  session.panel     = nullptr;
  get_chat_controller ()->restoreSessionMeta (session);
}

void
qt_chat_tab_set_source_doc_id (string sessionId, string docId) {
  // glue 单函数参数上限为 10，sourceDocId 不能随 restore 一并传入，恢复后
  // 由 scheme 侧单独设置（insertSession 之后调用，写入已存入的副本）
  get_chat_controller ()->sessionManager ().setSourceDocId (sessionId, docId);
}

void
qt_chat_tab_set_session_type (string sessionId, string type) {
  // 与 sourceDocId 同理：restore 参数已满，type 由 scheme 侧恢复后单独设置
  get_chat_controller ()->sessionManager ().setType (sessionId, type);
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
