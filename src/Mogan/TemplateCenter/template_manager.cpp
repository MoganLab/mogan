
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

  // Connect API signals (liiistem.cn API format)
  connect (
      api_,
      QOverload<
          const QHash<QString, TemplateMetadataPtr>&,
          const QList<TemplateCategory>&>::of (&TemplateAPI::metadataLoaded),
      this, &TemplateManager::onRemoteMetadataLoaded);
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
    // Don't emit templatesLoaded here - wait for remote data or emit after
    // checking
  }

  // Try to fetch remote metadata
  // If remote fetch fails, we'll emit templatesLoaded from
  // onRemoteMetadataFailed
  checkForUpdates ();

  // If no remote update check needed (cache is fresh), emit loaded
  QDateTime lastUpdate= cache_->lastMetadataUpdate ();
  if (lastUpdate.isValid () &&
      lastUpdate.secsTo (QDateTime::currentDateTime ()) <= 3600) {
    emit templatesLoaded ();
  }

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
  // Load categories matching liiistem.cn API format
  // These match the categories used in the frontend repository
  QList<TemplateCategory> defaultCategories;

  TemplateCategory thesis;
  thesis.id         = "thesis";
  thesis.name       = tr ("Thesis");
  thesis.description= tr ("Academic thesis and dissertation templates");
  thesis.icon       = "📄";
  thesis.order      = 1;
  defaultCategories.append (thesis);

  TemplateCategory labReport;
  labReport.id         = "lab-report";
  labReport.name       = tr ("Lab Report");
  labReport.description= tr ("Laboratory report templates");
  labReport.icon       = "🔬";
  labReport.order      = 2;
  defaultCategories.append (labReport);

  TemplateCategory mathModeling;
  mathModeling.id         = "math-modeling";
  mathModeling.name       = tr ("Math Modeling");
  mathModeling.description= tr ("Mathematical modeling competition templates");
  mathModeling.icon       = "📐";
  mathModeling.order      = 3;
  defaultCategories.append (mathModeling);

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
    const QHash<QString, TemplateMetadataPtr>& remoteMetadata,
    const QList<TemplateCategory>&             remoteCategories) {
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

  // Update categories from remote (liiistem.cn API format)
  if (!remoteCategories.isEmpty ()) {
    categories_= remoteCategories;
    categoryMap_.clear ();
    for (const auto& cat : categories_) {
      categoryMap_[cat.id]= cat;
    }
    emit categoriesLoaded ();
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
  // Remove templates that are no longer in the remote list
  QList<QString> toRemove;
  for (auto it= templates_.constBegin (); it != templates_.constEnd (); ++it) {
    if (!remoteMetadata.contains (it.key ())) {
      toRemove.append (it.key ());
    }
  }
  for (const QString& id : toRemove) {
    templates_.remove (id);
    cache_->removeCachedTemplate (id);
  }

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
      existing->license           = remoteTmpl->license;
      existing->thumbnailUrl      = remoteTmpl->thumbnailUrl;
      existing->previewUrl        = remoteTmpl->previewUrl;
      existing->fileUrl           = remoteTmpl->fileUrl;
      existing->fileSize          = remoteTmpl->fileSize;
      existing->fileMd5           = remoteTmpl->fileMd5;
      existing->createdAt         = remoteTmpl->createdAt;
      existing->updatedAt         = remoteTmpl->updatedAt;
      existing->language          = remoteTmpl->language;
      existing->tags              = remoteTmpl->tags;
      existing->moganMinVersion   = remoteTmpl->moganMinVersion;
      existing->downloadCount     = remoteTmpl->downloadCount;
      existing->rating            = remoteTmpl->rating;
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
