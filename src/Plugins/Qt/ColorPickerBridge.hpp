/******************************************************************************
 * MODULE      : ColorPickerBridge.hpp
 * DESCRIPTION : QML 调色板的屏幕取色 bridge（Pick Screen Color）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef COLOR_PICKER_BRIDGE_H
#define COLOR_PICKER_BRIDGE_H

#include <QObject>
#include <QPointer>
#include <QString>

class QWidget;

/*! @class ColorPickerBridge
 *  @brief 承载 QML 调色板需要 native 能力的交互：屏幕取色。
 *
 * @par 实现要点
 * QML 无法直接抓屏与拦截全屏鼠标点击，故屏幕取色走本 bridge：grabWindow
 * 抓全屏快照，铺满全屏的 frameless overlay 显示快照 + 十字光标，点击处按
 * devicePixelRatio 换算回图像像素取色，Esc 取消。overlay 全屏置顶，直接盖住
 * 调色板弹窗（快照里含弹窗自身，与 QColorDialog 行为一致）。
 *
 * @par 异步交付
 * pickScreenColor() 只发起取色，结果经 screenColorPicked 信号异步回流：
 * overlay 用 show() 而非 exec()，避免在 QML 事件处理器内嵌套事件循环——
 * 嵌套 exec 期间宿主弹窗的 QQuickWidget 被迫重绘，在 X11 + NVIDIA 下会
 * 段错误（QRhi::endOffscreenFrame）。
 *
 * @note macOS 下抓屏需「屏幕录制」权限：未授权时 grabWindow 静默返回空图，
 * 点击取色会毫无反应，故先 CGPreflightScreenCaptureAccess 预检，未授权即
 * CGRequestScreenCaptureAccess 请求系统弹授权引导（同 WPS 取色行为），并经
 * screenPickUnavailable 通知 QML 显示提示；授权在系统设置完成后需重启应用
 * 才生效。Wayland 会话抓不到真实屏幕（mogan 强制 QT_QPA_PLATFORM=xcb，经
 * XWayland 只能抓到黑色根窗口），canPickScreen 为 false，QML 侧据此隐藏
 * 「Pick screen color」按钮。
 */
class ColorPickerBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (bool canPickScreen READ canPickScreen CONSTANT)

public:
  explicit ColorPickerBridge (QObject* parent= nullptr) : QObject (parent) {}

  /** @brief 取色 overlay 未关闭时一并销毁，避免遗留全屏无父浮层。 */
  ~ColorPickerBridge () override;

  /** @brief 当前会话是否支持屏幕取色（Wayland 会话不支持）。 */
  bool canPickScreen () const;

  /**
   * @brief 发起屏幕取色：全屏快照 overlay 等待点击。
   *
   * 结果经 screenColorPicked 异步回流；取消（Esc）携带空串。会话不支持或
   * macOS 未授权屏幕录制时经 screenPickUnavailable 通知（区别于取消）。
   */
  Q_INVOKABLE void pickScreenColor ();

signals:
  /** @brief 取色结束；hex 为命中的 "#rrggbb"，取消（Esc）为空串。 */
  void screenColorPicked (const QString& hex);

  /**
   * @brief 屏幕取色不可用：macOS 未授权屏幕录制（已请求系统弹授权引导）、
   * 或当前会话/平台抓不到屏幕。QML 据此显示提示，避免点击毫无反应。
   */
  void screenPickUnavailable ();

private:
  QPointer<QWidget> m_overlay;
};

#endif // defined COLOR_PICKER_BRIDGE_H
