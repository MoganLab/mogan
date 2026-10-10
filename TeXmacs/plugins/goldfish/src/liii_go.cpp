//
// Copyright (C) 2026 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "liii_go.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace goldfish {

void glue_for_community_edition (s7_scheme* sc);

static std::string g_goldfish_lib_directory;
static std::mutex  g_lib_dir_mtx;

void
set_goldfish_lib_dir (const std::string& dir) {
  std::lock_guard<std::mutex> lock (g_lib_dir_mtx);
  g_goldfish_lib_directory= dir;
}

std::string
get_goldfish_lib_dir () {
  std::lock_guard<std::mutex> lock (g_lib_dir_mtx);
  return g_goldfish_lib_directory;
}

static s7_pointer
f_worker_notify_error (s7_scheme* sc, s7_pointer args) {
  char* tag_str= s7_object_to_c_string (sc, s7_car (args));
  char* err_str= s7_object_to_c_string (sc, s7_cadr (args));
  std::cerr << "[Goldfish Worker Error] " << (tag_str ? tag_str : "?") << ": " << (err_str ? err_str : "?")
            << std::endl;
  free (tag_str);
  free (err_str);
  return s7_unspecified (sc);
}

// ---------------------------------------------------------------------------
// Go Task and Thread Pool
// ---------------------------------------------------------------------------

struct GoTask {
  std::vector<std::string> var_names;
  std::vector<GFValue>     var_vals;
  GFValue                  code_expr;
};

static size_t
configured_worker_count () {
  static const size_t n= [] () {
    // GOLDFISH_GO_WORKERS：测试用，强制指定 worker 数（如 1 复现同会话排队）
    const char* env= std::getenv ("GOLDFISH_GO_WORKERS");
    if (env != nullptr && *env != '\0') {
      char* end   = nullptr;
      long  parsed= std::strtol (env, &end, 10);
      if (end != env && *end == '\0' && parsed >= 1 && parsed <= 1024) {
        return static_cast<size_t> (parsed);
      }
    }
    size_t c= std::thread::hardware_concurrency ();
    return c == 0 ? 4 : c;
  }();
  return n;
}

class GoThreadPool {
public:
  static GoThreadPool& instance () {
    static GoThreadPool pool;
    return pool;
  }

  void enqueue (GoTask task) {
    {
      std::unique_lock<std::mutex> lock (mtx);
      tasks.push (std::move (task));
    }
    cv.notify_one ();
  }

  // gate fire 时调用：唤醒空闲 worker 进调度器消费事件（挂起 fiber 的及时唤醒）
  void notify_workers () {
    {
      std::lock_guard<std::mutex> lock (mtx);
      ++wake_epoch;
    }
    cv.notify_all ();
  }

  void shutdown () {
    {
      std::unique_lock<std::mutex> lock (mtx);
      if (stop) return;
      stop= true;
    }
    cv.notify_all ();
    for (auto& w : workers) {
      if (w.joinable ()) {
        w.join ();
      }
    }
    // 丢弃队列中未执行的任务，显式迭代释放防止深层析构递归
    while (!tasks.empty ()) {
      auto& t= tasks.front ();
      for (auto& v : t.var_vals) {
        gfvalue_destroy_deep (v);
      }
      gfvalue_destroy_deep (t.code_expr);
      tasks.pop ();
    }
    workers.clear ();
  }

private:
  GoThreadPool () {
    size_t n= configured_worker_count ();
    for (size_t i= 0; i < n; ++i) {
      workers.emplace_back ([this] () { worker_loop (); });
    }
  }

  ~GoThreadPool () { shutdown (); }

  void worker_loop () {
    s7_scheme*  worker_sc= s7_init ();
    std::string lib_dir  = get_goldfish_lib_dir ();
    if (!lib_dir.empty ()) {
      s7_add_to_load_path (worker_sc, lib_dir.c_str ());
    }
    glue_for_community_edition (worker_sc);
    if (!lib_dir.empty ()) {
      namespace fs      = std::filesystem;
      fs::path boot_path= fs::path (lib_dir) / "scheme" / "boot.scm";
      if (fs::exists (boot_path)) {
        s7_load (worker_sc, boot_path.string ().c_str ());
      }
    }
    else {
      s7_load (worker_sc, "scheme/boot.scm");
    }
    s7_eval_c_string (worker_sc, "(import (scheme base) (scheme time) (liii base) (liii go))");
    // 错误处理闭包定义一次并锚定在 rootlet，避免每个任务重建及被 GC 回收
    s7_eval_c_string (worker_sc,
                      "(define *go-err-handler* (lambda (err-tag err-args) (g_worker-notify-error err-tag err-args)))");
    // 任务 fiber 化（方案 C）：调度器空闲时返回本循环而非 park 线程。
    // chan-recv!/chan-send! 的无 timeout 形态在库定义中即 fiber 实现
    // （挂起协程而非阻塞物理线程），主/worker 会话与 import 复制均无版本分歧
    s7_eval_c_string (worker_sc, "(%set-scheduler-idle-return!)");

    // 任务执行入口与错误 handler 一次取出并锚定，跨任务复用。
    // 调度包装（catch + spawn-fiber + fiber-scheduler-run!）在 %run-worker-task
    // 的源码中定义：C 层拼装的同构表达式走求值器通用路径，continuation 栈布局
    // 与源码路径不同，调度收尾逃逸恢复时 op 错位会误报 list-ref 错误
    s7_pointer run_task    = s7_name_to_value (worker_sc, "%run-worker-task");
    s7_pointer drain_proc  = s7_name_to_value (worker_sc, "%drain-suspended!");
    s7_pointer handler_proc= s7_name_to_value (worker_sc, "*go-err-handler*");
    s7_int     run_task_loc= s7_gc_protect (worker_sc, run_task);
    s7_int     drain_loc   = s7_gc_protect (worker_sc, drain_proc);
    s7_int     handler_loc = s7_gc_protect (worker_sc, handler_proc);

    // 以下符号均被 rootlet 常驻引用，跨任务复用是 GC 安全的
    s7_pointer lambda_sym= s7_make_symbol (worker_sc, "lambda");
    s7_pointer let_sym   = s7_make_symbol (worker_sc, "let");

    uint64_t my_epoch= 0;
    while (true) {
      GoTask task;
      bool   has_task= false;
      {
        std::unique_lock<std::mutex> lock (mtx);
        // 空唤醒来源：gate fire 递增 wake_epoch（挂起 fiber 有事件待消费）
        cv.wait (lock, [this, &my_epoch] () { return stop || !tasks.empty () || wake_epoch != my_epoch; });
        if (stop && tasks.empty ()) break;
        if (!tasks.empty ()) {
          task= std::move (tasks.front ());
          tasks.pop ();
          has_task= true;
        }
        my_epoch= wake_epoch;
      }

      if (!has_task) {
        // 空唤醒：本会话若有挂起 fiber，进调度器消费 gate 事件
        s7_call (worker_sc, drain_proc, s7_nil (worker_sc));
        continue;
      }

      // 构造链上的中间 s7 对象逐个挂 GC 保护位置：fiber 化后 worker 会话跨任务
      // 存活、GC 更频繁，未扎根的 bindings/let_expr 可能在构造期间被回收
      std::vector<s7_int> gc_locs;
      auto                prot= [&] (s7_pointer obj) { gc_locs.push_back (s7_gc_protect (worker_sc, obj)); };

      // Build bindings: ((name1 val1) (name2 val2) ...)
      s7_pointer bindings= s7_nil (worker_sc);
      prot (bindings);
      for (size_t i= task.var_names.size (); i > 0; --i) {
        size_t     idx= i - 1;
        s7_pointer sym= s7_make_symbol (worker_sc, task.var_names[idx].c_str ());
        s7_pointer val= gfvalue_to_s7 (worker_sc, task.var_vals[idx]);
        prot (val);
        s7_pointer pair= s7_list (worker_sc, 2, sym, val);
        prot (pair);
        bindings= s7_cons (worker_sc, pair, bindings);
        prot (bindings);
      }

      // (let bindings body)：任务体不做 C 层 catch 包裹，错误隔离由
      // %run-worker-task 的源码 catch 统一承担（C 拼装的 catch 参与调度
      // continuation 栈，逃逸恢复时会 op 错位）
      s7_pointer body= gfvalue_to_s7 (worker_sc, task.code_expr);
      prot (body);
      s7_pointer tail= s7_cons (worker_sc, body, s7_nil (worker_sc));
      prot (tail);
      tail= s7_cons (worker_sc, bindings, tail);
      prot (tail);
      s7_pointer let_expr= s7_cons (worker_sc, let_sym, tail);
      prot (let_expr);

      // 任务数据已全部反序列化进 worker 会话（bindings/body 持有），GFValue 可立即
      // 释放，不必等调度器返回——任务可能挂起为 fiber 跨任务存活
      for (auto& v : task.var_vals) {
        gfvalue_destroy_deep (v);
      }
      gfvalue_destroy_deep (task.code_expr);

      // 任务 thunk = (lambda () let_expr)：先求值成闭包再传给 %run-worker-task。
      // 未求值直接传的话是 pair，s7 的 apply_pair 会把它当 implicit list-ref 调用
      s7_pointer thunk_expr= s7_list (worker_sc, 3, lambda_sym, s7_nil (worker_sc), let_expr);
      prot (thunk_expr);
      s7_pointer task_thunk= s7_eval (worker_sc, thunk_expr, s7_rootlet (worker_sc));
      prot (task_thunk);
      s7_pointer call_args= s7_list (worker_sc, 2, task_thunk, handler_proc);
      prot (call_args);

      // 包 fiber 入就绪队列并运行调度器，空闲返回（挂起 fiber 留在会话）
      s7_call (worker_sc, run_task, call_args);

      for (auto loc : gc_locs) {
        s7_gc_unprotect_at (worker_sc, loc);
      }
    }
  }

  std::vector<std::thread> workers;
  std::queue<GoTask>       tasks;
  std::mutex               mtx;
  std::condition_variable  cv;
  uint64_t                 wake_epoch= 0; // gate fire 计数，空闲 worker 的空唤醒源
  bool                     stop      = false;
};

// ---------------------------------------------------------------------------
// Timer Facility：单一定时线程 + 最小堆，到点关闭 channel
// 避免每个 timeout context 独占一个 worker 线程导致线程池饿死
// ---------------------------------------------------------------------------

class GoTimer {
public:
  static GoTimer& instance () {
    static GoTimer timer;
    return timer;
  }

  void schedule_close (std::shared_ptr<GoldfishChannel> ch, int64_t delay_ms) {
    {
      std::lock_guard<std::mutex> lock (mtx);
      entries.push ({std::chrono::steady_clock::now () + std::chrono::milliseconds (delay_ms), std::move (ch)});
    }
    cv.notify_one ();
  }

private:
  struct Entry {
    std::chrono::steady_clock::time_point deadline;
    std::shared_ptr<GoldfishChannel>      ch;
    bool                                  operator> (const Entry& o) const { return deadline > o.deadline; }
  };

  GoTimer () : worker ([this] () { loop (); }) {}

  ~GoTimer () {
    {
      std::lock_guard<std::mutex> lock (mtx);
      stop= true;
    }
    cv.notify_one ();
    if (worker.joinable ()) worker.join ();
  }

  void loop () {
    std::unique_lock<std::mutex> lock (mtx);
    while (!stop) {
      if (entries.empty ()) {
        cv.wait (lock);
        continue;
      }
      if (entries.top ().deadline <= std::chrono::steady_clock::now ()) {
        auto ch= entries.top ().ch;
        entries.pop ();
        lock.unlock (); // 关闭 channel 会唤醒其等待者，不持有定时器锁执行
        ch->close ();
        lock.lock ();
      }
      else {
        cv.wait_until (lock, entries.top ().deadline);
      }
    }
  }

  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> entries;
  std::mutex                                                          mtx;
  std::condition_variable                                             cv;
  std::thread                                                         worker;
  bool                                                                stop= false;
};

static std::mutex                             g_type_mtx;
static std::unordered_map<s7_scheme*, s7_int> g_channel_type_tags;
static std::unordered_map<s7_scheme*, s7_int> g_gate_type_tags;

// 序列化错误消息缓冲（TLS，保证 go_error 抛出时该字符串仍然存活）
static thread_local std::string t_serialize_err;

// 重要约束：go_error 底层是 s7_error 的 longjmp，会直接跳过 C++ 栈帧上
// RAII 对象的析构。Linux 下这只是泄漏，但 MSVC 的 longjmp 跳过非平凡析构
// 对象会导致进程崩溃（已在 Windows CI 上以 access violation 复现）。
// 因此本文件中所有 go_error 调用点必须保证当前函数帧内没有存活的 RAII
// 对象：先用平凡局部变量做参数检查，RAII 操作收进内层作用域或辅助函数，
// 错误在 RAII 对象析构之后抛出。
static s7_pointer
go_error (s7_scheme* sc, const char* kind, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, kind), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

static void
channel_free_c_value (void* val) {
  if (val != nullptr) {
    auto* ch_ptr= static_cast<std::shared_ptr<GoldfishChannel>*> (val);
    delete ch_ptr;
  }
}

static s7_pointer
channel_to_string_glue (s7_scheme* sc, s7_pointer args) {
  s7_pointer         self  = s7_car (args);
  auto*              ch_ptr= static_cast<std::shared_ptr<GoldfishChannel>*> (s7_c_object_value (self));
  std::ostringstream oss;
  oss << "#<channel " << ch_ptr->get () << " cap=" << (*ch_ptr)->get_capacity () << ">";
  return s7_make_string (sc, oss.str ().c_str ());
}

static s7_pointer
channel_is_equal_glue (s7_scheme* sc, s7_pointer args) {
  s7_pointer a= s7_car (args);
  s7_pointer b= s7_cadr (args);
  if (!is_goldfish_channel (sc, a) || !is_goldfish_channel (sc, b)) {
    return s7_f (sc);
  }
  auto* ch_a= static_cast<std::shared_ptr<GoldfishChannel>*> (s7_c_object_value (a));
  auto* ch_b= static_cast<std::shared_ptr<GoldfishChannel>*> (s7_c_object_value (b));
  return s7_make_boolean (sc, ch_a->get () == ch_b->get ());
}

bool
is_goldfish_channel (s7_scheme* sc, s7_pointer obj) {
  if (!s7_is_c_object (obj)) return false;
  std::lock_guard<std::mutex> lock (g_type_mtx);
  auto                        it= g_channel_type_tags.find (sc);
  if (it == g_channel_type_tags.end ()) return false;
  return s7_c_object_type (obj) == it->second;
}

std::shared_ptr<GoldfishChannel>
get_goldfish_channel (s7_scheme* sc, s7_pointer obj) {
  if (!is_goldfish_channel (sc, obj)) return nullptr;
  auto* ch_ptr= static_cast<std::shared_ptr<GoldfishChannel>*> (s7_c_object_value (obj));
  return *ch_ptr;
}

s7_pointer
make_goldfish_channel_object (s7_scheme* sc, std::shared_ptr<GoldfishChannel> ch) {
  s7_int tag= 0;
  {
    std::lock_guard<std::mutex> lock (g_type_mtx);
    auto                        it= g_channel_type_tags.find (sc);
    if (it == g_channel_type_tags.end ()) {
      return s7_f (sc);
    }
    tag= it->second;
  }
  auto* p= new std::shared_ptr<GoldfishChannel> (std::move (ch));
  return s7_make_c_object (sc, tag, p);
}

// ---------------------------------------------------------------------------
// GoGate（fiber 跨线程唤醒门闩）的 c_object 封装
// ---------------------------------------------------------------------------

static void
gate_free_c_value (void* val) {
  if (val != nullptr) {
    delete static_cast<std::shared_ptr<GoGate>*> (val);
  }
}

static s7_pointer
gate_to_string_glue (s7_scheme* sc, s7_pointer args) {
  return s7_make_string (sc, "#<go-gate>");
}

static bool
is_go_gate (s7_scheme* sc, s7_pointer obj) {
  if (!s7_is_c_object (obj)) return false;
  std::lock_guard<std::mutex> lock (g_type_mtx);
  auto                        it= g_gate_type_tags.find (sc);
  if (it == g_gate_type_tags.end ()) return false;
  return s7_c_object_type (obj) == it->second;
}

static std::shared_ptr<GoGate>
get_go_gate (s7_scheme* sc, s7_pointer obj) {
  if (!is_go_gate (sc, obj)) return nullptr;
  return *static_cast<std::shared_ptr<GoGate>*> (s7_c_object_value (obj));
}

static s7_pointer
make_go_gate_object (s7_scheme* sc, std::shared_ptr<GoGate> gate) {
  s7_int tag= 0;
  {
    std::lock_guard<std::mutex> lock (g_type_mtx);
    auto                        it= g_gate_type_tags.find (sc);
    if (it == g_gate_type_tags.end ()) {
      return s7_f (sc);
    }
    tag= it->second;
  }
  auto* p= new std::shared_ptr<GoGate> (std::move (gate));
  return s7_make_c_object (sc, tag, p);
}

// ---------------------------------------------------------------------------
// GFValue Serialization & Deserialization
// ---------------------------------------------------------------------------

struct SerializeCtx {
  std::unordered_map<void*, size_t> visited;
  size_t                            next_id= 1;
  // s7_let_to_list 的产物在入栈待处理期间需要 GC 保护
  std::vector<s7_int> protected_alists;

  ~SerializeCtx ()= default;
};

struct DeserializeCtx {
  std::unordered_map<size_t, s7_pointer> reconstructed;
  s7_pointer                             gc_anchor= nullptr;
  s7_int                                 gc_loc   = -1;
};

// 序列化单个节点。标量直接转换；复合类型分配载荷后把子节点压入工作栈，
// 由外层循环继续处理（显式栈迭代，避免深层结构导致 C++ 栈溢出崩溃）。
// 返回值：false 表示遇到不支持的类型。
static bool
serialize_node (s7_scheme* sc, s7_pointer obj, GFValue& out, std::string& err_msg, SerializeCtx& ctx,
                std::vector<std::pair<s7_pointer, GFValue*>>& work_stack) {
  if (s7_is_null (sc, obj)) {
    out.type= GFValueType::Nil;
    return true;
  }
  if (s7_is_boolean (obj)) {
    out.type    = GFValueType::Boolean;
    out.bool_val= (obj == s7_t (sc));
    return true;
  }
  if (s7_is_integer (obj)) {
    out.type   = GFValueType::Integer;
    out.int_val= s7_integer (obj);
    return true;
  }
  if (s7_is_real (obj)) {
    out.type      = GFValueType::Real;
    out.double_val= s7_real (obj);
    return true;
  }
  if (s7_is_character (obj)) {
    out.type    = GFValueType::Character;
    out.char_val= static_cast<uint32_t> (s7_character (obj));
    return true;
  }
  if (s7_is_string (obj)) {
    out.type   = GFValueType::String;
    out.str_val= std::string (s7_string (obj), s7_string_length (obj));
    return true;
  }
  if (s7_is_symbol (obj)) {
    out.type   = GFValueType::Symbol;
    out.str_val= s7_symbol_name (obj);
    return true;
  }
  if (s7_is_syntax (obj) || s7_is_procedure (obj)) {
    char* str= s7_object_to_c_string (sc, obj);
    if (str != nullptr && std::strncmp (str, "#_", 2) == 0) {
      out.type   = GFValueType::Symbol;
      out.str_val= str + 2;
      free (str);
      return true;
    }
    free (str);
  }
  if (is_goldfish_channel (sc, obj)) {
    out.type    = GFValueType::Channel;
    out.chan_val= get_goldfish_channel (sc, obj);
    return true;
  }
  if (obj == s7_eof_object (sc)) {
    out.type= GFValueType::Eof;
    return true;
  }

  // Composite structures: Check cycle/memoization
  void* ptr= reinterpret_cast<void*> (obj);
  auto  it = ctx.visited.find (ptr);
  if (it != ctx.visited.end ()) {
    out.type  = GFValueType::Ref;
    out.ref_id= it->second;
    return true;
  }

  size_t my_id    = ctx.next_id++;
  ctx.visited[ptr]= my_id;
  out.node_id     = my_id;

  if (s7_is_pair (obj)) {
    out.type     = GFValueType::Pair;
    auto pair_ptr= std::make_shared<std::pair<GFValue, GFValue>> ();
    // 与反序列化保持一致的子节点处理顺序：car 先于 cdr（LIFO，故 cdr 先入栈）
    work_stack.push_back ({s7_cdr (obj), &pair_ptr->second});
    work_stack.push_back ({s7_car (obj), &pair_ptr->first});
    out.pair_val= pair_ptr;
    return true;
  }
  if (s7_is_byte_vector (obj)) {
    out.type            = GFValueType::ByteVector;
    s7_int         len  = s7_vector_length (obj);
    const uint8_t* bytes= reinterpret_cast<const uint8_t*> (s7_byte_vector_elements (obj));
    out.bytevec_val     = std::make_shared<const std::vector<uint8_t>> (bytes, bytes + len);
    return true;
  }
  if (s7_is_vector (obj)) {
    out.type      = GFValueType::Vector;
    s7_int len    = s7_vector_length (obj);
    auto   vec_ptr= std::make_shared<std::vector<GFValue>> ();
    vec_ptr->resize (len);
    for (s7_int i= 0; i < len; ++i) {
      work_stack.push_back ({s7_vector_ref (sc, obj, i), &(*vec_ptr)[i]});
    }
    out.vec_val= vec_ptr;
    return true;
  }
  if (s7_is_let (obj)) {
    out.type        = GFValueType::Let;
    s7_pointer alist= s7_let_to_list (sc, obj);
    // alist 是新分配对象且不被源对象引用，入栈期间必须 GC 保护
    ctx.protected_alists.push_back (s7_gc_protect (sc, alist));
    auto pair_ptr= std::make_shared<std::pair<GFValue, GFValue>> ();
    work_stack.push_back ({alist, &pair_ptr->first});
    out.pair_val= pair_ptr;
    return true;
  }

  char* obj_str= s7_object_to_c_string (sc, obj);
  err_msg      = std::string ("unsupported object type for channel serialization: ") + (obj_str ? obj_str : "?");
  free (obj_str);
  return false;
}

bool
s7_to_gfvalue (s7_scheme* sc, s7_pointer obj, GFValue& out, std::string& err_msg) {
  SerializeCtx                                 ctx;
  std::vector<std::pair<s7_pointer, GFValue*>> work_stack;
  work_stack.push_back ({obj, &out});
  bool ok= true;
  while (!work_stack.empty ()) {
    auto item= work_stack.back ();
    work_stack.pop_back ();
    if (!serialize_node (sc, item.first, *item.second, err_msg, ctx, work_stack)) {
      ok= false;
      break;
    }
  }
  for (auto loc : ctx.protected_alists) {
    s7_gc_unprotect_at (sc, loc);
  }
  return ok;
}

// 反序列化的待填充槽位：复合节点先建壳并注册（供环引用解析），子节点入栈后填充
struct FillItem {
  enum class Slot { PairCar, PairCdr, VectorElem, LetContent, FillLet };
  Slot           slot;
  s7_pointer     target;
  s7_int         index  = 0;
  const GFValue* src    = nullptr;
  s7_pointer     payload= nullptr; // FillLet 专用：构建完成的 alist 根
};

// 构建单个节点的壳并注册/锚定；复合类型的子节点压入工作栈由主循环填充
// （显式栈迭代，避免深层结构导致 C++ 栈溢出崩溃）。
static s7_pointer
build_node (s7_scheme* sc, const GFValue& val, DeserializeCtx& ctx, std::vector<FillItem>& work_stack) {
  if (val.type == GFValueType::Ref) {
    auto it= ctx.reconstructed.find (val.ref_id);
    if (it != ctx.reconstructed.end ()) {
      return it->second;
    }
    return s7_nil (sc);
  }

  auto anchor= [&ctx, sc] (s7_pointer obj) {
    if (ctx.gc_loc >= 0) {
      ctx.gc_anchor= s7_cons (sc, obj, ctx.gc_anchor);
      s7_gc_protect_via_location (sc, ctx.gc_anchor, ctx.gc_loc);
    }
  };

  switch (val.type) {
  case GFValueType::Nil:
    return s7_nil (sc);
  case GFValueType::Boolean:
    return s7_make_boolean (sc, val.bool_val);
  case GFValueType::Integer:
    return s7_make_integer (sc, val.int_val);
  case GFValueType::Real:
    return s7_make_real (sc, val.double_val);
  case GFValueType::Character:
    return s7_make_character (sc, val.char_val);
  case GFValueType::String:
    return s7_make_string_with_length (sc, val.str_val.data (), val.str_val.size ());
  case GFValueType::Symbol:
    return s7_make_symbol (sc, val.str_val.c_str ());
  case GFValueType::Channel:
    return make_goldfish_channel_object (sc, val.chan_val);
  case GFValueType::Pair: {
    s7_pointer cell= s7_cons (sc, s7_nil (sc), s7_nil (sc));
    anchor (cell);
    if (val.node_id != 0) {
      ctx.reconstructed[val.node_id]= cell;
    }
    if (val.pair_val) {
      // 与序列化保持一致的子节点构建顺序：car 先于 cdr（LIFO，故 cdr 先入栈）
      work_stack.push_back ({FillItem::Slot::PairCdr, cell, 0, &val.pair_val->second});
      work_stack.push_back ({FillItem::Slot::PairCar, cell, 0, &val.pair_val->first});
    }
    return cell;
  }
  case GFValueType::Vector: {
    s7_int     len= val.vec_val ? static_cast<s7_int> (val.vec_val->size ()) : 0;
    s7_pointer vec= s7_make_vector (sc, len);
    anchor (vec);
    if (val.node_id != 0) {
      ctx.reconstructed[val.node_id]= vec;
    }
    if (val.vec_val) {
      for (s7_int i= 0; i < len; ++i) {
        work_stack.push_back ({FillItem::Slot::VectorElem, vec, i, &(*val.vec_val)[i]});
      }
    }
    return vec;
  }
  case GFValueType::ByteVector: {
    if (!val.bytevec_val) return s7_make_byte_vector (sc, 0, 1, nullptr);
    s7_int     len= static_cast<s7_int> (val.bytevec_val->size ());
    s7_pointer bv = s7_make_byte_vector (sc, len, 1, nullptr);
    anchor (bv);
    if (val.node_id != 0) {
      ctx.reconstructed[val.node_id]= bv;
    }
    uint8_t* p= reinterpret_cast<uint8_t*> (s7_byte_vector_elements (bv));
    if (len > 0) {
      std::memcpy (p, val.bytevec_val->data (), len);
    }
    return bv;
  }
  case GFValueType::Eof:
    return s7_eof_object (sc);
  case GFValueType::Let: {
    // 先建空壳并注册，使环/共享引用可解析，alist 由主循环构建后再填充字段
    s7_pointer let= s7_inlet (sc, s7_nil (sc));
    anchor (let);
    if (val.node_id != 0) {
      ctx.reconstructed[val.node_id]= let;
    }
    if (val.pair_val) {
      work_stack.push_back ({FillItem::Slot::LetContent, let, 0, &val.pair_val->first});
    }
    return let;
  }
  case GFValueType::Undefined:
  default:
    return s7_undefined (sc);
  }
}

s7_pointer
gfvalue_to_s7 (s7_scheme* sc, const GFValue& val) {
  DeserializeCtx        ctx;
  std::vector<FillItem> work_stack;
  switch (val.type) {
  // 只有复合类型需要 GC anchor 与环重建表；标量直接转换，零额外开销
  case GFValueType::Pair:
  case GFValueType::Vector:
  case GFValueType::ByteVector:
  case GFValueType::Let:
  case GFValueType::Ref:
    ctx.gc_anchor= s7_nil (sc);
    ctx.gc_loc   = s7_gc_protect (sc, ctx.gc_anchor);
    {
      s7_pointer res= build_node (sc, val, ctx, work_stack);
      while (!work_stack.empty ()) {
        auto item= work_stack.back ();
        work_stack.pop_back ();
        switch (item.slot) {
        case FillItem::Slot::PairCar:
          s7_set_car (item.target, build_node (sc, *item.src, ctx, work_stack));
          break;
        case FillItem::Slot::PairCdr:
          s7_set_cdr (item.target, build_node (sc, *item.src, ctx, work_stack));
          break;
        case FillItem::Slot::VectorElem:
          s7_vector_set (sc, item.target, item.index, build_node (sc, *item.src, ctx, work_stack));
          break;
        case FillItem::Slot::LetContent: {
          // 先占位填充任务再构建 alist，LIFO 保证 alist 子树全部完成后才填充
          size_t fill_idx= work_stack.size ();
          work_stack.push_back ({FillItem::Slot::FillLet, item.target, 0, nullptr, nullptr});
          s7_pointer alist_root       = build_node (sc, *item.src, ctx, work_stack);
          work_stack[fill_idx].payload= alist_root;
          break;
        }
        case FillItem::Slot::FillLet:
          // payload 是构建完成的 alist，逐字段填入 let 壳
          for (s7_pointer p= item.payload; s7_is_pair (p); p= s7_cdr (p)) {
            s7_pointer entry= s7_car (p);
            if (s7_is_pair (entry) && s7_is_symbol (s7_car (entry))) {
              s7_varlet (sc, item.target, s7_car (entry), s7_cdr (entry));
            }
          }
          break;
        }
      }
      s7_gc_unprotect_at (sc, ctx.gc_loc);
      return res;
    }
  default:
    return build_node (sc, val, ctx, work_stack);
  }
}

// ---------------------------------------------------------------------------
// GoldfishChannel Implementation
// ---------------------------------------------------------------------------

void
gfvalue_destroy_deep (GFValue& root) {
  // 把复合子节点的 shared_ptr 移入工作清单再销毁，析构永不递归
  std::vector<std::shared_ptr<std::pair<GFValue, GFValue>>> pairs;
  std::vector<std::shared_ptr<std::vector<GFValue>>>        vecs;
  auto                                                      drain= [&pairs, &vecs] (GFValue& v) {
    if (v.pair_val) pairs.push_back (std::move (v.pair_val));
    if (v.vec_val) vecs.push_back (std::move (v.vec_val));
  };
  drain (root);
  while (!pairs.empty () || !vecs.empty ()) {
    if (!pairs.empty ()) {
      auto p= std::move (pairs.back ());
      pairs.pop_back ();
      drain (p->first);
      drain (p->second);
      // p 离开作用域时其子节点的复合指针已掏空，析构不再递归
    }
    else {
      auto v= std::move (vecs.back ());
      vecs.pop_back ();
      for (auto& elem : *v) {
        drain (elem);
      }
    }
  }
}

GoldfishChannel::~GoldfishChannel () {
  close ();
  for (auto& v : buffer) {
    gfvalue_destroy_deep (v);
  }
}

GoldfishChannel::SendStatus
GoldfishChannel::send (GFValue val, int64_t timeout_ms) {
  std::unique_lock<std::mutex> lock (mtx);
  if (closed) return SendStatus::Closed;

  if (capacity == 0) {
    // Unbuffered channel: rendezvous
    if (!waiting_receivers.empty ()) {
      RendezvousReceiver* r= waiting_receivers.front ();
      waiting_receivers.pop_front ();
      *(r->out)   = std::move (val);
      r->completed= true;
      r->cv.notify_one ();
      return SendStatus::Ok;
    }

    if (timeout_ms == 0) {
      return SendStatus::Timeout;
    }

    RendezvousSender s;
    s.val= std::move (val);
    waiting_senders.push_back (&s);
    notify_select_recv_ready (); // 出现 rendezvous 发送者，select 接收方可就绪

    if (timeout_ms < 0) {
      s.cv.wait (lock, [&s, this] () { return s.completed || closed; });
    }
    else {
      s.cv.wait_for (lock, std::chrono::milliseconds (timeout_ms), [&s, this] () { return s.completed || closed; });
    }

    if (!s.completed) {
      for (auto it= waiting_senders.begin (); it != waiting_senders.end (); ++it) {
        if (*it == &s) {
          waiting_senders.erase (it);
          break;
        }
      }
      gfvalue_destroy_deep (s.val); // 超时/关闭导致发送失败，载荷不再传递，防止深层析构递归
      return closed ? SendStatus::Closed : SendStatus::Timeout;
    }
    return SendStatus::Ok;
  }
  else {
    // Buffered channel
    if (timeout_ms < 0) {
      cv_buf_send.wait (lock, [this] () { return closed || buffer.size () < capacity; });
    }
    else if (timeout_ms == 0) {
      if (closed) return SendStatus::Closed;
      if (buffer.size () >= capacity) return SendStatus::Timeout;
    }
    else {
      bool ok= cv_buf_send.wait_for (lock, std::chrono::milliseconds (timeout_ms),
                                     [this] () { return closed || buffer.size () < capacity; });
      if (!ok && !closed && buffer.size () >= capacity) {
        return SendStatus::Timeout;
      }
    }

    if (closed) return SendStatus::Closed;

    buffer.push_back (std::move (val));
    cv_buf_recv.notify_one ();
    notify_select_recv_ready (); // 缓冲有新数据，select 接收方可就绪
    return SendStatus::Ok;
  }
}

GoldfishChannel::RecvStatus
GoldfishChannel::recv (GFValue& out, int64_t timeout_ms) {
  std::unique_lock<std::mutex> lock (mtx);

  if (capacity == 0) {
    if (!waiting_senders.empty ()) {
      RendezvousSender* s= waiting_senders.front ();
      waiting_senders.pop_front ();
      out         = std::move (s->val);
      s->completed= true;
      s->cv.notify_one ();
      return RecvStatus::Ok;
    }

    if (closed) {
      return RecvStatus::Closed;
    }

    if (timeout_ms == 0) {
      return RecvStatus::Timeout;
    }

    RendezvousReceiver r;
    r.out= &out;
    waiting_receivers.push_back (&r);
    notify_select_send_ready (); // 出现 rendezvous 接收者，select 发送方可就绪

    if (timeout_ms < 0) {
      r.cv.wait (lock, [&r, this] () { return r.completed || closed; });
    }
    else {
      r.cv.wait_for (lock, std::chrono::milliseconds (timeout_ms), [&r, this] () { return r.completed || closed; });
    }

    if (!r.completed) {
      for (auto it= waiting_receivers.begin (); it != waiting_receivers.end (); ++it) {
        if (*it == &r) {
          waiting_receivers.erase (it);
          break;
        }
      }
      return closed ? RecvStatus::Closed : RecvStatus::Timeout;
    }
    return RecvStatus::Ok;
  }
  else {
    // Buffered channel
    if (buffer.empty () && !closed) {
      if (timeout_ms == 0) {
        return RecvStatus::Timeout;
      }
      else if (timeout_ms < 0) {
        cv_buf_recv.wait (lock, [this] () { return closed || !buffer.empty (); });
      }
      else {
        cv_buf_recv.wait_for (lock, std::chrono::milliseconds (timeout_ms),
                              [this] () { return closed || !buffer.empty (); });
      }
    }

    if (!buffer.empty ()) {
      out= std::move (buffer.front ());
      buffer.pop_front ();
      cv_buf_send.notify_one ();
      notify_select_send_ready (); // 缓冲腾出空间，select 发送方可就绪
      return RecvStatus::Ok;
    }

    return closed ? RecvStatus::Closed : RecvStatus::Timeout;
  }
}

void
GoldfishChannel::close () {
  std::unique_lock<std::mutex> lock (mtx);
  if (closed) return;
  closed= true;

  // Wake up all buffered waiters
  cv_buf_send.notify_all ();
  cv_buf_recv.notify_all ();

  // Wake up all rendezvous waiters
  for (auto* s : waiting_senders) {
    s->cv.notify_one ();
  }
  waiting_senders.clear ();

  for (auto* r : waiting_receivers) {
    r->cv.notify_one ();
  }
  waiting_receivers.clear ();

  // 关闭使 select 的接收方（读出 eof）与发送方（报错）都就绪
  notify_select_recv_ready ();
  notify_select_send_ready ();
}

bool
GoldfishChannel::is_closed () const {
  std::lock_guard<std::mutex> lock (mtx);
  return closed;
}

// ---------------------------------------------------------------------------
// select wait-set 支持
// ---------------------------------------------------------------------------

// 前置：调用方持有 mtx。唤醒所有登记的可读等待者（只通知，不搬运数据）。
void
GoldfishChannel::notify_select_recv_ready () {
  while (!select_recv_waiters.empty ()) {
    SelectWaiter* w= select_recv_waiters.front ();
    select_recv_waiters.pop_front ();
    {
      std::lock_guard<std::mutex> lk (w->mtx);
      w->ready= true;
    }
    w->cv.notify_one ();
  }
  // 同时触发 fiber watcher（一次性）
  while (!watch_recv_entries.empty ()) {
    auto e= std::move (watch_recv_entries.front ());
    watch_recv_entries.pop_front ();
    e.gate->fire (e.id);
  }
}

// 前置：调用方持有 mtx。唤醒所有登记的可写等待者。
void
GoldfishChannel::notify_select_send_ready () {
  while (!select_send_waiters.empty ()) {
    SelectWaiter* w= select_send_waiters.front ();
    select_send_waiters.pop_front ();
    {
      std::lock_guard<std::mutex> lk (w->mtx);
      w->ready= true;
    }
    w->cv.notify_one ();
  }
  while (!watch_send_entries.empty ()) {
    auto e= std::move (watch_send_entries.front ());
    watch_send_entries.pop_front ();
    gfvalue_destroy_deep (e.payload); // 未被交接的载荷不再传递，防深层析构递归
    e.gate->fire (e.id);
  }
}

// 前置：调用方持有 mtx。与 send (val, 0) 等价；成功时才移动 val。
GoldfishChannel::SendStatus
GoldfishChannel::try_send_unlocked (GFValue& val) {
  if (closed) return SendStatus::Closed;
  if (capacity == 0) {
    if (!waiting_receivers.empty ()) {
      RendezvousReceiver* r= waiting_receivers.front ();
      waiting_receivers.pop_front ();
      *(r->out)   = std::move (val);
      r->completed= true;
      r->cv.notify_one ();
      return SendStatus::Ok;
    }
    return SendStatus::Timeout;
  }
  if (buffer.size () >= capacity) return SendStatus::Timeout;
  buffer.push_back (std::move (val));
  cv_buf_recv.notify_one ();
  notify_select_recv_ready ();
  return SendStatus::Ok;
}

// 前置：调用方持有 mtx。与 recv (out, 0) 等价。
GoldfishChannel::RecvStatus
GoldfishChannel::try_recv_unlocked (GFValue& out) {
  if (capacity == 0) {
    if (!waiting_senders.empty ()) {
      RendezvousSender* s= waiting_senders.front ();
      waiting_senders.pop_front ();
      out         = std::move (s->val);
      s->completed= true;
      s->cv.notify_one ();
      return RecvStatus::Ok;
    }
    return closed ? RecvStatus::Closed : RecvStatus::Timeout;
  }
  if (!buffer.empty ()) {
    out= std::move (buffer.front ());
    buffer.pop_front ();
    cv_buf_send.notify_one ();
    notify_select_send_ready ();
    return RecvStatus::Ok;
  }
  return closed ? RecvStatus::Closed : RecvStatus::Timeout;
}

GoldfishChannel::SendStatus
GoldfishChannel::select_try_send_or_wait (GFValue& val, SelectWaiter* w) {
  std::unique_lock<std::mutex> lock (mtx);
  SendStatus                   st= try_send_unlocked (val);
  if (st == SendStatus::Timeout) {
    select_send_waiters.push_back (w);
  }
  return st;
}

GoldfishChannel::RecvStatus
GoldfishChannel::select_try_recv_or_wait (GFValue& out, SelectWaiter* w) {
  std::unique_lock<std::mutex> lock (mtx);
  RecvStatus                   st= try_recv_unlocked (out);
  if (st == RecvStatus::Timeout) {
    select_recv_waiters.push_back (w);
  }
  return st;
}

void
GoldfishChannel::select_remove_waiter (SelectWaiter* w) {
  std::lock_guard<std::mutex> lock (mtx);
  auto                        rm= [w] (std::deque<SelectWaiter*>& q) {
    for (auto it= q.begin (); it != q.end ();) {
      if (*it == w) it= q.erase (it);
      else ++it;
    }
  };
  rm (select_recv_waiters);
  rm (select_send_waiters);
}

GoldfishChannel::RecvStatus
GoldfishChannel::recv_or_watch (GFValue& out, std::shared_ptr<GoGate> gate, int64_t id) {
  std::unique_lock<std::mutex> lock (mtx);
  RecvStatus                   st= try_recv_unlocked (out);
  if (st != RecvStatus::Timeout) return st;

  // fiber↔fiber rendezvous：send watcher 带载荷等待时直接交接——
  // 取走载荷、fire 该 send watcher（其重试经 completed_send_ids 确认完成）
  for (auto it= watch_send_entries.begin (); it != watch_send_entries.end (); ++it) {
    if (it->has_payload) {
      out           = std::move (it->payload);
      auto    s_gate= it->gate;
      int64_t s_id  = it->id;
      completed_send_ids.insert (s_id);
      watch_send_entries.erase (it);
      s_gate->fire (s_id);
      return RecvStatus::Ok;
    }
  }

  watch_recv_entries.push_back ({std::move (gate), id, false, GFValue{}});
  return RecvStatus::Timeout;
}

GoldfishChannel::SendStatus
GoldfishChannel::send_or_watch (GFValue& val, std::shared_ptr<GoGate> gate, int64_t id) {
  std::unique_lock<std::mutex> lock (mtx);
  // 载荷已被 recv watcher 取走（fire 唤醒后的重试）：确认完成，不重发
  if (completed_send_ids.count (id) != 0) {
    completed_send_ids.erase (id);
    return SendStatus::Ok;
  }
  SendStatus st= try_send_unlocked (val);
  if (st == SendStatus::Timeout) {
    // 登记带载荷的 watcher，并唤醒所有挂起的 recv watcher 来竞争交接
    // （竞争输家重试 recv_or_watch 时载荷已被取走，会重新登记 watcher）
    watch_send_entries.push_back ({std::move (gate), id, true, std::move (val)});
    while (!watch_recv_entries.empty ()) {
      auto e= std::move (watch_recv_entries.front ());
      watch_recv_entries.pop_front ();
      e.gate->fire (e.id);
    }
  }
  return st;
}

// ---------------------------------------------------------------------------
// S7 Glue Functions
// ---------------------------------------------------------------------------

// 通道对象的实际构造拆到独立函数：含 make_shared 的函数帧会携带 C++ EH 展开信息，
// 与 s7_error 的裸 longjmp 在 Windows/MSVC 下交互存在风险（CI 曾出现布局敏感的崩溃），
// raise 路径所在函数保持帧内无重内容。
static s7_pointer
make_chan_impl (s7_scheme* sc, s7_int cap) {
  auto ch= std::make_shared<GoldfishChannel> (static_cast<size_t> (cap));
  return make_goldfish_channel_object (sc, ch);
}

static s7_pointer
f_make_chan (s7_scheme* sc, s7_pointer args) {
  s7_int cap= 0;
  if (!s7_is_null (sc, args)) {
    s7_pointer cap_arg= s7_car (args);
    if (!s7_is_integer (cap_arg) || s7_integer (cap_arg) < 0) {
      return go_error (sc, "type-error", "make-chan: capacity must be a non-negative integer", cap_arg);
    }
    cap= s7_integer (cap_arg);
  }
  return make_chan_impl (sc, cap);
}

static s7_pointer
f_chan_p (s7_scheme* sc, s7_pointer args) {
  return s7_make_boolean (sc, is_goldfish_channel (sc, s7_car (args)));
}

static s7_pointer
f_chan_send (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg = s7_car (args);
  s7_pointer val_arg= s7_cadr (args);

  // 以下检查点只允许平凡局部变量存活（见 go_error 的 longjmp 约束注释）
  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "chan-send!: first argument must be a channel", ch_arg);
  }

  int64_t    timeout_ms= -1; // Default -1: infinite wait (Go channel semantics)
  s7_pointer rest      = s7_cddr (args);
  if (!s7_is_null (sc, rest)) {
    s7_pointer to_arg= s7_car (rest);
    if (!s7_is_integer (to_arg) || s7_integer (to_arg) < 0) {
      return go_error (sc, "type-error", "chan-send!: timeout must be a non-negative integer (milliseconds)", to_arg);
    }
    timeout_ms= s7_integer (to_arg);
  }

  // RAII 对象（GFValue/shared_ptr/string）限制在内层作用域，出作用域后再 raise
  GoldfishChannel::SendStatus status= GoldfishChannel::SendStatus::Timeout;
  bool                        ser_ok= false;
  {
    GFValue val;
    ser_ok= s7_to_gfvalue (sc, val_arg, val, t_serialize_err);
    if (ser_ok) {
      status= get_goldfish_channel (sc, ch_arg)->send (std::move (val), timeout_ms);
    }
    gfvalue_destroy_deep (val); // 序列化失败时 val 可能是深层部分树
  }

  if (!ser_ok) {
    return go_error (sc, "type-error", t_serialize_err.c_str (), val_arg);
  }
  if (status == GoldfishChannel::SendStatus::Closed) {
    return go_error (sc, "value-error", "chan-send!: cannot send on closed channel", ch_arg);
  }
  else if (status == GoldfishChannel::SendStatus::Timeout) {
    return s7_f (sc);
  }
  return s7_t (sc);
}

static s7_pointer
f_chan_recv (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg= s7_car (args);

  // raise 时只允许平凡局部变量存活（见 go_error 的 longjmp 约束注释）
  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "chan-recv!: first argument must be a channel", ch_arg);
  }

  int64_t    timeout_ms = -1; // Default -1: infinite wait (Go channel semantics)
  s7_pointer default_val= s7_make_symbol (sc, "timeout");

  s7_pointer rest= s7_cdr (args);
  if (!s7_is_null (sc, rest)) {
    s7_pointer to_arg= s7_car (rest);
    if (!s7_is_integer (to_arg) || s7_integer (to_arg) < 0) {
      return go_error (sc, "type-error", "chan-recv!: timeout must be a non-negative integer (milliseconds)", to_arg);
    }
    timeout_ms= s7_integer (to_arg);

    s7_pointer rest2= s7_cdr (rest);
    if (!s7_is_null (sc, rest2)) {
      default_val= s7_car (rest2);
    }
  }

  auto    ch= get_goldfish_channel (sc, ch_arg);
  GFValue val;
  auto    status= ch->recv (val, timeout_ms);

  if (status == GoldfishChannel::RecvStatus::Closed) {
    return s7_eof_object (sc);
  }
  else if (status == GoldfishChannel::RecvStatus::Timeout) {
    return default_val;
  }
  s7_pointer res= gfvalue_to_s7 (sc, val);
  gfvalue_destroy_deep (val); // 转换完成后显式迭代释放，防止深层析构递归
  return res;
}

static s7_pointer
f_chan_try_recv (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg= s7_car (args);

  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "chan-try-recv!: first argument must be a channel", ch_arg);
  }

  s7_pointer default_val= s7_f (sc);
  s7_pointer rest       = s7_cdr (args);
  if (!s7_is_null (sc, rest)) {
    default_val= s7_car (rest);
  }

  auto    ch= get_goldfish_channel (sc, ch_arg);
  GFValue val;
  auto    status= ch->try_recv (val);

  if (status == GoldfishChannel::RecvStatus::Ok) {
    s7_pointer res= gfvalue_to_s7 (sc, val);
    gfvalue_destroy_deep (val); // 转换完成后显式迭代释放，防止深层析构递归
    return res;
  }
  else if (status == GoldfishChannel::RecvStatus::Closed) {
    return s7_eof_object (sc);
  }
  else {
    // Timeout (非阻塞语义下即为通道为空)
    return default_val;
  }
}

static s7_pointer
f_chan_close (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg= s7_car (args);

  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "chan-close!: argument must be a channel", ch_arg);
  }

  auto ch= get_goldfish_channel (sc, ch_arg);
  ch->close ();
  return s7_unspecified (sc);
}

static s7_pointer
f_chan_closed_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg= s7_car (args);

  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "chan-closed?: argument must be a channel", ch_arg);
  }

  auto ch= get_goldfish_channel (sc, ch_arg);
  return s7_make_boolean (sc, ch->is_closed ());
}

struct SpawnError {
  const char* kind= nullptr;
  const char* msg = nullptr;
  s7_pointer  arg = nullptr;
};

// 所有 RAII 对象（GoTask/GFValue/string）都在本函数帧内析构；
// 调用方拿到错误信息后再 raise（见 go_error 的 longjmp 约束注释）。
static SpawnError
try_spawn_task (s7_scheme* sc, s7_pointer names_arg, s7_pointer vals_arg, s7_pointer code_arg) {
  GoTask     task;
  s7_pointer cur_name= names_arg;
  while (s7_is_pair (cur_name)) {
    s7_pointer sym= s7_car (cur_name);
    if (!s7_is_symbol (sym)) {
      return {"type-error", "go: variable name must be a symbol", sym};
    }
    task.var_names.push_back (s7_symbol_name (sym));
    cur_name= s7_cdr (cur_name);
  }

  s7_pointer cur_val= vals_arg;
  while (s7_is_pair (cur_val)) {
    GFValue val;
    if (!s7_to_gfvalue (sc, s7_car (cur_val), val, t_serialize_err)) {
      // t_serialize_err 是 TLS，在调用方 raise 时仍然有效
      return {"type-error", t_serialize_err.c_str (), s7_car (cur_val)};
    }
    task.var_vals.push_back (std::move (val));
    cur_val= s7_cdr (cur_val);
  }

  if (task.var_names.size () != task.var_vals.size ()) {
    return {"value-error", "go: variable names and values count mismatch", names_arg};
  }

  if (!s7_to_gfvalue (sc, code_arg, task.code_expr, t_serialize_err)) {
    return {"type-error", t_serialize_err.c_str (), code_arg};
  }

  GoThreadPool::instance ().enqueue (std::move (task));
  return {};
}

static s7_pointer
f_go_spawn (s7_scheme* sc, s7_pointer args) {
  SpawnError err= try_spawn_task (sc, s7_car (args), s7_cadr (args), s7_caddr (args));
  if (err.kind != nullptr) {
    return go_error (sc, err.kind, err.msg, err.arg);
  }
  return s7_unspecified (sc);
}

static s7_pointer
f_go_worker_count (s7_scheme* sc, s7_pointer args) {
  // 只返回配置值，不触发线程池构造
  return s7_make_integer (sc, static_cast<s7_int> (configured_worker_count ()));
}

// ---------------------------------------------------------------------------
// select wait-set glue
// ---------------------------------------------------------------------------

namespace {

struct SelectOutcome {
  int         kind       = -2; // 0=recv 就绪, 1=send 就绪, -2=超时
  int         index      = -1;
  s7_pointer  value      = nullptr;
  bool        closed_send= false;   // send 分支遇到已关闭通道
  const char* err_kind   = nullptr; // 解析/序列化错误（raise 由外层无 RAII 区执行）
  s7_pointer  err_arg    = nullptr;
};

SelectOutcome
select_impl (s7_scheme* sc, s7_pointer recv_list, s7_pointer send_list, int64_t timeout_ms) {
  SelectOutcome out;

  // 提取 channel 与序列化 send 载荷（RAII 区，全部在本函数内析构）
  std::vector<std::shared_ptr<GoldfishChannel>> recv_chs;
  for (s7_pointer p= recv_list; s7_is_pair (p); p= s7_cdr (p)) {
    auto ch= get_goldfish_channel (sc, s7_car (p));
    if (!ch) {
      out.err_kind   = "type-error";
      out.err_arg    = s7_car (p);
      t_serialize_err= "select: recv clause expects a channel";
      return out;
    }
    recv_chs.push_back (std::move (ch));
  }

  std::vector<std::pair<std::shared_ptr<GoldfishChannel>, GFValue>> send_cases;
  for (s7_pointer p= send_list; s7_is_pair (p); p= s7_cdr (p)) {
    s7_pointer pair= s7_car (p);
    auto       ch  = get_goldfish_channel (sc, s7_car (pair));
    if (!ch) {
      out.err_kind   = "type-error";
      out.err_arg    = s7_car (pair);
      t_serialize_err= "select: send clause expects a channel";
      return out;
    }
    GFValue val;
    if (!s7_to_gfvalue (sc, s7_cdr (pair), val, t_serialize_err)) {
      out.err_kind= "type-error";
      out.err_arg = s7_cdr (pair);
      gfvalue_destroy_deep (val);
      return out;
    }
    send_cases.push_back ({std::move (ch), std::move (val)});
  }

  SelectWaiter                  w;
  std::vector<GoldfishChannel*> registered;
  const auto deadline= std::chrono::steady_clock::now () + std::chrono::milliseconds (timeout_ms < 0 ? 0 : timeout_ms);

  auto deregister_all= [&] () {
    for (auto* ch : registered) {
      ch->select_remove_waiter (&w);
    }
    registered.clear ();
  };

  // 统一整合 recv 与 send 分支，按伪随机洗牌顺序探测（Go select 规范：多 case 就绪时均匀随机选择）
  enum class CaseKind { Recv, Send };
  struct CaseDesc {
    CaseKind         kind;
    size_t           orig_idx;
    GoldfishChannel* ch;
    GFValue*         send_val;
  };

  std::vector<CaseDesc> all_cases;
  all_cases.reserve (recv_chs.size () + send_cases.size ());
  for (size_t i= 0; i < recv_chs.size (); ++i) {
    all_cases.push_back ({CaseKind::Recv, i, recv_chs[i].get (), nullptr});
  }
  for (size_t i= 0; i < send_cases.size (); ++i) {
    all_cases.push_back ({CaseKind::Send, i, send_cases[i].first.get (), &send_cases[i].second});
  }

  std::vector<size_t> poll_order (all_cases.size ());
  for (size_t i= 0; i < all_cases.size (); ++i) {
    poll_order[i]= i;
  }

  static thread_local std::minstd_rand tls_rng ([] () {
    try {
      return std::minstd_rand (std::random_device{}());
    } catch (...) {
      return std::minstd_rand (
          static_cast<unsigned int> (std::chrono::high_resolution_clock::now ().time_since_epoch ().count ()));
    }
  }());

  bool done= false;
  while (!done) {
    std::shuffle (poll_order.begin (), poll_order.end (), tls_rng);

    // 按随机洗牌顺序探测：就绪立即执行，未就绪则登记等待
    for (size_t idx : poll_order) {
      const auto& c= all_cases[idx];
      if (c.kind == CaseKind::Recv) {
        GFValue val;
        auto    st= c.ch->select_try_recv_or_wait (val, &w);
        if (st == GoldfishChannel::RecvStatus::Timeout) {
          registered.push_back (c.ch);
          continue;
        }
        out.kind = 0;
        out.index= static_cast<int> (c.orig_idx);
        out.value= (st == GoldfishChannel::RecvStatus::Closed) ? s7_eof_object (sc) : gfvalue_to_s7 (sc, val);
        gfvalue_destroy_deep (val);
        done= true;
        break;
      }
      else {
        GFValue tmp= *c.send_val; // 浅拷贝（shared_ptr），成功时才被消耗
        auto    st = c.ch->select_try_send_or_wait (tmp, &w);
        if (st == GoldfishChannel::SendStatus::Timeout) {
          registered.push_back (c.ch);
          continue;
        }
        gfvalue_destroy_deep (tmp);
        if (st == GoldfishChannel::SendStatus::Closed) {
          out.closed_send= true;
        }
        else {
          out.kind= 1;
        }
        out.index= static_cast<int> (c.orig_idx);
        done     = true;
        break;
      }
    }
    if (done) break;
    if (timeout_ms == 0) break; // 非阻塞语义（宏的 default 分支）

    // 等待唤醒或超时（按绝对截止时间，竞争失败重试时不重复计费）
    bool expired= false;
    {
      std::unique_lock<std::mutex> lk (w.mtx);
      if (timeout_ms < 0) {
        w.cv.wait (lk, [&w] () { return w.ready; });
      }
      else {
        expired= !w.cv.wait_until (lk, deadline, [&w] () { return w.ready; });
      }
      w.ready= false;
    }
    deregister_all ();
    if (expired) break; // out.kind 保持 -2 超时
    // 被唤醒：回到快速路径重试（就绪事件可能被竞争者抢先消费，重试会重新登记）
  }

  deregister_all ();
  for (auto& c : send_cases) {
    gfvalue_destroy_deep (c.second);
  }
  return out;
}

} // namespace

static s7_pointer
f_select (s7_scheme* sc, s7_pointer args) {
  s7_pointer recv_list  = s7_car (args);
  s7_pointer send_list  = s7_cadr (args);
  s7_pointer timeout_arg= s7_caddr (args);

  // raise 安全区：只允许平凡局部变量存活（见 go_error 的 longjmp 约束注释）
  if (!s7_is_list (sc, recv_list) || !s7_is_list (sc, send_list)) {
    return go_error (sc, "type-error", "g_select: first two arguments must be lists", args);
  }
  if (!s7_is_integer (timeout_arg) || s7_integer (timeout_arg) < -1) {
    return go_error (sc, "type-error", "g_select: timeout must be -1 (infinite) or non-negative milliseconds",
                     timeout_arg);
  }

  SelectOutcome out= select_impl (sc, recv_list, send_list, s7_integer (timeout_arg));

  if (out.err_kind != nullptr) {
    return go_error (sc, out.err_kind, t_serialize_err.c_str (), out.err_arg);
  }
  if (out.closed_send) {
    return go_error (sc, "value-error", "select: cannot send on closed channel", s7_nil (sc));
  }
  if (out.kind == -2) {
    return s7_f (sc); // 超时
  }
  if (out.kind == 0) {
    // recv 就绪：#(0 idx val)。out.value 未扎根，构建结果前先保护
    s7_int     loc= s7_gc_protect (sc, out.value);
    s7_pointer res= s7_make_vector (sc, 3);
    s7_vector_set (sc, res, 0, s7_make_integer (sc, 0));
    s7_vector_set (sc, res, 1, s7_make_integer (sc, out.index));
    s7_vector_set (sc, res, 2, out.value);
    s7_gc_unprotect_at (sc, loc);
    return res;
  }
  // send 就绪：#(1 idx)
  s7_pointer res= s7_make_vector (sc, 2);
  s7_vector_set (sc, res, 0, s7_make_integer (sc, 1));
  s7_vector_set (sc, res, 1, s7_make_integer (sc, out.index));
  return res;
}

static s7_pointer
f_chan_timeout_close (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg= s7_car (args);
  s7_pointer ms_arg= s7_cadr (args);

  // raise 时只允许平凡局部变量存活（见 go_error 的 longjmp 约束注释）
  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "g_chan-timeout-close!: first argument must be a channel", ch_arg);
  }
  if (!s7_is_integer (ms_arg) || s7_integer (ms_arg) < 0) {
    return go_error (sc, "type-error", "g_chan-timeout-close!: ms must be a non-negative integer", ms_arg);
  }

  GoTimer::instance ().schedule_close (get_goldfish_channel (sc, ch_arg), s7_integer (ms_arg));
  return s7_unspecified (sc);
}

static s7_pointer
f_make_gate (s7_scheme* sc, s7_pointer args) {
  // 构造拆到独立函数调用（帧内无重内容原则，见 make_chan_impl 注释）。
  // gate fire 时唤醒线程池：空闲 worker 能及时进调度器消费事件，
  // 否则挂起 fiber 的唤醒要等到该会话下一个任务才发生
  auto gate= std::make_shared<GoGate> ();
  gate->set_on_fire ([] () { GoThreadPool::instance ().notify_workers (); });
  return make_go_gate_object (sc, std::move (gate));
}

static s7_pointer
f_gate_wait (s7_scheme* sc, s7_pointer args) {
  s7_pointer gate_arg= s7_car (args);
  if (!is_go_gate (sc, gate_arg)) {
    return go_error (sc, "type-error", "g_gate-wait: argument must be a gate", gate_arg);
  }
  int64_t id= 0;
  {
    auto gate= get_go_gate (sc, gate_arg);
    id       = gate->wait ();
  }
  return s7_make_integer (sc, id);
}

static s7_pointer
f_gate_try_wait (s7_scheme* sc, s7_pointer args) {
  s7_pointer gate_arg= s7_car (args);
  // raise 安全区：平凡局部变量（见 go_error 的 longjmp 约束注释）
  if (!is_go_gate (sc, gate_arg)) {
    return go_error (sc, "type-error", "g_gate-try-wait: argument must be a gate", gate_arg);
  }
  int64_t id = 0;
  bool    got= false;
  {
    auto gate= get_go_gate (sc, gate_arg);
    got      = gate->try_wait (id);
  }
  if (!got) return s7_f (sc);
  return s7_make_integer (sc, id);
}

// (g_chan-recv-or-watch! ch gate id default)
// 原子地：尝试非阻塞 recv；不可行则登记一次性 watcher（就绪时 gate fire id）。
// 返回：收到的值 | eof-object（通道关闭）| default（已登记 watcher）
static s7_pointer
f_chan_recv_or_watch (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg  = s7_car (args);
  s7_pointer gate_arg= s7_cadr (args);
  s7_pointer id_arg  = s7_caddr (args);
  s7_pointer dflt    = s7_cadddr (args);

  // raise 安全区：平凡局部变量（见 go_error 的 longjmp 约束注释）
  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "g_chan-recv-or-watch!: first argument must be a channel", ch_arg);
  }
  if (!is_go_gate (sc, gate_arg)) {
    return go_error (sc, "type-error", "g_chan-recv-or-watch!: second argument must be a gate", gate_arg);
  }
  if (!s7_is_integer (id_arg)) {
    return go_error (sc, "type-error", "g_chan-recv-or-watch!: third argument must be an integer id", id_arg);
  }

  GoldfishChannel::RecvStatus st;
  s7_pointer                  res= nullptr;
  {
    GFValue val;
    st= get_goldfish_channel (sc, ch_arg)->recv_or_watch (val, get_go_gate (sc, gate_arg), s7_integer (id_arg));
    if (st == GoldfishChannel::RecvStatus::Ok) {
      res= gfvalue_to_s7 (sc, val);
    }
    gfvalue_destroy_deep (val);
  }
  if (st == GoldfishChannel::RecvStatus::Ok) return res;
  if (st == GoldfishChannel::RecvStatus::Closed) return s7_eof_object (sc);
  return dflt; // Timeout：已登记 watcher
}

// (g_chan-send-or-watch! ch val gate id default)
// 原子地：尝试非阻塞 send；不可行则登记一次性 watcher。
// 返回：#t（成功）| default（已登记 watcher）；通道关闭抛 value-error
static s7_pointer
f_chan_send_or_watch (s7_scheme* sc, s7_pointer args) {
  s7_pointer ch_arg  = s7_car (args);
  s7_pointer val_arg = s7_cadr (args);
  s7_pointer gate_arg= s7_caddr (args);
  s7_pointer id_arg  = s7_cadddr (args);
  s7_pointer dflt    = s7_car (s7_cddddr (args));

  // raise 安全区：平凡局部变量
  if (!is_goldfish_channel (sc, ch_arg)) {
    return go_error (sc, "type-error", "g_chan-send-or-watch!: first argument must be a channel", ch_arg);
  }
  if (!is_go_gate (sc, gate_arg)) {
    return go_error (sc, "type-error", "g_chan-send-or-watch!: third argument must be a gate", gate_arg);
  }
  if (!s7_is_integer (id_arg)) {
    return go_error (sc, "type-error", "g_chan-send-or-watch!: fourth argument must be an integer id", id_arg);
  }

  GoldfishChannel::SendStatus st    = GoldfishChannel::SendStatus::Timeout;
  bool                        ser_ok= false;
  {
    GFValue val;
    ser_ok= s7_to_gfvalue (sc, val_arg, val, t_serialize_err);
    if (ser_ok) {
      st= get_goldfish_channel (sc, ch_arg)->send_or_watch (val, get_go_gate (sc, gate_arg), s7_integer (id_arg));
    }
    gfvalue_destroy_deep (val);
  }
  if (!ser_ok) {
    return go_error (sc, "type-error", t_serialize_err.c_str (), val_arg);
  }
  if (st == GoldfishChannel::SendStatus::Closed) {
    return go_error (sc, "value-error", "g_chan-send-or-watch!: cannot send on closed channel", ch_arg);
  }
  if (st == GoldfishChannel::SendStatus::Ok) return s7_t (sc);
  return dflt; // Timeout：已登记 watcher
}

static s7_pointer
f_msleep (s7_scheme* sc, s7_pointer args) {
  s7_pointer ms_arg= s7_car (args);
  if (!s7_is_integer (ms_arg) || s7_integer (ms_arg) < 0) {
    return go_error (sc, "type-error", "g_msleep: ms must be a non-negative integer", ms_arg);
  }
  int64_t ms= s7_integer (ms_arg);
  if (ms > 0) {
    std::this_thread::sleep_for (std::chrono::milliseconds (ms));
  }
  else {
    std::this_thread::yield ();
  }
  return s7_unspecified (sc);
}

static s7_pointer
f_now_ms (s7_scheme* sc, s7_pointer args) {
  auto now= std::chrono::steady_clock::now ();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (now.time_since_epoch ()).count ();
  return s7_make_integer (sc, static_cast<s7_int> (ms));
}

void
glue_liii_go (s7_scheme* sc) {
  // Register C-Type for channel
  s7_int tag= s7_make_c_type (sc, "channel");
  s7_c_type_set_free (sc, tag, channel_free_c_value);
  s7_c_type_set_to_string (sc, tag, channel_to_string_glue);
  s7_c_type_set_is_equal (sc, tag, channel_is_equal_glue);

  {
    std::lock_guard<std::mutex> lock (g_type_mtx);
    g_channel_type_tags[sc]= tag;
  }

  // Register C-Type for go-gate（fiber 跨线程唤醒门闩）
  s7_int gate_tag= s7_make_c_type (sc, "go-gate");
  s7_c_type_set_free (sc, gate_tag, gate_free_c_value);
  s7_c_type_set_to_string (sc, gate_tag, gate_to_string_glue);

  {
    std::lock_guard<std::mutex> lock (g_type_mtx);
    g_gate_type_tags[sc]= gate_tag;
  }

  s7_define_function (sc, "g_make-chan", f_make_chan, 0, 1, false, "(g_make-chan [capacity]) => channel");
  s7_define_function (sc, "g_chan?", f_chan_p, 1, 0, false, "(g_chan? obj) => boolean");
  s7_define_function (sc, "g_chan-send!", f_chan_send, 2, 1, false, "(g_chan-send! ch val [timeout-ms]) => boolean");
  s7_define_function (sc, "g_chan-recv!", f_chan_recv, 1, 2, false,
                      "(g_chan-recv! ch [timeout-ms [default]]) => value | default | eof-object");
  s7_define_function (sc, "g_chan-try-recv!", f_chan_try_recv, 1, 1, false,
                      "(g_chan-try-recv! ch [default]) => value | default | eof-object");
  s7_define_function (sc, "g_chan-close!", f_chan_close, 1, 0, false, "(g_chan-close! ch) => unspecified");
  s7_define_function (sc, "g_chan-closed?", f_chan_closed_p, 1, 0, false, "(g_chan-closed? ch) => boolean");
  s7_define_function (sc, "g_go-spawn", f_go_spawn, 3, 0, false, "(g_go-spawn names vals code) => unspecified");
  s7_define_function (sc, "g_worker-notify-error", f_worker_notify_error, 2, 0, false,
                      "(g_worker-notify-error tag args) => unspecified");
  s7_define_function (sc, "g_go-worker-count", f_go_worker_count, 0, 0, false, "(g_go-worker-count) => integer");
  s7_define_function (sc, "g_select", f_select, 3, 0, false,
                      "(g_select recv-chs send-pairs timeout-ms) => #(0 idx val) | #(1 idx) | #f");
  s7_define_function (sc, "g_chan-timeout-close!", f_chan_timeout_close, 2, 0, false,
                      "(g_chan-timeout-close! ch ms) => unspecified, closes ch after ms milliseconds");
  s7_define_function (sc, "g_make-gate", f_make_gate, 0, 0, false, "(g_make-gate) => go-gate");
  s7_define_function (sc, "g_gate-wait", f_gate_wait, 1, 0, false, "(g_gate-wait gate) => integer id");
  s7_define_function (sc, "g_gate-try-wait", f_gate_try_wait, 1, 0, false, "(g_gate-try-wait gate) => integer id | #f");
  s7_define_function (sc, "g_chan-recv-or-watch!", f_chan_recv_or_watch, 4, 0, false,
                      "(g_chan-recv-or-watch! ch gate id default) => value | eof | default");
  s7_define_function (sc, "g_chan-send-or-watch!", f_chan_send_or_watch, 5, 0, false,
                      "(g_chan-send-or-watch! ch val gate id default) => #t | default");
  s7_define_function (sc, "g_msleep", f_msleep, 1, 0, false, "(g_msleep ms) => unspecified");
  s7_define_function (sc, "g_now-ms", f_now_ms, 0, 0, false, "(g_now-ms) => integer");
}

} // namespace goldfish
