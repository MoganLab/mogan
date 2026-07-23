#ifndef LORO_COLLAB_INTERNAL_HPP
#define LORO_COLLAB_INTERNAL_HPP

#include "loro_collab.hpp"
#include "tm_websocket.hpp"
#include "url.hpp"

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
  tm_websocket_client* ws   = nullptr;
  collab_state         state= collab_state::idle;
  collab_mode          mode = collab_mode::create;
  string               doc_id;
  string               server_url;
  url                  buffer_url;
  bool                 buffer_known     = false;
  time_t               await_frame_since= 0;
  bool                 want_reconnect   = false;
  time_t               next_reconnect_at= 0;
  int                  reconnect_attempt= 0;
  array<string>        pending_updates;

  bool want_create () const { return mode == collab_mode::create; }
};

extern collab_session g_session;
extern void (*g_loro_broadcast_update) (string bytes);
extern void broadcast_to_server (string bytes);
extern void collab_set_message (string left);
extern void schedule_reconnect ();
extern void collab_become_ready ();

tm_websocket_client* create_collab_ws_client();

#endif // LORO_COLLAB_INTERNAL_HPP
