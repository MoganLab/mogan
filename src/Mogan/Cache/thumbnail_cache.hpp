
/******************************************************************************
 * MODULE     : thumbnail_cache.hpp
 * DESCRIPTION: Thumbnail image cache with memory + disk persistence
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef THUMBNAIL_CACHE_HPP
#define THUMBNAIL_CACHE_HPP

#include <QCache>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QTimer>

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
   * @brief Cache entry with HTTP metadata
   */
  struct ThumbnailCacheEntry {
    QPixmap   pixmap;
    QString   etag;
    QDateTime lastModified;
    bool      isValid () const { return !pixmap.isNull (); }
  };

  /**
   * @brief Get thumbnail from cache (with HTTP metadata)
   * @param url Image URL (used as cache key)
   * @param targetSize Target size for scaling (cached separately for different
   * sizes)
   * @return Cache entry with pixmap and ETag/Last-Modified
   */
  ThumbnailCacheEntry getEntry (const QString& url, const QSize& targetSize);

  /**
   * @brief Get thumbnail pixmap from cache (convenience wrapper)
   */
  QPixmap get (const QString& url, const QSize& targetSize);

  /**
   * @brief Store thumbnail in cache (with HTTP metadata)
   * @param url Image URL
   * @param targetSize Target size
   * @param pixmap Thumbnail pixmap
   * @param etag HTTP ETag header value
   * @param lastModified HTTP Last-Modified header value
   */
  void put (const QString& url, const QSize& targetSize, const QPixmap& pixmap,
            const QString& etag, const QDateTime& lastModified);

  /**
   * @brief Store thumbnail in cache (without HTTP metadata)
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

  /**
   * @brief Flush pending index changes to disk immediately
   */
  void flushIndex ();

  // Cache statistics
  qint64 memoryCacheSize () const;
  qint64 diskCacheSize () const;
  qint64 memoryHits () const { return memoryHits_; }
  qint64 diskHits () const { return diskHits_; }
  qint64 misses () const { return misses_; }

private:
  QString cacheKey (const QString& url, const QSize& size) const;
  QString diskPath (const QString& key) const;
  QString indexPath () const;
  void    loadIndex ();
  void    saveIndex ();
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

  // Disk index: key -> metadata JSON object (loaded from thumbnail-index.json)
  QHash<QString, QJsonObject> diskIndex_;

  // Index flush debounce (batch multiple puts into a single disk write)
  bool    indexDirty_    = false;
  QTimer* saveIndexTimer_= nullptr;

  // Configuration
  static constexpr int  MAX_MEMORY_COST_MB= 50;
  static constexpr int  DISK_CACHE_DAYS   = 30;
  static constexpr char CACHE_SUBDIR[]    = "thumbnails";
};

#endif // THUMBNAIL_CACHE_HPP
