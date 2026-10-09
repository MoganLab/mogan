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

#ifndef LIII_GO_HPP
#define LIII_GO_HPP

#include "s7.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace goldfish {

class GoldfishChannel;

enum class GFValueType {
  Nil,
  Boolean,
  Integer,
  Real,
  Character,
  String,
  Symbol,
  Pair,
  Vector,
  ByteVector,
  Channel,
  Eof,
  Ref,
  Let,
  Undefined
};

struct GFValue {
  GFValueType                                  type      = GFValueType::Undefined;
  size_t                                       node_id   = 0;
  size_t                                       ref_id    = 0;
  bool                                         bool_val  = false;
  s7_int                                       int_val   = 0;
  s7_double                                    double_val= 0.0;
  uint32_t                                     char_val  = 0;
  std::string                                  str_val;
  std::shared_ptr<std::pair<GFValue, GFValue>> pair_val;
  std::shared_ptr<std::vector<GFValue>>        vec_val;
  std::shared_ptr<const std::vector<uint8_t>>  bytevec_val;
  std::shared_ptr<GoldfishChannel>             chan_val;
};

// S7 pointer <-> GFValue conversion
bool       s7_to_gfvalue (s7_scheme* sc, s7_pointer obj, GFValue& out, std::string& err_msg);
s7_pointer gfvalue_to_s7 (s7_scheme* sc, const GFValue& val);

// 显式工作栈逐层释放深层 GFValue 树（pair/vector 链），避免析构递归溢出栈
// （Windows 默认线程栈 1MB，十万层深的结构递归析构会崩溃）
void gfvalue_destroy_deep (GFValue& root);

// select wait-set：可同时挂到多个 channel 的等待者（仅就绪通知，不搬运数据，
// 唤醒后由调用方重试实际操作）
struct SelectWaiter {
  std::mutex              mtx;
  std::condition_variable cv;
  bool                    ready= false;
};

// fiber 跨线程唤醒门闩：计数信号量语义（fire 投入 id，wait 阻塞取出）。
// 事件不丢：fire 出的 id 攒在队列里，直到被 wait/try_wait 消费。
// on_fire 回调用于事件驱动联动：gate fire 时唤醒空闲 worker 进调度器消费事件
class GoGate {
public:
  void fire (int64_t id) {
    {
      std::lock_guard<std::mutex> lock (mtx);
      fired.push_back (id);
    }
    cv.notify_one ();
    std::function<void ()> cb;
    {
      std::lock_guard<std::mutex> lock (fire_mtx);
      cb= on_fire;
    }
    if (cb) cb (); // 锁外调用，避免回调重入 fire 死锁
  }

  // 设置 fire 通知回调（会话初始化期单线程调用，读取侧加锁保证可见）
  void set_on_fire (std::function<void ()> cb) {
    std::lock_guard<std::mutex> lock (fire_mtx);
    on_fire= std::move (cb);
  }

  int64_t wait () {
    std::unique_lock<std::mutex> lock (mtx);
    cv.wait (lock, [this] () { return !fired.empty (); });
    int64_t id= fired.front ();
    fired.pop_front ();
    return id;
  }

  // 非阻塞取出：队列空立即返回 false（调度器 drain 用，永不 park 线程）
  bool try_wait (int64_t& out) {
    std::lock_guard<std::mutex> lock (mtx);
    if (fired.empty ()) return false;
    out= fired.front ();
    fired.pop_front ();
    return true;
  }

private:
  std::mutex              mtx;
  std::condition_variable cv;
  std::deque<int64_t>     fired;
  std::mutex              fire_mtx;
  std::function<void ()>  on_fire;
};

class GoldfishChannel {
public:
  explicit GoldfishChannel (size_t cap= 0) : capacity (cap), closed (false) {}
  ~GoldfishChannel ();

  enum class SendStatus { Ok, Closed, Timeout };

  enum class RecvStatus { Ok, Closed, Timeout };

  // timeout_ms < 0 means infinite wait
  SendStatus send (GFValue val, int64_t timeout_ms= -1);
  RecvStatus recv (GFValue& out, int64_t timeout_ms= -1);
  // 非阻塞接收：等价于 recv (out, 0)
  RecvStatus try_recv (GFValue& out) { return recv (out, 0); }

  // select 支持：原子地"尝试非阻塞操作；不可行则登记等待者"
  SendStatus select_try_send_or_wait (GFValue& val, SelectWaiter* w);
  RecvStatus select_try_recv_or_wait (GFValue& out, SelectWaiter* w);
  void       select_remove_waiter (SelectWaiter* w);

  // fiber 支持：原子地"尝试非阻塞操作；不可行则登记一次性 watcher，
  // 通道就绪时 watcher 触发 gate->fire(id)（一次性，触发后自动摘除）"
  RecvStatus recv_or_watch (GFValue& out, std::shared_ptr<GoGate> gate, int64_t id);
  SendStatus send_or_watch (GFValue& val, std::shared_ptr<GoGate> gate, int64_t id);

  void   close ();
  bool   is_closed () const;
  size_t get_capacity () const { return capacity; }

private:
  struct RendezvousSender {
    GFValue                 val;
    bool                    completed= false;
    std::condition_variable cv;
  };

  struct RendezvousReceiver {
    GFValue*                out      = nullptr;
    bool                    completed= false;
    std::condition_variable cv;
  };

  // 前置条件：调用方持有 mtx
  SendStatus try_send_unlocked (GFValue& val);
  RecvStatus try_recv_unlocked (GFValue& out);
  void       notify_select_recv_ready (); // 前置：持有 mtx
  void       notify_select_send_ready (); // 前置：持有 mtx

  size_t             capacity; // 0: unbuffered (rendezvous), > 0: buffered
  bool               closed;
  mutable std::mutex mtx;

  // Buffered channel synchronization
  std::condition_variable cv_buf_send;
  std::condition_variable cv_buf_recv;
  std::deque<GFValue>     buffer;

  // Unbuffered channel synchronization (rendezvous queues)
  std::deque<RendezvousSender*>   waiting_senders;
  std::deque<RendezvousReceiver*> waiting_receivers;

  // select wait-set 队列（只存指针，唤醒即摘出；超时方负责摘除自己）
  std::deque<SelectWaiter*> select_recv_waiters;
  std::deque<SelectWaiter*> select_send_waiters;

  // fiber watcher 队列（一次性，触发即摘除）。send watcher 携带待发载荷，
  // 支持 fiber↔fiber 的无缓冲 rendezvous：recv watcher 直接取走载荷完成交接
  struct WatchEntry {
    std::shared_ptr<GoGate> gate;
    int64_t                 id;
    bool                    has_payload= false;
    GFValue                 payload;
  };
  std::deque<WatchEntry> watch_recv_entries;
  std::deque<WatchEntry> watch_send_entries;

  // fiber↔fiber 交接的完成记账：载荷被 recv watcher 取走时记录 send 的 id，
  // send fiber 唤醒后以同 id 重试时据此确认完成，不再重发载荷
  std::unordered_set<int64_t> completed_send_ids;
};

void        set_goldfish_lib_dir (const std::string& dir);
std::string get_goldfish_lib_dir ();

bool                             is_goldfish_channel (s7_scheme* sc, s7_pointer obj);
std::shared_ptr<GoldfishChannel> get_goldfish_channel (s7_scheme* sc, s7_pointer obj);
s7_pointer                       make_goldfish_channel_object (s7_scheme* sc, std::shared_ptr<GoldfishChannel> ch);

void glue_liii_go (s7_scheme* sc);

} // namespace goldfish

#endif // LIII_GO_HPP
