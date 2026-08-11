/******************************************************************************
 * MODULE     : tm_updater.hpp
 * DESCRIPTION: Base class for auto-update frameworks
 * COPYRIGHT  : (C) 2013 Miguel de Benito Delgado
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TM_UPDATER_HPP
#define TM_UPDATER_HPP

#include "url.hpp"
#include <time.h>

/******************************************************************************
 * Auto-update state machine
 ******************************************************************************/

// 注意：枚举类型名不能叫 updater_state——同名 Scheme 接口函数
// (updater_state) 在类作用域会遮蔽标签名，导致 MSVC 语法错误。
enum tm_updater_state {
  UPDATER_IDLE= 0,
  UPDATER_CHECKING,
  UPDATER_AVAILABLE,
  UPDATER_DOWNLOADING,
  UPDATER_READY,
  UPDATER_APPLYING,
  UPDATER_FAILED
};

class tm_updater {
protected:
  static const int MinimumCheckInterval= 24;      //<! in hours
  static const int MaximumCheckInterval= 24 * 31; //<! in hours

  url appcast;
  int interval;

  tm_updater () : interval (0) {}
  tm_updater (const tm_updater&);
  void operator= (const tm_updater&);
  virtual ~tm_updater () {};

public:
  static tm_updater* instance ();

  virtual bool checkInBackground () { return false; } // non-blocking
  virtual bool checkInForeground () { return false; } // non-blocking
  virtual bool isRunning () const { return false; }

  virtual time_t lastCheck () const { return 0; }
  virtual bool   getCheckInterval () const { return interval; }
  virtual bool   setCheckInterval (int hours) {
    (void) hours;
    return false;
  }

  virtual tm_updater_state state () const { return UPDATER_IDLE; }
  virtual string           availableVersion () const { return string (); }
  virtual string           releaseNotes () const { return string (); }
  virtual int              progress () const { return 0; }
  virtual string           errorCode () const { return string (); }
  virtual bool             downloadUpdate () { return false; }
  virtual bool             applyUpdate () { return false; }
  virtual bool             setAppcast (url _url) {
    appcast= _url;
    return true;
  }
};

/******************************************************************************
 * Scheme interface
 ******************************************************************************/

bool   updater_is_running ();
bool   updater_check_background ();
bool   updater_check_foreground ();
bool   updater_set_interval (int hours);
time_t updater_last_check ();

int    updater_state ();
string updater_available_version ();
string updater_release_notes ();
int    updater_progress ();
string updater_error_code ();
bool   updater_download ();
bool   updater_apply ();

#endif // TM_UPDATER_HPP
