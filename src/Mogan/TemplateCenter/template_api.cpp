
/******************************************************************************
 * MODULE     : template_api.cpp
 * DESCRIPTION: Gitee Releases API client implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "template_api.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTimer>

TemplateAPI::TemplateAPI (QObject* parent)
    : QObject (parent), networkManager_ (nullptr), offlineMode_ (false),
      metadataReply_ (nullptr) {
  networkManager_= new QNetworkAccessManager (this);

  // Set default API endpoint
  apiBaseUrl_= QString (DEFAULT_API_BASE_URL);
}

TemplateAPI::~TemplateAPI () {
  // Cancel all active downloads
  for (auto reply : downloadReplies_) {
    if (!reply) continue;
    disconnect (reply, nullptr, this, nullptr);
    reply->abort ();
    reply->deleteLater ();
  }
  downloadReplies_.clear ();

  if (metadataReply_) {
    disconnect (metadataReply_, nullptr, this, nullptr);
    metadataReply_->abort ();
    metadataReply_->deleteLater ();
    metadataReply_= nullptr;
  }
}

void
TemplateAPI::setApiBaseUrl (const QString& baseUrl) {
  apiBaseUrl_= baseUrl;
}

void
TemplateAPI::fetchMetadata () {
  if (offlineMode_) {
    emit metadataLoadFailed (tr ("Offline mode"));
    return;
  }

  // Cancel any existing request
  if (metadataReply_) {
    disconnect (metadataReply_, nullptr, this, nullptr);
    metadataReply_->abort ();
    metadataReply_->deleteLater ();
    metadataReply_= nullptr;
  }

  QNetworkRequest request{metadataUrl ()};
  setupRequestHeaders (request);

  metadataReply_= networkManager_->get (request);

  connect (metadataReply_, &QNetworkReply::finished, this,
           &TemplateAPI::onMetadataReplyFinished);
}

void
TemplateAPI::downloadTemplate (const QString& templateId,
                               const QString& downloadUrl,
                               const QString& targetPath) {
  if (offlineMode_) {
    emit downloadFailed (templateId, tr ("Offline mode"));
    return;
  }

  // Cancel any existing download for this template
  cancelDownload (templateId);

  QNetworkRequest request{QUrl (downloadUrl)};
  setupRequestHeaders (request);

  QNetworkReply* reply        = networkManager_->get (request);
  downloadReplies_[templateId]= reply;

  // Store target path as property
  reply->setProperty ("templateId", templateId);
  reply->setProperty ("targetPath", targetPath);

  connect (reply, &QNetworkReply::finished, this,
           &TemplateAPI::onDownloadFinished);
  connect (reply, &QNetworkReply::downloadProgress, this,
           &TemplateAPI::onDownloadProgress);
}

void
TemplateAPI::cancelDownload (const QString& templateId) {
  auto it= downloadReplies_.find (templateId);
  if (it != downloadReplies_.end () && it.value ()) {
    disconnect (it.value (), nullptr, this, nullptr);
    it.value ()->abort ();
    it.value ()->deleteLater ();
    downloadReplies_.erase (it);
  }
}

bool
TemplateAPI::isOnline () const {
  return !offlineMode_;
}

void
TemplateAPI::setOfflineMode (bool offline) {
  offlineMode_= offline;
  emit networkStateChanged (!offline);
}

void
TemplateAPI::onMetadataReplyFinished () {
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  metadataReply_= nullptr;

  if (reply->error () != QNetworkReply::NoError) {
    QString error= tr ("Network error: %1").arg (reply->errorString ());
    emit    metadataLoadFailed (error);
    reply->deleteLater ();
    return;
  }

  QByteArray response= reply->readAll ();
  reply->deleteLater ();

  QList<TemplateCategory> categories;
  bool                    isValidResponse= false;
  auto metadata= parseMetadataResponse (response, categories, &isValidResponse);
  if (!isValidResponse) {
    emit metadataLoadFailed (tr ("Invalid metadata response"));
    return;
  }
  emit metadataLoaded (metadata, categories);
}

void
TemplateAPI::onDownloadProgress (qint64 bytesReceived, qint64 bytesTotal) {
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  QString templateId= reply->property ("templateId").toString ();
  emit    downloadProgress (templateId, bytesReceived, bytesTotal);
}

void
TemplateAPI::onDownloadFinished () {
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  QString templateId= reply->property ("templateId").toString ();
  QString targetPath= reply->property ("targetPath").toString ();

  // Remove from active downloads
  downloadReplies_.remove (templateId);

  if (reply->error () != QNetworkReply::NoError) {
    emit downloadFailed (
        templateId, tr ("Download failed: %1").arg (reply->errorString ()));
    reply->deleteLater ();
    return;
  }

  // Ensure target directory exists
  QDir dir (QFileInfo (targetPath).path ());
  if (!dir.exists ()) {
    dir.mkpath (".");
  }

  // Save file
  QFile file (targetPath);
  if (!file.open (QIODevice::WriteOnly)) {
    emit downloadFailed (templateId,
                         tr ("Cannot save file: %1").arg (file.errorString ()));
    reply->deleteLater ();
    return;
  }

  QByteArray data   = reply->readAll ();
  qint64     written= file.write (data);
  file.close ();
  if (written != data.size ()) {
    emit downloadFailed (templateId, tr ("Failed to write complete file"));
    reply->deleteLater ();
    return;
  }

  emit downloadCompleted (templateId, targetPath);
  reply->deleteLater ();
}

void
TemplateAPI::onNetworkError (QNetworkReply::NetworkError error) {
  Q_UNUSED (error);
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  // Only handle metadata reply errors here
  // Download errors are handled in onDownloadFinished
  if (reply == metadataReply_) {
    metadataReply_= nullptr;
    emit metadataLoadFailed (
        tr ("Network error: %1").arg (reply->errorString ()));
    reply->deleteLater ();
  }
}

QString
TemplateAPI::metadataUrl () const {
  // Fetch templates.json from liiistem.cn API
  return QString ("%1/templates.json").arg (apiBaseUrl_);
}

QHash<QString, TemplateMetadataPtr>
TemplateAPI::parseMetadataResponse (const QByteArray&        data,
                                    QList<TemplateCategory>& outCategories,
                                    bool*                    isValidResponse) {
  QHash<QString, TemplateMetadataPtr> metadata;
  if (isValidResponse) {
    *isValidResponse= false;
  }

  QJsonDocument doc= QJsonDocument::fromJson (data);
  if (doc.isNull () || !doc.isObject ()) {
    qWarning () << "Invalid JSON response";
    return metadata;
  }

  QJsonObject root= doc.object ();

  // Check if this is the nested categories format (liiistem.cn API v2)
  bool hasSchemaField=
      (root.contains ("categories") && root.value ("categories").isArray ()) ||
      (root.contains ("templates") && root.value ("templates").isArray ());
  if (!hasSchemaField) {
    qWarning () << "Invalid metadata schema";
    return metadata;
  }

  QJsonArray categories= root.value ("categories").toArray ();
  if (!categories.isEmpty ()) {
    // Parse categories array with nested templates
    for (const auto& catValue : categories) {
      QJsonObject catObj= catValue.toObject ();

      // Parse category info
      TemplateCategory category;
      category.id         = catObj.value ("id").toString ();
      category.name       = catObj.value ("name").toString ();
      category.description= catObj.value ("description").toString ();
      category.icon       = catObj.value ("icon").toString ();
      category.order      = catObj.value ("order").toInt ();
      outCategories.append (category);

      QString categoryId= category.id;

      QJsonArray templates= catObj.value ("templates").toArray ();
      for (const auto& tmplValue : templates) {
        parseTemplateObject (tmplValue.toObject (), categoryId, metadata);
      }
    }
  }
  else {
    // Fallback: flat templates array format (legacy/Gitee style)
    QJsonArray templates= root.value ("templates").toArray ();
    for (const auto& tmplValue : templates) {
      parseTemplateObject (tmplValue.toObject (), QString (), metadata);
    }
  }

  if (isValidResponse) {
    *isValidResponse= true;
  }
  return metadata;
}

void
TemplateAPI::parseTemplateObject (
    const QJsonObject& tmplObj, const QString& defaultCategoryId,
    QHash<QString, TemplateMetadataPtr>& metadata) {
  TemplateMetadataPtr tmpl= QSharedPointer<TemplateMetadata>::create ();
  tmpl->id                = tmplObj.value ("id").toString ();
  tmpl->name              = tmplObj.value ("name").toString ();
  tmpl->description       = tmplObj.value ("description").toString ();
  // Use category field if present, otherwise use parent category
  tmpl->category    = tmplObj.value ("category").toString (defaultCategoryId);
  tmpl->author      = tmplObj.value ("author").toString ();
  tmpl->version     = tmplObj.value ("version").toString ();
  tmpl->license     = tmplObj.value ("license").toString ();
  tmpl->thumbnailUrl= tmplObj.value ("thumbnail_url").toString ();
  tmpl->previewUrl  = tmplObj.value ("preview_url").toString ();
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

  // Parse compatibility info
  QJsonObject compatObj= tmplObj.value ("compatibility").toObject ();
  tmpl->moganMinVersion= compatObj.value ("mogan_min_version").toString ();

  // Parse statistics
  QJsonObject statsObj= tmplObj.value ("statistics").toObject ();
  tmpl->downloadCount = statsObj.value ("downloads").toInt ();
  tmpl->rating        = statsObj.value ("rating").toDouble ();

  if (!tmpl->id.isEmpty ()) {
    metadata.insert (tmpl->id, tmpl);
  }
}

void
TemplateAPI::setupRequestHeaders (QNetworkRequest& request) {
  request.setHeader (QNetworkRequest::UserAgentHeader,
                     "Mogan-TemplateCenter/1.0");
  request.setRawHeader ("Accept", "application/json");
}
