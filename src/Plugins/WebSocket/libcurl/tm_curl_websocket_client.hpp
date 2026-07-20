/******************************************************************************
 * MODULE     : tm_curl_websocket_client.hpp
 * DESCRIPTION: libcurl implementation of tm_websocket_client.
 *
 *              Native platforms (TM_WS_THREADED): all libcurl activity runs on
 *              a background worker thread; the GUI thread only exchanges
 *              complete messages through a mutex-protected std::deque in
 *              send/poll. Emscripten: single-threaded, poll() drives libcurl
 *              directly.
 *
 *              Threading invariants (threaded path, MUST NOT be violated):
 *              - The lolly allocator (fast_alloc) is process-global and NOT
 *                thread-safe. The worker thread must NEVER construct, copy,
 *                destroy or otherwise touch any lolly type (string, list,
 *                c_string, ...). All cross-thread data is carried in std::
 *                containers; conversion to lolly string happens on the GUI
 *                thread inside poll().
 *              - cout (tm_ostream) is not thread-safe: the worker must not
 *                print; errors are queued and dispatched from poll() on the
 *                GUI thread.
 *              - All on_* callbacks fire from poll() on the GUI thread.
 ******************************************************************************/

#ifndef TM_CURL_WEBSOCKET_CLIENT_HPP
#define TM_CURL_WEBSOCKET_CLIENT_HPP

#include "list.hpp"
#include "tm_websocket.hpp"
#include <curl/curl.h>

#if !defined(__EMSCRIPTEN__)
#define TM_WS_THREADED 1
// std threading primitives and containers are an intentional exception to the
// no-std rule: the worker thread must stay entirely off lolly types (see
// invariants above), so the exchange queues are pure std.
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#endif

struct tm_ws_msg {
  string data;
  bool   is_binary;
  size_t offset;
};

/**
 * @brief WebSocket client backed by libcurl.
 *
 * Native platforms run libcurl on a background worker thread; Emscripten
 * keeps the single-threaded poll() driver. See the threading invariants in
 * the module header.
 */
class tm_curl_websocket_client : public tm_websocket_client {
private:
  CURLM* multi_handle;
  CURL*  easy_handle;
  string current_url;

#ifdef TM_WS_THREADED
  struct ws_std_msg {
    std::string data;
    bool        is_binary;
    size_t      offset;
  };
  typedef std::deque<ws_std_msg>::iterator ws_std_msg_iter;

  std::string rx_buffer; // partial frames, worker-only

  std::deque<ws_std_msg>  out_pending; // GUI -> worker: outbound messages
  std::deque<ws_std_msg>  in_pending;  // worker -> GUI: inbound messages
  std::deque<ws_std_msg>  tx_queue;    // partial-send progress, worker-only
  std::deque<std::string> err_pending; // worker -> GUI: error strings

  std::mutex              q_mutex;
  std::condition_variable cv; // wakes worker on send/quit
  std::thread             worker;
  std::atomic<bool>       is_connected{false};
  std::atomic<bool>       stop_requested{false};
  std::atomic<int>        conn_event{0}; // 0 none, 1 connected, 2 disconnected

  void worker_main (std::string url);
  void push_error (std::string msg); // worker-side: enqueue under q_mutex
  void process_tx_queue ();          // worker-side
#else
  bool            is_connected;
  string          rx_buffer; // Buffer for partial frames
  list<tm_ws_msg> tx_queue;

  void process_tx_queue ();
#endif

  // Internal helpers
  void cleanup ();

public:
  tm_curl_websocket_client ();
  ~tm_curl_websocket_client () override;

  void connect (string url) override;
  void disconnect () override;
  void send (string data, bool is_binary= true) override;
  void poll () override;

#ifdef TM_WS_THREADED
  bool connected () const override { return is_connected.load (); }
#else
  bool connected () const override { return is_connected; }
#endif
};

// Platform default transport. Emscripten will switch this to the emscripten
// WebSocket API client once that subclass exists.
typedef tm_curl_websocket_client tm_websocket_client_impl;

#endif // TM_CURL_WEBSOCKET_CLIENT_HPP
