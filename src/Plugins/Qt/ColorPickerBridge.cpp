/******************************************************************************
 * MODULE      : ColorPickerBridge.cpp
 * DESCRIPTION : QML 调色板的屏幕取色 bridge（Pick Screen Color）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "ColorPickerBridge.hpp"

#include <QColor>
#include <QCursor>
#include <QDialog>
#include <QGuiApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>

namespace {

/**
 * @brief 全屏取色浮层：铺满屏幕显示快照，十字光标点击取色，Esc 取消。
 *
 * 快照 QPixmap 带 devicePixelRatio，绘制时铺满逻辑坐标窗口即可；取色时把
 * 点击逻辑坐标乘 DPR 换算回图像像素。
 */
class ScreenPickOverlay : public QDialog {
public:
  explicit ScreenPickOverlay (const QPixmap& snapshot, QWidget* parent= nullptr)
      : QDialog (parent,
                 Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
        m_snapshot (snapshot) {
    setCursor (Qt::CrossCursor);
  }

  QColor pickedColor () const { return m_picked; }

protected:
  void paintEvent (QPaintEvent*) override {
    QPainter p (this);
    p.drawPixmap (rect (), m_snapshot);
  }

  void mousePressEvent (QMouseEvent* ev) override {
    const qreal dpr= m_snapshot.devicePixelRatio ();
    const QImage img  = m_snapshot.toImage ();
    const int    px   = qRound (ev->position ().x () * dpr);
    const int    py   = qRound (ev->position ().y () * dpr);
    if (px >= 0 && py >= 0 && px < img.width () && py < img.height ())
      m_picked= img.pixelColor (px, py);
    accept ();
  }

  void keyPressEvent (QKeyEvent* ev) override {
    if (ev->key () == Qt::Key_Escape) reject ();
    else QDialog::keyPressEvent (ev);
  }

private:
  QPixmap m_snapshot;
  QColor  m_picked;
};

} // namespace

QString
ColorPickerBridge::pickScreenColor () {
  QScreen* screen= QGuiApplication::screenAt (QCursor::pos ());
  if (screen == nullptr) screen= QGuiApplication::primaryScreen ();
  if (screen == nullptr) return QString ();
  const QPixmap snapshot= screen->grabWindow (0);
  // macOS 未授「屏幕录制」权限时快照为空，取色不可用。
  if (snapshot.isNull ()) return QString ();

  ScreenPickOverlay overlay (snapshot);
  overlay.setGeometry (screen->geometry ());
  if (overlay.exec () != QDialog::Accepted) return QString ();
  const QColor c= overlay.pickedColor ();
  return c.isValid () ? c.name () : QString ();
}
