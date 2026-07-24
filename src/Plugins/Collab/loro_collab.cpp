/******************************************************************************
 * MODULE     : loro_collab.cpp
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "editor.hpp"
#include "loro_collab_ws.hpp"
#include "new_view.hpp"
#include "server.hpp"
#include "sys_utils.hpp"
#include "tm_buffer.hpp"
#include "tm_timer.hpp"
#include "url.hpp"
#include <cstdint>

void (*g_loro_broadcast_update) (string bytes)= nullptr;
void (*g_loro_cursor_dirty) ()                = nullptr;
collab_session_manager g_session_manager;

void
broadcast_to_server (string bytes) {
  url             buf    = get_current_buffer ();
  collab_session* session= g_session_manager.find_by_buffer (buf);
  if (session) {
    session->broadcast (bytes);
    // 编辑已即时上行；光标须紧跟其后补发，避免对端先收到编辑、后收到（节流的）
    // 光标，导致远程光标短暂落在新插入文本之前（如 "Line 1|test"）。
    session->flush_cursor ();
  }
}

// 多光标：编辑器侧 collab_cursor_moved_hook 经此把「当前 buffer 的会话」标脏，
// 待 poll() 节流上行。
void
mark_current_cursor_dirty () {
  url             buf    = get_current_buffer ();
  collab_session* session= g_session_manager.find_by_buffer (buf);
  if (session) session->mark_cursor_dirty ();
}

// -----------------------------------------------------------------------------
// collab_session
// -----------------------------------------------------------------------------

collab_session::collab_session (url buf_url) : buffer_url (buf_url) {
  // 会话级 peer id：可由 MOGAN_LORO_PEER
  // 强制（便于调试/指定颜色），否则随机生成。
  string env= get_env ("MOGAN_LORO_PEER");
  if (env != "") peer_id= env;
  else {
    static int counter= 0;
    int seed= (int) texmacs_time () ^ (++counter) ^ (int) (intptr_t) this;
    peer_id = "p" * as_string (seed);
  }
}

collab_session::~collab_session () { disconnect (); }

void
collab_session::enter_idle () {
  state= collab_state::idle;
}

void
collab_session::enter_connecting () {
  state= collab_state::connecting;
}

void
collab_session::enter_await_doc () {
  state= collab_state::await_doc;
}

void
collab_session::enter_await_frame () {
  state            = collab_state::await_frame;
  await_frame_since= texmacs_time ();
}

void
collab_session::enter_ready () {
  state            = collab_state::ready;
  reconnect_attempt= 0;
}

void
collab_session::enter_reconnecting () {
  state= collab_state::reconnecting;
}

void
collab_session::set_message (string left) {
  get_server ()->set_message (tree (left), tree ("Collaborative"));
}

time_t
collab_session::reconnect_backoff (int attempt) {
  if (attempt <= 0) return 0;
  time_t ms= 1000L << (attempt - 1);
  if (ms <= 0 || ms > 30000L) ms= 30000L;
  return ms;
}

void
collab_session::schedule_reconnect () {
  time_t wait= reconnect_backoff (reconnect_attempt);
  enter_reconnecting ();
  next_reconnect_at= texmacs_time () + wait;
  reconnect_attempt++;
  if (DEBUG_LORO)
    debug_loro << "调度重连：第 " << reconnect_attempt << " 次，" << wait
               << "ms 后\n";
  set_message ("Connection lost; reconnecting… (attempt " *
               as_string (reconnect_attempt) * ")");
}

editor
collab_session::get_editor () const {
  array<url> views= buffer_to_views (buffer_url);
  if (N (views) == 0) return nullptr;
  return view_to_editor (views[0]);
}

void
collab_session::become_ready () {
  bool was_reconnect= (reconnect_attempt > 0);
  enter_ready ();
  buffer_known           = true;
  g_loro_broadcast_update= broadcast_to_server;
  g_loro_cursor_dirty    = mark_current_cursor_dirty;

  editor ed= get_editor ();
  if (is_nil (ed)) {
    std_error << "become_ready: 当前编辑器为空！\n";
  }
  else {
    ed->collab_enable ();
  }

  if (N (pending_updates) > 0) {
    if (DEBUG_LORO)
      debug_loro << "重连后 flush " << N (pending_updates)
                 << " 条缓冲 update\n";
    for (int i= 0; i < N (pending_updates); i++)
      ws->send (pending_updates[i], true);
    pending_updates= array<string> ();
  }

  set_title_buffer (buffer_url, doc_id);
  set_message (was_reconnect ? "Reconnected to " * doc_id
                             : "Session ready: " * doc_id);
  if (DEBUG_LORO) debug_loro << "会话就绪 doc=" << doc_id << "\n";
}

void
collab_session::maybe_reconnect () {
  if (state != collab_state::reconnecting) return;
  if (texmacs_time () < next_reconnect_at) return;
  if (ws) {
    ws->disconnect ();
    ws.reset ();
  }
  if (DEBUG_LORO) debug_loro << "尝试重连 " << server_url << "\n";
  set_message ("Reconnecting… (attempt " * as_string (reconnect_attempt) * ")");

  ws= std::unique_ptr<tm_websocket_client> (create_collab_ws_client (this));
  enter_connecting ();
  ws->connect (server_url);
}

void
collab_session::create (string url_str) {
  disconnect ();
  mode             = collab_mode::create;
  doc_id           = "";
  server_url       = url_str;
  reconnect_attempt= 0;
  want_reconnect   = true;

  ws= std::unique_ptr<tm_websocket_client> (create_collab_ws_client (this));
  enter_connecting ();
  ws->connect (server_url);
}

void
collab_session::join (string url_str, string id) {
  disconnect ();
  mode             = collab_mode::join;
  doc_id           = id;
  server_url       = url_str;
  reconnect_attempt= 0;
  want_reconnect   = true;

  ws= std::unique_ptr<tm_websocket_client> (create_collab_ws_client (this));
  enter_connecting ();
  ws->connect (server_url);
}

void
collab_session::disconnect () {
  want_reconnect= false;
  enter_idle ();
  doc_id           = "";
  reconnect_attempt= 0;
  buffer_known     = false;
  pending_updates  = array<string> ();
  cursor_dirty     = false;
  if (ws) {
    ws->disconnect ();
    ws.reset ();
  }
}

void
collab_session::poll () {
  if (ws) ws->poll ();
  if (state != collab_state::idle && buffer_known &&
      is_nil (concrete_buffer (buffer_url))) {
    if (DEBUG_LORO) debug_loro << "协作 buffer 已关闭，断开会话\n";
    disconnect ();
    // We do not remove it from manager here, let the manager handle it or just
    // keep it idle
    return;
  }
  if (state == collab_state::await_frame && await_frame_since > 0 &&
      texmacs_time () - await_frame_since >= 1000) {
    std_error << "JOIN 超时未收到历史帧，按空文档就绪\n";
    await_frame_since= 0;
    become_ready ();
  }
  // 多光标：脏标记 + ≥50ms 节流上行本地光标
  if (cursor_dirty && state == collab_state::ready &&
      texmacs_time () - last_cursor_send >= 50) {
    editor ed= get_editor ();
    if (!is_nil (ed)) {
      string payload= ed->collab_cursor_payload ();
      if (payload != "") send_cursor ("CURSOR " * peer_id * " " * payload);
    }
    cursor_dirty    = false;
    last_cursor_send= texmacs_time ();
  }
  maybe_reconnect ();
}

void
collab_session::broadcast (string bytes) {
  if (state == collab_state::ready && ws && ws->connected ()) {
    if (DEBUG_LORO) debug_loro << "上行 " << N (bytes) << " 字节 update\n";
    ws->send (bytes, true);
    return;
  }
  if (want_reconnect) {
    pending_updates << bytes;
    if (DEBUG_LORO)
      debug_loro << "缓冲本地 update（" << N (bytes)
                 << " 字节），待重连后上行\n";
  }
}

// 多光标：发文本帧 "CURSOR <peer> <payload>"。光标瞬态，仅在 ready 且已连接时
// 发送，断线不缓冲（丢失即丢弃，下个节流周期自然补发）。
void
collab_session::send_cursor (string payload) {
  if (state == collab_state::ready && ws && ws->connected ())
    ws->send (payload, false);
}

// 编辑上行后由 broadcast_to_server 调用：若光标脏则立即补发（绕过 poll 的 50ms
// 节流），让光标更新紧随编辑、同 WS 有序到达对端。tp 已在 post_notify 中更新到
// 编辑后位置（post_notify 先于 mirror_loro），故此处读到的是最新光标。
void
collab_session::flush_cursor () {
  if (!cursor_dirty || state != collab_state::ready) return;
  editor ed= get_editor ();
  if (!is_nil (ed)) {
    string payload= ed->collab_cursor_payload ();
    if (payload != "") send_cursor ("CURSOR " * peer_id * " " * payload);
  }
  cursor_dirty    = false;
  last_cursor_send= texmacs_time ();
}

void
collab_session::on_connect () {
  if (DEBUG_LORO) debug_loro << "已连接服务端 " << server_url << "\n";
  enter_await_doc ();
  if (N (doc_id) > 0) ws->send ("JOIN " * doc_id, false);
  else ws->send ("CREATE", false);
}

void
collab_session::on_message (string data, bool is_binary) {
  if (!is_binary) {
    if (starts (data, "DOC ")) {
      doc_id= data (4, N (data));
      if (DEBUG_LORO)
        debug_loro << "服务端确认文档 " << doc_id
                   << "（mode=" << (want_create () ? "create" : "join")
                   << "）\n";
      if (want_create ()) become_ready ();
      else {
        enter_await_frame ();
      }
    }
    else if (starts (data, "ERR ")) {
      std_error << "服务端错误: " << data << "\n";
      want_reconnect= false;
      enter_idle ();
      set_message (data * " — stopped reconnecting");
    }
    else if (starts (data, "CURSOR ")) {
      // "CURSOR <peer>
      // <payload>"：peer=首空格前，payload=其后（含空格，不透明）
      string rest= data (7, N (data));
      int    sp  = search_forwards (" ", rest);
      if (sp >= 0) {
        editor ed= get_editor ();
        if (!is_nil (ed))
          ed->set_remote_cursor (rest (0, sp), rest (sp + 1, N (rest)));
      }
    }
    return;
  }
  editor ed= get_editor ();
  if (is_nil (ed)) return;
  if (state == collab_state::await_frame) {
    ed->apply_remote (data);
    become_ready ();
    flush_cursor (); // 首帧同步后补发本端初始光标
  }
  else if (state == collab_state::ready) {
    ed->apply_remote (data);
    // 远程编辑可能移动了本端光标/选区（如他人删除了与己重叠的选区内容）。
    // 直接置本会话脏并立即补发（apply_remote 的 restore 经 get_current_buffer
    // 找会话，对后台 buffer 可能漏标脏），避免对端视图滞后仍显示旧选区。
    cursor_dirty= true;
    flush_cursor ();
  }
}

void
collab_session::on_error (string msg) {
  std_error << "WS Error: " << msg << "\n";
}

void
collab_session::on_disconnect () {
  if (DEBUG_LORO)
    debug_loro << "WS 断开（want_reconnect=" << want_reconnect
               << ", state=" << (int) state << "）\n";
  if (want_reconnect) schedule_reconnect ();
  else enter_idle ();
}

// -----------------------------------------------------------------------------
// collab_session_manager
// -----------------------------------------------------------------------------

collab_session_manager::~collab_session_manager () {
  for (int i= 0; i < N (sessions); i++) {
    delete sessions[i];
  }
}

collab_session*
collab_session_manager::find_by_buffer (url buf_url) {
  for (int i= 0; i < N (sessions); i++) {
    if (sessions[i]->get_buffer_url () == buf_url) {
      return sessions[i];
    }
  }
  return nullptr;
}

collab_session*
collab_session_manager::get_or_create (url buf_url) {
  collab_session* session= find_by_buffer (buf_url);
  if (!session) {
    session= new collab_session (buf_url);
    sessions << session;
  }
  return session;
}

void
collab_session_manager::remove_session (collab_session* session) {
  array<collab_session*> new_sessions;
  for (int i= 0; i < N (sessions); i++) {
    if (sessions[i] != session) {
      new_sessions << sessions[i];
    }
  }
  sessions= new_sessions;
  delete session;
}

void
collab_session_manager::poll_all () {
  // Use a copy to allow deletion during poll
  array<collab_session*> copy= sessions;
  for (int i= 0; i < N (copy); i++) {
    copy[i]->poll ();
    if (!copy[i]->is_active () && copy[i]->is_buffer_known () &&
        is_nil (concrete_buffer (copy[i]->get_buffer_url ()))) {
      remove_session (copy[i]);
    }
  }
}

// -----------------------------------------------------------------------------
// Public C API (loro_collab.hpp)
// -----------------------------------------------------------------------------

string
loro_collab_create (string server_url) {
  editor ed= get_current_editor ();
  if (is_nil (ed)) {
    std_error << "无当前编辑器，无法创建协作文档\n";
    return "";
  }
  url             buf    = get_current_buffer ();
  collab_session* session= g_session_manager.get_or_create (buf);
  session->create (server_url);
  return "";
}

void
loro_collab_join (string server_url, string doc_id) {
  editor ed= get_current_editor ();
  if (is_nil (ed)) {
    std_error << "无当前编辑器，无法加入协作文档\n";
    return;
  }
  url             buf    = get_current_buffer ();
  collab_session* session= g_session_manager.get_or_create (buf);
  session->join (server_url, doc_id);
}

void
loro_collab_disconnect () {
  editor ed= get_current_editor ();
  if (is_nil (ed)) return;
  url             buf    = get_current_buffer ();
  collab_session* session= g_session_manager.find_by_buffer (buf);
  if (session) {
    session->disconnect ();
    g_session_manager.remove_session (session);
  }
}

bool
loro_collab_is_active () {
  editor ed= get_current_editor ();
  if (is_nil (ed)) return false;
  url             buf    = get_current_buffer ();
  collab_session* session= g_session_manager.find_by_buffer (buf);
  return session ? session->is_active () : false;
}

string
loro_collab_doc_id () {
  editor ed= get_current_editor ();
  if (is_nil (ed)) return "";
  url             buf    = get_current_buffer ();
  collab_session* session= g_session_manager.find_by_buffer (buf);
  return session ? session->get_doc_id () : "";
}

void
loro_collab_poll () {
  g_session_manager.poll_all ();
}
