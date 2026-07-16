/******************************************************************************
 * MODULE     : im_websocket.cpp
 * DESCRIPTION: A single-threaded, non-blocking WebSocket client using libcurl
 ******************************************************************************/

#include "im_websocket.hpp"
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

im_websocket_client::im_websocket_client ()
    : multi_handle (NULL), easy_handle (NULL), is_connected (false) {
  multi_handle = curl_multi_init ();
}

im_websocket_client::~im_websocket_client () {
  cleanup ();
  if (multi_handle) {
    curl_multi_cleanup (multi_handle);
    multi_handle = NULL;
  }
}

void
im_websocket_client::cleanup () {
  if (easy_handle) {
    if (multi_handle) {
      curl_multi_remove_handle (multi_handle, easy_handle);
    }
    curl_easy_cleanup (easy_handle);
    easy_handle = NULL;
  }
  is_connected = false;
  rx_buffer = "";
}

void
im_websocket_client::connect (string url) {
  disconnect (); // cleanup any existing connection

  current_url = url;
  easy_handle = curl_easy_init ();
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
im_websocket_client::disconnect () {
  if (is_connected || easy_handle) {
    cleanup ();
    on_disconnect ();
  }
}

void
im_websocket_client::send (string data, bool is_binary) {
  if (!is_connected || !easy_handle) return;
  im_ws_msg msg;
  msg.data = data;
  msg.is_binary = is_binary;
  msg.offset = 0;
  tx_queue = tx_queue * list<im_ws_msg> (msg);
  process_tx_queue ();
}

void
im_websocket_client::process_tx_queue () {
  if (!is_connected || !easy_handle) return;

#ifdef MOGAN_HAS_CURL_WS
  while (!is_nil (tx_queue)) {
    im_ws_msg& msg = tx_queue->item;
    size_t total_len = (size_t) N (msg.data);
    size_t remaining = total_len - msg.offset;
    size_t chunk_size = remaining > 65536 ? 65536 : remaining; // Send in 64KB chunks to avoid large buffer issues
    
    unsigned int flags = msg.is_binary ? CURLWS_BINARY : CURLWS_TEXT;
    curl_off_t fragsize = 0;
    
    if (remaining > chunk_size) {
      flags |= CURLWS_OFFSET;
    }
    
    if (msg.offset == 0 && total_len > chunk_size) {
      fragsize = total_len;
    } else {
      fragsize = 0;
    }

    size_t sent = 0;
    const char* ptr = &msg.data[(int)msg.offset];
    CURLcode res = curl_ws_send (easy_handle, (const void*) ptr, chunk_size, &sent, fragsize, flags);

    if (res == CURLE_AGAIN) {
      // Socket full, try again later
      break;
    } else if (res != CURLE_OK) {
      string err_msg = "Failed to send WebSocket message: ";
      err_msg << curl_easy_strerror(res);
      on_error (err_msg);
      disconnect ();
      break;
    }
    
    msg.offset += sent;
    if (msg.offset >= total_len) {
      // Finished this message
      tx_queue = tx_queue->next;
    } else {
      // Partial send, wait for next poll
      break;
    }
  }
#else
  if (!is_nil(tx_queue)) {
    on_error ("libcurl version too old, websocket not supported");
    tx_queue = list<im_ws_msg>();
  }
#endif
}

void
im_websocket_client::poll () {
  if (!multi_handle || !easy_handle) return;

  int still_running = 0;
  CURLMcode mc = curl_multi_perform (multi_handle, &still_running);

  if (mc != CURLM_OK) {
    on_error ("curl_multi_perform failed");
    disconnect ();
    return;
  }

  // Check connection status
  int msgs_in_queue = 0;
  CURLMsg* msg;
  while ((msg = curl_multi_info_read (multi_handle, &msgs_in_queue))) {
    if (msg->msg == CURLMSG_DONE) {
      if (msg->data.result == CURLE_OK) {
        if (!is_connected) {
          is_connected = true;
          on_connect ();
        }
      } else {
        string err_msg = "WebSocket connection failed: ";
        err_msg << curl_easy_strerror(msg->data.result);
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
    size_t recv_len = 0;
    const struct curl_ws_frame *meta = NULL;
    char buffer[4096];
    
    // We might receive multiple frames or partial frames
    while (true) {
      CURLcode res = curl_ws_recv (easy_handle, buffer, sizeof(buffer), &recv_len, &meta);
      
      if (res == CURLE_OK && recv_len > 0) {
        // Append to our buffer
        string chunk(buffer, (int) recv_len);
        rx_buffer << chunk;
        
        if (meta && meta->bytesleft == 0 && !(meta->flags & CURLWS_CONT)) {
          // Full message received
          bool is_binary = true;
          if (meta->flags & CURLWS_TEXT) {
            is_binary = false;
          }
          
          string complete_msg = rx_buffer;
          rx_buffer = "";
          
          if (meta->flags & CURLWS_CLOSE) {
            disconnect ();
            return;
          }
          
          if (!(meta->flags & CURLWS_PING) && !(meta->flags & CURLWS_PONG)) {
             on_message (complete_msg, is_binary);
          }
        }
      } else if (res == CURLE_AGAIN) {
        // No data available right now
        break;
      } else {
        // Error or closed
        on_error ("WebSocket receive error or connection closed");
        disconnect ();
        break;
      }
    }
#endif
  }
}
