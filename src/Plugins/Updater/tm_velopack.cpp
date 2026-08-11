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

#if defined(USE_PLUGIN_VELOPACK) && (defined(OS_MINGW) || defined(OS_WIN))

#include "string.hpp"
#include "tm_velopack.hpp"

#include <Velopack.hpp>

// Velopack 桥接层使用 C++ 标准库线程原语与容器，工作线程不触碰 lolly 类型
// （与 src/Plugins/WebSocket/libcurl/tm_curl_websocket_client.* 同理）。
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>

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

struct tm_velopack::tm_velopack_rep {
  Velopack::UpdateManager*            mgr;      // 惰性创建
  std::optional<Velopack::UpdateInfo> info;     // 最近一次检查结果
  std::thread                         worker;   // 当前检查/下载线程
  std::mutex                          mtx;      // 保护以下字段
  tm_updater_state                    st;       // = UPDATER_IDLE
  std::string                         version;  // 目标版本
  std::string                         notes;    // 发行说明 (markdown)
  std::string                         error;    // 错误码/消息
  int                                 progress; // 0..100
  time_t                              last;     // 最近检查时间
  bool                                running;  // 是否有线程在跑
  std::string                         feed_url; // 更新源

  tm_velopack_rep ()
      : mgr (nullptr), st (UPDATER_IDLE), progress (0), last (0),
        running (false),
        // TODO: feed 地址占位符（正式地址待定，发布前替换）
        feed_url ("https://feed.invalid/mogan/windows-x64/stable") {}
  ~tm_velopack_rep () {
    if (worker.joinable ()) worker.join ();
    delete mgr;
  }
};

tm_velopack::tm_velopack () : rep (new tm_velopack_rep ()) {}

tm_velopack::~tm_velopack () { delete rep; }

bool
tm_velopack::setAppcast (url _url) {
  std::lock_guard<std::mutex> lk (rep->mtx);
  rep->feed_url= std::string ((const char*) c_string (as_string (_url)));
  return true;
}

void
tm_velopack::ensure_mgr () {
  std::lock_guard<std::mutex> lk (rep->mtx);
  if (!rep->mgr) rep->mgr= new Velopack::UpdateManager (rep->feed_url);
}

bool
tm_velopack::checkInBackground () {
  std::lock_guard<std::mutex> lk (rep->mtx);
  if (rep->running) return false;
  if (rep->worker.joinable ()) rep->worker.join ();
  rep->st     = UPDATER_CHECKING;
  rep->running= true;
  rep->worker = std::thread ([this] { do_check (); });
  return true;
}

bool
tm_velopack::checkInForeground () {
  return checkInBackground ();
}

bool
tm_velopack::isRunning () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return rep->running;
}

time_t
tm_velopack::lastCheck () const {
  std::lock_guard<std::mutex> lk (rep->mtx);
  return rep->last;
}

bool
tm_velopack::setCheckInterval (int hours) {
  interval= hours;
  return true;
}

void
tm_velopack::do_check () {
  try {
    ensure_mgr ();
    std::optional<Velopack::UpdateInfo> u= rep->mgr->CheckForUpdates ();
    std::lock_guard<std::mutex>         lk (rep->mtx);
    if (u) {
      rep->info   = u;
      rep->st     = UPDATER_AVAILABLE;
      rep->version= u->TargetFullRelease.Version;
      rep->notes  = u->TargetFullRelease.NotesMarkdown;
    }
    else {
      rep->st= UPDATER_IDLE;
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
    ensure_mgr ();
    rep->mgr->DownloadUpdates (*rep->info, &tm_velopack::progress_cb, this);
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
  {
    std::lock_guard<std::mutex> lk (rep->mtx);
    if (rep->st != UPDATER_READY || !rep->info) return false;
    rep->st= UPDATER_APPLYING;
  }
  try {
    rep->mgr->WaitExitThenApplyUpdates (rep->info->TargetFullRelease,
                                        /*silent*/ false, /*restart*/ true);
  } catch (...) {
    std::lock_guard<std::mutex> lk (rep->mtx);
    rep->st   = UPDATER_FAILED;
    rep->error= exception_message ();
    return false;
  }
  exit (0);
  return true; // 不可达：成功时进程退出，由更新器应用并重启
}

#endif // defined (USE_PLUGIN_VELOPACK) && (defined (OS_MINGW) || defined
       // (OS_WIN))
