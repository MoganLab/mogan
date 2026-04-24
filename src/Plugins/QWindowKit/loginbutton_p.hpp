/******************************************************************************
 * MODULE     : loginbutton_p.hpp
 * COPYRIGHT  : (C) 2025 Liii
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LOGINBUTTONPRIVATE_H
#define LOGINBUTTONPRIVATE_H

#include <QIcon>

#include "loginbutton.hpp"

namespace QWK {

class LoginButtonPrivate {
  Q_DECLARE_PUBLIC (LoginButton)
public:
  LoginButtonPrivate ();
  virtual ~LoginButtonPrivate ();

  void init ();
  void reloadIcon ();

  LoginButton* q_ptr;

  QIcon iconNormal;
  QIcon iconHover;
  QIcon iconPressed;
  QIcon iconDisabled;

  bool hovered;
  bool pressed;
  bool badgeVisible; // 小红点显示状态

private:
  Q_DISABLE_COPY (LoginButtonPrivate)
};

} // namespace QWK

#endif // LOGINBUTTONPRIVATE_H