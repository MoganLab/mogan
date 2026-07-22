/******************************************************************************
 * MODULE: tm_websocket.hpp
 * DESCRIPTION: 抽象的非阻塞 WebSocket 客户端接口。
 *
 *              tm_websocket_client 定义了与底层传输实现无关的统一接口
 *              （connect、disconnect、send、poll、connected 及 on_*
 *              回调），使不同平台能够提供各自的 WebSocket 实现
 *              （如 libcurl、Emscripten WebSocket API 等），而无需修改
 *              上层调用代码。
 *
 *              各平台的具体实现位于各自的头文件中（如
 *              tm_curl_websocket_client.hpp），并通过
 *              tm_websocket_client_impl 选择平台默认实现。
 *
 *              g_loro_broadcast_update 与 WS 客户端均由
 *              src/Plugins/Collab/loro_collab
 *              持有（前端无关），ImGui/Qt/WASM 三前端只各自在事件循环里调
 *              loro_collab_poll()、退出时 loro_collab_disconnect()。
 *              故 loro 可与任意前端共存。
 *
 * AUTHOR: JimZhouZZY
 * DATE:   July, 2026
 ******************************************************************************/

#ifndef TM_WEBSOCKET_HPP
#define TM_WEBSOCKET_HPP

#include "string.hpp"

class tm_websocket_client {
public:
  tm_websocket_client ()         = default;
  virtual ~tm_websocket_client ()= default;

  virtual void connect (string url)                    = 0;
  virtual void disconnect ()                           = 0;
  virtual void send (string data, bool is_binary= true)= 0;

  // 必须在事件循环（如 im_interpose 或 im_main_loop）中调用
  // 多线程实现：消费接收队列中的消息并触发回调。
  // 单线程实现：直接驱动底层 WebSocket。
  virtual void poll ()= 0;

  virtual bool connected () const= 0;

  // 虚回调（由子类重写处理），应该始终在 GUI 线程中触发。
  virtual void on_message (string data, bool is_binary) {}
  virtual void on_connect () {}
  virtual void on_disconnect () {}
  virtual void on_error (string msg) {}
};

#endif // TM_WEBSOCKET_HPP
