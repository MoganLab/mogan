/******************************************************************************
 * MODULE     : tm_velopack.hpp
 * DESCRIPTION: Manager class for the autoupdater Velopack framework
 * COPYRIGHT  : (C) 2026 Mogan
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TM_VELOPACK_HPP
#define TM_VELOPACK_HPP

#include "tm_updater.hpp"
#include <time.h>

/**
 * @brief 基于 Velopack C++ 运行时的自动更新器。
 *
 * 采用 pimpl 封装：内部实现（tm_velopack_rep）在 .cpp 中定义，头文件不依赖
 * Velopack.hpp，避免把 std::vector 等标准库类型泄漏到 glue 等编译单元。
 */
class tm_velopack : public tm_updater {
  struct tm_velopack_rep; // 内部实现，定义在 .cpp
  tm_velopack_rep* rep;

  tm_velopack ();
  ~tm_velopack ();
  friend class tm_updater;

  void        do_check ();    // 工作线程：检查更新
  void        do_download (); // 工作线程：下载更新
  void        ensure_mgr ();  // 惰性创建 UpdateManager
  static void progress_cb (void*  user_data,
                           size_t progress); // Velopack 进度回调

public:
  bool   checkInBackground ();
  bool   checkInForeground ();
  bool   isRunning () const;
  time_t lastCheck () const;
  bool   setCheckInterval (int hours);
  bool   setAppcast (url _url);

  tm_updater_state state () const;
  string           availableVersion () const;
  string           releaseNotes () const;
  int              progress () const;
  string           errorCode () const;
  bool             downloadUpdate ();
  bool             applyUpdate ();
};

#endif // TM_VELOPACK_HPP
