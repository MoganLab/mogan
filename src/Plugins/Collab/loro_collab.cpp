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
#include "new_buffer.hpp" // get_current_buffer, set_title_buffer, concrete_buffer
#include "new_view.hpp"   // get_current_editor
#include "server.hpp"     // get_server()->set_message（重连状态提示）
#include "url.hpp"        // url（collab buffer 标识）

#include "analyze.hpp" // starts
#include "tm_buffer.hpp"
#include "tm_timer.hpp" // texmacs_time

#include <cstdlib> // getenv (native), free (WASM)
#include <cstring> // strlen (WASM)

#ifndef __EMSCRIPTEN__
#include <atomic>
#include <curl/curl.h>
#include <mutex>
#include <thread>
#include <vector>
#endif

// edit_modify 侧的上行回调：shadow 产生本地增量时调用。
// 本层拥有其存储：会话就绪时指向 broadcast_to_server，断开时清空。
void (*g_loro_broadcast_update) (string bytes)= nullptr;

// 文档发现状态机。native: libcurl + worker 线程（std 容器，不触 lolly
// 分配器）； WASM: 浏览器 fetch，结果经 ccall 回调写入 C++
// 普通全局（单线程，菜单每帧只读 C++ 变量，无 EM_JS 往返）。docs_status
// 枚举两端共用。
enum class docs_status { idle, loading, ready, error };

#ifndef __EMSCRIPTEN__
static std::mutex               g_docs_mutex;
static std::atomic<docs_status> g_docs_status{docs_status::idle};
static std::vector<std::string>
    g_docs_data; // worker 写、GUI 读，均持 g_docs_mutex

// libcurl 写回调：把 HTTP 响应体累积到 std::string。
static size_t
http_write_cb (char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* buf= static_cast<std::string*> (userdata);
  buf->append (ptr, size * nmemb);
  return size * nmemb;
}
#else
#include <emscripten.h>
#include <string>

static docs_status g_docs_status= docs_status::idle; // 单线程，无需原子
static std::string g_docs_data;                      // \n 连接的 UUID 文本

// JS fetch 完成/失败时经 ccall 回调入此（EMSCRIPTEN_KEEPALIVE + ccall，与
// mogan_ime_commit 同款）。ok=2 成功（text 为 \n 分隔结果），ok=3 失败。
extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_collab_docs_received (const char* text, int ok) {
  if (ok == 2) {
    g_docs_status= docs_status::ready;
    g_docs_data  = (text != nullptr) ? std::string (text) : std::string ();
  }
  else {
    g_docs_status= docs_status::error;
    g_docs_data.clear ();
  }
}

// 把 ccall 拉入 EM_JS 作用域（无需改 EXPORTED_RUNTIME_METHODS）。
EM_JS_DEPS (mogan_collab, "$ccall");

// 读协作服务端地址：window.MOGAN_LORO_SERVER（HTML
// 包装里设，浏览器里的"环境变量"） → ?loro_server= 查询参数 → 空。返回 _malloc
// 的 C 串，调用方 free()。
EM_JS (char*, collab_read_server_url_js, (), {
  try {
    var s= window.MOGAN_LORO_SERVER;
    if (!s) {
      var p= new URLSearchParams (window.location.search);
      s    = p.get ('loro_server');
    }
    s      = s || '';
    var len= lengthBytesUTF8 (s) + 1;
    var buf= _malloc (len);
    stringToUTF8 (s, buf, len);
    return buf;
  } catch (e) {
    return 0;
  }
});

// 异步 GET url：成功时把响应文本（\n 分隔 UUID）malloc 成 C 串，ccall
// 回调后释放； 失败/异常时 ccall(0,3)。全程 try/catch，杜绝 EM_JS
// 内未捕获异常。
EM_JS (void, collab_fetch_docs_js, (const char* url_cstr), {
  try {
    var url= UTF8ToString (url_cstr);
    fetch (url)
        .then (function (r) { return r.text (); })
        .then (function (text) {
          var len= lengthBytesUTF8 (text);
          var buf= _malloc (len + 1);
          stringToUTF8 (text, buf, len + 1);
          ccall ('mogan_collab_docs_received', null, [ 'number', 'number' ],
                 [ buf, 2 ]);
          _free (buf);
        })
        .catch (function (e) {
          ccall ('mogan_collab_docs_received', null, [ 'number', 'number' ],
                 [ 0, 3 ]);
        });
  } catch (e) {
    ccall ('mogan_collab_docs_received', null, [ 'number', 'number' ],
           [ 0, 3 ]);
  }
});
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
  connecting,   // WS 连接中（初次或重连尝试）
  await_doc,    // 已发 CREATE/JOIN，等服务端 DOC 回复
  await_frame,  // JOIN：DOC 已回，等首帧历史（snapshot）构建 buffer
  ready,        // 协作中
  reconnecting, // 意外断开，按退避等待下次重连尝试
};

enum class collab_mode { create, join };

class collab_session {
public:
  tm_websocket_client_impl* ws   = nullptr;
  collab_state              state= collab_state::idle;
  collab_mode               mode = collab_mode::create;
  string                    doc_id; // JOIN 请求的 / 服务端分配的 UUID
  string                    server_url;
  url buffer_url; // 会话绑定的 buffer（become_ready 记下），poll
                  // 检测其被关闭即断连
  bool   buffer_known     = false; // buffer_url 是否已记下（避免对 url 判空）
  time_t await_frame_since= 0;     // JOIN 进入 await_frame 的时刻(ms)
  // 自动重连（仅意外断开触发；loro_collab_disconnect 手动退出时置 false）：
  //   want_reconnect=true 时 on_disconnect 调度重连，poll 到点尝试。
  //   首次断开立即重连（backoff(0)=0），失败后间隔指数增长，封顶 30s。
  bool   want_reconnect   = false;
  time_t next_reconnect_at= 0; // 下次重连尝试时刻(ms)
  int    reconnect_attempt= 0; // 已失败的重连次数（退避基准）
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

// 状态栏提示（重连过程给用户反馈）。get_server() 在编辑期必非空（与
// edit_interface 用法一致），collab 仅在编辑期运行，故不判空。
static void
collab_set_message (string left) {
  get_server ()->set_message (tree (left), tree ("Collaborative"));
}

// 重连退避：attempt=0 立即(0ms)，之后 1s/2s/4s/8s/16s，封顶 30s。
static time_t
reconnect_backoff (int attempt) {
  if (attempt <= 0) return 0;
  time_t ms= 1000L << (attempt - 1);
  if (ms <= 0 || ms > 30000L) ms= 30000L; // 封顶 + 防位移溢出
  return ms;
}

// 意外断开时调度下次重连（首次立即，之后按已失败次数退避）。
static void
schedule_reconnect () {
  time_t wait                = reconnect_backoff (g_session.reconnect_attempt);
  g_session.state            = collab_state::reconnecting;
  g_session.next_reconnect_at= texmacs_time () + wait;
  g_session.reconnect_attempt++;
  cout << "[collab] 调度重连：第 " << g_session.reconnect_attempt << " 次，"
       << wait << "ms 后\n";
  collab_set_message ("Connection lost; reconnecting… (attempt " *
                      as_string (g_session.reconnect_attempt) * ")");
}

// WS 客户端：协议状态机驱动
class collab_ws_client : public tm_websocket_client_impl {
public:
  void on_connect () override {
    cout << "[collab] 已连接服务端 " << g_session.server_url << "\n";
    g_session.state= collab_state::await_doc;
    // doc_id 已知（含 CREATE 成功后的重连）→ JOIN 回原 doc；否则首次 CREATE
    if (N (g_session.doc_id) > 0) send ("JOIN " * g_session.doc_id, false);
    else send ("CREATE", false);
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
          g_session.state            = collab_state::await_frame;
          g_session.await_frame_since= texmacs_time ();
        }
      }
      else if (starts (data, "ERR ")) {
        cout << "[collab] 服务端错误: " << data << "\n";
        // 服务端拒绝（如文档已删）：停止重连，留在 idle 供用户处置
        g_session.want_reconnect= false;
        g_session.state         = collab_state::idle;
        collab_set_message (data * " — stopped reconnecting");
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

  void on_error (string msg) override {
    cout << "[collab] WS Error: " << msg << "\n";
  }
  void on_disconnect () override {
    cout << "[collab] WS 断开（want_reconnect=" << g_session.want_reconnect
         << ", state=" << (int) g_session.state << "）\n";
    // 仅意外断开才重连；loro_collab_disconnect 手动退出前已置
    // want_reconnect=false
    if (g_session.want_reconnect) schedule_reconnect ();
    else g_session.state= collab_state::idle;
  }

  void become_ready () {
    cout << "[collab] become_ready 入口\n";
    bool was_reconnect         = g_session.reconnect_attempt > 0;
    g_session.state            = collab_state::ready;
    g_session.reconnect_attempt= 0;
    g_session.buffer_url=
        get_current_buffer (); // 记下，poll 检测其被关闭即断连
    g_session.buffer_known = true;
    g_loro_broadcast_update= broadcast_to_server;
    // 对当前编辑器置位协作开关：之后该 editor 的本地编辑才会镜像 + 上行
    editor ed= get_current_editor ();
    if (is_nil (ed)) cout << "[collab] become_ready: 当前编辑器为空！\n";
    else {
      ed->collab_enable ();
      ed->collab_resync (); // 重连后把断线期间累积的本地增量重新上行
    }
    // 把当前 buffer 标题设为文档 UUID（替代默认 "No Name [n]"），便于识别与分享
    set_title_buffer (g_session.buffer_url, g_session.doc_id);
    collab_set_message (was_reconnect ? "Reconnected to " * g_session.doc_id
                                      : "Session ready: " * g_session.doc_id);
    cout << "[collab] 会话就绪 doc=" << g_session.doc_id << "\n";
  }
};

// poll 驱动的重连尝试：到点则销毁旧 ws、新建并连接；on_connect 据 doc_id 重发
// JOIN/CREATE。失败会再次触发 on_disconnect→schedule_reconnect（退避增长）。
static void
collab_maybe_reconnect () {
  if (g_session.state != collab_state::reconnecting) return;
  if (texmacs_time () < g_session.next_reconnect_at) return;
  if (g_session.ws) {
    g_session.ws->disconnect ();
    delete g_session.ws;
    g_session.ws= nullptr;
  }
  cout << "[collab] 尝试重连 " << g_session.server_url << "\n";
  collab_set_message ("Reconnecting… (attempt " *
                      as_string (g_session.reconnect_attempt) * ")");
  g_session.ws   = new collab_ws_client ();
  g_session.state= collab_state::connecting;
  g_session.ws->connect (g_session.server_url);
}

} // namespace

string
loro_collab_server_url () {
  string url;
#ifdef __EMSCRIPTEN__
  char* buf= collab_read_server_url_js ();
  if (buf != nullptr) {
    int len= (int) strlen (buf);
    url    = string (buf, len);
    free (buf);
  }
#else
  const char* e= getenv ("MOGAN_LORO_SERVER");
  if (e != nullptr) url= string (e);
#endif
  if (N (url) == 0) url= "ws://127.0.0.1:8765";
  return url;
}

string
loro_collab_create (string server_url) {
  if (g_session.ws) loro_collab_disconnect ();
  if (is_nil (get_current_editor ())) {
    cout << "[collab] 无当前编辑器，无法创建协作文档\n";
    return "";
  }
  g_session.mode             = collab_mode::create;
  g_session.doc_id           = "";
  g_session.server_url       = server_url;
  g_session.state            = collab_state::connecting;
  g_session.reconnect_attempt= 0;
  g_session.want_reconnect   = true; // 启用意外断开自动重连
  g_session.ws               = new collab_ws_client ();
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
  g_session.mode             = collab_mode::join;
  g_session.doc_id           = doc_id;
  g_session.server_url       = server_url;
  g_session.state            = collab_state::connecting;
  g_session.reconnect_attempt= 0;
  g_session.want_reconnect   = true; // 启用意外断开自动重连
  g_session.ws               = new collab_ws_client ();
  g_session.ws->connect (server_url);
}

void
loro_collab_disconnect () {
  // 手动退出：先关重连意愿，再断 ws，使 on_disconnect 不再触发重连
  g_session.want_reconnect   = false;
  g_loro_broadcast_update    = nullptr;
  g_session.state            = collab_state::idle;
  g_session.doc_id           = "";
  g_session.reconnect_attempt= 0;
  g_session.buffer_known     = false;
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
  // collab buffer 被关闭（关标签页/窗口/退出）→
  // 视为手动结束会话，断开且不重连。 用 concrete_buffer 检测：buffer 删除后返回
  // nil。
  if (g_session.state != collab_state::idle && g_session.buffer_known &&
      is_nil (concrete_buffer (g_session.buffer_url))) {
    cout << "[collab] 协作 buffer 已关闭，断开会话\n";
    loro_collab_disconnect ();
    return;
  }
  // JOIN 空文档兜底：DOC 后 1s 内未收到任何历史帧（snapshot/updates），
  // 视为空文档直接就绪，否则会永久卡在 await_frame、collab 永不开启。
  if (g_session.state == collab_state::await_frame &&
      g_session.await_frame_since > 0 &&
      texmacs_time () - g_session.await_frame_since >= 1000) {
    cout << "[collab] JOIN 超时未收到历史帧，按空文档就绪\n";
    g_session.await_frame_since= 0;
    static_cast<collab_ws_client*> (g_session.ws)->become_ready ();
  }
  // 意外断开后按退避尝试重连（want_reconnect=false 时不触发，见 on_disconnect）
  collab_maybe_reconnect ();
}

void
loro_collab_fetch_docs (string server_url) {
  string http_url= ws_to_http (server_url);
  if (!ends (http_url, "/")) http_url= http_url * "/";
  http_url= http_url * "docs";
#ifndef __EMSCRIPTEN__
  // 已在加载中：跳过（ImGui 每帧重建菜单会反复触发，故必须幂等）
  if (g_docs_status.load () == docs_status::loading) return;
  g_docs_status.store (docs_status::loading);
  // worker 线程：std::string URL（从 lolly string 转出，跨线程边界后不再触
  // lolly）
  std::string url_std ((const char*) c_string (http_url));
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
  // WASM：幂等（loading 中跳过），浏览器 fetch 异步，回调写入 C++ 全局
  if (g_docs_status == docs_status::loading) return;
  g_docs_status= docs_status::loading;
  c_string u (http_url);
  collab_fetch_docs_js ((const char*) u);
#endif
}

string
loro_collab_docs_status () {
#ifndef __EMSCRIPTEN__
  switch (g_docs_status.load ()) {
  case docs_status::idle:
    return "idle";
  case docs_status::loading:
    return "loading";
  case docs_status::ready:
    return "ready";
  case docs_status::error:
    return "error";
  }
  return "idle";
#else
  // 读 C++ 全局（ccall 回调在主线程写入），无 EM_JS 往返
  switch (g_docs_status) {
  case docs_status::idle:
    return "idle";
  case docs_status::loading:
    return "loading";
  case docs_status::ready:
    return "ready";
  case docs_status::error:
    return "error";
  }
  return "idle";
#endif
}

array<string>
loro_collab_docs () {
  array<string> result;
#ifdef __EMSCRIPTEN__
  // 解析 C++ g_docs_data（\n 连接），按行切分为 lolly array<string>。
  // 单线程：ccall 回调与此处同在主运行时线程，操作 lolly 安全。
  const std::string& body = g_docs_data;
  size_t             start= 0;
  while (start <= body.size ()) {
    size_t next= body.find ('\n', start);
    if (next == std::string::npos) next= body.size ();
    std::string line= body.substr (start, next - start);
    if (!line.empty () && line.back () == '\r') line.pop_back ();
    if (!line.empty ()) result << string (line.data (), (int) line.size ());
    start= next + 1;
  }
#else
  std::lock_guard<std::mutex> lk (g_docs_mutex);
  for (const auto& s : g_docs_data) {
    string line ((const char*) s.data (), (int) s.size ());
    result << line;
  }
#endif
  return result;
}

#endif // LORO_ENABLED
