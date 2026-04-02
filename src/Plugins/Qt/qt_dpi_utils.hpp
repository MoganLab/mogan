
/******************************************************************************
 * MODULE     : qt_dpi_utils.hpp
 * DESCRIPTION: Unified DPI and scale factor utilities for Qt widgets
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 ******************************************************************************/

#ifndef QT_DPI_UTILS_HPP
#define QT_DPI_UTILS_HPP

#include <QRect>
#include <QPoint>
#include <QSize>

class QScreen;

/**
 * Unified DPI/Scale factor utilities for cross-platform HiDPI handling.
 * 
 * Windows: Uses logicalDotsPerInch() / 96.0
 * macOS/Linux: Uses devicePixelRatio()
 */
class DpiUtils {
public:
    /**
     * Get the scale factor for the given screen.
     * @param screen The screen to query. If null, uses primary screen.
     * @return Scale factor (1.0 = 96 DPI standard)
     */
    static qreal scaleFactor(QScreen* screen = nullptr);
    
    /**
     * Get the scale factor for the primary screen.
     * Convenience method for scaleFactor() with no arguments.
     */
    static qreal mainScreenScale();
    
    /**
     * Scale an integer size by the screen's scale factor.
     * Uses floor(x * scale + 0.5) for proper pixel alignment.
     * @param baseSize Base size at 96 DPI
     * @param screen Target screen (null = primary)
     * @return Scaled and rounded size
     */
    static int scaled(int baseSize, QScreen* screen = nullptr);
    
    /**
     * Scale an integer size by a specific scale factor.
     * @param baseSize Base size at 96 DPI
     * @param scale Scale factor to apply
     * @return Scaled and rounded size
     */
    static int scaled(int baseSize, qreal scale);
    
    /**
     * Scale a floating-point size by the screen's scale factor.
     * @param baseSize Base size at 96 DPI
     * @param screen Target screen (null = primary)
     * @return Scaled size (not rounded)
     */
    static qreal scaledF(qreal baseSize, QScreen* screen = nullptr);
    
    /**
     * Scale a floating-point size by a specific scale factor.
     * @param baseSize Base size at 96 DPI
     * @param scale Scale factor to apply
     * @return Scaled size
     */
    static qreal scaledF(qreal baseSize, qreal scale);
    
    /**
     * Convert logical (device-independent) rectangle to physical pixels.
     * Used for screenshot coordinates, image extraction, etc.
     */
    static QRect toPhysicalRect(const QRect& logicalRect, QScreen* screen = nullptr);
    static QPoint toPhysicalPoint(const QPoint& logicalPoint, QScreen* screen = nullptr);
    static QSize toPhysicalSize(const QSize& logicalSize, QScreen* screen = nullptr);
    
    /**
     * Convert physical pixel coordinates to logical (device-independent) coordinates.
     */
    static QRect toLogicalRect(const QRect& physicalRect, QScreen* screen = nullptr);
    static QPoint toLogicalPoint(const QPoint& physicalPoint, QScreen* screen = nullptr);
    static QSize toLogicalSize(const QSize& physicalSize, QScreen* screen = nullptr);

private:
    // Base DPI for Windows (standard 96 DPI)
    static constexpr qreal BASE_DPI = 96.0;
    
    // Prevent instantiation - static utility class
    DpiUtils() = delete;
};

#endif // QT_DPI_UTILS_HPP
