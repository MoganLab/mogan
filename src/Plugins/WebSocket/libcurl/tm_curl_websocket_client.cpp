/******************************************************************************
 * MODULE     : tm_curl_websocket_client.cpp
 * DESCRIPTION: libcurl implementation of tm_websocket_client
 *              (tm_curl_websocket_client). Native platforms run libcurl on a
 *              background worker thread; Emscripten keeps the single-threaded
 *              poll() driver. See tm_websocket.hpp for the interface and the
 *              threading invariants.
 ******************************************************************************/

#include "tm_curl_websocket_client.hpp"
#include "basic.hpp" // for cout, debug_std
#include <cstdio>
#include <cstdlib>
#include <poll.h>

/******************************************************************************
 * Threaded implementation (native platforms)
 *
 * The worker thread never touches lolly types: it builds c-strings and
 * std::strings on its stack, and all queue payloads are std::. Conversion to
 * lolly string happens on the GUI thread in send/poll. This keeps the
 * process-global (not thread-safe) fast_alloc allocator single-threaded.
 ******************************************************************************/

tm_curl_websocket_client::tm_curl_websocket_client ()
    : multi_handle (NULL), easy_handle (NULL) {
  // curl handles are created/used/destroyed on the worker thread only
}

tm_curl_websocket_client::~tm_curl_websocket_client () { disconnect (); }

void
tm_curl_websocket_client::push_error (std::string msg) {
  std::lock_guard<std::mutex> lk (q_mutex);
  err_pending.push_back (std::move (msg));
}

void
tm_curl_websocket_client::connect (string url) {
  disconnect (); // join any previous worker first

  current_url= url;
  stop_requested.store (false);
  conn_event.store (0);
  worker= std::thread ([this, u= std::string ((const char*) c_string (url))] {
    worker_main (std::move (u));
  });
}

void
tm_curl_websocket_client::disconnect () {
  if (!worker.joinable ()) return;
  bool was_connected= is_connected.exchange (false);
  stop_requested.store (true);
  cv.notify_one ();
  worker.join ();
  if (was_connected) {
    conn_event.store (2); // dispatch on_disconnect from the next poll()
  }
}

void
tm_curl_websocket_client::send (string data, bool is_binary) {
  if (!is_connected.load ()) return;
  {
    std::lock_guard<std::mutex> lk (q_mutex);
    ws_std_msg                  msg;
    msg.data.assign ((const char*) c_string (data), (size_t) N (data));
    msg.is_binary= is_binary;
    msg.offset   = 0;
    out_pending.push_back (std::move (msg));
  }
  cv.notify_one ();
}

void
tm_curl_websocket_client::poll () {
  std::deque<ws_std_msg>  msgs;
  std::deque<std::string> errs;
  int                     ev;
  {
    std::lock_guard<std::mutex> lk (q_mutex);
    msgs.swap (in_pending);
    errs.swap (err_pending);
    ev= conn_event.exchange (0);
  }

  if (ev == 1) on_connect ();
  for (std::deque<std::string>::iterator it= errs.begin (); it != errs.end ();
       ++it)
    on_error (string (it->data (), (int) it->size ()));
  for (ws_std_msg_iter it= msgs.begin (); it != msgs.end (); ++it)
    on_message (string (it->data.data (), (int) it->data.size ()),
                it->is_binary);
  // disconnect fires last so queued messages are not dropped
  if (ev == 2) on_disconnect ();
}

void
tm_curl_websocket_client::cleanup () {
  // worker-side only: handles live and die on the worker thread
  if (easy_handle) {
    if (multi_handle) {
      curl_multi_remove_handle (multi_handle, easy_handle);
    }
    curl_easy_cleanup (easy_handle);
    easy_handle= NULL;
  }
  if (multi_handle) {
    curl_multi_cleanup (multi_handle);
    multi_handle= NULL;
  }
  rx_buffer.clear ();
}

void
tm_curl_websocket_client::process_tx_queue () {
  // worker-side only; on failure reports via push_error and drops the
  // connection state so worker_main's loop exits
  if (!is_connected.load () || !easy_handle) return;

  // 发送前用 poll(POLLOUT) 确认 socket 可写，避免在 socket 已满时调用
  // curl_ws_send——其内部 ws_flush(complete=TRUE) 在 EAGAIN 下会 n=0;continue
  // 无限忙转、锁死 worker 线程（大帧 + LAN 背压下复现：worker 卡死、服务端只收
  // 到半帧且永不完成；环回无背压故不复现）。
  curl_socket_t sockfd= CURL_SOCKET_BAD;
  curl_easy_getinfo (easy_handle, CURLINFO_ACTIVESOCKET, &sockfd);

  while (!tx_queue.empty ()) {
    ws_std_msg&  msg       = tx_queue.front ();
    size_t       total_len = msg.data.size ();
    unsigned int base_flags= msg.is_binary ? CURLWS_BINARY : CURLWS_TEXT;
    // 用 CURLWS_OFFSET 把一条消息作为「单个 WebSocket 帧」分多次 curl_ws_send
    // 发送：首次以 fragsize=total 声明整帧长度并写一次帧头（带 MASK+FIN），其后
    // fragsize=0 仅续写同一帧的已掩码载荷（mask 偏移 xori 跨调用延续）。
    //
    // 三个关键点（均只在 LAN 大帧背压下暴露，环回不背压故不复现）：
    //  1) 每次 buflen 限 64KB，与 libcurl sendbuf（~128KB 固定容量）量级匹配，
    //     避免一次把数十万字节制进 sendbuf 致 ws_flush/掩码异常（曾报 MASK
    //     错）。
    //  2) header_written 保证帧头只写一次：重试（CURLE_AGAIN）时不重置
    //     fragsize=total，杜绝「重复帧头」损坏帧流（曾报 RSV2/RSV3）。
    //  3) 无论 OK 还是 AGAIN 都按 sent 推进 offset，与 libcurl 的
    //  payload_remain
    //     同步，避免重传已入帧字节造成载荷重复/缺失（曾报 Checksum mismatch）。
    while (msg.offset < total_len) {
      if (stop_requested.load ()) return; // 让 disconnect() 能及时中断大帧发送
      // 等 socket 可写（最多 25ms）；不可写则交还 worker 循环做 recv/重试，
      // 绝不在 socket 满时盲目 curl_ws_send（否则 ws_flush 忙转锁死 worker）。
      if (sockfd != CURL_SOCKET_BAD) {
        struct pollfd pfd;
        pfd.fd     = sockfd;
        pfd.events = POLLOUT;
        pfd.revents= 0;
        int pr     = ::poll (&pfd, 1, 25);
        // socket 出错（对端关闭/连接断）：surface
        // 为断连，避免误判为「等可写」永悬
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
          push_error (
              std::string ("WebSocket send: socket closed (poll revents=") +
              std::to_string (pfd.revents) + ")");
          is_connected.store (false);
          conn_event.store (2);
          return;
        }
        if (pr <= 0 || !(pfd.revents & POLLOUT)) return;
      }
      size_t       remaining= total_len - msg.offset;
      size_t       chunk    = remaining > 65536 ? 65536 : remaining;
      unsigned int flags    = base_flags | CURLWS_OFFSET;
      curl_off_t   fragsz   = msg.header_written ? 0 : (curl_off_t) total_len;
      size_t       sent     = 0;
      CURLcode res= curl_ws_send (easy_handle, msg.data.data () + msg.offset,
                                  chunk, &sent, fragsz, flags);
      msg.offset+= sent; // 已入帧字节（OK 与 AGAIN 均有效）
      if (fragsz > 0 && (res == CURLE_OK || res == CURLE_AGAIN))
        msg.header_written=
            true; // 帧头已写入 sendbuf（即便 ws_flush 返回 AGAIN）
      if (total_len > 65536 && getenv ("MOGAN_LORO_DEBUG"))
        std::fprintf (stderr, "[ws-tx] total=%zu off=%zu sent=%zu res=%d%s\n",
                      total_len, msg.offset, sent, (int) res,
                      fragsz > 0 ? " head" : "");
      if (res == CURLE_AGAIN) return; // socket 满：交还 worker 循环续写
      if (res != CURLE_OK) {
        std::string err_msg= "Failed to send WebSocket message: ";
        err_msg+= curl_easy_strerror (res);
        push_error (std::move (err_msg));
        is_connected.store (false);
        conn_event.store (2);
        return;
      }
      if (sent == 0) return; // 无进展：交还 worker 循环
    }
    tx_queue.pop_front (); // 本帧已整体发完
    if (total_len > 65536 && getenv ("MOGAN_LORO_DEBUG"))
      std::fprintf (stderr, "[ws-tx] large send done: %zu bytes\n", total_len);
  }
}

void
tm_curl_websocket_client::worker_main (std::string url) {
  multi_handle= curl_multi_init ();
  easy_handle = curl_easy_init ();
  if (!multi_handle || !easy_handle) {
    push_error ("Failed to initialize libcurl handles");
    conn_event.store (2);
    cleanup ();
    return;
  }

  curl_easy_setopt (easy_handle, CURLOPT_URL, url.c_str ());
  curl_easy_setopt (easy_handle, CURLOPT_CONNECT_ONLY, 2L); // 2 = WebSocket
  // dev/测试：接受自签证书（mkcert 等）。协作服务端常配自签 TLS，libcurl 默认
  // VERIFYPEER 会拒绝（自签 CA 不在信任链），导致 wss:// 握手失败。浏览器
  // （WASM）因 mkcert -install 信任了系统 CA 故能过。
  curl_easy_setopt (easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt (easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_multi_add_handle (multi_handle, easy_handle);

  // Phase 1: handshake, driven by curl_multi_poll (no fixed sleep)
  bool handshake_done= false;
  bool handshake_fail= false;
  while (!stop_requested.load () && !handshake_done && !handshake_fail) {
    int       still_running= 0;
    CURLMcode mc           = curl_multi_perform (multi_handle, &still_running);
    if (mc != CURLM_OK) {
      push_error ("curl_multi_perform failed");
      handshake_fail= true;
      break;
    }

    int      msgs_in_queue= 0;
    CURLMsg* msg;
    while ((msg= curl_multi_info_read (multi_handle, &msgs_in_queue))) {
      if (msg->msg == CURLMSG_DONE) {
        if (msg->data.result == CURLE_OK) {
          is_connected.store (true);
          conn_event.store (1);
          handshake_done= true;
        }
        else {
          std::string err_msg= "WebSocket connection failed: ";
          err_msg+= curl_easy_strerror (msg->data.result);
          push_error (std::move (err_msg));
          handshake_fail= true;
        }
      }
    }

    if (!handshake_done && !handshake_fail) {
      curl_multi_poll (multi_handle, NULL, 0, 100, NULL);
    }
  }

  if (handshake_fail || (stop_requested.load () && !handshake_done)) {
    is_connected.store (false);
    conn_event.store (2);
    cleanup ();
    return;
  }

  // Phase 2: connected. The easy handle was removed from the multi handle by
  // CONNECT_ONLY, so curl_multi_poll no longer fires on ws traffic; poll the
  // socket non-blocking and wait on cv between rounds (woken early by send).
  while (!stop_requested.load () && is_connected.load ()) {
    {
      std::lock_guard<std::mutex> lk (q_mutex);
      while (!out_pending.empty ()) {
        tx_queue.push_back (std::move (out_pending.front ()));
        out_pending.pop_front ();
      }
    }
    process_tx_queue ();
    if (!is_connected.load ()) break;

    size_t                      recv_len= 0;
    const struct curl_ws_frame* meta    = NULL;
    char                        buffer[4096];

    // We might receive multiple frames or partial frames
    while (true) {
      CURLcode res=
          curl_ws_recv (easy_handle, buffer, sizeof (buffer), &recv_len, &meta);

      if (res == CURLE_OK && recv_len > 0) {
        rx_buffer.append (buffer, recv_len);

        if (meta && meta->bytesleft == 0 && !(meta->flags & CURLWS_CONT)) {
          // Full message received
          bool is_binary= true;
          if (meta->flags & CURLWS_TEXT) {
            is_binary= false;
          }

          if (meta->flags & CURLWS_CLOSE) {
            rx_buffer.clear ();
            is_connected.store (false);
            conn_event.store (2);
            break;
          }

          if (!(meta->flags & CURLWS_PING) && !(meta->flags & CURLWS_PONG)) {
            std::lock_guard<std::mutex> lk (q_mutex);
            in_pending.push_back (
                ws_std_msg{std::move (rx_buffer), is_binary, 0});
          }
          rx_buffer.clear ();
        }
      }
      else if (res == CURLE_AGAIN) {
        // No data available right now
        break;
      }
      else {
        // Error or closed
        push_error ("WebSocket receive error or connection closed");
        is_connected.store (false);
        conn_event.store (2);
        break;
      }
    }

    {
      std::unique_lock<std::mutex> lk (q_mutex);
      cv.wait_for (lk, std::chrono::milliseconds (5), [&] {
        return stop_requested.load () || !out_pending.empty () ||
               !is_connected.load ();
      });
    }
  }

  is_connected.store (false);
  cleanup ();
}
