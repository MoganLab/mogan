/******************************************************************************
 * MODULE     : logindialog.hpp
 * COPYRIGHT  : (C) 2025 Liii
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QtWidgets/QDialog>

namespace QWK {

class LoginDialogPrivate;

class LoginDialog : public QDialog {
  Q_OBJECT
  Q_DECLARE_PRIVATE (LoginDialog)
public:
  explicit LoginDialog (QWidget* parent= nullptr);
  ~LoginDialog ();

public:
  void     setContentWidget (QWidget* widget);
  QWidget* contentWidget () const;
  void     showAtPosition (const QPoint& globalPos);
  void     showAtRect (const QRect& globalRect, int gap= 6);

protected:
  void showEvent (QShowEvent* event) override;
  void closeEvent (QCloseEvent* event) override;
  bool eventFilter (QObject* obj, QEvent* event) override;

protected:
  LoginDialog (LoginDialogPrivate& d, QWidget* parent= nullptr);

  QScopedPointer<LoginDialogPrivate> d_ptr;
};

} // namespace QWK

#endif // LOGINDIALOG_H
