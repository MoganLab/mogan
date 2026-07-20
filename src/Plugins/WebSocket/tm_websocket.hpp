/******************************************************************************
 * MODULE     : tm_websocket.hpp
 * DESCRIPTION: Abstract non-blocking WebSocket client interface.
 *
 *              tm_websocket_client defines the transport-independent contract
 *              (connect/disconnect/send/poll/connected + on_* callbacks) so
 *              that each platform can plug in its own subclass (libcurl,
 *              emscripten WebSocket API, ...) without touching call sites.
 *              Concrete transports live in their own headers, e.g.
 *              tm_curl_websocket_client.hpp; tm_websocket_client_impl is the
 *              platform default implementation alias selected there.
 *
 *              Contract: all on_* callbacks fire on the GUI thread (from
 *              poll()).
 ******************************************************************************/

#ifndef TM_WEBSOCKET_HPP
#define TM_WEBSOCKET_HPP

#include "string.hpp"

/**
 * @brief Transport-independent WebSocket client interface.
 *
 * Concrete transports (libcurl, emscripten WebSocket API, ...) subclass this.
 * All on_* callbacks must fire on the GUI thread (from poll()).
 */
class tm_websocket_client {
public:
  tm_websocket_client ()         = default;
  virtual ~tm_websocket_client ()= default;

  virtual void connect (string url)                    = 0;
  virtual void disconnect ()                           = 0;
  virtual void send (string data, bool is_binary= true)= 0;

  // Must be called in the event loop (e.g. im_interpose or im_main_loop).
  // Threaded transports: drain the inbound queues and fire callbacks.
  // Single-threaded transports: drive the underlying socket directly.
  virtual void poll ()= 0;

  virtual bool connected () const= 0;

  // Virtual callbacks (override to handle); always fire on the GUI thread
  virtual void on_message (string data, bool is_binary) {}
  virtual void on_connect () {}
  virtual void on_disconnect () {}
  virtual void on_error (string msg) {}
};

#endif // TM_WEBSOCKET_HPP
