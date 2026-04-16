
/******************************************************************************
 * MODULE     : pdf_preview_cache.cpp
 * DESCRIPTION: PDF preview page cache (memory-based with optional disk)
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "pdf_preview_cache.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QThread>

// Singleton instance
static PdfPreviewCache* g_instance= nullptr;
static QMutex           s_instanceMutex;

PdfPreviewCache::PdfPreviewCache (QObject* parent)
    : QObject (parent), memoryCache_ (DEFAULT_MEMORY_COST_MB * 1024 * 1024),
      hits_ (0), misses_ (0) {}

PdfPreviewCache::~PdfPreviewCache () {
  if (g_instance == this) {
    g_instance= nullptr;
  }
}

PdfPreviewCache*
PdfPreviewCache::instance () {
  QMutexLocker locker (&s_instanceMutex);
  if (!g_instance) {
    g_instance= new PdfPreviewCache ();
  }
  return g_instance;
}

QPixmap
PdfPreviewCache::get (const QString& url, int pageNumber, int dpi) {
  QString key= cacheKey (url, pageNumber, dpi);

  QMutexLocker locker (&mutex_);

  // Check memory cache first
  ImageCacheEntry* entry= memoryCache_.object (key);
  if (entry) {
    hits_++;
    return entry->pixmap;
  }

  // Optionally try disk cache (if enabled for this entry)
  QString path= diskPath (key);
  if (QFile::exists (path) &&
      !ImageCacheUtils::isFileExpired (path, DISK_CACHE_DAYS)) {
    QPixmap pixmap (path);
    if (!pixmap.isNull ()) {
      qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
      memoryCache_.insert (key, new ImageCacheEntry (pixmap, key, cost), cost);
      hits_++;
      return pixmap;
    }
  }

  misses_++;
  return QPixmap ();
}

void
PdfPreviewCache::put (const QString& url, int pageNumber, int dpi,
                      const QPixmap& pixmap, bool persistToDisk) {
  if (pixmap.isNull ()) return;

  QString key= cacheKey (url, pageNumber, dpi);

  QMutexLocker locker (&mutex_);

  // Store in memory cache
  qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
  memoryCache_.insert (key, new ImageCacheEntry (pixmap, key, cost), cost);

  // Optionally save to disk
  if (persistToDisk) {
    Qt::ConnectionType connType= QThread::currentThread () == this->thread ()
                                     ? Qt::DirectConnection
                                     : Qt::QueuedConnection;
    QMetaObject::invokeMethod (
        this, [this, key, pixmap] () { saveToDisk (key, pixmap); }, connType);
  }
}

bool
PdfPreviewCache::contains (const QString& url, int pageNumber, int dpi) const {
  QString key= cacheKey (url, pageNumber, dpi);

  QMutexLocker locker (&mutex_);
  return memoryCache_.contains (key);
}

void
PdfPreviewCache::clear () {
  QMutexLocker locker (&mutex_);
  memoryCache_.clear ();

  // Also clear disk cache
  QString dir= ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  QDir    cacheDir (dir);
  for (const QString& file : cacheDir.entryList (QDir::Files)) {
    cacheDir.remove (file);
  }
}

void
PdfPreviewCache::setMaxMemorySize (int mb) {
  QMutexLocker locker (&mutex_);
  memoryCache_.setMaxCost (mb * 1024 * 1024);
}

qint64
PdfPreviewCache::totalCost () const {
  QMutexLocker locker (&mutex_);
  return memoryCache_.totalCost ();
}

QString
PdfPreviewCache::cacheKey (const QString& url, int pageNumber, int dpi) const {
  return ImageCacheUtils::makeKey (
      url, {QString::number (pageNumber), QString::number (dpi)});
}

QString
PdfPreviewCache::diskPath (const QString& key) const {
  QString dir = ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  QString hash= ImageCacheUtils::urlToFilename (key);
  return QDir (dir).filePath (hash + ".png");
}

void
PdfPreviewCache::saveToDisk (const QString& key, const QPixmap& pixmap) {
  QString path= diskPath (key);
  pixmap.save (path, "PNG"); // PNG for lossless quality
}
