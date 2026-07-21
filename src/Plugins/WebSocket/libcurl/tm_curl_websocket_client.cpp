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

#if defined(CURL_VERSION_MAJOR)
#if LIBCURL_VERSION_NUM >= 0x075600 // 7.86.0 for websocket
#define MOGAN_HAS_CURL_WS 1
#endif
#endif

// Fallback in case header defines CURLWS_BINARY directly
#if !defined(MOGAN_HAS_CURL_WS) && defined(CURLWS_BINARY)
#define MOGAN_HAS_CURL_WS 1
#endif

#ifdef TM_WS_THREADED

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
  stop_requested.store (true);
  cv.notify_one ();
  worker.join ();
  if (is_connected.exchange (false)) {
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

#ifdef MOGAN_HAS_CURL_WS
  while (!tx_queue.empty ()) {
    ws_std_msg& msg      = tx_queue.front ();
    size_t      total_len= msg.data.size ();
    size_t      remaining= total_len - msg.offset;
    size_t      chunk_size=
        remaining > 65536
                 ? 65536
                 : remaining; // Send in 64KB chunks to avoid large buffer issues

    unsigned int flags   = msg.is_binary ? CURLWS_BINARY : CURLWS_TEXT;
    curl_off_t   fragsize= 0;

    if (remaining > chunk_size) {
      flags|= CURLWS_OFFSET;
    }

    if (msg.offset == 0 && total_len > chunk_size) {
      fragsize= total_len;
    }
    else {
      fragsize= 0;
    }

    size_t   sent= 0;
    CURLcode res = curl_ws_send (easy_handle, msg.data.data () + msg.offset,
                                 chunk_size, &sent, fragsize, flags);

    if (res == CURLE_AGAIN) {
      // Socket full, try again later
      break;
    }
    else if (res != CURLE_OK) {
      std::string err_msg= "Failed to send WebSocket message: ";
      err_msg+= curl_easy_strerror (res);
      push_error (std::move (err_msg));
      is_connected.store (false);
      conn_event.store (2);
      return;
    }

    msg.offset+= sent;
    if (msg.offset >= total_len) {
      // Finished this message
      tx_queue.pop_front ();
    }
    else {
      // Partial send, wait for next poll
      break;
    }
  }
#else
  if (!tx_queue.empty ()) {
    push_error ("libcurl version too old, websocket not supported");
    tx_queue.clear ();
  }
#endif
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

#ifdef MOGAN_HAS_CURL_WS
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
#endif

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

#else // !TM_WS_THREADED (Emscripten: single-threaded)

/******************************************************************************
 * Single-threaded implementation
 ******************************************************************************/

tm_curl_websocket_client::tm_curl_websocket_client ()
    : multi_handle (NULL), easy_handle (NULL), is_connected (false) {
  multi_handle= curl_multi_init ();
}

tm_curl_websocket_client::~tm_curl_websocket_client () {
  cleanup ();
  if (multi_handle) {
    curl_multi_cleanup (multi_handle);
    multi_handle= NULL;
  }
}

void
tm_curl_websocket_client::cleanup () {
  if (easy_handle) {
    if (multi_handle) {
      curl_multi_remove_handle (multi_handle, easy_handle);
    }
    curl_easy_cleanup (easy_handle);
    easy_handle= NULL;
  }
  is_connected= false;
  rx_buffer   = "";
}

void
tm_curl_websocket_client::connect (string url) {
  disconnect (); // cleanup any existing connection

  current_url= url;
  easy_handle= curl_easy_init ();
  if (!easy_handle) {
    on_error ("Failed to initialize libcurl easy handle");
    return;
  }

  c_string curl_url (url);
  curl_easy_setopt (easy_handle, CURLOPT_URL, (const char*) curl_url);
  curl_easy_setopt (easy_handle, CURLOPT_CONNECT_ONLY, 2L); // 2 = WebSocket

  curl_multi_add_handle (multi_handle, easy_handle);
}

void
tm_curl_websocket_client::disconnect () {
  if (is_connected || easy_handle) {
    cleanup ();
    on_disconnect ();
  }
}

void
tm_curl_websocket_client::send (string data, bool is_binary) {
  if (!is_connected || !easy_handle) return;
  tm_ws_msg msg;
  msg.data     = data;
  msg.is_binary= is_binary;
  msg.offset   = 0;
  tx_queue     = tx_queue * list<tm_ws_msg> (msg);
  process_tx_queue ();
}

void
tm_curl_websocket_client::process_tx_queue () {
  if (!is_connected || !easy_handle) return;

#ifdef MOGAN_HAS_CURL_WS
  while (!is_nil (tx_queue)) {
    tm_ws_msg& msg      = tx_queue->item;
    size_t     total_len= (size_t) N (msg.data);
    size_t     remaining= total_len - msg.offset;
    size_t     chunk_size=
        remaining > 65536
                ? 65536
                : remaining; // Send in 64KB chunks to avoid large buffer issues

    unsigned int flags   = msg.is_binary ? CURLWS_BINARY : CURLWS_TEXT;
    curl_off_t   fragsize= 0;

    if (remaining > chunk_size) {
      flags|= CURLWS_OFFSET;
    }

    if (msg.offset == 0 && total_len > chunk_size) {
      fragsize= total_len;
    }
    else {
      fragsize= 0;
    }

    size_t      sent= 0;
    const char* ptr = &msg.data[(int) msg.offset];
    CURLcode    res = curl_ws_send (easy_handle, (const void*) ptr, chunk_size,
                                    &sent, fragsize, flags);

    if (res == CURLE_AGAIN) {
      // Socket full, try again later
      break;
    }
    else if (res != CURLE_OK) {
      string err_msg= "Failed to send WebSocket message: ";
      err_msg << curl_easy_strerror (res);
      on_error (err_msg);
      disconnect ();
      break;
    }

    msg.offset+= sent;
    if (msg.offset >= total_len) {
      // Finished this message
      tx_queue= tx_queue->next;
    }
    else {
      // Partial send, wait for next poll
      break;
    }
  }
#else
  if (!is_nil (tx_queue)) {
    on_error ("libcurl version too old, websocket not supported");
    tx_queue= list<tm_ws_msg> ();
  }
#endif
}

void
tm_curl_websocket_client::poll () {
  if (!multi_handle || !easy_handle) return;

  int       still_running= 0;
  CURLMcode mc           = curl_multi_perform (multi_handle, &still_running);

  if (mc != CURLM_OK) {
    on_error ("curl_multi_perform failed");
    disconnect ();
    return;
  }

  // Check connection status
  int      msgs_in_queue= 0;
  CURLMsg* msg;
  while ((msg= curl_multi_info_read (multi_handle, &msgs_in_queue))) {
    if (msg->msg == CURLMSG_DONE) {
      if (msg->data.result == CURLE_OK) {
        if (!is_connected) {
          is_connected= true;
          on_connect ();
        }
      }
      else {
        string err_msg= "WebSocket connection failed: ";
        err_msg << curl_easy_strerror (msg->data.result);
        on_error (err_msg);
        disconnect ();
        return; // handle is cleaned up
      }
    }
  }

  // If connected, try to receive data
  if (is_connected) {
    process_tx_queue ();

#ifdef MOGAN_HAS_CURL_WS
    size_t                      recv_len= 0;
    const struct curl_ws_frame* meta    = NULL;
    char                        buffer[4096];

    // We might receive multiple frames or partial frames
    while (true) {
      CURLcode res=
          curl_ws_recv (easy_handle, buffer, sizeof (buffer), &recv_len, &meta);

      if (res == CURLE_OK && recv_len > 0) {
        // Append to our buffer
        string chunk (buffer, (int) recv_len);
        rx_buffer << chunk;

        if (meta && meta->bytesleft == 0 && !(meta->flags & CURLWS_CONT)) {
          // Full message received
          bool is_binary= true;
          if (meta->flags & CURLWS_TEXT) {
            is_binary= false;
          }

          string complete_msg= rx_buffer;
          rx_buffer          = "";

          if (meta->flags & CURLWS_CLOSE) {
            disconnect ();
            return;
          }

          if (!(meta->flags & CURLWS_PING) && !(meta->flags & CURLWS_PONG)) {
            on_message (complete_msg, is_binary);
          }
        }
      }
      else if (res == CURLE_AGAIN) {
        // No data available right now
        break;
      }
      else {
        // Error or closed
        on_error ("WebSocket receive error or connection closed");
        disconnect ();
        break;
      }
    }
#endif
  }
}

#endif // TM_WS_THREADED
