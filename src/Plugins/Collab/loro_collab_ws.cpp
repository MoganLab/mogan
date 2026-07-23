/******************************************************************************
 * MODULE     : loro_collab_ws.cpp
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "loro_collab_ws.hpp"

#ifdef OS_WASM
#include "tm_emscripten_websocket_client.hpp"
#else
#include "tm_curl_websocket_client.hpp"
#endif

class collab_ws_client : public tm_websocket_client_impl {
private:
  collab_session* session;

public:
  collab_ws_client (collab_session* s) : session (s) {}

  void on_connect () override { session->on_connect (); }

  void on_message (string data, bool is_binary) override {
    session->on_message (data, is_binary);
  }

  void on_error (string msg) override { session->on_error (msg); }

  void on_disconnect () override { session->on_disconnect (); }
};

tm_websocket_client*
create_collab_ws_client (collab_session* session) {
  return new collab_ws_client (session);
}
