
/******************************************************************************
 * MODULE     : image_cache_base.hpp
 * DESCRIPTION: Base utilities for image/thumbnail caching
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef IMAGE_CACHE_BASE_HPP
#define IMAGE_CACHE_BASE_HPP

#include <QCache>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QMutex>
#include <QPixmap>
#include <QString>

/**
 * @brief Cache entry with metadata for memory cache
 */
struct ImageCacheEntry {
  QPixmap   pixmap;
  QString   key;
  QDateTime cachedAt;
  qint64    cost; // Memory cost (bytes)
  QString   etag;
  QDateTime lastModified;

  ImageCacheEntry () : cost (0) {}
  ImageCacheEntry (const QPixmap& px, const QString& k, qint64 c,
                   const QString&   e = QString (),
                   const QDateTime& lm= QDateTime ())
      : pixmap (px), key (k), cachedAt (QDateTime::currentDateTime ()),
        cost (c), etag (e), lastModified (lm) {}
};

/**
 * @brief Base utilities for image caching
 * Provides common functionality without forcing inheritance
 */
class ImageCacheUtils {
public:
  /**
   * @brief Generate cache file name from URL
   * Uses MD5 hash to create safe filename
   */
  static QString urlToFilename (const QString& url);

  /**
   * @brief Generate cache key with parameters
   * Format: "url|param1|param2"
   */
  static QString makeKey (const QString& url, const QList<QString>& params);

  /**
   * @brief Get cache subdirectory path
   * Creates directory if it doesn't exist
   */
  static QString cacheSubdir (const QString& subdirName);

  /**
   * @brief Calculate pixmap memory cost in bytes
   */
  static qint64 pixmapCost (const QPixmap& pixmap);

  /**
   * @brief Check if cache file is expired
   */
  static bool isFileExpired (const QString& filePath, int maxAgeDays);

  /**
   * @brief Clean old cache files
   * @param cacheDir Directory to clean
   * @param maxAgeDays Maximum age in days
   * @param maxSizeBytes Maximum total size (0 = unlimited)
   */
  static void cleanupCacheDir (const QString& cacheDir, int maxAgeDays,
                               qint64 maxSizeBytes= 0);

  /**
   * @brief Get cache hit rate statistics
   */
  static double hitRate (qint64 hits, qint64 misses);

  /**
   * @brief Get environment variable as QString
   * Convenience wrapper for get_env -> QString conversion
   */
  static QString getEnvQString (const char* varName);
};

#endif // IMAGE_CACHE_BASE_HPP
