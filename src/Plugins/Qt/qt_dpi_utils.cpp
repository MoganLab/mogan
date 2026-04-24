
/******************************************************************************
 * MODULE     : qt_dpi_utils.cpp
 * DESCRIPTION: Unified DPI and scale factor utilities for Qt widgets
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 ******************************************************************************/

#include "qt_dpi_utils.hpp"

#include <QFont>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

qreal
DpiUtils::scaleFactor (QScreen* screen) {
  if (!screen) {
    screen= QGuiApplication::primaryScreen ();
    if (!screen) {
      return 1.0;
    }
  }

#ifdef Q_OS_MAC
  // macOS: 使用逻辑 DPI / 72.0
  // macOS 传统上使用 72 DPI 作为基准
  qreal dpi= screen->logicalDotsPerInch ();
  return dpi / MACOS_BASE_DPI;
#else
  // Windows/Linux: 使用逻辑 DPI 计算缩放比例
  // 与系统自身的缩放行为保持一致（基准 96 DPI）
  qreal dpi= screen->logicalDotsPerInch ();
  return dpi / BASE_DPI;
#endif
}

int
DpiUtils::scaled (int baseSize, QScreen* screen) {
  return scaled (baseSize, scaleFactor (screen));
}

int
DpiUtils::scaled (int baseSize, qreal scale) {
  // 使用 qRound 进行四舍五入，确保像素对齐
  return qRound (baseSize * scale);
}

qreal
DpiUtils::scaledF (qreal baseSize, QScreen* screen) {
  return scaledF (baseSize, scaleFactor (screen));
}

qreal
DpiUtils::scaledF (qreal baseSize, qreal scale) {
  return baseSize * scale;
}

QFont
DpiUtils::scaledFont (const QFont& baseFont, int basePixelSize,
                      QScreen* screen) {
  QFont font= baseFont;
  font.setPixelSize (scaled (basePixelSize, screen));
  return font;
}

void
DpiUtils::applyScaledFont (QWidget* widget, int basePixelSize,
                           QScreen* screen) {
  if (!widget) return;
  widget->setFont (scaledFont (widget->font (), basePixelSize, screen));
}

// ========== 坐标转换：逻辑 → 物理 ==========

QRect
DpiUtils::toPhysicalRect (const QRect& logicalRect, QScreen* screen) {
  qreal scale= scaleFactor (screen);
  return QRect (qRound (logicalRect.x () * scale),
                qRound (logicalRect.y () * scale),
                qRound (logicalRect.width () * scale),
                qRound (logicalRect.height () * scale));
}

QPoint
DpiUtils::toPhysicalPoint (const QPoint& logicalPoint, QScreen* screen) {
  qreal scale= scaleFactor (screen);
  return QPoint (qRound (logicalPoint.x () * scale),
                 qRound (logicalPoint.y () * scale));
}

QSize
DpiUtils::toPhysicalSize (const QSize& logicalSize, QScreen* screen) {
  qreal scale= scaleFactor (screen);
  return QSize (qRound (logicalSize.width () * scale),
                qRound (logicalSize.height () * scale));
}

// ========== 坐标转换：物理 → 逻辑 ==========

QRect
DpiUtils::toLogicalRect (const QRect& physicalRect, QScreen* screen) {
  qreal scale= scaleFactor (screen);
  return QRect (qRound (physicalRect.x () / scale),
                qRound (physicalRect.y () / scale),
                qRound (physicalRect.width () / scale),
                qRound (physicalRect.height () / scale));
}

QPoint
DpiUtils::toLogicalPoint (const QPoint& physicalPoint, QScreen* screen) {
  qreal scale= scaleFactor (screen);
  return QPoint (qRound (physicalPoint.x () / scale),
                 qRound (physicalPoint.y () / scale));
}

QSize
DpiUtils::toLogicalSize (const QSize& physicalSize, QScreen* screen) {
  qreal scale= scaleFactor (screen);
  return QSize (qRound (physicalSize.width () / scale),
                qRound (physicalSize.height () / scale));
}
