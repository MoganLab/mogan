
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

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QSharedPointer>

// Forward declarations
class TemplateCache;
class TemplateAPI;

/**
 * @brief Template metadata structure
 */
struct TemplateMetadata {
    QString   id;                // Unique template identifier
    QString   name;              // Display name
    QString   description;       // Template description
    QString   category;          // Category ID
    QString   author;            // Author name
    QString   version;           // Template version
    QString   thumbnailUrl;      // Thumbnail image URL
    QString   fileUrl;           // Template file (.tmu) download URL
    qint64    fileSize;          // File size in bytes
    QDateTime updatedAt;         // Last update time
    QString   localPath;         // Local cached file path (if downloaded)
    bool      isLocal;           // Whether template is locally available

    TemplateMetadata ()
        : fileSize (0), isLocal (false) {}
};

using TemplateMetadataPtr= QSharedPointer<TemplateMetadata>;

/**
 * @brief Template category structure
 */
struct TemplateCategory {
    QString id;          // Unique category identifier
    QString name;        // Display name (localized)
    QString icon;        // Icon name or path
    int     order;       // Display order

    TemplateCategory () : order (0) {}
};

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

    // Category operations
    QList<TemplateCategory> categories () const;
    QString                 categoryName (const QString& categoryId) const;

    // Template queries
    QList<TemplateMetadataPtr> templates () const;
    QList<TemplateMetadataPtr> templatesByCategory (const QString& categoryId) const;
    TemplateMetadataPtr        templateById (const QString& templateId) const;

    // Template availability
    bool isTemplateAvailableLocally (const QString& templateId) const;
    QString localTemplatePath (const QString& templateId) const;

    // Operations
    void refreshTemplates ();     // Force refresh from remote
    void checkForUpdates ();      // Check for updates without full refresh

    // Template download
    void downloadTemplate (const QString& templateId);
    void cancelDownload (const QString& templateId);

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

    // Template download progress
    void downloadProgress (const QString& templateId, qint64 bytesReceived,
                           qint64 bytesTotal);
    void downloadCompleted (const QString& templateId, const QString& localPath);
    void downloadFailed (const QString& templateId, const QString& error);

    // Update notifications
    void updateAvailable (int newTemplatesCount, int updatedTemplatesCount);

private slots:
    void onRemoteMetadataLoaded (const QHash<QString, TemplateMetadataPtr>& metadata);
    void onRemoteMetadataFailed (const QString& error);
    void onTemplateDownloaded (const QString& templateId, const QString& localPath);
    void onTemplateDownloadFailed (const QString& templateId, const QString& error);

private:
    // Load local templates
    void loadLocalTemplates ();
    void loadLocalCategories ();

    // Merge remote metadata with local cache
    void mergeMetadata (const QHash<QString, TemplateMetadataPtr>& remoteMetadata);

    // Utility functions
    QString localTemplatesDir () const;
    QString templateFilePath (const QString& templateId) const;

private:
    bool initialized_;

    // Data storage
    QList<TemplateCategory>                       categories_;
    QHash<QString, TemplateCategory>              categoryMap_;
    QHash<QString, TemplateMetadataPtr>           templates_;
    QHash<QString, TemplateMetadataPtr>           templatesByCategory_;

    // Components
    TemplateCache* cache_;
    TemplateAPI*   api_;

    // State
    bool isOnline_;
    bool isRefreshing_;
};

#endif // TEMPLATE_MANAGER_HPP
