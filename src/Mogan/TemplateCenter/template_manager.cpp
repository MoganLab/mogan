
/******************************************************************************
 * MODULE     : template_manager.cpp
 * DESCRIPTION: Template manager implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "template_manager.hpp"
#include "template_api.hpp"
#include "template_cache.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

// Singleton instance
static TemplateManager* g_instance= nullptr;

TemplateManager::TemplateManager (QObject* parent)
    : QObject (parent), initialized_ (false), cache_ (nullptr), api_ (nullptr),
      isOnline_ (true), isRefreshing_ (false) {
  cache_= new TemplateCache (this);
  api_  = new TemplateAPI (this);

  // Connect API signals
  connect (api_, &TemplateAPI::metadataLoaded, this,
           &TemplateManager::onRemoteMetadataLoaded);
  connect (api_, &TemplateAPI::metadataLoadFailed, this,
           &TemplateManager::onRemoteMetadataFailed);
  connect (api_, &TemplateAPI::downloadCompleted, this,
           &TemplateManager::onTemplateDownloaded);
  connect (api_, &TemplateAPI::downloadFailed, this,
           &TemplateManager::onTemplateDownloadFailed);
  connect (api_, &TemplateAPI::downloadProgress, this,
           &TemplateManager::downloadProgress);
  connect (api_, &TemplateAPI::networkStateChanged, this,
           &TemplateManager::onNetworkStateChanged);
}

TemplateManager::~TemplateManager () { g_instance= nullptr; }

TemplateManager*
TemplateManager::instance () {
  if (!g_instance) {
    g_instance= new TemplateManager ();
  }
  return g_instance;
}

void
TemplateManager::initialize () {
  if (initialized_) {
    emit initialized (true);
    return;
  }

  // Initialize cache
  if (!cache_->initialize ()) {
    qWarning () << "Failed to initialize template cache";
    // Continue without cache - will work in degraded mode
  }

  // Load local templates first (offline fallback)
  loadLocalTemplates ();
  loadLocalCategories ();

  // Load cached metadata if available
  QHash<QString, TemplateMetadataPtr> cachedMetadata=
      cache_->loadMetadataCache ();
  if (!cachedMetadata.isEmpty ()) {
    mergeMetadata (cachedMetadata);
    emit templatesLoaded ();
  }

  // Try to fetch remote metadata
  checkForUpdates ();

  initialized_= true;
  emit initialized (true);
}

void
TemplateManager::loadLocalTemplates () {
  // Load templates from TeXmacs/templates/metadata.scm
  // TODO: Parse Scheme file and populate templates_
  // For now, we'll rely on the cache and remote fetch
}

void
TemplateManager::loadLocalCategories () {
  // Load categories from TeXmacs/templates/categories.scm
  // This provides the category structure even when offline

  // Default categories (fallback)
  QList<TemplateCategory> defaultCategories;

  TemplateCategory academic;
  academic.id   = "academic";
  academic.name = tr ("Academic");
  academic.icon = "template-academic";
  academic.order= 1;
  defaultCategories.append (academic);

  TemplateCategory report;
  report.id   = "report";
  report.name = tr ("Report");
  report.icon = "template-report";
  report.order= 2;
  defaultCategories.append (report);

  TemplateCategory slides;
  slides.id   = "slides";
  slides.name = tr ("Slides");
  slides.icon = "template-slides";
  slides.order= 3;
  defaultCategories.append (slides);

  TemplateCategory letter;
  letter.id   = "letter";
  letter.name = tr ("Letter");
  letter.icon = "template-letter";
  letter.order= 4;
  defaultCategories.append (letter);

  TemplateCategory book;
  book.id   = "book";
  book.name = tr ("Book");
  book.icon = "template-book";
  book.order= 5;
  defaultCategories.append (book);

  TemplateCategory exam;
  exam.id   = "exam";
  exam.name = tr ("Exam");
  exam.icon = "template-exam";
  exam.order= 6;
  defaultCategories.append (exam);

  // Sort by order
  std::sort (defaultCategories.begin (), defaultCategories.end (),
             [] (const TemplateCategory& a, const TemplateCategory& b) {
               return a.order < b.order;
             });

  categories_= defaultCategories;
  for (const auto& cat : categories_) {
    categoryMap_[cat.id]= cat;
  }

  emit categoriesLoaded ();
}

QList<TemplateCategory>
TemplateManager::categories () const {
  return categories_;
}

QString
TemplateManager::categoryName (const QString& categoryId) const {
  auto it= categoryMap_.find (categoryId);
  if (it != categoryMap_.end ()) {
    return it->name;
  }
  return categoryId;
}

QList<TemplateMetadataPtr>
TemplateManager::templates () const {
  return templates_.values ();
}

QList<TemplateMetadataPtr>
TemplateManager::templatesByCategory (const QString& categoryId) const {
  QList<TemplateMetadataPtr> result;
  for (const auto& tmpl : templates_) {
    if (tmpl->category == categoryId) {
      result.append (tmpl);
    }
  }
  return result;
}

TemplateMetadataPtr
TemplateManager::templateById (const QString& templateId) const {
  return templates_.value (templateId);
}

bool
TemplateManager::isTemplateAvailableLocally (const QString& templateId) const {
  auto tmpl= templates_.value (templateId);
  if (tmpl) {
    return tmpl->isLocal || cache_->isTemplateCached (templateId);
  }
  return false;
}

QString
TemplateManager::localTemplatePath (const QString& templateId) const {
  // Check if already loaded template has local path
  auto tmpl= templates_.value (templateId);
  if (tmpl && !tmpl->localPath.isEmpty () && QFile::exists (tmpl->localPath)) {
    return tmpl->localPath;
  }

  // Check cache
  return cache_->cachedTemplatePath (templateId);
}

void
TemplateManager::refreshTemplates () {
  if (isRefreshing_) {
    return;
  }

  isRefreshing_= true;
  api_->fetchMetadata ();
}

void
TemplateManager::checkForUpdates () {
  // Check if we need to refresh based on last update time
  QDateTime lastUpdate= cache_->lastMetadataUpdate ();
  if (!lastUpdate.isValid () ||
      lastUpdate.secsTo (QDateTime::currentDateTime ()) > 3600) {
    // No recent update, fetch fresh metadata
    refreshTemplates ();
  }
}

void
TemplateManager::downloadTemplate (const QString& templateId) {
  auto tmpl= templates_.value (templateId);
  if (!tmpl) {
    emit downloadFailed (templateId, tr ("Template not found"));
    return;
  }

  if (tmpl->fileUrl.isEmpty ()) {
    emit downloadFailed (templateId, tr ("No download URL available"));
    return;
  }

  QString targetPath= templateFilePath (templateId);
  api_->downloadTemplate (templateId, tmpl->fileUrl, targetPath);
}

void
TemplateManager::cancelDownload (const QString& templateId) {
  api_->cancelDownload (templateId);
}

void
TemplateManager::onNetworkStateChanged (bool isOnline) {
  isOnline_= isOnline;
  if (isOnline && !initialized_) {
    // Try to fetch metadata when coming back online
    checkForUpdates ();
  }
}

void
TemplateManager::onRemoteMetadataLoaded (
    const QHash<QString, TemplateMetadataPtr>& remoteMetadata) {
  isRefreshing_= false;

  int newCount    = 0;
  int updatedCount= 0;

  // Count new and updated templates
  for (auto it= remoteMetadata.constBegin (); it != remoteMetadata.constEnd ();
       ++it) {
    const QString&            id          = it.key ();
    const TemplateMetadataPtr remoteTmpl  = it.value ();
    const TemplateMetadataPtr existingTmpl= templates_.value (id);

    if (!existingTmpl) {
      newCount++;
    }
    else if (remoteTmpl->updatedAt > existingTmpl->updatedAt) {
      updatedCount++;
    }
  }

  // Merge with existing data
  mergeMetadata (remoteMetadata);

  // Save to cache
  cache_->saveMetadataCache (templates_);
  cache_->setLastMetadataUpdate (QDateTime::currentDateTime ());

  // Notify UI
  emit templatesLoaded ();

  if (newCount > 0 || updatedCount > 0) {
    emit updateAvailable (newCount, updatedCount);
  }
}

void
TemplateManager::onRemoteMetadataFailed (const QString& error) {
  isRefreshing_= false;
  qWarning () << "Failed to load remote metadata:" << error;

  // We still have local/cache data, so emit success for cached data
  emit templatesLoaded ();
  emit templatesLoadFailed (error);
}

void
TemplateManager::onTemplateDownloaded (const QString& templateId,
                                       const QString& localPath) {
  // Update template metadata
  auto tmpl= templates_.value (templateId);
  if (tmpl) {
    tmpl->localPath= localPath;
    tmpl->isLocal  = true;
  }

  // Register in cache
  QFileInfo fileInfo (localPath);
  cache_->registerCachedTemplate (templateId, localPath, fileInfo.size ());

  emit downloadCompleted (templateId, localPath);
}

void
TemplateManager::onTemplateDownloadFailed (const QString& templateId,
                                           const QString& error) {
  emit downloadFailed (templateId, error);
}

void
TemplateManager::mergeMetadata (
    const QHash<QString, TemplateMetadataPtr>& remoteMetadata) {
  for (auto it= remoteMetadata.constBegin (); it != remoteMetadata.constEnd ();
       ++it) {
    const QString&            id        = it.key ();
    const TemplateMetadataPtr remoteTmpl= it.value ();

    auto existingIt= templates_.find (id);
    if (existingIt == templates_.end ()) {
      // New template
      templates_.insert (id, remoteTmpl);
    }
    else {
      // Update existing template
      TemplateMetadataPtr existing= existingIt.value ();
      existing->name              = remoteTmpl->name;
      existing->description       = remoteTmpl->description;
      existing->category          = remoteTmpl->category;
      existing->author            = remoteTmpl->author;
      existing->version           = remoteTmpl->version;
      existing->thumbnailUrl      = remoteTmpl->thumbnailUrl;
      existing->fileUrl           = remoteTmpl->fileUrl;
      existing->fileSize          = remoteTmpl->fileSize;
      existing->updatedAt         = remoteTmpl->updatedAt;
      // Preserve local path if file still exists
      if (!existing->localPath.isEmpty () &&
          !QFile::exists (existing->localPath)) {
        existing->localPath.clear ();
        existing->isLocal= false;
      }
    }
  }

  // Update cache availability flag for all templates
  for (auto it= templates_.begin (); it != templates_.end (); ++it) {
    TemplateMetadataPtr tmpl= it.value ();
    if (cache_->isTemplateCached (tmpl->id)) {
      tmpl->isLocal  = true;
      tmpl->localPath= cache_->cachedTemplatePath (tmpl->id);
    }
  }
}

QString
TemplateManager::localTemplatesDir () const {
  QString dataDir=
      QStandardPaths::writableLocation (QStandardPaths::AppDataLocation);
  return QDir (dataDir).filePath ("templates");
}

QString
TemplateManager::templateFilePath (const QString& templateId) const {
  QDir dir (localTemplatesDir ());
  if (!dir.exists ()) {
    dir.mkpath (".");
  }
  return dir.filePath (templateId + ".tmu");
}
