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
#include <QPointer>
#include <QScreen>
#include <QWidget>

namespace {

/**
 * @brief 全屏取色浮层：铺满屏幕显示快照，十字光标点击取色，Esc 取消。
 *
 * 快照 QPixmap 带 devicePixelRatio，绘制时铺满逻辑坐标窗口即可；取色时把
 * 点击逻辑坐标乘 DPR 换算回图像像素。
 */
class ScreenPickOverlay : public QDialog {
public:
  explicit ScreenPickOverlay (const QPixmap& snapshot)
      : QDialog (nullptr,
                 Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
        m_snapshot (snapshot) {
    setCursor (Qt::CrossCursor);
    setAttribute (Qt::WA_DeleteOnClose);
    setWindowModality (Qt::ApplicationModal);
  }

  QColor pickedColor () const { return m_picked; }

protected:
  void paintEvent (QPaintEvent*) override {
    QPainter p (this);
    p.drawPixmap (rect (), m_snapshot);
  }

  void mousePressEvent (QMouseEvent* ev) override {
    const qreal  dpr= m_snapshot.devicePixelRatio ();
    const QImage img= m_snapshot.toImage ();
    const int    px = qRound (ev->position ().x () * dpr);
    const int    py = qRound (ev->position ().y () * dpr);
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

ColorPickerBridge::ColorPickerBridge (QWidget* host, QObject* parent)
    : QObject (parent), m_host (host) {}

ColorPickerBridge::~ColorPickerBridge () {
  // 直接 delete 不触发 finished，finished 连接的 context（本对象）此时仍有效。
  delete m_overlay;
}

bool
ColorPickerBridge::canPickScreen () const {
  // Wayland 不允许应用任意抓屏：grabWindow 返回空图或全黑快照，取色不可用。
  return !QGuiApplication::platformName ().startsWith ("wayland",
                                                       Qt::CaseInsensitive);
}

void
ColorPickerBridge::pickScreenColor () {
  // 上一次取色的 overlay 未关闭时忽略重复触发，避免 m_overlay 覆盖泄漏。
  if (m_overlay != nullptr) return;
  if (!canPickScreen ()) {
    emit screenColorPicked (QString ());
    return;
  }
  QScreen* screen= QGuiApplication::screenAt (QCursor::pos ());
  if (screen == nullptr) screen= QGuiApplication::primaryScreen ();
  if (screen == nullptr) {
    emit screenColorPicked (QString ());
    return;
  }
  const QPixmap snapshot= screen->grabWindow (0);
  // macOS 未授「屏幕录制」权限时快照为空，取色不可用。
  if (snapshot.isNull ()) {
    emit screenColorPicked (QString ());
    return;
  }

  // 隐藏宿主须在抓快照之后：否则 hide 触发的重绘来不及完成，快照可能
  // 残留弹窗残影；先抓则快照含弹窗自身（与 QColorDialog 行为一致）。
  ScreenPickOverlay* overlay= new ScreenPickOverlay (snapshot);
  overlay->setGeometry (screen->geometry ());
  QPointer<QWidget> host= m_host;
  if (host != nullptr) host->hide ();
  connect (overlay, &QDialog::finished, this, [this, host, overlay] (int res) {
    if (host != nullptr) host->show ();
    QString hex;
    if (res == QDialog::Accepted) {
      const QColor c= overlay->pickedColor ();
      if (c.isValid ()) hex= c.name ();
    }
    emit screenColorPicked (hex);
  });
  // 记录到成员：bridge 先毁（宿主弹窗被关）时由析构负责销毁 overlay，
  // 避免遗留全屏无父浮层。
  m_overlay= overlay;
  overlay->open ();
}
