
/******************************************************************************
 * MODULE     : template_manager.hpp
 * DESCRIPTION: Template manager for Mogan Template Center
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TEMPLATE_MANAGER_HPP
#define TEMPLATE_MANAGER_HPP

#include <QHash>
#include <QList>
#include <QObject>

// Common type definitions
#include "template_types.hpp"

// Forward declaration for Scheme integration
class string;

// Forward declarations
class TemplateCache;
class TemplateAPI;

/**
 * @brief Template manager - main entry point for template operations
 *
 * Responsibilities:
 * - Load and manage template metadata from local and remote sources
 * - Coordinate cache updates and API requests
 * - Provide template list filtered by category
 * - Handle template download and local storage
 */
class TemplateManager : public QObject {
  Q_OBJECT

public:
  explicit TemplateManager (QObject* parent= nullptr);
  ~TemplateManager ();

  // Singleton instance
  static TemplateManager* instance ();

  // Initialization
  void initialize ();
  bool isInitialized () const { return initialized_; }

  // 分类操作
  QList<TemplateCategory> categories () const;
  QString                 categoryName (const QString& categoryId) const;

  // 模板查询
  QList<TemplateMetadataPtr> templates () const;
  QList<TemplateMetadataPtr>
                      templatesByCategory (const QString& categoryId) const;
  TemplateMetadataPtr templateById (const QString& templateId) const;
  QList<TemplateMetadataPtr> recommendTemplates () const;

  // 本地模板可用性（纯查询，不验证 MD5）
  bool    isTemplateAvailableLocally (const QString& templateId) const;
  QString localTemplatePath (const QString& templateId) const;

  // 验证本地模板完整性（MD5 校验），损坏时自动清理缓存
  bool verifyLocalTemplate (const QString& templateId);

  // 刷新操作
  void refreshCategories (); // 强制刷新分类列表
  void refreshTemplates ();  // 强制刷新全部模板
  void
  refreshTemplatesByCategory (const QString& categoryId); // 按分类增量刷新模板
  void refreshRecommendTemplates ();

  // Template download
  void downloadTemplate (const QString& templateId);
  void cancelDownload (const QString& templateId);
  /**
   * @brief Synchronously download a template with optional timeout.
   *
   * Blocks until the download completes, fails, or the timeout expires.
   * The caller should run this from the UI thread so that progress
   * dialogs and event processing remain responsive.
   *
   * @param templateId   The template to download.
   * @param timeoutMs    Maximum time to wait in milliseconds (default 30s).
   * @param errorMessage If non-null, receives a human-readable error on
   * failure.
   * @return The local file path on success, or an empty string on failure.
   */
  QString downloadTemplateSync (const QString& templateId, int timeoutMs= 30000,
                                QString* errorMessage= nullptr);

  // Signals for UI updates
  void onNetworkStateChanged (bool isOnline);

signals:
  // Initialization
  void initialized (bool success);

  // Data updates
  void templatesLoaded ();
  void templatesLoadFailed (const QString& error);

  // Category updates
  void categoriesLoaded ();

  void recommendTemplatesLoaded ();
  void recommendTemplatesLoadFailed (const QString& error);

  // Template download progress
  void downloadProgress (const QString& templateId, qint64 bytesReceived,
                         qint64 bytesTotal);
  void downloadCompleted (const QString& templateId, const QString& localPath);
  void downloadFailed (const QString& templateId, const QString& error);

  // Update notifications
  void updateAvailable (int newTemplatesCount, int updatedTemplatesCount);

private slots:
  void onRemoteCategoriesLoaded (const QList<TemplateCategory>& categories);
  void onRemoteCategoriesFailed (const QString& error);
  void
  onRemoteTemplatesLoaded (const QHash<QString, TemplateMetadataPtr>& metadata);
  void onRemoteTemplatesFailed (const QString& error);
  void onRemoteRecommendTemplatesLoaded (
      const QHash<QString, TemplateMetadataPtr>& metadata);
  void onRemoteRecommendTemplatesFailed (const QString& error);
  void onTemplateDownloaded (const QString& templateId,
                             const QString& localPath);
  void onTemplateDownloadFailed (const QString& templateId,
                                 const QString& error);

private:
  // Load local templates and categories
  void                    loadLocalTemplates ();
  void                    loadLocalCategories ();
  void                    loadCachedCategories ();
  QList<TemplateCategory> loadCategoriesFromScheme (const string& filePath);
  QList<TemplateCategory> loadLocalCategoriesFromScheme ();

  // Merge remote metadata with local cache
  void mergeMetadata (const QHash<QString, TemplateMetadataPtr>& remoteMetadata,
                      bool incremental= false);

  // Utility functions
  QString localTemplatesDir () const;
  QString templateFilePath (const QString& templateId) const;

private:
  bool initialized_;

  // Data storage
  QList<TemplateCategory>             categories_;
  QHash<QString, TemplateCategory>    categoryMap_;
  QHash<QString, TemplateMetadataPtr> templates_;
  QList<QString>                      recommendTemplateIds_;

  // Components
  TemplateCache* cache_;
  TemplateAPI*   api_;

  // State
  bool          isOnline_;
  bool          isRefreshingCategories_;
  bool          isRefreshingTemplates_;
  bool          categoriesFetched_;
  bool          isRefreshingRecommendTemplates_;
  bool          recommendTemplatesFetched_;
  QSet<QString> fetchedCategories_;
  QString       pendingIncrementalCategoryId_;
};

#endif // TEMPLATE_MANAGER_HPP
