
/******************************************************************************
 * MODULE     : template_cache.cpp
 * DESCRIPTION: Template cache implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "template_cache.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QStandardPaths>
#include <QTimeZone>

#include "image_cache_base.hpp"
#include "sys_utils.hpp"

TemplateCache::TemplateCache (QObject* parent)
    : QObject (parent), initialized_ (false) {}

TemplateCache::~TemplateCache () {}

bool
TemplateCache::initialize () {
  if (initialized_) {
    return true;
  }

  // Ensure cache directories exist
  ensureCacheDirectory ();

  // Load cache index
  loadCacheIndex ();

  initialized_= true;
  return true;
}

QHash<QString, TemplateMetadataPtr>
TemplateCache::loadMetadataCache () {
  QHash<QString, TemplateMetadataPtr> metadata;

  QString cachePath= metadataCachePath ();
  if (!QFile::exists (cachePath)) {
    return metadata;
  }

  QFile file (cachePath);
  if (!file.open (QIODevice::ReadOnly)) {
    qWarning () << "Failed to open metadata cache:" << cachePath;
    return metadata;
  }

  QByteArray    data= file.readAll ();
  QJsonDocument doc = QJsonDocument::fromJson (data);
  if (doc.isNull () || !doc.isObject ()) {
    qWarning () << "Invalid metadata cache format";
    return metadata;
  }

  QJsonObject root= doc.object ();

  // Parse last update time
  QString lastUpdate= root.value ("lastUpdated").toString ();
  if (!lastUpdate.isEmpty ()) {
    lastMetadataUpdate_= QDateTime::fromString (lastUpdate, Qt::ISODate);
  }

  // Parse templates
  QJsonArray templates= root.value ("templates").toArray ();
  for (const auto& tmplValue : templates) {
    QJsonObject tmplObj= tmplValue.toObject ();

    TemplateMetadataPtr tmpl= QSharedPointer<TemplateMetadata>::create ();
    tmpl->id                = tmplObj.value ("id").toString ();
    tmpl->name              = tmplObj.value ("name").toString ();
    tmpl->description       = tmplObj.value ("description").toString ();
    tmpl->category          = tmplObj.value ("category").toString ();
    tmpl->author            = tmplObj.value ("author").toString ();
    tmpl->version           = tmplObj.value ("version").toString ();
    tmpl->license           = tmplObj.value ("license").toString ();
    tmpl->thumbnailUrl      = tmplObj.value ("thumbnail_url").toString ();
    tmpl->previewUrl        = tmplObj.value ("preview_url").toString ();
    // Support both download_url (new) and file_url (legacy)
    tmpl->fileUrl= tmplObj.value ("download_url")
                       .toString (tmplObj.value ("file_url").toString ());
    tmpl->fileSize = tmplObj.value ("file_size").toVariant ().toLongLong ();
    tmpl->fileMd5  = tmplObj.value ("file_md5").toString ();
    tmpl->createdAt= QDateTime::fromString (
        tmplObj.value ("created_at").toString (), Qt::ISODate);
    tmpl->updatedAt= QDateTime::fromString (
        tmplObj.value ("updated_at").toString (), Qt::ISODate);
    tmpl->language= tmplObj.value ("language").toString ();

    // Parse tags array
    QJsonArray  tagsArray= tmplObj.value ("tags").toArray ();
    QStringList tags;
    for (const auto& tag : tagsArray) {
      tags.append (tag.toString ());
    }
    tmpl->tags= tags;

    // Check if locally cached
    tmpl->isLocal= isTemplateCached (tmpl->id);
    if (tmpl->isLocal) {
      tmpl->localPath= cachedTemplatePath (tmpl->id);
    }

    metadata.insert (tmpl->id, tmpl);
  }

  return metadata;
}

void
TemplateCache::saveMetadataCache (
    const QHash<QString, TemplateMetadataPtr>& metadata) {
  QJsonObject root;
  root.insert ("version", "1.0");
  root.insert ("lastUpdated",
               QDateTime::currentDateTime ().toString (Qt::ISODate));

  QJsonArray templates;
  for (const auto& tmpl : metadata) {
    QJsonObject tmplObj;
    tmplObj.insert ("id", tmpl->id);
    tmplObj.insert ("name", tmpl->name);
    tmplObj.insert ("description", tmpl->description);
    tmplObj.insert ("category", tmpl->category);
    tmplObj.insert ("author", tmpl->author);
    tmplObj.insert ("version", tmpl->version);
    tmplObj.insert ("license", tmpl->license);
    tmplObj.insert ("thumbnail_url", tmpl->thumbnailUrl);
    tmplObj.insert ("preview_url", tmpl->previewUrl);
    tmplObj.insert ("file_url", tmpl->fileUrl);
    tmplObj.insert ("file_size", static_cast<qint64> (tmpl->fileSize));
    tmplObj.insert ("file_md5", tmpl->fileMd5);
    tmplObj.insert ("created_at", tmpl->createdAt.toString (Qt::ISODate));
    tmplObj.insert ("updated_at", tmpl->updatedAt.toString (Qt::ISODate));
    tmplObj.insert ("language", tmpl->language);

    // Save tags array
    QJsonArray tagsArray;
    for (const auto& tag : tmpl->tags) {
      tagsArray.append (tag);
    }
    tmplObj.insert ("tags", tagsArray);

    templates.append (tmplObj);
  }
  root.insert ("templates", templates);

  QJsonDocument doc (root);

  QString cachePath= metadataCachePath ();
  QFile   file (cachePath);
  if (!file.open (QIODevice::WriteOnly)) {
    qWarning () << "Failed to write metadata cache:" << cachePath;
    return;
  }

  file.write (doc.toJson (QJsonDocument::Compact));
}

bool
TemplateCache::isTemplateCached (const QString& templateId) const {
  auto it= cacheIndex_.find (templateId);
  if (it == cacheIndex_.end ()) {
    return false;
  }
  return QFile::exists (it->localPath);
}

QString
TemplateCache::cachedTemplatePath (const QString& templateId) const {
  auto it= cacheIndex_.find (templateId);
  if (it != cacheIndex_.end ()) {
    const QString& path= it->localPath;
    if (QFile::exists (path)) {
      return path;
    }
  }
  return QString ();
}

void
TemplateCache::registerCachedTemplate (const QString& templateId,
                                       const QString& localPath,
                                       qint64         fileSize) {
  CacheEntry entry;
  entry.templateId= templateId;
  entry.localPath = localPath;
  entry.fileSize  = fileSize;
  entry.cachedAt  = QDateTime::currentDateTime ();
  entry.expiresAt = entry.cachedAt.addDays (CACHE_EXPIRY_DAYS);

  cacheIndex_[templateId]= entry;
  saveCacheIndex ();
}

void
TemplateCache::removeCachedTemplate (const QString& templateId) {
  auto it= cacheIndex_.find (templateId);
  if (it != cacheIndex_.end ()) {
    // Remove file
    QFile::remove (it->localPath);

    cacheIndex_.erase (it);
    saveCacheIndex ();

    emit cacheEntryRemoved (templateId);
  }
}

QList<CacheEntry>
TemplateCache::cachedTemplates () const {
  return cacheIndex_.values ();
}

void
TemplateCache::clearCache () {
  // Remove all cached files
  for (const auto& entry : cacheIndex_) {
    QFile::remove (entry.localPath);
  }

  cacheIndex_.clear ();
  saveCacheIndex ();

  // Clear metadata cache
  QString metadataPath= metadataCachePath ();
  QFile::remove (metadataPath);

  emit cacheCleared ();
}

void
TemplateCache::cleanupExpiredCache () {
  QDateTime now= QDateTime::currentDateTime ();

  QList<QString> toRemove;
  for (auto it= cacheIndex_.begin (); it != cacheIndex_.end (); ++it) {
    if (it->expiresAt < now) {
      toRemove.append (it.key ());
    }
  }

  for (const QString& templateId : toRemove) {
    removeCachedTemplate (templateId);
  }
}

qint64
TemplateCache::cacheSize () const {
  qint64 total= 0;
  for (const auto& entry : cacheIndex_) {
    total+= entry.fileSize;
  }
  return total;
}

QDateTime
TemplateCache::lastMetadataUpdate () const {
  return lastMetadataUpdate_;
}

void
TemplateCache::setLastMetadataUpdate (const QDateTime& time) {
  lastMetadataUpdate_= time;
}

QString
TemplateCache::cacheDirectory () const {
  // Use TEXMACS_HOME_PATH for consistency with other caches
  QString dataDir= ImageCacheUtils::getEnvQString ("TEXMACS_HOME_PATH");
  if (dataDir.isEmpty ()) {
    // Fallback to AppDataLocation if TEXMACS_HOME_PATH is not set
    dataDir= QStandardPaths::writableLocation (QStandardPaths::AppDataLocation);
  }
  return QDir (dataDir).filePath ("system/template_cache");
}

QString
TemplateCache::metadataCachePath () const {
  return QDir (cacheDirectory ()).filePath ("metadata.json");
}

QString
TemplateCache::categoriesCachePath () const {
  return QDir (cacheDirectory ()).filePath ("categories.json");
}

QList<TemplateCategory>
TemplateCache::loadCategoriesCache () {
  QList<TemplateCategory> categories;

  QString cachePath= categoriesCachePath ();
  if (!QFile::exists (cachePath)) {
    return categories;
  }

  // Use lock file to prevent concurrent read/write conflicts
  QString   lockPath= cachePath + ".lock";
  QLockFile lockFile (lockPath);
  if (!lockFile.tryLock (5000)) { // Wait up to 5 seconds
    qWarning () << "Could not acquire lock for categories cache read:"
                << lockPath;
    return categories;
  }

  QFile file (cachePath);
  if (!file.open (QIODevice::ReadOnly)) {
    qWarning () << "Failed to open categories cache:" << cachePath
                << "Error:" << file.errorString ();
    return categories;
  }

  QByteArray      data= file.readAll ();
  QJsonParseError parseError;
  QJsonDocument   doc= QJsonDocument::fromJson (data, &parseError);
  if (doc.isNull () || !doc.isObject ()) {
    qWarning () << "Invalid categories cache format:"
                << parseError.errorString () << "at offset"
                << parseError.offset;
    // Remove corrupted cache file to trigger regeneration
    file.close ();
    QFile::remove (cachePath);
    return categories;
  }

  QJsonObject root= doc.object ();

  // Check cache expiration (24 hours) - using UTC for timezone safety
  QString   lastUpdatedStr= root.value ("lastUpdated").toString ();
  QDateTime lastUpdated   = QDateTime::fromString (lastUpdatedStr, Qt::ISODate);
  if (lastUpdated.isValid ()) {
    // Convert to UTC for consistent comparison across timezones
    lastUpdated.setTimeZone (QTimeZone::UTC);
    qint64 hoursSinceUpdate=
        lastUpdated.secsTo (QDateTime::currentDateTimeUtc ()) / 3600;
    if (hoursSinceUpdate >= CATEGORY_CACHE_EXPIRY_HOURS) {
      qDebug () << "Categories cache expired (" << hoursSinceUpdate
                << "hours old), triggering refresh";
      return categories; // Return empty to trigger refresh
    }
  }

  QJsonArray categoriesArray= root.value ("categories").toArray ();

  for (const auto& catValue : categoriesArray) {
    QJsonObject catObj= catValue.toObject ();

    TemplateCategory category;
    category.id         = catObj.value ("id").toString ();
    category.name       = catObj.value ("name").toString ();
    category.description= catObj.value ("description").toString ();
    category.icon       = catObj.value ("icon").toString ();
    category.order      = catObj.value ("order").toInt ();

    if (!category.id.isEmpty () && !category.name.isEmpty ()) {
      categories.append (category);
    }
    else {
      qWarning () << "Skipping invalid category: missing id or name. ID:"
                  << category.id << "Name:" << category.name;
    }
  }

  // Sort by order
  std::sort (categories.begin (), categories.end (),
             [] (const TemplateCategory& a, const TemplateCategory& b) {
               return a.order < b.order;
             });

  qDebug () << "Loaded" << categories.size () << "categories from cache";
  return categories;
}

void
TemplateCache::saveCategoriesCache (const QList<TemplateCategory>& categories) {
  QJsonObject root;
  root.insert ("version", "1.0");
  // Use UTC time for timezone safety
  root.insert ("lastUpdated",
               QDateTime::currentDateTimeUtc ().toString (Qt::ISODate));

  QJsonArray categoriesArray;
  for (const auto& cat : categories) {
    QJsonObject catObj;
    catObj.insert ("id", cat.id);
    catObj.insert ("name", cat.name);
    catObj.insert ("description", cat.description);
    catObj.insert ("icon", cat.icon);
    catObj.insert ("order", cat.order);
    categoriesArray.append (catObj);
  }
  root.insert ("categories", categoriesArray);

  QJsonDocument doc (root);

  QString cachePath= categoriesCachePath ();

  // Use lock file to prevent concurrent write conflicts
  QString   lockPath= cachePath + ".lock";
  QLockFile lockFile (lockPath);
  if (!lockFile.tryLock (5000)) { // Wait up to 5 seconds
    qWarning () << "Could not acquire lock for categories cache write:"
                << lockPath;
    return;
  }

  QFile file (cachePath);
  if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning () << "Failed to write categories cache:" << cachePath
                << "Error:" << file.errorString ();
    return;
  }

  qint64 bytesWritten= file.write (doc.toJson (QJsonDocument::Compact));
  if (bytesWritten == -1) {
    qWarning () << "Failed to write categories cache data:"
                << file.errorString ();
    file.close ();
    QFile::remove (cachePath);
  }
  else {
    qDebug () << "Saved" << categories.size () << "categories to cache";
  }
}

QString
TemplateCache::templatesCacheDir () const {
  return QDir (cacheDirectory ()).filePath ("templates");
}

QString
TemplateCache::cacheIndexPath () const {
  return QDir (cacheDirectory ()).filePath ("index.json");
}

void
TemplateCache::loadCacheIndex () {
  QString indexPath= cacheIndexPath ();
  if (!QFile::exists (indexPath)) {
    return;
  }

  QFile file (indexPath);
  if (!file.open (QIODevice::ReadOnly)) {
    return;
  }

  QByteArray    data= file.readAll ();
  QJsonDocument doc = QJsonDocument::fromJson (data);
  if (doc.isNull () || !doc.isObject ()) {
    return;
  }

  QJsonObject root   = doc.object ();
  QJsonArray  entries= root.value ("entries").toArray ();

  for (const auto& entryValue : entries) {
    QJsonObject entryObj= entryValue.toObject ();

    CacheEntry entry;
    entry.templateId= entryObj.value ("templateId").toString ();
    entry.localPath = entryObj.value ("localPath").toString ();
    entry.etag      = entryObj.value ("etag").toString ();
    entry.fileSize  = entryObj.value ("fileSize").toVariant ().toLongLong ();
    entry.cachedAt  = QDateTime::fromString (
        entryObj.value ("cachedAt").toString (), Qt::ISODate);
    entry.expiresAt= QDateTime::fromString (
        entryObj.value ("expiresAt").toString (), Qt::ISODate);

    // Only add if file still exists
    if (QFile::exists (entry.localPath)) {
      cacheIndex_[entry.templateId]= entry;
    }
  }
}

void
TemplateCache::saveCacheIndex () {
  QJsonObject root;
  root.insert ("version", "1.0");

  QJsonArray entries;
  for (const auto& entry : cacheIndex_) {
    QJsonObject entryObj;
    entryObj.insert ("templateId", entry.templateId);
    entryObj.insert ("localPath", entry.localPath);
    entryObj.insert ("etag", entry.etag);
    entryObj.insert ("fileSize", entry.fileSize);
    entryObj.insert ("cachedAt", entry.cachedAt.toString (Qt::ISODate));
    entryObj.insert ("expiresAt", entry.expiresAt.toString (Qt::ISODate));
    entries.append (entryObj);
  }
  root.insert ("entries", entries);

  QJsonDocument doc (root);

  QString indexPath= cacheIndexPath ();
  QFile   file (indexPath);
  if (!file.open (QIODevice::WriteOnly)) {
    qWarning () << "Failed to write cache index:" << indexPath;
    return;
  }

  file.write (doc.toJson (QJsonDocument::Compact));
}

void
TemplateCache::ensureCacheDirectory () const {
  QDir cacheDir (cacheDirectory ());
  if (!cacheDir.exists ()) {
    cacheDir.mkpath (".");
  }

  QDir templatesDir (templatesCacheDir ());
  if (!templatesDir.exists ()) {
    templatesDir.mkpath (".");
  }
}
