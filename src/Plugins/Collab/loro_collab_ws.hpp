#ifndef LORO_COLLAB_INTERNAL_HPP
#define LORO_COLLAB_INTERNAL_HPP

#include "loro_collab.hpp"
#include "tm_websocket.hpp"
#include "url.hpp"
#include <memory>

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
private:
  std::unique_ptr<tm_websocket_client> ws;
  collab_state                         state= collab_state::idle;
  collab_mode                          mode = collab_mode::create;
  string                               doc_id;
  string                               server_url;
  url                                  buffer_url;
  bool                                 buffer_known     = false;
  time_t                               await_frame_since= 0;
  bool                                 want_reconnect   = false;
  time_t                               next_reconnect_at= 0;
  int                                  reconnect_attempt= 0;
  array<string>                        pending_updates;
  // 多光标：本端 peer id（会话级稳定）、光标节流时间戳
  string peer_id;
  time_t last_cursor_send= 0;
  // 节流丢弃的光标/选区变化标记：被 50ms 节流挡下时置位，待 poll() 补发，
  // 保证拖动选区/取消选区的最终状态一定送达对端（参见 flush_cursor / poll）。
  bool cursor_dirty= false;

  void   set_message (string left);
  time_t reconnect_backoff (int attempt);
  void   become_ready ();

public:
  collab_session (url buf_url);
  ~collab_session ();

  bool want_create () const { return mode == collab_mode::create; }

  // Business logic API
  void create (string server_url);
  void join (string server_url, string doc_id);
  void disconnect ();
  void poll ();
  void broadcast (string bytes);
  void
  send_cursor (string payload); // 多光标：发文本帧 "CURSOR <peer> <payload>"
  void flush_cursor (
      bool force= false); // force=true 强制发（编辑后）；否则 ≥50ms 节流
  void schedule_reconnect ();
  void maybe_reconnect ();

  // State transitions
  void enter_idle ();
  void enter_connecting ();
  void enter_await_doc ();
  void enter_await_frame ();
  void enter_ready ();
  void enter_reconnecting ();

  // Getters
  bool         is_active () const { return state == collab_state::ready; }
  string       get_doc_id () const { return doc_id; }
  url          get_buffer_url () const { return buffer_url; }
  bool         is_buffer_known () const { return buffer_known; }
  class editor get_editor () const;

  // WS Callbacks
  void on_connect ();
  void on_message (string data, bool is_binary);
  void on_error (string msg);
  void on_disconnect ();
};

class collab_session_manager {
private:
  array<collab_session*> sessions;

public:
  ~collab_session_manager ();

  collab_session* find_by_buffer (url buffer_url);
  collab_session* get_or_create (url buffer_url);
  void            remove_session (collab_session* session);
  void            poll_all ();
};

extern collab_session_manager g_session_manager;
extern void (*g_loro_broadcast_update) (string bytes);
extern void (*g_loro_cursor_flush) ();
extern void broadcast_to_server (string bytes);
extern void flush_current_cursor ();

tm_websocket_client* create_collab_ws_client (collab_session* session);

#endif // LORO_COLLAB_INTERNAL_HPP
