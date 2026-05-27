
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

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

// Scheme integration for loading local config
#include "s7_tm.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"

#include "image_cache_base.hpp"
#include "qt_utilities.hpp"

#if !IS_COMMUNITY
#include "telemetry.hpp"
#endif

// Singleton instance
static TemplateManager* g_instance= nullptr;

TemplateManager::TemplateManager (QObject* parent)
    : QObject (parent), initialized_ (false), cache_ (nullptr), api_ (nullptr),
      isOnline_ (true), isRefreshingCategories_ (false),
      isRefreshingTemplates_ (false), categoriesFetched_ (false),
      isRefreshingRecommendTemplates_ (false),
      recommendTemplatesFetched_ (false) {
  cache_= new TemplateCache (this);
  api_  = new TemplateAPI (this);

  // Connect API signals (liiistem.cn API format)
  connect (api_, &TemplateAPI::categoriesLoaded, this,
           &TemplateManager::onRemoteCategoriesLoaded);
  connect (api_, &TemplateAPI::categoriesLoadFailed, this,
           &TemplateManager::onRemoteCategoriesFailed);
  connect (api_, &TemplateAPI::templatesLoaded, this,
           &TemplateManager::onRemoteTemplatesLoaded);
  connect (api_, &TemplateAPI::templatesLoadFailed, this,
           &TemplateManager::onRemoteTemplatesFailed);
  connect (api_, &TemplateAPI::downloadCompleted, this,
           &TemplateManager::onTemplateDownloaded);
  connect (api_, &TemplateAPI::downloadFailed, this,
           &TemplateManager::onTemplateDownloadFailed);
  connect (api_, &TemplateAPI::downloadProgress, this,
           &TemplateManager::downloadProgress);
  connect (api_, &TemplateAPI::networkStateChanged, this,
           &TemplateManager::onNetworkStateChanged);
  connect (api_, &TemplateAPI::recommendTemplatesLoaded, this,
           &TemplateManager::onRemoteRecommendTemplatesLoaded);
  connect (api_, &TemplateAPI::recommendTemplatesLoadFailed, this,
           &TemplateManager::onRemoteRecommendTemplatesFailed);
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
    qWarning () << "[Template] Failed to initialize template cache";
    // Continue without cache - will work in degraded mode
  }

  // Load local templates first (offline fallback)
  loadLocalTemplates ();

  // Load categories from cache or Scheme file for immediate UI display
  loadCachedCategories ();

  // Load cached metadata if available
  QHash<QString, TemplateMetadataPtr> cachedMetadata=
      cache_->loadMetadataCache ();
  if (!cachedMetadata.isEmpty ()) {
    mergeMetadata (cachedMetadata);
  }

  // Always try to refresh categories in the background
  refreshCategories ();

  refreshRecommendTemplates ();

  initialized_= true;
  emit initialized (true);

  // Load cached recommend template IDs AFTER initialized_= true so that
  // UI callbacks like refreshTemplateCards() can safely query isInitialized().
  recommendTemplateIds_= cache_->loadRecommendIdsCache ();
  if (!recommendTemplateIds_.isEmpty ()) {
    // Emit signal so UI can show cached recommendations immediately;
    // do NOT set recommendTemplatesFetched_ here — we still want to
    // refresh from network when online.
    emit recommendTemplatesLoaded ();
  }
}

void
TemplateManager::loadLocalTemplates () {
  // Load templates from TeXmacs/templates/metadata.scm
  // TODO: Parse Scheme file and populate templates_
}

static QString
computeFileMd5 (const QString& filePath) {
  QFile file (filePath);
  if (!file.open (QIODevice::ReadOnly)) {
    return QString ();
  }
  QCryptographicHash hasher (QCryptographicHash::Md5);
  while (!file.atEnd ()) {
    hasher.addData (
        file.read (64 * 1024)); // 64KB chunks, avoid OOM on large files
  }
  return hasher.result ().toHex ();
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
    qWarning () << "[Template] Scheme interpreter not available";
    return categories;
  }

  // Load and evaluate the Scheme file
  tmscm result= eval_scheme_file (filePath);
  if (tmscm_is_null (result)) {
    qWarning () << "[Template] Failed to load categories from Scheme file:"
                << QString::fromUtf8 (as_charp (filePath));
    return categories;
  }

  // Call (template-get-categories) to get the category list
  tmscm categoriesFunc= s7_name_to_value (tm_s7, "template-get-categories");
  if (categoriesFunc == s7_undefined (tm_s7)) {
    qWarning () << "[Template] template-get-categories function not found";
    return categories;
  }

  // Use eval_scheme with string expression to call the function
  tmscm categoriesList= eval_scheme ("(template-get-categories)");
  if (tmscm_is_null (categoriesList) || !tmscm_is_list (categoriesList)) {
    qWarning () << "[Template] Invalid categories list from Scheme";
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
            if (keyStr == "categoryKey" && tmscm_is_string (value)) {
              category.id=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "name" && tmscm_is_string (value)) {
              category.name=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "nameEn" && tmscm_is_string (value)) {
              category.nameEn=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "description" && tmscm_is_string (value)) {
              category.description=
                  QString::fromUtf8 (as_charp (tmscm_to_string (value)));
            }
            else if (keyStr == "order" && tmscm_is_int (value)) {
              category.order= tmscm_to_int (value);
            }
            else if (keyStr == "templateCount" && tmscm_is_int (value)) {
              category.templateCount= tmscm_to_int (value);
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

QList<TemplateMetadataPtr>
TemplateManager::recommendTemplates () const {
  QList<TemplateMetadataPtr> result;
  for (const QString& id : recommendTemplateIds_) {
    auto tmpl= templates_.value (id);
    if (tmpl) result.append (tmpl);
  }
  return result;
}

bool
TemplateManager::isTemplateAvailableLocally (const QString& templateId) const {
  auto tmpl= templates_.value (templateId);
  if (!tmpl) {
    return false;
  }

  if (!tmpl->localPath.isEmpty () && QFile::exists (tmpl->localPath)) {
    return true;
  }

  return cache_->isTemplateCached (templateId);
}

bool
TemplateManager::verifyLocalTemplate (const QString& templateId) {
  auto tmpl= templates_.value (templateId);
  if (!tmpl) {
    return false;
  }

  if (!tmpl->localPath.isEmpty () && QFile::exists (tmpl->localPath)) {
    if (!tmpl->fileMd5.isEmpty ()) {
      QString actualMd5= computeFileMd5 (tmpl->localPath);
      if (actualMd5 == tmpl->fileMd5) {
        qDebug () << "[Template]" << templateId << "MD5 verified:" << actualMd5;
        return true;
      }
      qWarning () << "[Template]" << templateId
                  << "MD5 mismatch (expected:" << tmpl->fileMd5
                  << "actual:" << actualMd5 << "), clearing cache";
      cache_->removeCachedTemplate (templateId);
      tmpl->localPath.clear ();
      tmpl->isLocal= false;
      return false;
    }
    return true;
  }

  return cache_->isTemplateCached (templateId);
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
TemplateManager::refreshCategories () {
  if (isRefreshingCategories_ || categoriesFetched_) {
    return;
  }
  isRefreshingCategories_= true;
  api_->fetchCategories ();
}

void
TemplateManager::refreshTemplates () {
  if (isRefreshingTemplates_) {
    return;
  }
  isRefreshingTemplates_= true;
  api_->fetchTemplates ();
}

void
TemplateManager::refreshTemplatesByCategory (const QString& categoryId) {
  if (isRefreshingTemplates_ || fetchedCategories_.contains (categoryId)) {
    return;
  }
  isRefreshingTemplates_       = true;
  pendingIncrementalCategoryId_= categoryId;
  api_->fetchTemplates (categoryId);
}

void
TemplateManager::refreshRecommendTemplates () {
  if (isRefreshingRecommendTemplates_ || recommendTemplatesFetched_) {
    return;
  }
  isRefreshingRecommendTemplates_= true;
  api_->fetchRecommendTemplates ();
}

void
TemplateManager::downloadTemplate (const QString& templateId) {
  auto tmpl= templates_.value (templateId);
  if (!tmpl) {
    emit downloadFailed (templateId, qt_translate ("Template not found"));
    return;
  }

  if (tmpl->fileUrl.isEmpty ()) {
    emit downloadFailed (templateId,
                         qt_translate ("No download URL available"));
    return;
  }

  QString targetPath= templateFilePath (templateId);
  if (targetPath.isEmpty ()) {
    emit downloadFailed (templateId, qt_translate ("Invalid template ID"));
    return;
  }

  api_->downloadTemplate (templateId, tmpl->fileUrl, targetPath);
}

void
TemplateManager::cancelDownload (const QString& templateId) {
  api_->cancelDownload (templateId);
}

QString
TemplateManager::downloadTemplateSync (const QString& templateId, int timeoutMs,
                                       QString* errorMessage) {
  if (verifyLocalTemplate (templateId)) {
    return localTemplatePath (templateId);
  }

  QEventLoop loop;
  QString    resultPath;
  QString    errorStr;
  bool       finished= false;

  QMetaObject::Connection completedConn=
      connect (this, &TemplateManager::downloadCompleted,
               [&] (const QString& id, const QString& localPath) {
                 if (id != templateId || finished) return;
                 resultPath= localPath;
                 finished  = true;
                 loop.quit ();
               });

  QMetaObject::Connection failedConn=
      connect (this, &TemplateManager::downloadFailed,
               [&] (const QString& id, const QString& error) {
                 if (id != templateId || finished) return;
                 errorStr= error;
                 finished= true;
                 loop.quit ();
               });

  QTimer timer;
  timer.setSingleShot (true);
  connect (&timer, &QTimer::timeout, [&] () {
    if (finished) return;
    errorStr= qt_translate ("Download timed out");
    finished= true;
    cancelDownload (templateId);
    loop.quit ();
  });
  timer.start (timeoutMs);

  downloadTemplate (templateId);

  loop.exec ();

  disconnect (completedConn);
  disconnect (failedConn);

  if (!finished || resultPath.isEmpty ()) {
    if (errorMessage) {
      *errorMessage=
          errorStr.isEmpty () ? qt_translate ("Download failed") : errorStr;
    }
    return QString ();
  }

  return resultPath;
}

void
TemplateManager::onNetworkStateChanged (bool isOnline) {
  isOnline_= isOnline;
  if (isOnline && initialized_) {
    refreshCategories ();
    refreshRecommendTemplates ();
  }
}

void
TemplateManager::onRemoteCategoriesLoaded (
    const QList<TemplateCategory>& remoteCategories) {
  isRefreshingCategories_= false;
  categoriesFetched_     = true;

  if (!remoteCategories.isEmpty ()) {
    categories_= remoteCategories;
    categoryMap_.clear ();
    for (const auto& cat : categories_) {
      categoryMap_[cat.id]= cat;
    }
    cache_->saveCategoriesCache (categories_);
    emit categoriesLoaded ();
  }
}

void
TemplateManager::onRemoteCategoriesFailed (const QString& error) {
  isRefreshingCategories_= false;
  qWarning () << "[Template] Failed to load remote categories:" << error;
}

void
TemplateManager::onRemoteTemplatesLoaded (
    const QHash<QString, TemplateMetadataPtr>& remoteMetadata) {
  isRefreshingTemplates_= false;

  // 空数据保护：本地已有数据时，空响应视为异常
  if (remoteMetadata.isEmpty () && !templates_.isEmpty ()) {
    QString error= qt_translate ("Remote templates list is empty");
    qWarning () << "[Template] Skip templates merge:" << error;
    pendingIncrementalCategoryId_.clear ();
    emit templatesLoaded ();
    emit templatesLoadFailed (error);
    return;
  }

  // 增量检测：通过显式标记判断，替代旧方案的数据推断
  bool incremental= !pendingIncrementalCategoryId_.isEmpty ();

  int newCount    = 0;
  int updatedCount= 0;

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

  mergeMetadata (remoteMetadata, incremental);
  cache_->saveMetadataCache (templates_);
  emit templatesLoaded ();

  if (newCount > 0 || updatedCount > 0) {
    emit updateAvailable (newCount, updatedCount);
  }

  // 记录该分类已获取，避免重复请求
  if (incremental && !remoteMetadata.isEmpty ()) {
    QString categoryId= remoteMetadata.constBegin ().value ()->category;
    if (!categoryId.isEmpty ()) {
      fetchedCategories_.insert (categoryId);
    }
  }
  pendingIncrementalCategoryId_.clear ();
}

void
TemplateManager::onRemoteTemplatesFailed (const QString& error) {
  isRefreshingTemplates_= false;
  pendingIncrementalCategoryId_.clear ();
  qWarning () << "[Template] Failed to load remote templates:" << error;
  emit templatesLoaded ();
  emit templatesLoadFailed (error);
}

void
TemplateManager::onRemoteRecommendTemplatesLoaded (
    const QHash<QString, TemplateMetadataPtr>& metadata) {
  isRefreshingRecommendTemplates_= false;
  recommendTemplatesFetched_     = true;

  if (metadata.isEmpty () && !templates_.isEmpty ()) {
    qWarning () << "[Template] Skip recommend templates merge: empty list";
    emit recommendTemplatesLoaded ();
    return;
  }

  recommendTemplateIds_.clear ();
  for (auto it= metadata.constBegin (); it != metadata.constEnd (); ++it) {
    recommendTemplateIds_.append (it.key ());
  }

  mergeMetadata (metadata, true);
  cache_->saveMetadataCache (templates_);
  cache_->saveRecommendIdsCache (recommendTemplateIds_);
  emit recommendTemplatesLoaded ();
}

void
TemplateManager::onRemoteRecommendTemplatesFailed (const QString& error) {
  isRefreshingRecommendTemplates_= false;
  qWarning () << "[Template] Failed to load recommend templates:" << error;
  emit recommendTemplatesLoadFailed (error);
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

  QFileInfo fileInfo (localPath);
  QString   fileMd5= computeFileMd5 (localPath);
  cache_->registerCachedTemplate (templateId, localPath, fileInfo.size (),
                                  fileMd5);

  api_->incrementDownloadCount (templateId);

#if !IS_COMMUNITY
  telemetry_track (
      "TEMPLATE_DOWNLOAD",
      from_qstring (
          QString ("'((\"template_id\" . \"%1\"))").arg (templateId)));
#endif

  emit downloadCompleted (templateId, localPath);
}

void
TemplateManager::onTemplateDownloadFailed (const QString& templateId,
                                           const QString& error) {
  emit downloadFailed (templateId, error);
}

void
TemplateManager::mergeMetadata (
    const QHash<QString, TemplateMetadataPtr>& remoteMetadata,
    bool                                       incremental) {
  if (!incremental) {
    // Full refresh: remove templates that are no longer in the remote list
    QList<QString> toRemove;
    for (auto it= templates_.constBegin (); it != templates_.constEnd ();
         ++it) {
      if (!remoteMetadata.contains (it.key ())) {
        toRemove.append (it.key ());
      }
    }
    for (const QString& id : toRemove) {
      templates_.remove (id);
    }
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

      // Check if remote template is newer or MD5 differs
      bool timestampUpdated= remoteTmpl->updatedAt > existing->updatedAt;
      bool md5Changed      = !remoteTmpl->fileMd5.isEmpty () &&
                       !existing->fileMd5.isEmpty () &&
                       remoteTmpl->fileMd5 != existing->fileMd5;
      bool isUpdated= timestampUpdated || md5Changed;

      if (isUpdated && existing->isLocal) {
        // Remote template has been updated, clear local cache to force
        // re-download
        qDebug () << "[Template]" << id << "updated, clearing cache";
        cache_->removeCachedTemplate (id);
        existing->localPath.clear ();
        existing->isLocal= false;
      }

      existing->name           = remoteTmpl->name;
      existing->description    = remoteTmpl->description;
      existing->category       = remoteTmpl->category;
      existing->author         = remoteTmpl->author;
      existing->version        = remoteTmpl->version;
      existing->license        = remoteTmpl->license;
      existing->thumbnailUrl   = remoteTmpl->thumbnailUrl;
      existing->previewUrl     = remoteTmpl->previewUrl;
      existing->fileUrl        = remoteTmpl->fileUrl;
      existing->fileSize       = remoteTmpl->fileSize;
      existing->fileMd5        = remoteTmpl->fileMd5;
      existing->createdAt      = remoteTmpl->createdAt;
      existing->updatedAt      = remoteTmpl->updatedAt;
      existing->language       = remoteTmpl->language;
      existing->tags           = remoteTmpl->tags;
      existing->moganMinVersion= remoteTmpl->moganMinVersion;
      existing->downloadCount  = remoteTmpl->downloadCount;
      existing->rating         = remoteTmpl->rating;
      // Preserve local path only if file still exists and not updated
      if (!existing->localPath.isEmpty () &&
          !QFile::exists (existing->localPath)) {
        existing->localPath.clear ();
        existing->isLocal= false;
      }
    }
  }

  // Update cache availability flag for all templates
  // Note: Only update if template doesn't already have localPath set
  // This prevents overwriting the clearing we just did for updated templates
  for (auto it= templates_.begin (); it != templates_.end (); ++it) {
    TemplateMetadataPtr tmpl= it.value ();
    if (!tmpl->isLocal && cache_->isTemplateCached (tmpl->id)) {
      tmpl->isLocal  = true;
      tmpl->localPath= cache_->cachedTemplatePath (tmpl->id);
      qDebug () << "[Template]" << tmpl->id << "found in cache";
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
    qWarning ()
        << "[Template] Invalid templateId (potential path traversal attempt):"
        << templateId;
    return QString ();
  }

  QDir dir (localTemplatesDir ());
  if (!dir.exists ()) {
    dir.mkpath (".");
  }
  return dir.filePath (templateId + ".tmu");
}
