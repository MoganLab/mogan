
/******************************************************************************
 * MODULE     : thumbnail_cache.hpp
 * DESCRIPTION: Thumbnail image cache with memory + disk persistence
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef THUMBNAIL_CACHE_HPP
#define THUMBNAIL_CACHE_HPP

#include <QCache>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QString>

#include "image_cache_base.hpp"

/**
 * @brief Thumbnail cache with memory LRU + disk persistence
 *
 * Features:
 * - Memory cache with size limit (evicts LRU when full)
 * - Automatic disk persistence
 * - Configurable expiration (default 30 days)
 * - Thread-safe access
 */
class ThumbnailCache : public QObject {
  Q_OBJECT

public:
  explicit ThumbnailCache (QObject* parent= nullptr);
  ~ThumbnailCache ();

  // Singleton access
  static ThumbnailCache* instance ();

  /**
   * @brief Get thumbnail from cache
   * @param url Image URL (used as cache key)
   * @param targetSize Target size for scaling (cached separately for different
   * sizes)
   * @return Thumbnail pixmap, or null if not cached
   */
  QPixmap get (const QString& url, const QSize& targetSize);

  /**
   * @brief Store thumbnail in cache
   * @param url Image URL
   * @param targetSize Target size
   * @param pixmap Thumbnail pixmap
   */
  void put (const QString& url, const QSize& targetSize, const QPixmap& pixmap);

  /**
   * @brief Check if thumbnail is cached
   */
  bool contains (const QString& url, const QSize& targetSize) const;

  /**
   * @brief Preload thumbnail from disk to memory
   */
  void preload (const QString& url, const QSize& targetSize);

  /**
   * @brief Clear all cached thumbnails
   */
  void clear ();

  /**
   * @brief Clean expired cache files
   */
  void cleanupExpired ();

  // Cache statistics
  qint64 memoryCacheSize () const;
  qint64 diskCacheSize () const;
  qint64 memoryHits () const { return memoryHits_; }
  qint64 diskHits () const { return diskHits_; }
  qint64 misses () const { return misses_; }

private:
  QString cacheKey (const QString& url, const QSize& size) const;
  QString diskPath (const QString& key) const;
  void    loadFromDisk (const QString& key);
  void    saveToDisk (const QString& key, const QPixmap& pixmap);

private:
  // Memory cache: key -> ImageCacheEntry
  // Max cost = 50MB (adjustable)
  QCache<QString, ImageCacheEntry> memoryCache_;
  mutable QMutex                   mutex_;

  // Statistics
  mutable qint64 memoryHits_;
  mutable qint64 diskHits_;
  mutable qint64 misses_;

  // Configuration
  static constexpr int  MAX_MEMORY_COST_MB= 50;
  static constexpr int  DISK_CACHE_DAYS   = 30;
  static constexpr char CACHE_SUBDIR[]    = "thumbnails";
};

#endif // THUMBNAIL_CACHE_HPP
