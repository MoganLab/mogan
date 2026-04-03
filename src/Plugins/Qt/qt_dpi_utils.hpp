
/******************************************************************************
 * MODULE     : qt_dpi_utils.hpp
 * DESCRIPTION: Unified DPI and scale factor utilities for Qt widgets
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 ******************************************************************************/

#ifndef QT_DPI_UTILS_HPP
#define QT_DPI_UTILS_HPP

#include <QPoint>
#include <QRect>
#include <QSize>

class QScreen;

/**
 * DPI/Scale 工具类，用于跨平台 HiDPI 处理。
 *
 * Windows/Linux: 使用 logicalDotsPerInch() / 96.0 计算缩放比例
 * macOS: 使用 logicalDotsPerInch() / 72.0 计算缩放比例（macOS 传统基准）
 */
class DpiUtils {
public:
  /**
   * 获取指定屏幕的缩放比例。
   * @param screen 目标屏幕，传入 nullptr 则使用主屏幕
   * @return 缩放比例 (1.0 = 96 DPI 标准)
   */
  static qreal scaleFactor (QScreen* screen= nullptr);

  /**
   * 按屏幕缩放比例对整数值进行缩放。
   * @param baseSize 96 DPI 下的基准大小
   * @param screen 目标屏幕，传入 nullptr 则使用主屏幕
   * @return 缩放后并四舍五入的整数
   */
  static int scaled (int baseSize, QScreen* screen= nullptr);

  /**
   * 按指定缩放比例对整数值进行缩放。
   * @param baseSize 96 DPI 下的基准大小
   * @param scale 要应用的缩放比例
   * @return 缩放后并四舍五入的整数
   */
  static int scaled (int baseSize, qreal scale);

  /**
   * 按屏幕缩放比例对浮点值进行缩放。
   * @param baseSize 96 DPI 下的基准大小
   * @param screen 目标屏幕，传入 nullptr 则使用主屏幕
   * @return 缩放后的浮点值（不进行四舍五入）
   */
  static qreal scaledF (qreal baseSize, QScreen* screen= nullptr);

  /**
   * 按指定缩放比例对浮点值进行缩放。
   * @param baseSize 96 DPI 下的基准大小
   * @param scale 要应用的缩放比例
   * @return 缩放后的浮点值
   */
  static qreal scaledF (qreal baseSize, qreal scale);

  /**
   * 将逻辑坐标（设备无关）转换为物理像素坐标。
   * 用于截图坐标、图像提取等场景。
   */
  static QRect  toPhysicalRect (const QRect& logicalRect,
                                QScreen*     screen= nullptr);
  static QPoint toPhysicalPoint (const QPoint& logicalPoint,
                                 QScreen*      screen= nullptr);
  static QSize  toPhysicalSize (const QSize& logicalSize,
                                QScreen*     screen= nullptr);

  /**
   * 将物理像素坐标转换为逻辑坐标（设备无关）。
   */
  static QRect  toLogicalRect (const QRect& physicalRect,
                               QScreen*     screen= nullptr);
  static QPoint toLogicalPoint (const QPoint& physicalPoint,
                                QScreen*      screen= nullptr);
  static QSize  toLogicalSize (const QSize& physicalSize,
                               QScreen*     screen= nullptr);

private:
  // Windows 基准 DPI（标准 96 DPI）
  static constexpr qreal BASE_DPI= 96.0;
  // macOS 传统基准 DPI（72 DPI）
  static constexpr qreal MACOS_BASE_DPI= 72.0;

  // 禁止实例化 - 静态工具类
  DpiUtils ()= delete;
};

#endif // QT_DPI_UTILS_HPP
