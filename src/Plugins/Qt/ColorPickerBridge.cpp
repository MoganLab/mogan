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
 * @brief 全屏取色浮层：显示快照，十字光标点击取色，Esc 取消。
 *
 * 绘制与取色都按全局屏幕坐标对齐，不按窗口 rect() 拉伸：WM 可能把窗口
 * 约束进工作区（KWin 对「整屏大小」的窗口会压到 availableGeometry），
 * 拉伸会让快照错位/压缩（面板区域与真实面板叠成「双重任务栏」）。
 * 对齐后未被窗口覆盖的区域（如面板）露出真实内容，与快照自然衔接。
 */
class ScreenPickOverlay : public QDialog {
public:
  explicit ScreenPickOverlay (const QPixmap& snapshot,
                              const QPoint&  screenOrigin)
      : QDialog (nullptr,
                 Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
        m_snapshot (snapshot), m_screenOrigin (screenOrigin) {
    setCursor (Qt::CrossCursor);
    setAttribute (Qt::WA_DeleteOnClose);
    setWindowModality (Qt::ApplicationModal);
  }

  QColor pickedColor () const { return m_picked; }

protected:
  void paintEvent (QPaintEvent*) override {
    QPainter      p (this);
    const QPointF off= geometry ().topLeft () - m_screenOrigin;
    p.drawPixmap (-off, m_snapshot);
  }

  void mousePressEvent (QMouseEvent* ev) override {
    const qreal  dpr= m_snapshot.devicePixelRatio ();
    const QImage img= m_snapshot.toImage ();
    // 窗口逻辑坐标 → 屏幕全局逻辑坐标 → 乘 DPR 换算图像像素
    const QPointF globalPos=
        geometry ().topLeft () + ev->position () - m_screenOrigin;
    const int px= qRound (globalPos.x () * dpr);
    const int py= qRound (globalPos.y () * dpr);
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
  QPoint  m_screenOrigin;
  QColor  m_picked;
};

} // namespace

ColorPickerBridge::~ColorPickerBridge () {
  // 直接 delete 不触发 finished，finished 连接的 context（本对象）此时仍有效。
  delete m_overlay;
}

bool
ColorPickerBridge::canPickScreen () const {
  // Wayland 会话抓不到真实屏幕：qtwayland 平台不支持 grabWindow；mogan
  // 强制 QT_QPA_PLATFORM=xcb（research.cpp），Wayland 下实际跑在 XWayland，
  // platformName 恒为 "xcb"、grabWindow 只能抓到 XWayland 的黑色根窗口，
  // 故须看 WAYLAND_DISPLAY 而非 platformName。
  if (QGuiApplication::platformName ().startsWith ("wayland",
                                                   Qt::CaseInsensitive))
    return false;
  return !qEnvironmentVariableIsSet ("WAYLAND_DISPLAY");
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

  // 不 hide 宿主弹窗：hide 会退出 exec() 中对话框的事件循环
  // （QDialog::setVisible 的隐藏分支 exit 其 eventLoop），整个弹窗随之销毁。
  // overlay 全屏置顶，已足够盖住弹窗。
  ScreenPickOverlay* overlay=
      new ScreenPickOverlay (snapshot, screen->geometry ().topLeft ());
  overlay->setGeometry (screen->geometry ());
  connect (overlay, &QDialog::finished, this, [this, overlay] (int res) {
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
