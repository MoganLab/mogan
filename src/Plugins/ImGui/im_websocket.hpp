/******************************************************************************
 * MODULE     : im_websocket.hpp
 * DESCRIPTION: A single-threaded, non-blocking WebSocket client using libcurl
 ******************************************************************************/

#ifndef IM_WEBSOCKET_HPP
#define IM_WEBSOCKET_HPP

#include "list.hpp"
#include "string.hpp"
#include <curl/curl.h>

struct im_ws_msg {
  string data;
  bool   is_binary;
  size_t offset;
};

class im_websocket_client {
private:
  CURLM* multi_handle;
  CURL*  easy_handle;
  bool   is_connected;
  string current_url;
  string rx_buffer; // Buffer for partial frames

  list<im_ws_msg> tx_queue;

  // Internal helpers
  void cleanup ();
  void process_tx_queue ();

public:
  im_websocket_client ();
  virtual ~im_websocket_client ();

  void connect (string url);
  void disconnect ();
  void send (string data, bool is_binary= true);

  // Must be called in the event loop (e.g. im_interpose or im_main_loop)
  void poll ();

  bool connected () const { return is_connected; }

  // Virtual callbacks (override to handle)
  virtual void on_message (string data, bool is_binary) {}
  virtual void on_connect () {}
  virtual void on_disconnect () {}
  virtual void on_error (string msg) {}
};

#endif // IM_WEBSOCKET_HPP
