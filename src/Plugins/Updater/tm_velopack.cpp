/******************************************************************************
 * MODULE     : tm_velopack.cpp
 * DESCRIPTION: Manager class for the autoupdater Velopack framework
 * COPYRIGHT  : (C) 2026 Mogan
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tm_configure.hpp"

#if defined(USE_PLUGIN_VELOPACK) && defined(OS_WIN)

#include "string.hpp"
#include "tm_velopack.hpp"

#include <Velopack.hpp>

// Velopack 桥接层使用 C++ 标准库线程原语与容器，工作线程不触碰 lolly 类型
// （与 src/Plugins/WebSocket/libcurl/tm_curl_websocket_client.* 同理）。
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

// 默认更新源（feed 根 URL）。当前按静态 feed 设计编译期写死；若将来要运行时
// 切源（stable/beta 渠道、内网镜像），需改为可配置并在 setAppcast 里重建 mgr。
static const std::string default_feed_url=
    "https://updates.mogan.app/mogan/windows-x64/stable";

static std::string
exception_message () {
  try {
    throw;
  } catch (std::exception& e) {
    return e.what ();
  } catch (...) {
    return "unknown error";
  }
}

// 按 semver2 优先级比较两个版本串（忽略 build 元数据；核心段逐数字比较，预发布
// 段按标识符比较，数字标识符小于字母标识符）。返回 a 是否严格高于 b。
// 版本串来自 Velopack feed（如 "2026.3.0-rc9"），字符串序处理不了 rc9/rc10。
static int
compare_versions (const std::string& a, const std::string& b) {
  auto split= [] (const std::string& s, char c) {
    std::vector<std::string> parts;
    std::string              cur;
    for (char ch : s) {
      if (ch == c) {
        parts.push_back (cur);
        cur.clear ();
      }
      else cur+= ch;
    }
    parts.push_back (cur);
    return parts;
  };
  auto is_num= [] (const std::string& s) {
    return !s.empty () && std::all_of (s.begin (), s.end (), [] (char ch) {
      return ::isdigit ((unsigned char) ch);
    });
  };
  // 剥离 build 元数据；a0/b0 即「核心段[-预发布段]」
  auto   a0= a.substr (0, a.find ('+')), b0= b.substr (0, b.find ('+'));
  auto   ac= split (a0.substr (0, a0.find ('-')), '.');
  auto   bc= split (b0.substr (0, b0.find ('-')), '.');
  size_t n = std::max (ac.size (), bc.size ());
  for (size_t i= 0; i < n; i++) {
    long an= i < ac.size () ? std::atol (ac[i].c_str ()) : 0;
    long bn= i < bc.size () ? std::atol (bc[i].c_str ()) : 0;
    if (an != bn) return an < bn ? -1 : 1;
  }
  // 核心段相等：无预发布段 > 有预发布段；预发布段按标识符逐个比较
  bool aPre= a0.find ('-') != std::string::npos;
  bool bPre= b0.find ('-') != std::string::npos;
  if (aPre != bPre) return aPre ? -1 : 1;
  if (!aPre) return 0;
  auto ap= split (a0.substr (a0.find ('-') + 1), '.');
  auto bp= split (b0.substr (b0.find ('-') + 1), '.');
  n      = std::max (ap.size (), bp.size ());
  for (size_t i= 0; i < n; i++) {
    if (i >= ap.size ()) return -1; // a 缺少标识符，a < b
    if (i >= bp.size ()) return 1;
    bool aNum= is_num (ap[i]), bNum= is_num (bp[i]);
    if (aNum && bNum) {
      long an= std::atol (ap[i].c_str ()), bn= std::atol (bp[i].c_str ());
      if (an != bn) return an < bn ? -1 : 1;
    }
    else if (aNum) return -1; // 数字标识符 < 字母标识符
    else if (bNum) return 1;
    else if (ap[i] != bp[i]) return ap[i] < bp[i] ? -1 : 1;
  }
  return 0;
}

static bool
newer_version (const std::string& a, const std::string& b) {
  return compare_versions (a, b) > 0;
}

struct tm_velopack::tm_velopack_rep {
  std::unique_ptr<Velopack::UpdateManager>
                 mgr;      // 惰性创建，恰好一次（call_once 保证）
  std::once_flag mgr_once; // 保护 mgr 的单次初始化
  std::optional<Velopack::UpdateInfo> info;            // 最近一次检查结果
  std::thread                         worker;          // 当前检查/下载线程
  std::mutex                          mtx;             // 保护以下字段
  tm_updater_state                    st;              // = UPDATER_IDLE
  tm_updater_state                    st_before_check; // 检查启动前的状态
  std::string                         version;         // 目标版本
  std::string                         notes;           // 发行说明 (markdown)
  std::string                         error;           // 错误码/消息
  int                                 progress;        // 0..100
  time_t                              last;            // 最近检查时间
  bool                                running;         // 是否有线程在跑

  tm_velopack_rep ()
      : st (UPDATER_IDLE), st_before_check (UPDATER_IDLE), progress (0),
        last (0), running (false) {}
  ~tm_velopack_rep () {
    if (worker.joinable ()) worker.join ();
  }
};

tm_velopack::tm_velopack () : rep (std::make_unique<tm_velopack_rep> ()) {}

tm_velopack::~tm_velopack () {}

void
tm_velopack::ensure_mgr () {
  // mgr 由 call_once 保证恰好创建一次；feed URL 编译期写死，无需加锁。
  std::call_once (rep->mgr_once, [this] {
    rep->mgr= std::make_unique<Velopack::UpdateManager> (default_feed_url);
  });
}

bool
tm_velopack::checkInBackground () {
  std::lock_guard<std::mutex> lk (rep->mtx);
  if (rep->running) return false;
  if (rep->st == UPDATER_APPLYING) return false; // 应用更新期间不接受新检查
  if (rep->worker.joinable ()) rep->worker.join ();
  // 启动检查置 CHECKING；同时保存检查前状态，do_check 结果驱动时据此判断
  // 是否保持 READY「待应用」（复查不应把它冲掉，否则要重新下载）。
  rep->st_before_check= rep->st;
  rep->st             = UPDATER_CHECKING;
  rep->running        = true;
  rep->worker         = std::thread ([this] { do_check (); });
  return true;
}

time_t
tm_velopack::lastCheck () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return rep->last;
}

void
tm_velopack::do_check () {
  try {
    ensure_mgr ();
    std::optional<Velopack::UpdateInfo> u= rep->mgr->CheckForUpdates ();
    std::lock_guard<std::mutex>         lk (rep->mtx);
    if (rep->st == UPDATER_APPLYING) { // 应用已开始，本次结果作废
      rep->running= false;
      return;
    }
    if (u) {
      // 已就绪的同版（或更旧）更新不再进入 AVAILABLE：一次复查不应把 READY
      // 「待应用」冲掉，否则要重新下载。仅当版本更高（或当前不在 READY，如
      // 失败后重试）时才覆盖为目标版本。
      bool keep_ready=
          rep->st_before_check == UPDATER_READY &&
          !newer_version (u->TargetFullRelease.Version, rep->version);
      if (keep_ready) {
        rep->st= UPDATER_READY; // 复查后仍保持「待应用」
      }
      else {
        rep->info   = u;
        rep->st     = UPDATER_AVAILABLE;
        rep->version= u->TargetFullRelease.Version;
        rep->notes  = u->TargetFullRelease.NotesMarkdown;
      }
    }
    else {
      // 无更新：失败标记被清除；其余状态恢复到检查前的结论（复查不改变
      // 既有状态）。
      if (rep->st_before_check == UPDATER_FAILED) {
        rep->st   = UPDATER_IDLE;
        rep->error= "";
      }
      else {
        rep->st= rep->st_before_check;
      }
    }
    rep->last   = time (NULL);
    rep->running= false;
  } catch (...) {
    std::lock_guard<std::mutex> lk (rep->mtx);
    rep->st     = UPDATER_FAILED;
    rep->error  = exception_message ();
    rep->running= false;
  }
}

tm_updater_state
tm_velopack::state () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return rep->st;
}

string
tm_velopack::availableVersion () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return string (rep->version.c_str ());
}

string
tm_velopack::releaseNotes () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return string (rep->notes.c_str ());
}

int
tm_velopack::progress () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return rep->progress;
}

string
tm_velopack::errorCode () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return string (rep->error.c_str ());
}

bool
tm_velopack::downloadUpdate () {
  std::lock_guard<std::mutex> lk (rep->mtx);
  if (rep->st != UPDATER_AVAILABLE) return false;
  if (rep->running) return false;
  if (rep->worker.joinable ()) rep->worker.join ();
  rep->st     = UPDATER_DOWNLOADING;
  rep->running= true;
  rep->worker = std::thread ([this] { do_download (); });
  return true;
}

void
tm_velopack::progress_cb (void* user_data, size_t progress) {
  tm_velopack*                self= static_cast<tm_velopack*> (user_data);
  std::lock_guard<std::mutex> lk (self->rep->mtx);
  self->rep->progress= static_cast<int> (progress);
}

void
tm_velopack::do_download () {
  try {
    // info 在锁内取快照后释放锁：锁外读共享 optional 是潜在数据竞争，而
    // DownloadUpdates 是长时阻塞调用，更不能持锁（否则进度回调与轮询会卡死）。
    Velopack::UpdateInfo info;
    {
      std::lock_guard<std::mutex> lk (rep->mtx);
      if (rep->st != UPDATER_DOWNLOADING || !rep->info)
        return; // 仅 downloadUpdate 武装后可达
      info= *rep->info;
    }
    ensure_mgr ();
    rep->mgr->DownloadUpdates (info, &tm_velopack::progress_cb, this);
    std::lock_guard<std::mutex> lk (rep->mtx);
    rep->st     = UPDATER_READY;
    rep->running= false;
  } catch (...) {
    std::lock_guard<std::mutex> lk (rep->mtx);
    rep->st     = UPDATER_FAILED;
    rep->error  = exception_message ();
    rep->running= false;
  }
}

bool
tm_velopack::applyUpdate () {
  Velopack::UpdateInfo info;
  {
    std::lock_guard<std::mutex> lk (rep->mtx);
    if (rep->st != UPDATER_READY || !rep->info) return false;
    info   = *rep->info; // 锁内快照，锁外不再读共享 info
    rep->st= UPDATER_APPLYING;
  }
  try {
    // 拉起更新器进程，它等待本进程退出后应用并重启；本调用启动后即返回。
    // 不在 C++ 层自行 exit：由 scheme 调用方走 (safely-quit-TeXmacs) 正常退出
    // 通道，完成保存提示与 on-exit 清理后进程退出，更新器随即接管。
    // 注意：若调用方仅触发 apply 而不退出（如未来经 Qt controller 直连），
    // 更新器最多等待 60s 后放弃，且状态停留在 APPLYING。
    rep->mgr->WaitExitThenApplyUpdates (info.TargetFullRelease,
                                        /*silent*/ false, /*restart*/ true);
  } catch (...) {
    std::lock_guard<std::mutex> lk (rep->mtx);
    rep->st   = UPDATER_FAILED;
    rep->error= exception_message ();
    return false;
  }
  return true;
}

#endif // defined (USE_PLUGIN_VELOPACK) && defined (OS_WIN)
