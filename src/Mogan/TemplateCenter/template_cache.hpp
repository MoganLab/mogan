
/******************************************************************************
 * MODULE     : template_cache.hpp
 * DESCRIPTION: Template cache manager for offline access
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TEMPLATE_CACHE_HPP
#define TEMPLATE_CACHE_HPP

#include <QDateTime>
#include <QHash>
#include <QObject>

// Common type definitions
#include "template_types.hpp"

/**
 * @brief Cache entry metadata
 */
struct CacheEntry {
  QString   templateId;
  QString   localPath;
  QString   etag; // For HTTP caching
  QDateTime cachedAt;
  QDateTime expiresAt; // Cache expiration time
  qint64    fileSize;

  CacheEntry () : fileSize (0) {}
};

/**
 * @brief Template cache manager
 *
 * Responsibilities:
 * - Store and retrieve cached template metadata
 * - Manage cache expiration and cleanup
 * - Track downloaded template files
 * - Provide offline access to templates
 */
class TemplateCache : public QObject {
  Q_OBJECT

public:
  explicit TemplateCache (QObject* parent= nullptr);
  ~TemplateCache ();

  // Initialization
  bool initialize ();
  bool isInitialized () const { return initialized_; }

  // Metadata cache operations
  QHash<QString, TemplateMetadataPtr> loadMetadataCache ();
  void saveMetadataCache (const QHash<QString, TemplateMetadataPtr>& metadata);

  // Template file operations
  bool              isTemplateCached (const QString& templateId) const;
  QString           cachedTemplatePath (const QString& templateId) const;
  void              registerCachedTemplate (const QString& templateId,
                                            const QString& localPath, qint64 fileSize);
  void              removeCachedTemplate (const QString& templateId);
  QList<CacheEntry> cachedTemplates () const;

  // Cache management
  void   clearCache ();
  void   cleanupExpiredCache ();
  qint64 cacheSize () const;

  // Last update tracking
  QDateTime lastMetadataUpdate () const;
  void      setLastMetadataUpdate (const QDateTime& time);

  // Cache info
  QString cacheDirectory () const;

signals:
  void cacheCleared ();
  void cacheEntryRemoved (const QString& templateId);

private:
  // Cache file paths
  QString metadataCachePath () const;
  QString templatesCacheDir () const;
  QString cacheIndexPath () const;

  // Cache index management
  void loadCacheIndex ();
  void saveCacheIndex ();

  // Utility functions
  void ensureCacheDirectory () const;

private:
  bool initialized_;

  // Cache storage
  QHash<QString, CacheEntry> cacheIndex_;
  QDateTime                  lastMetadataUpdate_;

  // Cache configuration
  static constexpr int CACHE_EXPIRY_DAYS= 7;
};

#endif // TEMPLATE_CACHE_HPP
