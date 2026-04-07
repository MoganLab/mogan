
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
#include "template_manager.hpp"

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

  // Set default repository
  owner_= QString (DEFAULT_OWNER);
  repo_ = QString (DEFAULT_REPO);
}

TemplateAPI::~TemplateAPI () {
  // Cancel all active downloads
  for (auto* reply : downloadReplies_) {
    if (reply && reply->isRunning ()) {
      reply->abort ();
    }
  }

  if (metadataReply_ && metadataReply_->isRunning ()) {
    metadataReply_->abort ();
  }
}

void
TemplateAPI::setRepository (const QString& owner, const QString& repo) {
  owner_= owner;
  repo_ = repo;
}

void
TemplateAPI::fetchMetadata () {
  if (offlineMode_) {
    emit metadataLoadFailed (tr ("Offline mode"));
    return;
  }

  // Cancel any existing request
  if (metadataReply_) {
    metadataReply_->abort ();
    metadataReply_->deleteLater ();
    metadataReply_= nullptr;
  }

  QNetworkRequest request{metadataUrl ()};
  setupRequestHeaders (request);

  metadataReply_= networkManager_->get (request);

  connect (metadataReply_, &QNetworkReply::finished, this,
           &TemplateAPI::onMetadataReplyFinished);
  connect (metadataReply_, &QNetworkReply::errorOccurred, this,
           &TemplateAPI::onNetworkError);
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
  connect (reply, &QNetworkReply::errorOccurred, this,
           &TemplateAPI::onNetworkError);
}

void
TemplateAPI::cancelDownload (const QString& templateId) {
  auto it= downloadReplies_.find (templateId);
  if (it != downloadReplies_.end () && it.value () != nullptr) {
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

  auto metadata= parseMetadataResponse (response);
  emit metadataLoaded (metadata);
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

  file.write (reply->readAll ());
  file.close ();

  emit downloadCompleted (templateId, targetPath);
  reply->deleteLater ();
}

void
TemplateAPI::onNetworkError (QNetworkReply::NetworkError error) {
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  // Check if this is a metadata reply
  if (reply == metadataReply_) {
    emit metadataLoadFailed (
        tr ("Network error: %1").arg (reply->errorString ()));
  }
  // Otherwise it's a download reply
  else {
    QString templateId= reply->property ("templateId").toString ();
    if (!templateId.isEmpty ()) {
      emit downloadFailed (
          templateId, tr ("Network error: %1").arg (reply->errorString ()));
    }
  }
}

QString
TemplateAPI::metadataUrl () const {
  // Fetch templates.json from Gitee raw content
  return QString ("https://gitee.com/%1/%2/raw/main/templates.json")
      .arg (owner_, repo_);
}

QString
TemplateAPI::releasesApiUrl () const {
  return QString ("https://gitee.com/api/v5/repos/%1/%2/releases/latest")
      .arg (owner_, repo_);
}

QHash<QString, TemplateMetadataPtr>
TemplateAPI::parseMetadataResponse (const QByteArray& data) {
  QHash<QString, TemplateMetadataPtr> metadata;

  QJsonDocument doc= QJsonDocument::fromJson (data);
  if (doc.isNull () || !doc.isObject ()) {
    qWarning () << "Invalid JSON response";
    return metadata;
  }

  QJsonObject root= doc.object ();

  // Parse templates array
  QJsonArray templates= root.value ("templates").toArray ();
  for (const auto& tmplValue : templates) {
    QJsonObject tmplObj= tmplValue.toObject ();

    TemplateMetadataPtr tmpl= QSharedPointer<TemplateMetadata>::create ();
    tmpl->id                = tmplObj.value ("id").toString ();
    tmpl->name              = tmplObj.value ("name").toString ();
    tmpl->description       = tmplObj.value ("description").toString ();
    tmpl->category          = tmplObj.value ("category").toString ();
    tmpl->author            = tmplObj.value ("author").toString ();
    tmpl->version           = tmplObj.value ("version").toString ();
    tmpl->thumbnailUrl      = tmplObj.value ("thumbnail_url").toString ();
    tmpl->fileUrl           = tmplObj.value ("file_url").toString ();
    tmpl->updatedAt         = QDateTime::fromString (
        tmplObj.value ("updated_at").toString (), Qt::ISODate);

    if (!tmpl->id.isEmpty ()) {
      metadata.insert (tmpl->id, tmpl);
    }
  }

  return metadata;
}

void
TemplateAPI::setupRequestHeaders (QNetworkRequest& request) {
  request.setHeader (QNetworkRequest::UserAgentHeader,
                     "Mogan-TemplateCenter/1.0");
  request.setRawHeader ("Accept", "application/json");
}
