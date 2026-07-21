/******************************************************************************
 * MODULE     : tm_emscripten_websocket_client.cpp
 * DESCRIPTION: emscripten WebSocket API implementation of tm_websocket_client
 *              for WASM builds (browser-native WebSocket). See
 *              tm_emscripten_websocket_client.hpp for the event marshalling
 *              contract.
 ******************************************************************************/

#include "tm_emscripten_websocket_client.hpp"

#ifdef __EMSCRIPTEN__

#include "basic.hpp" // for cout, debug_std

#include <emscripten/websocket.h>

tm_emscripten_websocket_client::tm_emscripten_websocket_client ()
    : handle (0), is_connected (false), close_notified (false) {}

tm_emscripten_websocket_client::~tm_emscripten_websocket_client () {
  disconnect ();
}

void
tm_emscripten_websocket_client::queue_event (int kind, const char* data,
                                             int num_bytes, bool is_binary) {
  ws_evt evt;
  evt.kind     = kind;
  evt.is_binary= is_binary;
  if (data != NULL && num_bytes > 0) evt.data.assign (data, (size_t) num_bytes);
  events.push_back (std::move (evt));
}

EM_BOOL
tm_emscripten_websocket_client::handle_open_cb (
    int eventType, const EmscriptenWebSocketOpenEvent* e, void* userData) {
  (void) eventType;
  tm_emscripten_websocket_client* self=
      (tm_emscripten_websocket_client*) userData;
  self->is_connected  = true;
  self->close_notified= false;
  self->queue_event (1, NULL, 0, false);
  return EM_TRUE;
}

EM_BOOL
tm_emscripten_websocket_client::handle_message_cb (
    int eventType, const EmscriptenWebSocketMessageEvent* e, void* userData) {
  (void) eventType;
  tm_emscripten_websocket_client* self=
      (tm_emscripten_websocket_client*) userData;
  self->queue_event (0, (const char*) e->data, (int) e->numBytes,
                     e->isText == EM_FALSE);
  return EM_TRUE;
}

EM_BOOL
tm_emscripten_websocket_client::handle_error_cb (
    int eventType, const EmscriptenWebSocketErrorEvent* e, void* userData) {
  (void) eventType;
  (void) e;
  tm_emscripten_websocket_client* self=
      (tm_emscripten_websocket_client*) userData;
  // 浏览器 error 事件不携带原因；onclose 会随后补上断开通知
  self->queue_event (3, "WebSocket error", 0, false);
  return EM_TRUE;
}

EM_BOOL
tm_emscripten_websocket_client::handle_close_cb (
    int eventType, const EmscriptenWebSocketCloseEvent* e, void* userData) {
  (void) eventType;
  (void) e;
  tm_emscripten_websocket_client* self=
      (tm_emscripten_websocket_client*) userData;
  self->is_connected  = false;
  self->close_notified= true;
  self->queue_event (2, NULL, 0, false);
  return EM_TRUE;
}

void
tm_emscripten_websocket_client::connect (string url) {
  disconnect ();

  EmscriptenWebSocketCreateAttributes attrs;
  emscripten_websocket_init_create_attributes (&attrs);
  c_string url_c (url);
  attrs.url               = (const char*) url_c;
  attrs.protocols         = NULL;
  attrs.createOnMainThread= EM_TRUE;

  handle= emscripten_websocket_new (&attrs);
  if (handle <= 0) {
    handle= 0;
    queue_event (3, "emscripten_websocket_new failed", 0, false);
    queue_event (2, NULL, 0, false);
    close_notified= true;
    return;
  }

  emscripten_websocket_set_onopen_callback_on_thread (
      handle, this, handle_open_cb,
      EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_websocket_set_onmessage_callback_on_thread (
      handle, this, handle_message_cb,
      EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_websocket_set_onerror_callback_on_thread (
      handle, this, handle_error_cb,
      EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_websocket_set_onclose_callback_on_thread (
      handle, this, handle_close_cb,
      EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
}

void
tm_emscripten_websocket_client::disconnect () {
  if (handle <= 0) return;
  int h = handle;
  handle= 0; // 摘钩，避免浏览器回调触达半析构对象
  if (is_connected) {
    // 主动关闭会触发 onclose，由其排队 on_disconnect
    emscripten_websocket_close (h, 1000, "client disconnect");
  }
  else {
    // 未建立连接（握手中或已出错）：不会有 onclose，自行补发断开
    emscripten_websocket_close (h, 1000, "client disconnect");
    if (!close_notified) {
      queue_event (2, NULL, 0, false);
      close_notified= true;
    }
  }
  is_connected= false;
}

void
tm_emscripten_websocket_client::send (string data, bool is_binary) {
  if (!is_connected || handle <= 0) return;

  EMSCRIPTEN_RESULT res;
  if (is_binary) {
    res= emscripten_websocket_send_binary (handle, (void*) data.begin (),
                                           (uint32_t) N (data));
  }
  else {
    c_string text (data);
    res= emscripten_websocket_send_utf8_text (handle, (const char*) text);
  }
  if (res != EMSCRIPTEN_RESULT_SUCCESS) {
    queue_event (3, "emscripten_websocket_send failed", 0, false);
  }
}

void
tm_emscripten_websocket_client::poll () {
  std::deque<ws_evt> evts;
  evts.swap (events);

  for (std::deque<ws_evt>::iterator it= evts.begin (); it != evts.end ();
       ++it) {
    switch (it->kind) {
    case 0:
      on_message (string (it->data.data (), (int) it->data.size ()),
                  it->is_binary);
      break;
    case 1:
      on_connect ();
      break;
    case 2:
      on_disconnect ();
      break;
    case 3:
      on_error (it->data.empty ()
                    ? string ("WebSocket error")
                    : string (it->data.data (), (int) it->data.size ()));
      break;
    }
  }
}

#endif // __EMSCRIPTEN__
