
/******************************************************************************
 * MODULE     : thumbnail_cache.cpp
 * DESCRIPTION: Thumbnail image cache with memory + disk persistence
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "thumbnail_cache.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QStandardPaths>
#include <QThread>

// Singleton instance
static ThumbnailCache* g_instance= nullptr;
static QMutex          s_instanceMutex;

ThumbnailCache::ThumbnailCache (QObject* parent)
    : QObject (parent), memoryCache_ (MAX_MEMORY_COST_MB * 1024 * 1024),
      memoryHits_ (0), diskHits_ (0), misses_ (0) {}

ThumbnailCache::~ThumbnailCache () {
  if (g_instance == this) {
    g_instance= nullptr;
  }
}

ThumbnailCache*
ThumbnailCache::instance () {
  QMutexLocker locker (&s_instanceMutex);
  if (!g_instance) {
    g_instance= new ThumbnailCache ();
  }
  return g_instance;
}

QPixmap
ThumbnailCache::get (const QString& url, const QSize& targetSize) {
  QString key= cacheKey (url, targetSize);

  QMutexLocker locker (&mutex_);

  // Check memory cache first
  ImageCacheEntry* entry= memoryCache_.object (key);
  if (entry) {
    memoryHits_++;
    return entry->pixmap;
  }

  // Try to load from disk
  QString path= diskPath (key);
  if (QFile::exists (path) &&
      !ImageCacheUtils::isFileExpired (path, DISK_CACHE_DAYS)) {
    QPixmap pixmap (path);
    if (!pixmap.isNull ()) {
      // Store in memory cache for future access
      qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
      memoryCache_.insert (key, new ImageCacheEntry (pixmap, key, cost), cost);
      diskHits_++;
      return pixmap;
    }
  }

  misses_++;
  return QPixmap ();
}

void
ThumbnailCache::put (const QString& url, const QSize& targetSize,
                     const QPixmap& pixmap) {
  if (pixmap.isNull ()) return;

  QString key= cacheKey (url, targetSize);

  QMutexLocker locker (&mutex_);

  // Store in memory cache
  qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
  memoryCache_.insert (key, new ImageCacheEntry (pixmap, key, cost), cost);

  // Save to disk asynchronously (don't block)
  Qt::ConnectionType connType= QThread::currentThread () == this->thread ()
                                   ? Qt::DirectConnection
                                   : Qt::QueuedConnection;
  QMetaObject::invokeMethod (
      this, [this, key, pixmap] () { saveToDisk (key, pixmap); }, connType);
}

bool
ThumbnailCache::contains (const QString& url, const QSize& targetSize) const {
  QString key= cacheKey (url, targetSize);

  QMutexLocker locker (&mutex_);

  // Check memory cache
  if (memoryCache_.contains (key)) {
    return true;
  }

  // Check disk cache
  QString path= diskPath (key);
  return QFile::exists (path) &&
         !ImageCacheUtils::isFileExpired (path, DISK_CACHE_DAYS);
}

void
ThumbnailCache::preload (const QString& url, const QSize& targetSize) {
  QString key= cacheKey (url, targetSize);

  QMutexLocker locker (&mutex_);

  // Skip if already in memory
  if (memoryCache_.contains (key)) return;

  // Try to load from disk
  QString path= diskPath (key);
  if (QFile::exists (path)) {
    QPixmap pixmap (path);
    if (!pixmap.isNull ()) {
      qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
      memoryCache_.insert (key, new ImageCacheEntry (pixmap, key, cost), cost);
    }
  }
}

void
ThumbnailCache::clear () {
  QMutexLocker locker (&mutex_);

  memoryCache_.clear ();

  // Clear disk cache
  QString dir= ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  QDir    cacheDir (dir);
  for (const QString& file : cacheDir.entryList (QDir::Files)) {
    cacheDir.remove (file);
  }
}

void
ThumbnailCache::cleanupExpired () {
  QString dir= ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  ImageCacheUtils::cleanupCacheDir (dir, DISK_CACHE_DAYS,
                                    100 * 1024 * 1024); // Max 100MB
}

qint64
ThumbnailCache::memoryCacheSize () const {
  QMutexLocker locker (&mutex_);
  return memoryCache_.totalCost ();
}

qint64
ThumbnailCache::diskCacheSize () const {
  QString dir= ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  QDir    cacheDir (dir);

  qint64 total= 0;
  for (const QFileInfo& info : cacheDir.entryInfoList (QDir::Files)) {
    total+= info.size ();
  }
  return total;
}

QString
ThumbnailCache::cacheKey (const QString& url, const QSize& size) const {
  return ImageCacheUtils::makeKey (
      url, {QString::number (size.width ()), QString::number (size.height ())});
}

QString
ThumbnailCache::diskPath (const QString& key) const {
  QString dir = ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  QString hash= ImageCacheUtils::urlToFilename (key);
  return QDir (dir).filePath (hash + ".jpg");
}

void
ThumbnailCache::saveToDisk (const QString& key, const QPixmap& pixmap) {
  QString path= diskPath (key);
  pixmap.save (path, "JPEG", 85); // Good quality, smaller size
}
