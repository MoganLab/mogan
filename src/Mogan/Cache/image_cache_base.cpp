
/******************************************************************************
 * MODULE     : image_cache_base.cpp
 * DESCRIPTION: Base utilities for image/thumbnail caching
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "image_cache_base.hpp"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "string.hpp"

string get_env (string var);

QString
ImageCacheUtils::urlToFilename (const QString& url) {
  // Use MD5 hash for consistent, safe filename
  QByteArray hash=
      QCryptographicHash::hash (url.toUtf8 (), QCryptographicHash::Md5);
  QString filename= hash.toHex ();
  return filename;
}

QString
ImageCacheUtils::makeKey (const QString& url, const QList<QString>& params) {
  QString key= url;
  for (const QString& param : params) {
    key+= "|" + param;
  }
  return key;
}

QString
ImageCacheUtils::cacheSubdir (const QString& subdirName) {
  // Use TEXMACS_HOME_PATH for consistency
  QString baseDir= getEnvQString ("TEXMACS_HOME_PATH");
  if (baseDir.isEmpty ()) {
    // Fallback to standard cache location
    baseDir= QStandardPaths::writableLocation (QStandardPaths::CacheLocation);
    if (baseDir.isEmpty ()) {
      baseDir= QStandardPaths::writableLocation (QStandardPaths::TempLocation);
    }
  }
  else {
    baseDir= QDir (baseDir).filePath ("system/cache");
  }

  QString subdir= QDir (baseDir).filePath (subdirName);
  QDir    dir (subdir);
  if (!dir.exists ()) {
    dir.mkpath (".");
  }

  return subdir;
}

qint64
ImageCacheUtils::pixmapCost (const QPixmap& pixmap) {
  // Calculate approximate memory usage
  // width * height * depth (usually 4 bytes for RGBA)
  return static_cast<qint64> (pixmap.width ()) * pixmap.height () *
         (pixmap.depth () / 8);
}

bool
ImageCacheUtils::isFileExpired (const QString& filePath, int maxAgeDays) {
  QFileInfo info (filePath);
  if (!info.exists ()) return true;

  QDateTime modified= info.lastModified ();
  QDateTime now     = QDateTime::currentDateTime ();
  return modified.daysTo (now) > maxAgeDays;
}

void
ImageCacheUtils::cleanupCacheDir (const QString& cacheDir, int maxAgeDays,
                                  qint64 maxSizeBytes) {
  QDir dir (cacheDir);
  if (!dir.exists ()) return;

  QFileInfoList files= dir.entryInfoList (QDir::Files);

  // First pass: remove expired files
  for (const QFileInfo& info : files) {
    if (isFileExpired (info.filePath (), maxAgeDays)) {
      QFile::remove (info.filePath ());
    }
  }

  // Second pass: enforce size limit if specified
  if (maxSizeBytes > 0) {
    // Re-scan remaining files
    files= dir.entryInfoList (QDir::Files, QDir::Time | QDir::Reversed);

    qint64 totalSize= 0;
    for (const QFileInfo& info : files) {
      totalSize+= info.size ();
    }

    // Remove oldest files until under limit
    while (totalSize > maxSizeBytes && !files.isEmpty ()) {
      QFileInfo oldest= files.takeFirst ();
      totalSize-= oldest.size ();
      QFile::remove (oldest.filePath ());
    }
  }
}

double
ImageCacheUtils::hitRate (qint64 hits, qint64 misses) {
  qint64 total= hits + misses;
  if (total == 0) return 0.0;
  return static_cast<double> (hits) / total;
}

QString
ImageCacheUtils::getEnvQString (const char* varName) {
  // Use c_string RAII wrapper to automatically free memory
  c_string cs (get_env (varName));
  return QString::fromUtf8 (static_cast<char*> (cs));
}
