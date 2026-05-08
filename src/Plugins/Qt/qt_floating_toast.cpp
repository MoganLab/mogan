
/******************************************************************************
 * MODULE     : qt_floating_toast.cpp
 * DESCRIPTION: Floating toast implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_floating_toast.hpp"
#include "qt_dpi_utils.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>

QtFloatingToast::QtFloatingToast (QWidget* parent)
    : QWidget (parent,
               Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool) {
  setAttribute (Qt::WA_TranslucentBackground);

  label_= new QLabel (this);
  label_->setAlignment (Qt::AlignCenter);
  label_->setWordWrap (true);
  label_->setStyleSheet ("QLabel { color: #ffffff; }");
  label_->setFont (DpiUtils::scaledFont (label_->font (), 14));

  layout_= new QHBoxLayout (this);
  layout_->setContentsMargins (0, 0, 0, 0);
  layout_->addWidget (label_, 0, Qt::AlignCenter);

  hideTimer_= new QTimer (this);
  hideTimer_->setSingleShot (true);
  connect (hideTimer_, &QTimer::timeout, this, &QtFloatingToast::startFadeOut);

  fadeAnimation_= new QPropertyAnimation (this, "windowOpacity");
  fadeAnimation_->setDuration (200);
}

QtFloatingToast::~QtFloatingToast ()= default;

void
QtFloatingToast::showAbove (QWidget* anchorWidget, const QString& message,
                            int durationMs, Type type) {
  if (!anchorWidget) return;

  type_= type;
  label_->setText (message);
  label_->adjustSize ();

  int padX= DpiUtils::scaled (20);
  int padY= DpiUtils::scaled (10);
  layout_->setContentsMargins (padX, padY, padX, padY);

  adjustSize ();
  updatePosition (anchorWidget);

  setWindowOpacity (0.0);
  show ();
  raise ();
  startFadeIn ();

  hideTimer_->start (durationMs);
}

void
QtFloatingToast::showToast (QWidget* anchorWidget, const QString& message,
                            int durationMs, Type type) {
  if (!anchorWidget) return;
  auto* toast= new QtFloatingToast (anchorWidget->window ());
  toast->showAbove (anchorWidget, message, durationMs, type);
}

void
QtFloatingToast::updatePosition (QWidget* anchorWidget) {
  if (!anchorWidget) return;
  QWidget* window= anchorWidget->window ();
  QRect    geo   = window->geometry ();
  int      x     = geo.x () + (geo.width () - width ()) / 2;
  int      y     = geo.y () + (geo.height () - height ()) / 8;
  move (x, y);
}

void
QtFloatingToast::startFadeIn () {
  fadeAnimation_->stop ();
  fadeAnimation_->setStartValue (0.0);
  fadeAnimation_->setEndValue (1.0);
  fadeAnimation_->start ();
}

void
QtFloatingToast::startFadeOut () {
  fadeAnimation_->stop ();
  fadeAnimation_->setStartValue (1.0);
  fadeAnimation_->setEndValue (0.0);
  if (fadeConnection_) disconnect (fadeConnection_);
  fadeConnection_= connect (fadeAnimation_, &QPropertyAnimation::finished, this,
                            &QObject::deleteLater);
  fadeAnimation_->start ();
}

void
QtFloatingToast::paintEvent (QPaintEvent* event) {
  QPainter painter (this);
  painter.setRenderHint (QPainter::Antialiasing);

  QRectF rect  = this->rect ().adjusted (1, 1, -1, -1);
  int    radius= DpiUtils::scaled (8);

  painter.setPen (Qt::NoPen);
  QColor bg;
  switch (type_) {
  case Success:
    bg= QColor (46, 125, 50, 220);
    break;
  case Warning:
    bg= QColor (245, 124, 0, 220);
    break;
  case Error:
    bg= QColor (198, 40, 40, 220);
    break;
  default:
    bg= QColor (50, 50, 50, 220);
  }
  painter.setBrush (bg);
  painter.drawRoundedRect (rect, radius, radius);
}
