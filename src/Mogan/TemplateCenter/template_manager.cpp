
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
#include <QRegularExpression>
#include <QStandardPaths>

// Scheme integration for loading local config
#include "s7_tm.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"

#include "image_cache_base.hpp"

// Singleton instance
static TemplateManager* g_instance= nullptr;

TemplateManager::TemplateManager (QObject* parent)
    : QObject (parent), initialized_ (false), cache_ (nullptr), api_ (nullptr),
      isOnline_ (true), isRefreshing_ (false) {
  cache_= new TemplateCache (this);
  api_  = new TemplateAPI (this);

  // Connect API signals (liiistem.cn API format)
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

  // Try to load categories from cache first, fallback to Scheme file
  loadCachedCategories ();

  // Load cached metadata if available
  QHash<QString, TemplateMetadataPtr> cachedMetadata=
      cache_->loadMetadataCache ();
  if (!cachedMetadata.isEmpty ()) {
    mergeMetadata (cachedMetadata);
    // Don't emit templatesLoaded here - wait for remote data or emit after
    // checking
  }

  // Always try to refresh remote metadata in the background.
  // Cached data is already available for fast initial rendering and offline
  // use.
  refreshTemplates ();

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
TemplateManager::loadCachedCategories () {
  // Try to load categories from cache first
  QList<TemplateCategory> cachedCategories= cache_->loadCategoriesCache ();

  if (!cachedCategories.isEmpty ()) {
    // Use cached categories
    categories_= cachedCategories;
  }
  else {
    // Fallback to Scheme file
    categories_= loadLocalCategoriesFromScheme ();
  }

  // Build category map
  categoryMap_.clear ();
  for (const auto& cat : categories_) {
    categoryMap_[cat.id]= cat;
  }

  emit categoriesLoaded ();
}

QList<TemplateCategory>
TemplateManager::loadLocalCategoriesFromScheme () {
  QList<TemplateCategory> categories;

  // Load categories from Scheme file
  url categoriesFile= url_system ("$TEXMACS_PATH/templates/categories.scm");
  if (exists (categoriesFile)) {
    categories= loadCategoriesFromScheme (as_string (categoriesFile));
  }

  return categories;
}

void
TemplateManager::loadLocalCategories () {
  categories_= loadLocalCategoriesFromScheme ();
  categoryMap_.clear ();
  for (const auto& cat : categories_) {
    categoryMap_[cat.id]= cat;
  }

  emit categoriesLoaded ();
}

QList<TemplateCategory>
TemplateManager::loadCategoriesFromScheme (const string& filePath) {
  QList<TemplateCategory> categories;

  // Check if Scheme interpreter is available
  if (!tm_s7) {
    qWarning () << "Scheme interpreter not available";
    return categories;
  }

  // Load and evaluate the Scheme file
  tmscm result= eval_scheme_file (filePath);
  if (tmscm_is_null (result)) {
    qWarning () << "Failed to load categories from Scheme file:"
                << QString::fromUtf8 (as_charp (filePath));
    return categories;
  }

  // Call (template-get-categories) to get the category list
  tmscm categoriesFunc= s7_name_to_value (tm_s7, "template-get-categories");
  if (categoriesFunc == s7_undefined (tm_s7)) {
    qWarning () << "template-get-categories function not found";
    return categories;
  }

  // Use eval_scheme with string expression to call the function
  tmscm categoriesList= eval_scheme ("(template-get-categories)");
  if (tmscm_is_null (categoriesList) || !tmscm_is_list (categoriesList)) {
    qWarning () << "Invalid categories list from Scheme";
    return categories;
  }

  // Parse the Scheme list
  tmscm current= categoriesList;
  while (!tmscm_is_null (current)) {
    tmscm catObj= tmscm_car (current);

    if (tmscm_is_list (catObj)) {
      TemplateCategory category;

      // Parse category properties from association list format:
      // ((id . "thesis") (name . "Thesis") (icon . "template-thesis") (order .
      // 1))
      tmscm catProps= catObj;
      while (!tmscm_is_null (catProps)) {
        tmscm pair= tmscm_car (catProps);
        catProps  = tmscm_cdr (catProps);

        if (tmscm_is_pair (pair)) {
          tmscm key  = tmscm_car (pair);
          tmscm value= tmscm_cdr (pair);

          if (tmscm_is_symbol (key)) {
            string keyStr= tmscm_to_symbol (key);
            if (keyStr == "id" && tmscm_is_string (value)) {
              category.id=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "name" && tmscm_is_string (value)) {
              category.name=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "description" && tmscm_is_string (value)) {
              category.description=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "icon" && tmscm_is_string (value)) {
              category.icon=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "order" && tmscm_is_int (value)) {
              category.order= tmscm_to_int (value);
            }
          }
        }
      }

      if (!category.id.isEmpty () && !category.name.isEmpty ()) {
        categories.append (category);
      }
    }

    current= tmscm_cdr (current);
  }

  // Sort by order
  std::sort (categories.begin (), categories.end (),
             [] (const TemplateCategory& a, const TemplateCategory& b) {
               return a.order < b.order;
             });

  return categories;
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
  if (targetPath.isEmpty ()) {
    emit downloadFailed (templateId, tr ("Invalid template ID"));
    return;
  }

  api_->downloadTemplate (templateId, tmpl->fileUrl, targetPath);
}

void
TemplateManager::cancelDownload (const QString& templateId) {
  api_->cancelDownload (templateId);
}

void
TemplateManager::onNetworkStateChanged (bool isOnline) {
  isOnline_= isOnline;
  if (isOnline && initialized_) {
    // Refresh immediately when connectivity is restored.
    refreshTemplates ();
  }
}

void
TemplateManager::onRemoteMetadataLoaded (
    const QHash<QString, TemplateMetadataPtr>& remoteMetadata,
    const QList<TemplateCategory>&             remoteCategories) {
  isRefreshing_= false;

  if (remoteMetadata.isEmpty () && !templates_.isEmpty ()) {
    QString error= tr ("Remote metadata is empty");
    qWarning () << "Skip metadata merge:" << error;
    emit templatesLoaded ();
    emit templatesLoadFailed (error);
    return;
  }

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
    // Save categories to cache for offline use
    cache_->saveCategoriesCache (categories_);
    emit categoriesLoaded ();
  }

  // Merge with existing data
  mergeMetadata (remoteMetadata);

  // Save to cache
  cache_->saveMetadataCache (templates_);

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
  // Use TEXMACS_HOME_PATH for consistency with other caches
  QString dataDir= ImageCacheUtils::getEnvQString ("TEXMACS_HOME_PATH");
  if (dataDir.isEmpty ()) {
    // Fallback to AppDataLocation if TEXMACS_HOME_PATH is not set
    dataDir= QStandardPaths::writableLocation (QStandardPaths::AppDataLocation);
  }
  return QDir (dataDir).filePath ("system/templates");
}

QString
TemplateManager::templateFilePath (const QString& templateId) const {
  // Security: Validate templateId to prevent directory traversal attacks
  // Only allow alphanumeric characters, hyphens, underscores, and dots
  static const QRegularExpression validIdRegex ("^[a-zA-Z0-9._-]+$");
  if (!validIdRegex.match (templateId).hasMatch ()) {
    qWarning () << "Invalid templateId (potential path traversal attempt):"
                << templateId;
    return QString ();
  }

  QDir dir (localTemplatesDir ());
  if (!dir.exists ()) {
    dir.mkpath (".");
  }
  return dir.filePath (templateId + ".tmu");
}
