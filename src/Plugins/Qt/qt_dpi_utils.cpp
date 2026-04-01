/******************************************************************************
 * MODULE     : qtm_dpi_utils.cpp
 * DESCRIPTION: Unified DPI and scale factor utilities for Qt widgets
 * COPYRIGHT  : (C) 2026  Liii Labs
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 ******************************************************************************/

#include "qt_dpi_utils.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QtMath>

qreal DpiUtils::scaleFactor(QScreen* screen) {
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
        if (!screen) {
            return 1.0;
        }
    }
    
#ifdef Q_OS_WIN
    // Windows: Use logical DPI to calculate scale factor
    // This matches Windows' own scaling behavior
    qreal dpi = screen->logicalDotsPerInch();
    return dpi / BASE_DPI;
#else
    // macOS/Linux: Use devicePixelRatio
    // On macOS this accounts for Retina displays automatically
    return screen->devicePixelRatio();
#endif
}

qreal DpiUtils::mainScreenScale() {
    return scaleFactor();
}

int DpiUtils::scaled(int baseSize, QScreen* screen) {
    return scaled(baseSize, scaleFactor(screen));
}

int DpiUtils::scaled(int baseSize, qreal scale) {
    // Use floor(x + 0.5) for proper rounding to nearest integer
    // This ensures pixel-perfect alignment
    return static_cast<int>(qFloor(baseSize * scale + 0.5));
}

qreal DpiUtils::scaledF(qreal baseSize, QScreen* screen) {
    return scaledF(baseSize, scaleFactor(screen));
}

qreal DpiUtils::scaledF(qreal baseSize, qreal scale) {
    return baseSize * scale;
}

// ========== Coordinate Conversion: Logical → Physical ==========

QRect DpiUtils::toPhysicalRect(const QRect& logicalRect, QScreen* screen) {
    qreal scale = scaleFactor(screen);
    return QRect(
        static_cast<int>(logicalRect.x() * scale),
        static_cast<int>(logicalRect.y() * scale),
        static_cast<int>(logicalRect.width() * scale),
        static_cast<int>(logicalRect.height() * scale)
    );
}

QPoint DpiUtils::toPhysicalPoint(const QPoint& logicalPoint, QScreen* screen) {
    qreal scale = scaleFactor(screen);
    return QPoint(
        static_cast<int>(logicalPoint.x() * scale),
        static_cast<int>(logicalPoint.y() * scale)
    );
}

QSize DpiUtils::toPhysicalSize(const QSize& logicalSize, QScreen* screen) {
    qreal scale = scaleFactor(screen);
    return QSize(
        static_cast<int>(logicalSize.width() * scale),
        static_cast<int>(logicalSize.height() * scale)
    );
}

// ========== Coordinate Conversion: Physical → Logical ==========

QRect DpiUtils::toLogicalRect(const QRect& physicalRect, QScreen* screen) {
    qreal scale = scaleFactor(screen);
    return QRect(
        static_cast<int>(physicalRect.x() / scale),
        static_cast<int>(physicalRect.y() / scale),
        static_cast<int>(physicalRect.width() / scale),
        static_cast<int>(physicalRect.height() / scale)
    );
}

QPoint DpiUtils::toLogicalPoint(const QPoint& physicalPoint, QScreen* screen) {
    qreal scale = scaleFactor(screen);
    return QPoint(
        static_cast<int>(physicalPoint.x() / scale),
        static_cast<int>(physicalPoint.y() / scale)
    );
}

QSize DpiUtils::toLogicalSize(const QSize& physicalSize, QScreen* screen) {
    qreal scale = scaleFactor(screen);
    return QSize(
        static_cast<int>(physicalSize.width() / scale),
        static_cast<int>(physicalSize.height() / scale)
    );
}
