
/******************************************************************************
 * MODULE     : template_api.hpp
 * DESCRIPTION: Gitee Releases API client for template metadata and downloads
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
 * - Fetch template metadata from liiistem.cn API
 * - Download template files (.tm)
 * - Handle network errors and retries
 * - Support offline fallback
 */
class TemplateAPI : public QObject {
  Q_OBJECT

public:
  explicit TemplateAPI (QObject* parent= nullptr);
  ~TemplateAPI ();

  // Configuration (liiistem.cn API - no repository config needed)
  void    setApiBaseUrl (const QString& baseUrl);
  QString apiBaseUrl () const { return apiBaseUrl_; }

  // API operations
  void fetchMetadata ();
  void downloadTemplate (const QString& templateId, const QString& downloadUrl,
                         const QString& targetPath);
  void cancelDownload (const QString& templateId);

  // Metadata ETag for conditional requests
  void    setMetadataEtag (const QString& etag);
  QString lastMetadataEtag () const { return lastMetadataEtag_; }

  // Network state
  bool isOnline () const;
  void setOfflineMode (bool offline);

signals:
  // Metadata fetch results (liiistem.cn API format)
  void metadataLoaded (const QHash<QString, TemplateMetadataPtr>& metadata,
                       const QList<TemplateCategory>&             categories);
  void metadataLoadFailed (const QString& error);
  void metadataNotModified ();

  // Download progress
  void downloadProgress (const QString& templateId, qint64 bytesReceived,
                         qint64 bytesTotal);
  void downloadCompleted (const QString& templateId, const QString& localPath);
  void downloadFailed (const QString& templateId, const QString& error);

  // Network state
  void networkStateChanged (bool isOnline);

private slots:
  void onMetadataReplyFinished ();
  void onDownloadProgress (qint64 bytesReceived, qint64 bytesTotal);
  void onDownloadFinished ();
  void onNetworkError (QNetworkReply::NetworkError error);

private:
  // API URL construction
  QString metadataUrl () const;

  // Response parsing (liiistem.cn API format with nested categories)
  QHash<QString, TemplateMetadataPtr>
  parseMetadataResponse (const QByteArray&        data,
                         QList<TemplateCategory>& outCategories,
                         bool*                    isValidResponse= nullptr);

  // Helper to parse individual template objects
  void parseTemplateObject (const QJsonObject& tmplObj,
                            const QString&     defaultCategoryId,
                            QHash<QString, TemplateMetadataPtr>& metadata);

  // Request management
  void setupRequestHeaders (QNetworkRequest& request);

private:
  // API configuration
  QString apiBaseUrl_;

  // Network
  QNetworkAccessManager* networkManager_;
  bool                   offlineMode_;

  // Active requests
  QHash<QString, QPointer<QNetworkReply>> downloadReplies_;
  QPointer<QNetworkReply>                 metadataReply_;

  // Metadata ETag for conditional requests
  QString metadataEtag_;     // ETag sent in If-None-Match
  QString lastMetadataEtag_; // ETag received in last 200 response

  // Default API endpoint
  static constexpr const char* DEFAULT_API_BASE_URL=
      "https://liiistem.cn/template-api";
};

#endif // TEMPLATE_API_HPP
