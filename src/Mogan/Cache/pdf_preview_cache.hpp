
/******************************************************************************
 * MODULE     : pdf_preview_cache.hpp
 * DESCRIPTION: PDF preview page cache (memory-based with optional disk)
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef PDF_PREVIEW_CACHE_HPP
#define PDF_PREVIEW_CACHE_HPP

#include <QCache>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QString>

#include "image_cache_base.hpp"

/**
 * @brief PDF preview cache - memory-based with optional disk persistence
 *
 * Features:
 * - Memory cache with size limit (evicts LRU when full)
 * - Optional disk persistence for frequently viewed PDFs
 * - Thread-safe access
 * - Key includes URL, page number, and DPI for accurate caching
 */
class PdfPreviewCache : public QObject {
  Q_OBJECT

public:
  explicit PdfPreviewCache (QObject* parent= nullptr);
  ~PdfPreviewCache ();

  // Singleton access
  static PdfPreviewCache* instance ();

  /**
   * @brief Get PDF page preview from cache
   * @param url PDF URL
   * @param pageNumber Page number (0-based)
   * @param dpi DPI for rendering
   * @return Preview pixmap, or null if not cached
   */
  QPixmap get (const QString& url, int pageNumber, int dpi);

  /**
   * @brief Store PDF page preview in cache
   * @param url PDF URL
   * @param pageNumber Page number
   * @param dpi DPI used for rendering
   * @param pixmap Preview pixmap
   * @param persistToDisk Whether to also save to disk cache
   */
  void put (const QString& url, int pageNumber, int dpi, const QPixmap& pixmap,
            bool persistToDisk= false);

  /**
   * @brief Check if preview is cached
   */
  bool contains (const QString& url, int pageNumber, int dpi) const;

  /**
   * @brief Clear all cached previews
   */
  void clear ();

  /**
   * @brief Set maximum memory cache size in MB
   */
  void setMaxMemorySize (int mb);

  // Cache statistics
  qint64 totalCost () const;
  qint64 hits () const { return hits_; }
  qint64 misses () const { return misses_; }

private:
  QString cacheKey (const QString& url, int pageNumber, int dpi) const;
  QString diskPath (const QString& key) const;
  void    saveToDisk (const QString& key, const QPixmap& pixmap);

private:
  // Memory cache
  QCache<QString, ImageCacheEntry> memoryCache_;
  mutable QMutex                   mutex_;

  // Statistics
  mutable qint64 hits_;
  mutable qint64 misses_;

  // Configuration
  static constexpr int  DEFAULT_MEMORY_COST_MB= 100; // 100MB default
  static constexpr int  DISK_CACHE_DAYS       = 7;   // Shorter than thumbnails
  static constexpr char CACHE_SUBDIR[]        = "pdf_previews";
};

#endif // PDF_PREVIEW_CACHE_HPP
