/******************************************************************************
 * MODULE     : tm_emscripten_websocket_client.hpp
 * DESCRIPTION: emscripten WebSocket API implementation of tm_websocket_client
 *              for WASM builds (browser-native WebSocket).
 *
 *              Single-threaded: the browser event callbacks only enqueue
 *              std::-container events; poll() drains the queue on the GUI
 *              thread and fires the on_* callbacks there, keeping the same
 *              contract as the libcurl implementation. Never touches lolly
 *              types off the GUI thread.
 ******************************************************************************/

#ifndef TM_EMSCRIPTEN_WEBSOCKET_CLIENT_HPP
#define TM_EMSCRIPTEN_WEBSOCKET_CLIENT_HPP

#include "tm_websocket.hpp"

#include <emscripten/websocket.h>

#include <deque>
#include <string>

/**
 * @brief WebSocket client backed by the emscripten WebSocket API.
 *
 * Browser events are marshalled into an std::deque and dispatched from
 * poll() on the GUI thread, mirroring the libcurl client's contract.
 */
class tm_emscripten_websocket_client : public tm_websocket_client {
private:
  int  handle; // EMSCRIPTEN_WEBSOCKET_T, 0 = no socket
  bool is_connected;
  bool close_notified; // onclose already queued: ignore further events

  struct ws_evt {
    int         kind; // 0 message, 1 open, 2 close, 3 error
    std::string data;
    bool        is_binary;
  };
  std::deque<ws_evt> events;

  void queue_event (int kind, const char* data, int num_bytes, bool is_binary);

  static EM_BOOL handle_open_cb (int                                 eventType,
                                 const EmscriptenWebSocketOpenEvent* e,
                                 void*                               userData);
  static EM_BOOL handle_message_cb (int eventType,
                                    const EmscriptenWebSocketMessageEvent* e,
                                    void* userData);
  static EM_BOOL handle_error_cb (int eventType,
                                  const EmscriptenWebSocketErrorEvent* e,
                                  void* userData);
  static EM_BOOL handle_close_cb (int eventType,
                                  const EmscriptenWebSocketCloseEvent* e,
                                  void* userData);

public:
  tm_emscripten_websocket_client ();
  ~tm_emscripten_websocket_client () override;

  void connect (string url) override;
  void disconnect () override;
  void send (string data, bool is_binary= true) override;
  void poll () override;

  bool connected () const override { return is_connected; }
};

// Platform default transport (WASM).
typedef tm_emscripten_websocket_client tm_websocket_client_impl;

#endif // TM_EMSCRIPTEN_WEBSOCKET_CLIENT_HPP
