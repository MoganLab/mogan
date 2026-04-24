
/******************************************************************************
 * MODULE     : pdf_file_cache.cpp
 * DESCRIPTION: PDF file cache (download once, reuse locally)
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "pdf_file_cache.hpp"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "image_cache_base.hpp"

#include "string.hpp"

string get_env (string var);

// Singleton instance
static PdfFileCache* g_instance= nullptr;
static QMutex        s_instanceMutex;

PdfFileCache::PdfFileCache (QObject* parent)
    : QObject (parent), expirationDays_ (DEFAULT_EXPIRATION_DAYS) {}

PdfFileCache::~PdfFileCache () {
  if (g_instance == this) {
    g_instance= nullptr;
  }
}

PdfFileCache*
PdfFileCache::instance () {
  QMutexLocker locker (&s_instanceMutex);
  if (!g_instance) {
    g_instance= new PdfFileCache ();
  }
  return g_instance;
}

QString
PdfFileCache::cacheDirectory () {
  // Use unified cache directory from ImageCacheUtils
  return ImageCacheUtils::cacheSubdir (CACHE_SUBDIR);
}

QString
PdfFileCache::cacheFilePath (const QString& url) const {
  QString hash=
      QString (QCryptographicHash::hash (url.toUtf8 (), QCryptographicHash::Md5)
                   .toHex ());
  return QDir (cacheDirectory ()).filePath (hash + ".pdf");
}

QString
PdfFileCache::metaFilePath (const QString& url) const {
  QString hash=
      QString (QCryptographicHash::hash (url.toUtf8 (), QCryptographicHash::Md5)
                   .toHex ());
  return QDir (cacheDirectory ()).filePath (hash + ".json");
}

bool
PdfFileCache::isExpired (const QString& filePath) const {
  QFileInfo info (filePath);
  if (!info.exists ()) return true;
  return ImageCacheUtils::isFileExpired (filePath, expirationDays_);
}

void
PdfFileCache::saveMetadata (const QString& url, const PdfCacheEntry& entry) {
  QJsonObject obj;
  obj["url"]         = url;
  obj["etag"]        = entry.etag;
  obj["lastModified"]= entry.lastModified.toString (Qt::ISODate);
  obj["cachedAt"]    = entry.cachedAt.toString (Qt::ISODate);

  QFile file (metaFilePath (url));
  if (file.open (QIODevice::WriteOnly)) {
    file.write (QJsonDocument (obj).toJson ());
    file.close ();
  }
}

PdfCacheEntry
PdfFileCache::loadMetadata (const QString& url) const {
  PdfCacheEntry entry;
  QFile         file (metaFilePath (url));
  if (!file.open (QIODevice::ReadOnly)) {
    return entry;
  }

  QJsonDocument doc= QJsonDocument::fromJson (file.readAll ());
  file.close ();

  if (!doc.isObject ()) return entry;

  QJsonObject obj= doc.object ();
  entry.filePath = cacheFilePath (url);
  entry.etag     = obj["etag"].toString ();
  entry.lastModified=
      QDateTime::fromString (obj["lastModified"].toString (), Qt::ISODate);
  entry.cachedAt=
      QDateTime::fromString (obj["cachedAt"].toString (), Qt::ISODate);

  return entry;
}

PdfCacheEntry
PdfFileCache::getEntry (const QString& url) const {
  QMutexLocker locker (&mutex_);

  QString filePath= cacheFilePath (url);
  if (!QFile::exists (filePath) || isExpired (filePath)) {
    return PdfCacheEntry ();
  }

  PdfCacheEntry entry= loadMetadata (url);
  if (!entry.isValid ()) {
    // No metadata but file exists - return basic entry
    entry.filePath= filePath;
    entry.cachedAt= QFileInfo (filePath).lastModified ();
  }

  qDebug () << "[PdfFileCache] Cache hit:" << url;
  return entry;
}

bool
PdfFileCache::contains (const QString& url) const {
  return getEntry (url).isValid ();
}

QString
PdfFileCache::saveToCache (const QString& url, const QByteArray& data,
                           const QString& etag, const QDateTime& lastModified) {
  if (data.isEmpty ()) return QString ();

  QString filePath= cacheFilePath (url);

  QMutexLocker locker (&mutex_);

  QFile file (filePath);
  if (!file.open (QIODevice::WriteOnly)) {
    qWarning () << "[PdfFileCache] Failed to write cache file:" << filePath;
    return QString ();
  }

  file.write (data);
  file.close ();

  // Save metadata
  PdfCacheEntry entry;
  entry.filePath    = filePath;
  entry.etag        = etag;
  entry.lastModified= lastModified;
  entry.cachedAt    = QDateTime::currentDateTime ();
  saveMetadata (url, entry);

  qDebug () << "[PdfFileCache] Saved to cache:" << filePath
            << "size:" << data.size () << "bytes";
  return filePath;
}

void
PdfFileCache::clear () {
  QMutexLocker locker (&mutex_);

  QString dir= cacheDirectory ();
  QDir    cacheDir (dir);

  // Remove PDF files
  for (const QString& file :
       cacheDir.entryList (QStringList () << "*.pdf", QDir::Files)) {
    cacheDir.remove (file);
  }
  // Remove metadata files
  for (const QString& file :
       cacheDir.entryList (QStringList () << "*.json", QDir::Files)) {
    cacheDir.remove (file);
  }
  qDebug () << "[PdfFileCache] Cleared all cached PDF files";
}

void
PdfFileCache::setExpirationDays (int days) {
  QMutexLocker locker (&mutex_);
  expirationDays_= days;
}
