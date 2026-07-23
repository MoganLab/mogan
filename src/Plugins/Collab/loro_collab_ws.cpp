/******************************************************************************
 * MODULE     : loro_collab_ws.cpp
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "loro_collab_internal.hpp"

#ifdef OS_WASM
#include "tm_emscripten_websocket_client.hpp"
#else
#include "tm_curl_websocket_client.hpp"
#endif

#include "editor.hpp"
#include "tm_timer.hpp"

class collab_ws_client : public tm_websocket_client_impl {
public:
  void on_connect () override {
    if (DEBUG_LORO)
      debug_loro << "已连接服务端 " << g_session.server_url << "\n";
    g_session.state = collab_state::await_doc;
    if (N (g_session.doc_id) > 0) send ("JOIN " * g_session.doc_id, false);
    else send ("CREATE", false);
  }

  void on_message (string data, bool is_binary) override {
    if (!is_binary) {
      if (starts (data, "DOC ")) {
        g_session.doc_id = data (4, N (data));
        if (DEBUG_LORO)
          debug_loro << "服务端确认文档 " << g_session.doc_id << "（mode="
                     << (g_session.want_create () ? "create" : "join")
                     << "）\n";
        if (g_session.want_create ()) collab_become_ready ();
        else {
          g_session.state             = collab_state::await_frame;
          g_session.await_frame_since = texmacs_time ();
        }
      }
      else if (starts (data, "ERR ")) {
        std_error << "服务端错误: " << data << "\n";
        g_session.want_reconnect = false;
        g_session.state          = collab_state::idle;
        collab_set_message (data * " — stopped reconnecting");
      }
      return;
    }
    editor ed = get_current_editor ();
    if (is_nil (ed)) return;
    if (g_session.state == collab_state::await_frame) {
      ed->apply_remote (data);
      collab_become_ready ();
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
    else g_session.state = collab_state::idle;
  }
};

tm_websocket_client*
create_collab_ws_client () {
  return new collab_ws_client ();
}
