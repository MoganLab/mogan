
/******************************************************************************
 * MODULE     : template_api.hpp
 * DESCRIPTION: liiistem.cn API client for template metadata and downloads
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TEMPLATE_API_HPP
#define TEMPLATE_API_HPP

#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>

// Common type definitions
#include "template_types.hpp"

// Forward declaration
class QJsonObject;

/**
 * @brief liiistem.cn API client
 *
 * Responsibilities:
 * - Fetch template categories and templates from liiistem.cn API via POST
 * - Download template files (.tm)
 * - Handle network errors and retries
 * - Support offline fallback
 */
class TemplateAPI : public QObject {
  Q_OBJECT

public:
  explicit TemplateAPI (QObject* parent= nullptr);
  ~TemplateAPI ();

  // Configuration
  void    setApiBaseUrl (const QString& baseUrl);
  QString apiBaseUrl () const { return apiBaseUrl_; }

  // API operations (POST based)
  void fetchCategories ();
  void fetchTemplates (const QString& categoryId= QString ());
  void fetchRecommendTemplates ();

  void downloadTemplate (const QString& templateId, const QString& downloadUrl,
                         const QString& targetPath);

  void incrementDownloadCount (const QString& templateId);

  /**
   * @brief 终止下载并发射 downloadFailed（用户点击取消）。
   *
   * 用户在进度对话框中点击 Cancel 时调用。
   * 发射 downloadFailed(templateId, "Download cancelled")，
   * 使 TemplateManager::downloadTemplateSync() 中的 QEventLoop 立即退出。
   */
  void cancelDownload (const QString& templateId);

  // Network state
  bool isOnline () const;
  void setOfflineMode (bool offline);

signals:
  // Categories fetch results
  void categoriesLoaded (const QList<TemplateCategory>& categories);
  void categoriesLoadFailed (const QString& error);

  // Templates fetch results
  void templatesLoaded (const QHash<QString, TemplateMetadataPtr>& metadata);
  void templatesLoadFailed (const QString& error);

  void recommendTemplatesLoaded (
      const QHash<QString, TemplateMetadataPtr>& metadata);
  void recommendTemplatesLoadFailed (const QString& error);

  // Download progress
  void downloadProgress (const QString& templateId, qint64 bytesReceived,
                         qint64 bytesTotal);
  void downloadCompleted (const QString& templateId, const QString& localPath);
  void downloadFailed (const QString& templateId, const QString& error);

  // Network state
  void networkStateChanged (bool isOnline);

public:
  // Response parsing (exposed for unit testing)
  QList<TemplateCategory> parseCategoriesResponse (const QJsonValue& data);
  QHash<QString, TemplateMetadataPtr>
  parseTemplatesResponse (const QJsonValue& data);

private slots:
  void onCategoriesReplyFinished ();
  void onTemplatesReplyFinished ();
  void onRecommendTemplatesReplyFinished ();
  void onDownloadProgress (qint64 bytesReceived, qint64 bytesTotal);
  void onDownloadFinished ();

private:
  // API URL construction
  QString categoriesUrl () const;
  QString templatesUrl () const;
  QString recommendTemplatesUrl () const;
  QString downloadTemplatesUrl () const;

  // Helper to parse individual template objects
  void parseTemplateObject (const QJsonObject&                   tmplObj,
                            QHash<QString, TemplateMetadataPtr>& metadata);

  // Request management
  void setupRequestHeaders (QNetworkRequest& request);

  /**
   * @brief 静默终止下载，不发射任何信号（内部使用）。
   *
   * 在 downloadTemplate() 为同一 templateId 启动新请求前调用。
   * 安静地终止旧的 QNetworkReply 并清理 downloadReplies_，
   * 但不发射 downloadFailed，避免打断新下载流程。
   */
  void abortDownload (const QString& templateId);

private:
  bool abortAndRemoveReply (const QString& templateId);

private:
  // API configuration
  QString apiBaseUrl_;

  // Network
  QNetworkAccessManager* networkManager_;
  bool                   offlineMode_;

  // Active requests
  QHash<QString, QPointer<QNetworkReply>> downloadReplies_;
  QPointer<QNetworkReply>                 categoriesReply_;
  QPointer<QNetworkReply>                 templatesReply_;
  QPointer<QNetworkReply>                 recommendTemplatesReply_;

  // Default API endpoint
  static constexpr const char* DEFAULT_API_BASE_URL= "https://liiistem.cn";
};

#endif // TEMPLATE_API_HPP
