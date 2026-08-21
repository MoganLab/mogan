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
#include <QString>

/*! @class ColorPickerBridge
 *  @brief 承载 QML 调色板需要 native 能力的交互：屏幕取色。
 *
 * @par 实现要点
 * QML 无法直接抓屏与拦截全屏鼠标点击，故屏幕取色走本 bridge：grabWindow
 * 抓全屏快照，铺满全屏的 frameless overlay 显示快照 + 十字光标，点击处按
 * devicePixelRatio 换算回图像像素取色，Esc 取消。
 *
 * @note macOS 下抓屏需要「屏幕录制」权限，未授权时 grabWindow 返回空图，
 * 本方法返回空串（取色不可用），与 Qt 自带 QColorDialog 行为一致。
 */
class ColorPickerBridge : public QObject {
  Q_OBJECT

public:
  explicit ColorPickerBridge (QObject* parent= nullptr) : QObject (parent) {}

  /**
   * @brief 进入屏幕取色模式：全屏快照 overlay，点击取色。
   * @return 命中的颜色 "#rrggbb"；取消（Esc）或取色不可用返回空串。
   */
  Q_INVOKABLE QString pickScreenColor ();
};

#endif // defined COLOR_PICKER_BRIDGE_H
