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
  tm_updater () {}
  tm_updater (const tm_updater&);
  void operator= (const tm_updater&);
  virtual ~tm_updater () {};

public:
  static tm_updater* instance ();

  virtual bool checkInBackground () { return false; } // non-blocking

  virtual time_t lastCheck () const { return 0; }

  virtual tm_updater_state state () const { return UPDATER_IDLE; }
  virtual string           availableVersion () const { return string (); }
  virtual string           releaseNotes () const { return string (); }
  virtual int              progress () const { return 0; }
  virtual string           errorCode () const { return string (); }
  virtual bool             downloadUpdate () { return false; }
  virtual bool             applyUpdate () { return false; }
};

/******************************************************************************
 * Scheme interface
 ******************************************************************************/

bool   updater_check_background ();
time_t updater_last_check ();

int    updater_state ();
string updater_available_version ();
string updater_release_notes ();
int    updater_progress ();
string updater_error_code ();
bool   updater_download ();
bool   updater_apply ();

#endif // TM_UPDATER_HPP
