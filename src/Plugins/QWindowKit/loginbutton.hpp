/******************************************************************************
 * MODULE     : loginbutton.hpp
 * COPYRIGHT  : (C) 2025 Liii
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LOGINBUTTON_H
#define LOGINBUTTON_H

#include <QtWidgets/QPushButton>

namespace QWK {

class LoginButtonPrivate;

class LoginButton : public QPushButton {
  Q_OBJECT
  Q_DECLARE_PRIVATE (LoginButton)
  Q_PROPERTY (QIcon iconNormal READ iconNormal WRITE setIconNormal FINAL)
  Q_PROPERTY (QIcon iconHover READ iconHover WRITE setIconHover FINAL)
  Q_PROPERTY (QIcon iconPressed READ iconPressed WRITE setIconPressed FINAL)
  Q_PROPERTY (QIcon iconDisabled READ iconDisabled WRITE setIconDisabled FINAL)
  Q_PROPERTY (bool badgeVisible READ badgeVisible WRITE setBadgeVisible FINAL)
public:
  explicit LoginButton (QWidget* parent= nullptr);
  ~LoginButton ();

public:
  QIcon iconNormal () const;
  void  setIconNormal (const QIcon& icon);

  QIcon iconHover () const;
  void  setIconHover (const QIcon& icon);

  QIcon iconPressed () const;
  void  setIconPressed (const QIcon& icon);

  QIcon iconDisabled () const;
  void  setIconDisabled (const QIcon& icon);

  bool badgeVisible () const;
  void setBadgeVisible (bool visible);

protected:
  void enterEvent (QEnterEvent* event) override;
  void leaveEvent (QEvent* event) override;
  void mousePressEvent (QMouseEvent* event) override;
  void mouseReleaseEvent (QMouseEvent* event) override;
  void paintEvent (QPaintEvent* event) override;

protected:
  LoginButton (LoginButtonPrivate& d, QWidget* parent= nullptr);

  QScopedPointer<LoginButtonPrivate> d_ptr;
};

} // namespace QWK

#endif // LOGINBUTTON_H