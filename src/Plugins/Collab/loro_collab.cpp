/******************************************************************************
 * MODULE     : loro_collab.cpp
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "loro_collab_internal.hpp"
#include "editor.hpp"
#include "server.hpp"
#include "tm_buffer.hpp"
#include "tm_timer.hpp"
#include "url.hpp"

void (*g_loro_broadcast_update) (string bytes) = nullptr;
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

void
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

void
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

void
collab_become_ready () {
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

static void
collab_maybe_reconnect () {
  if (g_session.state != collab_state::reconnecting) return;
  if (texmacs_time () < g_session.next_reconnect_at) return;
  if (g_session.ws) {
    g_session.ws->disconnect ();
    delete g_session.ws;
    g_session.ws = nullptr;
  }
  if (DEBUG_LORO) debug_loro << "尝试重连 " << g_session.server_url << "\n";
  collab_set_message ("Reconnecting… (attempt " *
                      as_string (g_session.reconnect_attempt) * ")");
  g_session.ws    = create_collab_ws_client ();
  g_session.state = collab_state::connecting;
  g_session.ws->connect (g_session.server_url);
}

string
loro_collab_create (string server_url) {
  if (g_session.ws) loro_collab_disconnect ();
  if (is_nil (get_current_editor ())) {
    std_error << "无当前编辑器，无法创建协作文档\n";
    return "";
  }
  g_session.mode              = collab_mode::create;
  g_session.doc_id            = "";
  g_session.server_url        = server_url;
  g_session.state             = collab_state::connecting;
  g_session.reconnect_attempt = 0;
  g_session.want_reconnect    = true;
  g_session.ws                = create_collab_ws_client ();
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
  g_session.mode              = collab_mode::join;
  g_session.doc_id            = doc_id;
  g_session.server_url        = server_url;
  g_session.state             = collab_state::connecting;
  g_session.reconnect_attempt = 0;
  g_session.want_reconnect    = true;
  g_session.ws                = create_collab_ws_client ();
  g_session.ws->connect (server_url);
}

void
loro_collab_disconnect () {
  g_session.want_reconnect    = false;
  g_loro_broadcast_update     = nullptr;
  g_session.state             = collab_state::idle;
  g_session.doc_id            = "";
  g_session.reconnect_attempt = 0;
  g_session.buffer_known      = false;
  g_session.pending_updates   = array<string> ();
  if (g_session.ws) {
    g_session.ws->disconnect ();
    delete g_session.ws;
    g_session.ws = nullptr;
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
    g_session.await_frame_since = 0;
    collab_become_ready ();
  }
  collab_maybe_reconnect ();
}
