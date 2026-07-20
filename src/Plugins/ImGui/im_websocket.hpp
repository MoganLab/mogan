/******************************************************************************
 * MODULE     : im_websocket.hpp
 * DESCRIPTION: A non-blocking WebSocket client using libcurl.
 *              Native platforms (IM_WS_THREADED): all libcurl activity runs on
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

#ifndef IM_WEBSOCKET_HPP
#define IM_WEBSOCKET_HPP

#include "list.hpp"
#include "string.hpp"
#include <curl/curl.h>

#if !defined(__EMSCRIPTEN__)
#define IM_WS_THREADED 1
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

struct im_ws_msg {
  string data;
  bool   is_binary;
  size_t offset;
};

class im_websocket_client {
private:
  CURLM* multi_handle;
  CURL*  easy_handle;
  string current_url;

#ifdef IM_WS_THREADED
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
  list<im_ws_msg> tx_queue;

  void process_tx_queue ();
#endif

  // Internal helpers
  void cleanup ();

public:
  im_websocket_client ();
  virtual ~im_websocket_client ();

  void connect (string url);
  void disconnect ();
  void send (string data, bool is_binary= true);

  // Must be called in the event loop (e.g. im_interpose or im_main_loop).
  // Threaded path: drains the inbound queues and fires callbacks (GUI thread).
  // Single-threaded path: drives libcurl directly.
  void poll ();

#ifdef IM_WS_THREADED
  bool connected () const { return is_connected.load (); }
#else
  bool connected () const { return is_connected; }
#endif

  // Virtual callbacks (override to handle); always fire on the GUI thread
  virtual void on_message (string data, bool is_binary) {}
  virtual void on_connect () {}
  virtual void on_disconnect () {}
  virtual void on_error (string msg) {}
};

#endif // IM_WEBSOCKET_HPP
