
/******************************************************************************
 * MODULE     : qt_floating_toast.hpp
 * DESCRIPTION: Floating toast implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_FLOATING_TOAST_HPP
#define QT_FLOATING_TOAST_HPP

#include <QWidget>

class QLabel;
class QHBoxLayout;
class QPropertyAnimation;
class QTimer;

class QtFloatingToast : public QWidget {
  Q_OBJECT

public:
  enum Type { Success, Warning, Error };

  explicit QtFloatingToast (QWidget* parent= nullptr);
  ~QtFloatingToast ();

  void showAbove (QWidget* anchorWidget, const QString& message,
                  int durationMs= 3000, Type type= Success);

  static void showToast (QWidget* anchorWidget, const QString& message,
                         int durationMs= 3000, Type type= Success);

protected:
  void paintEvent (QPaintEvent* event) override;

private:
  void updatePosition (QWidget* anchorWidget);
  void startFadeIn ();
  void startFadeOut ();

  QLabel*                 label_        = nullptr;
  QHBoxLayout*            layout_       = nullptr;
  QPropertyAnimation*     fadeAnimation_= nullptr;
  QTimer*                 hideTimer_    = nullptr;
  QMetaObject::Connection fadeConnection_{};
  Type                    type_= Success;
};

#endif
