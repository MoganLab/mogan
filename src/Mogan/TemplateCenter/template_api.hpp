
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

#include <QObject>
#include <QHash>
#include <QSharedPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Forward declaration
struct TemplateMetadata;
using TemplateMetadataPtr= QSharedPointer<TemplateMetadata>;

/**
 * @brief Gitee Releases API client
 *
 * Responsibilities:
 * - Fetch template metadata from Gitee Releases
 * - Download template files (.tmu)
 * - Handle network errors and retries
 * - Support offline fallback
 */
class TemplateAPI : public QObject {
    Q_OBJECT

public:
    explicit TemplateAPI (QObject* parent= nullptr);
    ~TemplateAPI ();

    // Configuration
    void    setRepository (const QString& owner, const QString& repo);
    QString owner () const { return owner_; }
    QString repo () const { return repo_; }

    // API operations
    void fetchMetadata ();
    void downloadTemplate (const QString& templateId, const QString& downloadUrl,
                           const QString& targetPath);
    void cancelDownload (const QString& templateId);

    // Network state
    bool isOnline () const;
    void setOfflineMode (bool offline);

signals:
    // Metadata fetch results
    void metadataLoaded (const QHash<QString, TemplateMetadataPtr>& metadata);
    void metadataLoadFailed (const QString& error);

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
    QString releasesApiUrl () const;

    // Response parsing
    QHash<QString, TemplateMetadataPtr> parseMetadataResponse (
        const QByteArray& data);

    // Request management
    void setupRequestHeaders (QNetworkRequest& request);

private:
    // Repository configuration
    QString owner_;
    QString repo_;

    // Network
    QNetworkAccessManager* networkManager_;
    bool                   offlineMode_;

    // Active requests
    QHash<QString, QNetworkReply*> downloadReplies_;
    QNetworkReply*                 metadataReply_;

    // Retry configuration
    static constexpr int    MAX_RETRY_COUNT    = 3;
    static constexpr int    RETRY_DELAY_MS     = 1000;
    static constexpr int    REQUEST_TIMEOUT_MS = 30000;

    // Default repository
    static constexpr const char* DEFAULT_OWNER = "LiiiLabs";
    static constexpr const char* DEFAULT_REPO  = "liiistem-template";
};

#endif // TEMPLATE_API_HPP
