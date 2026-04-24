
/******************************************************************************
 * MODULE     : pdf_file_cache.hpp
 * DESCRIPTION: PDF file cache (download once, reuse locally)
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef PDF_FILE_CACHE_HPP
#define PDF_FILE_CACHE_HPP

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QString>

/**
 * @brief PDF file cache entry with HTTP cache metadata
 */
struct PdfCacheEntry {
  QString   filePath;
  QString   etag;
  QDateTime lastModified;
  QDateTime cachedAt;

  bool isValid () const { return !filePath.isEmpty (); }
};

/**
 * @brief PDF file cache - stores downloaded PDFs locally
 *
 * Features:
 * - Stores downloaded PDFs to disk cache
 * - Reuses local files instead of re-downloading
 * - Supports HTTP conditional requests (ETag, Last-Modified)
 * - Configurable expiration time
 * - Thread-safe access
 */
class PdfFileCache : public QObject {
  Q_OBJECT

public:
  explicit PdfFileCache (QObject* parent= nullptr);
  ~PdfFileCache ();

  // Singleton access
  static PdfFileCache* instance ();

  /**
   * @brief Get cached PDF file path for a URL
   * @param url PDF URL
   * @return Cache entry with metadata, or invalid entry if not cached
   */
  PdfCacheEntry getEntry (const QString& url) const;

  /**
   * @brief Save downloaded PDF data to cache
   * @param url Original URL
   * @param data PDF file data
   * @param etag HTTP ETag header (optional)
   * @param lastModified HTTP Last-Modified header (optional)
   * @return Path to cached file
   */
  QString saveToCache (const QString& url, const QByteArray& data,
                       const QString&   etag        = QString (),
                       const QDateTime& lastModified= QDateTime ());

  /**
   * @brief Check if PDF is cached and not expired
   */
  bool contains (const QString& url) const;

  /**
   * @brief Clear all cached PDF files
   */
  void clear ();

  /**
   * @brief Set cache expiration in days (default: 30)
   */
  void setExpirationDays (int days);

  /**
   * @brief Get cache directory path
   */
  static QString cacheDirectory ();

private:
  QString       cacheFilePath (const QString& url) const;
  QString       metaFilePath (const QString& url) const;
  bool          isExpired (const QString& filePath) const;
  void          saveMetadata (const QString& url, const PdfCacheEntry& entry);
  PdfCacheEntry loadMetadata (const QString& url) const;

private:
  mutable QMutex mutex_;
  int            expirationDays_;

  static constexpr int  DEFAULT_EXPIRATION_DAYS= 30;
  static constexpr char CACHE_SUBDIR[]         = "pdf_files";
};

#endif // PDF_FILE_CACHE_HPP
