/******************************************************************************
 * MODULE     : loro_collab.cpp
 * DESCRIPTION: 云文档协作会话层实现（ImGui 前端）。见 loro_collab.hpp。
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "loro_collab.hpp"

#ifdef LORO_ENABLED

#include "tm_websocket.hpp"
#ifdef __EMSCRIPTEN__
#include "tm_emscripten_websocket_client.hpp"
#else
#include "tm_curl_websocket_client.hpp"
#endif

#include "editor.hpp"
#include "new_buffer.hpp" // get_current_buffer, set_title_buffer
#include "new_view.hpp"   // get_current_editor

#include "analyze.hpp"  // starts
#include "tm_timer.hpp" // texmacs_time

#ifndef __EMSCRIPTEN__
#include <curl/curl.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#endif

// edit_modify 侧的上行回调：shadow 产生本地增量时调用。
// 本层拥有其存储：会话就绪时指向 broadcast_to_server，断开时清空。
void (*g_loro_broadcast_update) (string bytes)= nullptr;

#ifndef __EMSCRIPTEN__
// 文档发现的异步查询状态机（仅 native：libcurl + std::thread）。
// worker 线程只碰 std 类型（curl→std::string→std::vector），绝不触 lolly 分配器
// （lolly fast_alloc 进程全局非线程安全）；GUI 线程读 docs() 时才转 array<string>。
enum class docs_status { idle, loading, ready, error };
static std::mutex             g_docs_mutex;
static std::atomic<docs_status> g_docs_status{docs_status::idle};
static std::vector<std::string> g_docs_data; // worker 写、GUI 读，均持 g_docs_mutex

// libcurl 写回调：把 HTTP 响应体累积到 std::string。
static size_t
http_write_cb (char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* buf= static_cast<std::string*> (userdata);
  buf->append (ptr, size * nmemb);
  return size * nmemb;
}
#endif

// ws://host:port → http://host:port；wss:// → https://。用于把协作服务端地址
// 复用到同端口的 HTTP /docs 查询。
static string
ws_to_http (string url) {
  if (starts (url, "ws://")) return "http://" * url (5, N (url));
  if (starts (url, "wss://")) return "https://" * url (6, N (url));
  return url;
}

namespace {

enum class collab_state {
  idle,         // 无会话
  connecting,   // WS 连接中
  await_doc,    // 已发 CREATE/JOIN，等服务端 DOC 回复
  await_frame,  // JOIN：DOC 已回，等首帧历史（snapshot）构建 buffer
  ready,        // 协作中
};

enum class collab_mode { create, join };

class collab_session {
public:
  tm_websocket_client_impl* ws        = nullptr;
  collab_state              state     = collab_state::idle;
  collab_mode               mode      = collab_mode::create;
  string                    doc_id;   // JOIN 请求的 / 服务端分配的 UUID
  string                    server_url;
  time_t                    await_frame_since= 0; // JOIN 进入 await_frame 的时刻(ms)
  // 不持有固定 editor 句柄：一个文档对应一个 ws 连接，会话操作（enable/
  // apply_remote）一律对「当前编辑器」动态取用——避免捕获的 editor 失效或与
  // 用户正在编辑的 editor 不一致（曾是导致不广播的根因）。

  bool want_create () const { return mode == collab_mode::create; }
};

collab_session g_session;

// 上行：把本地 shadow 产生的增量 update 经会话的 WS 发出。会话单例，故无需
// 区分文档路径——local-update 回调仅对 collab 开启的 editor 触发（其余 editor
// collab 关闭，不镜像，不产生本地增量）。
void
broadcast_to_server (string bytes) {
  if (g_session.state != collab_state::ready || !g_session.ws ||
      !g_session.ws->connected ())
    return;
  cout << "[collab] 上行 " << N (bytes) << " 字节 update\n";
  g_session.ws->send (bytes, true);
}

// WS 客户端：协议状态机驱动
class collab_ws_client : public tm_websocket_client_impl {
public:
  void on_connect () override {
    cout << "[collab] 已连接服务端 " << g_session.server_url << "\n";
    g_session.state= collab_state::await_doc;
    if (g_session.want_create ()) send ("CREATE", false);
    else send ("JOIN " * g_session.doc_id, false);
  }

  void on_message (string data, bool is_binary) override {
    if (!is_binary) {
      if (starts (data, "DOC ")) {
        g_session.doc_id= data (4, N (data));
        cout << "[collab] 服务端确认文档 " << g_session.doc_id
             << "（mode=" << (g_session.want_create () ? "create" : "join")
             << "）\n";
        if (g_session.want_create ()) become_ready ();
        else {
          // JOIN：等服务端补发 snapshot/updates；记录时刻，poll 里超时兜底
          // （空文档无历史可发，避免永久卡在 await_frame）
          g_session.state           = collab_state::await_frame;
          g_session.await_frame_since= texmacs_time ();
        }
      }
      else if (starts (data, "ERR ")) {
        cout << "[collab] 服务端错误: " << data << "\n";
        g_session.state= collab_state::idle;
      }
      return;
    }
    // 二进制帧：snapshot 或 update，统一交「当前编辑器」apply_remote。
    editor ed= get_current_editor ();
    if (is_nil (ed)) return;
    if (g_session.state == collab_state::await_frame) {
      // JOIN 首帧：apply_remote 构建 buffer 后才置位协作（避免用户在
      // snapshot 到达前编辑触发空 buffer seed 起新根）
      ed->apply_remote (data);
      become_ready ();
    }
    else if (g_session.state == collab_state::ready) {
      ed->apply_remote (data);
    }
  }

  void on_error (string msg) override { cout << "[collab] WS Error: " << msg << "\n"; }
  void on_disconnect () override {
    cout << "[collab] WS 断开\n";
    g_session.state= collab_state::idle;
  }

  void become_ready () {
    cout << "[collab] become_ready 入口\n";
    g_session.state       = collab_state::ready;
    g_loro_broadcast_update= broadcast_to_server;
    // 对当前编辑器置位协作开关：之后该 editor 的本地编辑才会镜像 + 上行
    editor ed= get_current_editor ();
    if (is_nil (ed)) cout << "[collab] become_ready: 当前编辑器为空！\n";
    else ed->collab_enable ();
    // 把当前 buffer 标题设为文档 UUID（替代默认 "No Name [n]"），便于识别与分享
    set_title_buffer (get_current_buffer (), g_session.doc_id);
    cout << "[collab] 会话就绪 doc=" << g_session.doc_id << "\n";
  }
};

} // namespace

string
loro_collab_create (string server_url) {
  if (g_session.ws) loro_collab_disconnect ();
  if (is_nil (get_current_editor ())) {
    cout << "[collab] 无当前编辑器，无法创建协作文档\n";
    return "";
  }
  g_session.mode        = collab_mode::create;
  g_session.doc_id      = "";
  g_session.server_url  = server_url;
  g_session.state       = collab_state::connecting;
  g_session.ws          = new collab_ws_client ();
  g_session.ws->connect (server_url);
  return "";
}

void
loro_collab_join (string server_url, string doc_id) {
  if (g_session.ws) loro_collab_disconnect ();
  if (is_nil (get_current_editor ())) {
    cout << "[collab] 无当前编辑器，无法加入协作文档\n";
    return;
  }
  g_session.mode        = collab_mode::join;
  g_session.doc_id      = doc_id;
  g_session.server_url  = server_url;
  g_session.state       = collab_state::connecting;
  g_session.ws          = new collab_ws_client ();
  g_session.ws->connect (server_url);
}

void
loro_collab_disconnect () {
  g_loro_broadcast_update= nullptr;
  g_session.state = collab_state::idle;
  g_session.doc_id = "";
  if (g_session.ws) {
    g_session.ws->disconnect ();
    delete g_session.ws;
    g_session.ws= nullptr;
  }
}

bool
loro_collab_is_active () {
  return g_session.state == collab_state::ready;
}

string
loro_collab_doc_id () {
  return g_session.doc_id;
}

void
loro_collab_poll () {
  if (g_session.ws) g_session.ws->poll ();
  // JOIN 空文档兜底：DOC 后 1s 内未收到任何历史帧（snapshot/updates），
  // 视为空文档直接就绪，否则会永久卡在 await_frame、collab 永不开启。
  if (g_session.state == collab_state::await_frame &&
      g_session.await_frame_since > 0 &&
      texmacs_time () - g_session.await_frame_since >= 1000) {
    cout << "[collab] JOIN 超时未收到历史帧，按空文档就绪\n";
    g_session.await_frame_since= 0;
    static_cast<collab_ws_client*> (g_session.ws)->become_ready ();
  }
}

void
loro_collab_fetch_docs (string server_url) {
#ifndef __EMSCRIPTEN__
  // 已在加载中：跳过（ImGui 每帧重建菜单会反复触发，故必须幂等）
  if (g_docs_status.load () == docs_status::loading) return;
  g_docs_status.store (docs_status::loading);
  // worker 线程：std::string URL（从 lolly string 转出，跨线程边界后不再触 lolly）
  std::string url_std ((const char*) c_string (ws_to_http (server_url)));
  if (url_std.empty () || url_std.back () != '/') url_std+= '/';
  url_std+= "docs";
  std::thread ([url_std] () {
    std::string body;
    CURL*       h= curl_easy_init ();
    if (h == nullptr) {
      std::lock_guard<std::mutex> lk (g_docs_mutex);
      g_docs_data.clear ();
      g_docs_status.store (docs_status::error);
      return;
    }
    curl_easy_setopt (h, CURLOPT_URL, url_std.c_str ());
    curl_easy_setopt (h, CURLOPT_WRITEFUNCTION, http_write_cb);
    curl_easy_setopt (h, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt (h, CURLOPT_CONNECTTIMEOUT, 2L);
    curl_easy_setopt (h, CURLOPT_TIMEOUT, 3L);
    CURLcode rc= curl_easy_perform (h);
    curl_easy_cleanup (h);
    std::vector<std::string> lines;
    if (rc != CURLE_OK) {
      std::lock_guard<std::mutex> lk (g_docs_mutex);
      g_docs_data.clear ();
      g_docs_status.store (docs_status::error);
      return;
    }
    // 按行切分（服务端纯文本，每行一个 UUID）
    size_t start= 0;
    while (start <= body.size ()) {
      size_t next= body.find ('\n', start);
      if (next == std::string::npos) next= body.size ();
      std::string line= body.substr (start, next - start);
      if (!line.empty () && line.back () == '\r') line.pop_back ();
      if (!line.empty ()) lines.push_back (line);
      start= next + 1;
    }
    std::lock_guard<std::mutex> lk (g_docs_mutex);
    g_docs_data= std::move (lines);
    g_docs_status.store (docs_status::ready);
  }).detach ();
#else
  (void) server_url;
#endif
}

string
loro_collab_docs_status () {
#ifndef __EMSCRIPTEN__
  switch (g_docs_status.load ()) {
  case docs_status::idle: return "idle";
  case docs_status::loading: return "loading";
  case docs_status::ready: return "ready";
  case docs_status::error: return "error";
  }
#endif
  return "idle";
}

array<string>
loro_collab_docs () {
  array<string> result;
#ifndef __EMSCRIPTEN__
  std::lock_guard<std::mutex> lk (g_docs_mutex);
  for (const auto& s : g_docs_data) {
    string line ((const char*) s.data (), (int) s.size ());
    result << line;
  }
#endif
  return result;
}

#endif // LORO_ENABLED
