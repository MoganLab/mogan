
/******************************************************************************
 * MODULE     : thumbnail_cache.cpp
 * DESCRIPTION: Thumbnail image cache with memory + disk persistence
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "thumbnail_cache.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QStandardPaths>
#include <QThread>

// Singleton instance
static ThumbnailCache* g_instance= nullptr;
static QMutex          s_instanceMutex;

static void
cleanupThumbnailCache () {
  QMutexLocker locker (&s_instanceMutex);
  delete g_instance;
  g_instance= nullptr;
}

ThumbnailCache::ThumbnailCache (QObject* parent)
    : QObject (parent), memoryCache_ (MAX_MEMORY_COST_MB * 1024 * 1024),
      memoryHits_ (0), diskHits_ (0), misses_ (0), indexDirty_ (false),
      saveIndexTimer_ (nullptr) {
  loadIndex ();
}

ThumbnailCache::~ThumbnailCache () {
  // Ensure pending index changes are flushed before destruction
  flushIndex ();
  if (g_instance == this) {
    g_instance= nullptr;
  }
}

ThumbnailCache*
ThumbnailCache::instance () {
  QMutexLocker locker (&s_instanceMutex);
  if (!g_instance) {
    g_instance= new ThumbnailCache ();
    qAddPostRoutine (cleanupThumbnailCache);
  }
  return g_instance;
}

ThumbnailCache::ThumbnailCacheEntry
ThumbnailCache::getEntry (const QString& url, const QSize& targetSize) {
  QString             key= cacheKey (url, targetSize);
  ThumbnailCacheEntry result;

  QMutexLocker locker (&mutex_);

  // Check memory cache first
  ImageCacheEntry* entry= memoryCache_.object (key);
  if (entry) {
    memoryHits_++;
    result.pixmap      = entry->pixmap;
    result.etag        = entry->etag;
    result.lastModified= entry->lastModified;
    return result;
  }

  // Try to load from disk
  QString path= diskPath (key);
  if (QFile::exists (path) &&
      !ImageCacheUtils::isFileExpired (path, DISK_CACHE_DAYS)) {
    QPixmap pixmap (path);
    if (!pixmap.isNull ()) {
      QString   etag;
      QDateTime lastModified;
      auto      it= diskIndex_.find (key);
      if (it != diskIndex_.end ()) {
        QJsonObject meta= it.value ();
        etag            = meta["etag"].toString ();
        QString lmStr   = meta["lastModified"].toString ();
        if (!lmStr.isEmpty ()) {
          lastModified= QDateTime::fromString (lmStr, Qt::ISODate);
        }
      }

      // Store in memory cache for future access
      qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
      memoryCache_.insert (
          key, new ImageCacheEntry (pixmap, key, cost, etag, lastModified),
          cost);
      diskHits_++;
      result.pixmap      = pixmap;
      result.etag        = etag;
      result.lastModified= lastModified;
      return result;
    }
  }

  misses_++;
  return result;
}

QPixmap
ThumbnailCache::get (const QString& url, const QSize& targetSize) {
  return getEntry (url, targetSize).pixmap;
}

void
ThumbnailCache::put (const QString& url, const QSize& targetSize,
                     const QPixmap& pixmap, const QString& etag,
                     const QDateTime& lastModified) {
  if (pixmap.isNull ()) return;

  QString key= cacheKey (url, targetSize);

  QMutexLocker locker (&mutex_);

  // Store in memory cache
  qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
  memoryCache_.insert (
      key, new ImageCacheEntry (pixmap, key, cost, etag, lastModified), cost);

  // Update disk index
  QJsonObject meta;
  if (!etag.isEmpty ()) meta["etag"]= etag;
  if (lastModified.isValid ())
    meta["lastModified"]= lastModified.toString (Qt::ISODate);
  meta["cachedAt"]= QDateTime::currentDateTime ().toString (Qt::ISODate);
  diskIndex_[key] = meta;
  indexDirty_     = true;

  // Debounce index flush to batch multiple puts into a single disk write
  if (!saveIndexTimer_) {
    saveIndexTimer_= new QTimer (this);
    saveIndexTimer_->setSingleShot (true);
    saveIndexTimer_->setInterval (500);
    connect (saveIndexTimer_, &QTimer::timeout, this,
             &ThumbnailCache::flushIndex);
  }
  saveIndexTimer_->start ();

  // Save image to disk asynchronously (queued to avoid deadlock with mutex)
  QMetaObject::invokeMethod (
      this, [this, key, pixmap] () { saveToDisk (key, pixmap); },
      Qt::QueuedConnection);
}

void
ThumbnailCache::put (const QString& url, const QSize& targetSize,
                     const QPixmap& pixmap) {
  put (url, targetSize, pixmap, QString (), QDateTime ());
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
      QString   etag;
      QDateTime lastModified;
      auto      it= diskIndex_.find (key);
      if (it != diskIndex_.end ()) {
        QJsonObject meta= it.value ();
        etag            = meta["etag"].toString ();
        QString lmStr   = meta["lastModified"].toString ();
        if (!lmStr.isEmpty ()) {
          lastModified= QDateTime::fromString (lmStr, Qt::ISODate);
        }
      }
      qint64 cost= ImageCacheUtils::pixmapCost (pixmap);
      memoryCache_.insert (
          key, new ImageCacheEntry (pixmap, key, cost, etag, lastModified),
          cost);
    }
  }
}

void
ThumbnailCache::clear () {
  QMutexLocker locker (&mutex_);

  memoryCache_.clear ();
  diskIndex_.clear ();

  // Clear disk cache (including index file)
  QString dir= ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  QDir    cacheDir (dir);
  for (const QString& file : cacheDir.entryList (QDir::Files)) {
    cacheDir.remove (file);
  }
  qDebug () << "[ThumbnailCache] Cleared all cached thumbnails";
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

QString
ThumbnailCache::indexPath () const {
  QString dir= ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
  return QDir (dir).filePath ("thumbnail-index.json");
}

void
ThumbnailCache::loadIndex () {
  QString path= indexPath ();
  if (!QFile::exists (path)) return;

  QFile file (path);
  if (!file.open (QIODevice::ReadOnly)) {
    qWarning () << "[ThumbnailCache] Failed to read index:" << path;
    return;
  }

  QJsonDocument doc= QJsonDocument::fromJson (file.readAll ());
  file.close ();

  if (!doc.isObject ()) return;

  QJsonObject obj= doc.object ();
  for (auto it= obj.begin (); it != obj.end (); ++it) {
    diskIndex_[it.key ()]= it.value ().toObject ();
  }
  qDebug () << "[ThumbnailCache] Loaded index with" << diskIndex_.size ()
            << "entries";
}

void
ThumbnailCache::saveIndex () {
  QJsonObject obj;
  {
    QMutexLocker locker (&mutex_);
    for (auto it= diskIndex_.begin (); it != diskIndex_.end (); ++it) {
      obj[it.key ()]= it.value ();
    }
  }

  QString path= indexPath ();
  QFile   file (path);
  if (file.open (QIODevice::WriteOnly)) {
    file.write (QJsonDocument (obj).toJson ());
    file.close ();
  }
  else {
    qWarning () << "[ThumbnailCache] Failed to write index:" << path;
  }
}

void
ThumbnailCache::flushIndex () {
  if (!indexDirty_) return;
  saveIndex ();
  indexDirty_= false;
}

void
ThumbnailCache::saveToDisk (const QString& key, const QPixmap& pixmap) {
  QString path= diskPath (key);
  if (!pixmap.save (path, "JPEG", 85)) {
    qWarning () << "[ThumbnailCache] Failed to write image:" << path;
  }
}
