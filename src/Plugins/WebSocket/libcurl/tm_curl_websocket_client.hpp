/******************************************************************************
 * MODULE: tm_curl_websocket_client.hpp
 * DESCRIPTION: 使用 libcurl 实现的 WebSocket 客户端接口。
 * AUTHOR: JimZhouZZY
 * DATE:   July, 2026
 ******************************************************************************/

#ifndef TM_CURL_WEBSOCKET_CLIENT_HPP
#define TM_CURL_WEBSOCKET_CLIENT_HPP

#include "list.hpp"
#include "tm_websocket.hpp"
#include <curl/curl.h>

// lolly 的数据类型线程不安全，因此使用了 C++ 标准库的线程原语和容器
// 要求工作线程必须完全不依赖 lolly 类型
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

struct tm_ws_msg {
  string data;
  bool   is_binary;
  size_t offset;
};

/**
 * @brief 基于 libcurl 的 WebSocket 客户端实现。
 *
 * 在原生平台上，libcurl 运行于后台工作线程；
 */
class tm_curl_websocket_client : public tm_websocket_client {
private:
  CURLM* multi_handle;
  CURL*  easy_handle;
  string current_url;

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

  // Internal helpers
  void cleanup ();

public:
  tm_curl_websocket_client ();
  ~tm_curl_websocket_client () override;

  void connect (string url) override;
  void disconnect () override;
  void send (string data, bool is_binary= true) override;
  void poll () override;

  bool connected () const override { return is_connected.load (); }
};

typedef tm_curl_websocket_client tm_websocket_client_impl;

#endif // TM_CURL_WEBSOCKET_CLIENT_HPP
