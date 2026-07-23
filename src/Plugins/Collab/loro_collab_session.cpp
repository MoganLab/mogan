/******************************************************************************
 * MODULE     : loro_collab_session.cpp
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "loro_collab.hpp"

#include "tm_websocket.hpp"
#ifdef OS_WASM
#include "tm_emscripten_websocket_client.hpp"
#else
#include "tm_curl_websocket_client.hpp"
#endif

#include "editor.hpp"
#include "new_buffer.hpp"
#include "new_view.hpp"
#include "server.hpp"
#include "url.hpp"

#include "analyze.hpp"
#include "tm_buffer.hpp"
#include "tm_timer.hpp"

#include <cstdlib>

// edit_modify 侧的上行回调：shadow 产生本地增量时调用。
// 本层拥有其存储：会话就绪时指向 broadcast_to_server，断开时清空。
void (*g_loro_broadcast_update) (string bytes)= nullptr;

namespace {

enum class collab_state {
  idle,
  connecting,
  await_doc,
  await_frame,
  ready,
  reconnecting,
};

enum class collab_mode { create, join };

class collab_session {
public:
  tm_websocket_client_impl* ws   = nullptr;
  collab_state              state= collab_state::idle;
  collab_mode               mode = collab_mode::create;
  string                    doc_id;
  string                    server_url;
  url                       buffer_url;
  bool                      buffer_known     = false;
  time_t                    await_frame_since= 0;
  bool                      want_reconnect   = false;
  time_t                    next_reconnect_at= 0;
  int                       reconnect_attempt= 0;
  array<string>             pending_updates;

  bool want_create () const { return mode == collab_mode::create; }
};

collab_session g_session;

void
broadcast_to_server (string bytes) {
  if (g_session.state == collab_state::ready && g_session.ws &&
      g_session.ws->connected ()) {
    if (DEBUG_LORO) debug_loro << "上行 " << N (bytes) << " 字节 update\n";
    g_session.ws->send (bytes, true);
    return;
  }
  if (g_session.want_reconnect) {
    g_session.pending_updates << bytes;
    if (DEBUG_LORO)
      debug_loro << "缓冲本地 update（" << N (bytes)
                 << " 字节），待重连后上行\n";
  }
}

static void
collab_set_message (string left) {
  get_server ()->set_message (tree (left), tree ("Collaborative"));
}

static time_t
reconnect_backoff (int attempt) {
  if (attempt <= 0) return 0;
  time_t ms= 1000L << (attempt - 1);
  if (ms <= 0 || ms > 30000L) ms= 30000L;
  return ms;
}

static void
schedule_reconnect () {
  time_t wait                = reconnect_backoff (g_session.reconnect_attempt);
  g_session.state            = collab_state::reconnecting;
  g_session.next_reconnect_at= texmacs_time () + wait;
  g_session.reconnect_attempt++;
  if (DEBUG_LORO)
    debug_loro << "调度重连：第 " << g_session.reconnect_attempt << " 次，"
               << wait << "ms 后\n";
  collab_set_message ("Connection lost; reconnecting… (attempt " *
                      as_string (g_session.reconnect_attempt) * ")");
}

class collab_ws_client : public tm_websocket_client_impl {
public:
  void on_connect () override {
    if (DEBUG_LORO)
      debug_loro << "已连接服务端 " << g_session.server_url << "\n";
    g_session.state= collab_state::await_doc;
    if (N (g_session.doc_id) > 0) send ("JOIN " * g_session.doc_id, false);
    else send ("CREATE", false);
  }

  void on_message (string data, bool is_binary) override {
    if (!is_binary) {
      if (starts (data, "DOC ")) {
        g_session.doc_id= data (4, N (data));
        if (DEBUG_LORO)
          debug_loro << "服务端确认文档 " << g_session.doc_id << "（mode="
                     << (g_session.want_create () ? "create" : "join")
                     << "）\n";
        if (g_session.want_create ()) become_ready ();
        else {
          g_session.state            = collab_state::await_frame;
          g_session.await_frame_since= texmacs_time ();
        }
      }
      else if (starts (data, "ERR ")) {
        std_error << "服务端错误: " << data << "\n";
        g_session.want_reconnect= false;
        g_session.state         = collab_state::idle;
        collab_set_message (data * " — stopped reconnecting");
      }
      return;
    }
    editor ed= get_current_editor ();
    if (is_nil (ed)) return;
    if (g_session.state == collab_state::await_frame) {
      ed->apply_remote (data);
      become_ready ();
    }
    else if (g_session.state == collab_state::ready) {
      ed->apply_remote (data);
    }
  }

  void on_error (string msg) override {
    std_error << "WS Error: " << msg << "\n";
  }
  void on_disconnect () override {
    if (DEBUG_LORO)
      debug_loro << "WS 断开（want_reconnect=" << g_session.want_reconnect
                 << ", state=" << (int) g_session.state << "）\n";
    if (g_session.want_reconnect) schedule_reconnect ();
    else g_session.state= collab_state::idle;
  }

  void become_ready () {
    bool was_reconnect         = g_session.reconnect_attempt > 0;
    g_session.state            = collab_state::ready;
    g_session.reconnect_attempt= 0;
    g_session.buffer_url       = get_current_buffer ();
    g_session.buffer_known     = true;
    g_loro_broadcast_update    = broadcast_to_server;
    editor ed                  = get_current_editor ();
    if (is_nil (ed)) std_error << "become_ready: 当前编辑器为空！\n";
    else ed->collab_enable ();
    if (N (g_session.pending_updates) > 0) {
      if (DEBUG_LORO)
        debug_loro << "重连后 flush " << N (g_session.pending_updates)
                   << " 条缓冲 update\n";
      for (int i= 0; i < N (g_session.pending_updates); i++)
        g_session.ws->send (g_session.pending_updates[i], true);
      g_session.pending_updates= array<string> ();
    }
    set_title_buffer (g_session.buffer_url, g_session.doc_id);
    collab_set_message (was_reconnect ? "Reconnected to " * g_session.doc_id
                                      : "Session ready: " * g_session.doc_id);
    if (DEBUG_LORO) debug_loro << "会话就绪 doc=" << g_session.doc_id << "\n";
  }
};

static void
collab_maybe_reconnect () {
  if (g_session.state != collab_state::reconnecting) return;
  if (texmacs_time () < g_session.next_reconnect_at) return;
  if (g_session.ws) {
    g_session.ws->disconnect ();
    delete g_session.ws;
    g_session.ws= nullptr;
  }
  if (DEBUG_LORO) debug_loro << "尝试重连 " << g_session.server_url << "\n";
  collab_set_message ("Reconnecting… (attempt " *
                      as_string (g_session.reconnect_attempt) * ")");
  g_session.ws   = new collab_ws_client ();
  g_session.state= collab_state::connecting;
  g_session.ws->connect (g_session.server_url);
}

} // namespace

string
loro_collab_create (string server_url) {
  if (g_session.ws) loro_collab_disconnect ();
  if (is_nil (get_current_editor ())) {
    std_error << "无当前编辑器，无法创建协作文档\n";
    return "";
  }
  g_session.mode             = collab_mode::create;
  g_session.doc_id           = "";
  g_session.server_url       = server_url;
  g_session.state            = collab_state::connecting;
  g_session.reconnect_attempt= 0;
  g_session.want_reconnect   = true;
  g_session.ws               = new collab_ws_client ();
  g_session.ws->connect (server_url);
  return "";
}

void
loro_collab_join (string server_url, string doc_id) {
  if (g_session.ws) loro_collab_disconnect ();
  if (is_nil (get_current_editor ())) {
    std_error << "无当前编辑器，无法加入协作文档\n";
    return;
  }
  g_session.mode             = collab_mode::join;
  g_session.doc_id           = doc_id;
  g_session.server_url       = server_url;
  g_session.state            = collab_state::connecting;
  g_session.reconnect_attempt= 0;
  g_session.want_reconnect   = true;
  g_session.ws               = new collab_ws_client ();
  g_session.ws->connect (server_url);
}

void
loro_collab_disconnect () {
  g_session.want_reconnect   = false;
  g_loro_broadcast_update    = nullptr;
  g_session.state            = collab_state::idle;
  g_session.doc_id           = "";
  g_session.reconnect_attempt= 0;
  g_session.buffer_known     = false;
  g_session.pending_updates  = array<string> ();
  if (g_session.ws) {
    g_session.ws->disconnect ();
    delete g_session.ws;
    g_session.ws= nullptr;
  }
}

bool
loro_collab_is_active () {
  return g_session.state == collab_state::ready;
}

string
loro_collab_doc_id () {
  return g_session.doc_id;
}

void
loro_collab_poll () {
  if (g_session.ws) g_session.ws->poll ();
  if (g_session.state != collab_state::idle && g_session.buffer_known &&
      is_nil (concrete_buffer (g_session.buffer_url))) {
    if (DEBUG_LORO) debug_loro << "协作 buffer 已关闭，断开会话\n";
    loro_collab_disconnect ();
    return;
  }
  if (g_session.state == collab_state::await_frame &&
      g_session.await_frame_since > 0 &&
      texmacs_time () - g_session.await_frame_since >= 1000) {
    std_error << "JOIN 超时未收到历史帧，按空文档就绪\n";
    g_session.await_frame_since= 0;
    static_cast<collab_ws_client*> (g_session.ws)->become_ready ();
  }
  collab_maybe_reconnect ();
}
