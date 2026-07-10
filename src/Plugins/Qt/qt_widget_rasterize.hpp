
/******************************************************************************
 * MODULE      : qt_widget_rasterize.hpp
 * DESCRIPTION : 把 TeXmacs widget 光栅化为 PNG data URL，供 QML Image 显示。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file qt_widget_rasterize.hpp
 *  @brief widget → QImage → base64 PNG data URL 的光栅化入口。
 *
 * @par 用途
 * 字体选择器 QML 预览：scheme 侧把样本 tree 经 texmacs_output_widget 排版成 box
 * widget，本函数把它光栅化成 data URL，喂 QML Image。复用 impress()
 * 的渲染路径。
 *
 * @par 清晰度
 * QImage 按 retina_factor 放大物理分辨率渲染（hidpi 自动 2× 像素），QML Image
 * 按 逻辑尺寸显示时降采样 → 清晰不糊。retina_factor 在 qt_utilities
 * 取自系统设备像素 比。勿再手动翻倍 QImage 尺寸。
 *
 * @note 同步、亚帧级：样本 box 已被 texmacs_output_widget
 * 排版好，handle_repaint 仅绘制。
 */

#ifndef QT_WIDGET_RASTERIZE_H
#define QT_WIDGET_RASTERIZE_H

#include "string.hpp"
#include "widget.hpp"

/**
 * @brief 把 widget 光栅化为 PNG data URL。
 * @param wid 已排版的 widget（经 texmacs_output_widget 构造）。
 * @return 形如 "data:image/png;base64,..." 的 string；wid 非 simple_widget
 *         或光栅化失败返回空 string。
 */
string rasterize_widget_to_png_data_url (widget wid);

/**
 * @brief glue 入口（cpp-rasterize-widget）：转发到
 * rasterize_widget_to_png_data_url。
 * @note 头不含 Qt 头，可安全被生成的 glue 代码 include。
 */
string cpp_rasterize_widget (widget wid);

#endif // defined QT_WIDGET_RASTERIZE_H
