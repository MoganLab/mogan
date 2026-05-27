
/******************************************************************************
 * MODULE     : template_api.cpp
 * DESCRIPTION: liiistem.cn API client implementation
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

#include "qt_utilities.hpp"

TemplateAPI::TemplateAPI (QObject* parent)
    : QObject (parent), networkManager_ (nullptr), offlineMode_ (false) {
  networkManager_= new QNetworkAccessManager (this);
  apiBaseUrl_    = QString (DEFAULT_API_BASE_URL);
}

TemplateAPI::~TemplateAPI () {
  // Abort all active downloads (reuses abortDownload for consistency)
  while (!downloadReplies_.isEmpty ()) {
    abortDownload (downloadReplies_.begin ().key ());
  }

  if (categoriesReply_) {
    disconnect (categoriesReply_, nullptr, this, nullptr);
    categoriesReply_->abort ();
    categoriesReply_->deleteLater ();
    categoriesReply_= nullptr;
  }
  if (templatesReply_) {
    disconnect (templatesReply_, nullptr, this, nullptr);
    templatesReply_->abort ();
    templatesReply_->deleteLater ();
    templatesReply_= nullptr;
  }
  if (recommendTemplatesReply_) {
    disconnect (recommendTemplatesReply_, nullptr, this, nullptr);
    recommendTemplatesReply_->abort ();
    recommendTemplatesReply_->deleteLater ();
    recommendTemplatesReply_= nullptr;
  }
}

void
TemplateAPI::setApiBaseUrl (const QString& baseUrl) {
  apiBaseUrl_= baseUrl;
}

void
TemplateAPI::fetchCategories () {
  if (offlineMode_) {
    emit categoriesLoadFailed (qt_translate ("Offline mode"));
    return;
  }

  if (categoriesReply_) {
    disconnect (categoriesReply_, nullptr, this, nullptr);
    categoriesReply_->abort ();
    categoriesReply_->deleteLater ();
    categoriesReply_= nullptr;
  }

  QNetworkRequest request{categoriesUrl ()};
  setupRequestHeaders (request);

  QJsonObject bodyObj;
  QByteArray  bodyData= QJsonDocument (bodyObj).toJson ();

  categoriesReply_= networkManager_->post (request, bodyData);
  connect (categoriesReply_, &QNetworkReply::finished, this,
           &TemplateAPI::onCategoriesReplyFinished);
}

void
TemplateAPI::fetchTemplates (const QString& categoryId) {
  if (offlineMode_) {
    emit templatesLoadFailed (qt_translate ("Offline mode"));
    return;
  }

  if (templatesReply_) {
    disconnect (templatesReply_, nullptr, this, nullptr);
    templatesReply_->abort ();
    templatesReply_->deleteLater ();
    templatesReply_= nullptr;
  }

  QNetworkRequest request{templatesUrl ()};
  setupRequestHeaders (request);

  QJsonObject bodyObj;
  if (!categoryId.isEmpty ()) {
    bodyObj.insert ("categoryKey", categoryId);
  }
  QByteArray bodyData= QJsonDocument (bodyObj).toJson ();

  templatesReply_= networkManager_->post (request, bodyData);
  connect (templatesReply_, &QNetworkReply::finished, this,
           &TemplateAPI::onTemplatesReplyFinished);
}

void
TemplateAPI::fetchRecommendTemplates () {
  if (offlineMode_) {
    emit recommendTemplatesLoadFailed (qt_translate ("Offline mode"));
    return;
  }

  if (recommendTemplatesReply_) {
    disconnect (recommendTemplatesReply_, nullptr, this, nullptr);
    recommendTemplatesReply_->abort ();
    recommendTemplatesReply_->deleteLater ();
    recommendTemplatesReply_= nullptr;
  }

  QNetworkRequest request{recommendTemplatesUrl ()};
  setupRequestHeaders (request);

  QJsonObject bodyObj;
  QByteArray  bodyData= QJsonDocument (bodyObj).toJson ();

  recommendTemplatesReply_= networkManager_->post (request, bodyData);
  connect (recommendTemplatesReply_, &QNetworkReply::finished, this,
           &TemplateAPI::onRecommendTemplatesReplyFinished);
}

void
TemplateAPI::downloadTemplate (const QString& templateId,
                               const QString& downloadUrl,
                               const QString& targetPath) {
  if (offlineMode_) {
    emit downloadFailed (templateId, qt_translate ("Offline mode"));
    return;
  }

  // Abort any existing download for this template (internal cleanup, no signal)
  abortDownload (templateId);

  QNetworkRequest request{QUrl (downloadUrl)};
  setupRequestHeaders (request);

  QNetworkReply* reply        = networkManager_->get (request);
  downloadReplies_[templateId]= reply;

  reply->setProperty ("templateId", templateId);
  reply->setProperty ("targetPath", targetPath);

  connect (reply, &QNetworkReply::finished, this,
           &TemplateAPI::onDownloadFinished);
  connect (reply, &QNetworkReply::downloadProgress, this,
           &TemplateAPI::onDownloadProgress);
}

bool
TemplateAPI::abortAndRemoveReply (const QString& templateId) {
  auto it= downloadReplies_.find (templateId);
  if (it != downloadReplies_.end () && it.value ()) {
    disconnect (it.value (), nullptr, this, nullptr);
    it.value ()->abort ();
    it.value ()->deleteLater ();
    downloadReplies_.erase (it);
    return true;
  }
  return false;
}

void
TemplateAPI::abortDownload (const QString& templateId) {
  abortAndRemoveReply (templateId);
}

void
TemplateAPI::cancelDownload (const QString& templateId) {
  if (abortAndRemoveReply (templateId)) {
    emit downloadFailed (templateId, tr ("Download cancelled"));
  }
}

void
TemplateAPI::incrementDownloadCount (const QString& templateId) {
  if (offlineMode_) {
    return;
  }

  QString         url= downloadTemplatesUrl ();
  QNetworkRequest request{QUrl (url)};
  setupRequestHeaders (request);

  QJsonObject bodyObj;
  bodyObj.insert ("templateKey", templateId);
  QByteArray bodyData= QJsonDocument (bodyObj).toJson ();

  QNetworkReply* reply= networkManager_->post (request, bodyData);
  connect (reply, &QNetworkReply::finished, [reply, templateId] () {
    if (reply->error () != QNetworkReply::NoError) {
      qWarning ()
          << "[TemplateAPI] Failed to increment download count for template:"
          << templateId << "Error:" << reply->errorString ();
    }
    else {
      qDebug () << "[TemplateAPI] Successfully incremented download count for "
                   "template:"
                << templateId;
    }
    reply->deleteLater ();
  });
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

static bool
extractApiData (const QByteArray& data, QJsonValue& outData,
                QString& outError) {
  QJsonDocument doc= QJsonDocument::fromJson (data);
  if (doc.isNull () || !doc.isObject ()) {
    outError= qt_translate ("Invalid JSON response");
    return false;
  }

  QJsonObject root= doc.object ();
  int         code= root.value ("code").toInt (-1);
  if (code != 0) {
    outError= root.value ("message").toString ();
    if (outError.isEmpty ()) {
      outError= qt_translate ("API error: code %1").arg (code);
    }
    return false;
  }

  if (!root.value ("success").toBool (false)) {
    outError= root.value ("message").toString ();
    if (outError.isEmpty ()) {
      outError= qt_translate ("API returned failure");
    }
    return false;
  }

  outData= root.value ("data");
  return true;
}

void
TemplateAPI::onCategoriesReplyFinished () {
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  categoriesReply_= nullptr;

  if (reply->error () != QNetworkReply::NoError) {
    emit categoriesLoadFailed (
        qt_translate ("Network error: %1").arg (reply->errorString ()));
    reply->deleteLater ();
    return;
  }

  QByteArray response= reply->readAll ();
  reply->deleteLater ();

  QJsonValue data;
  QString    error;
  if (!extractApiData (response, data, error)) {
    emit categoriesLoadFailed (error);
    return;
  }

  auto categories= parseCategoriesResponse (data);
  if (categories.isEmpty ()) {
    emit categoriesLoadFailed (qt_translate ("Empty categories list"));
    return;
  }
  emit categoriesLoaded (categories);
}

void
TemplateAPI::onTemplatesReplyFinished () {
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  templatesReply_= nullptr;

  if (reply->error () != QNetworkReply::NoError) {
    emit templatesLoadFailed (
        qt_translate ("Network error: %1").arg (reply->errorString ()));
    reply->deleteLater ();
    return;
  }

  QByteArray response= reply->readAll ();
  reply->deleteLater ();

  QJsonValue data;
  QString    error;
  if (!extractApiData (response, data, error)) {
    emit templatesLoadFailed (error);
    return;
  }

  auto metadata= parseTemplatesResponse (data);
  emit templatesLoaded (metadata);
}

void
TemplateAPI::onRecommendTemplatesReplyFinished () {
  QNetworkReply* reply= qobject_cast<QNetworkReply*> (sender ());
  if (!reply) return;

  recommendTemplatesReply_= nullptr;

  if (reply->error () != QNetworkReply::NoError) {
    emit recommendTemplatesLoadFailed (
        qt_translate ("Network error: %1").arg (reply->errorString ()));
    reply->deleteLater ();
    return;
  }

  QByteArray response= reply->readAll ();
  reply->deleteLater ();

  QJsonValue data;
  QString    error;
  if (!extractApiData (response, data, error)) {
    emit recommendTemplatesLoadFailed (error);
    return;
  }

  auto metadata= parseTemplatesResponse (data);
  emit recommendTemplatesLoaded (metadata);
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
        templateId,
        qt_translate ("Download failed: %1").arg (reply->errorString ()));
    reply->deleteLater ();
    return;
  }

  // Check HTTP status code (e.g., 404 may not trigger QNetworkReply error)
  int httpStatus=
      reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
  if (httpStatus >= 400) {
    emit downloadFailed (templateId,
                         tr ("Download failed: HTTP %1").arg (httpStatus));
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
    emit downloadFailed (
        templateId,
        qt_translate ("Cannot save file: %1").arg (file.errorString ()));
    reply->deleteLater ();
    return;
  }

  QByteArray data   = reply->readAll ();
  qint64     written= file.write (data);
  file.close ();
  if (written != data.size ()) {
    emit downloadFailed (templateId,
                         qt_translate ("Failed to write complete file"));
    reply->deleteLater ();
    return;
  }

  emit downloadCompleted (templateId, targetPath);
  reply->deleteLater ();
}

QString
TemplateAPI::categoriesUrl () const {
  return QString ("%1/api/v1/doc/template/categories").arg (apiBaseUrl_);
}

QString
TemplateAPI::templatesUrl () const {
  return QString ("%1/api/v1/doc/template/list").arg (apiBaseUrl_);
}

QString
TemplateAPI::recommendTemplatesUrl () const {
  return QString ("%1/api/v1/doc/template/recommend").arg (apiBaseUrl_);
}

QString
TemplateAPI::downloadTemplatesUrl () const {
  return QString ("%1/api/v1/doc/template/download").arg (apiBaseUrl_);
}

QList<TemplateCategory>
TemplateAPI::parseCategoriesResponse (const QJsonValue& data) {
  QList<TemplateCategory> categories;

  QJsonArray array;
  if (data.isArray ()) {
    array= data.toArray ();
  }
  else {
    qWarning () << "[Template] Categories data is not an array";
    return categories;
  }

  for (const auto& val : array) {
    QJsonObject      obj= val.toObject ();
    TemplateCategory cat;
    cat.id           = obj.value ("categoryKey").toString ();
    cat.name         = obj.value ("name").toString ();
    cat.nameEn       = obj.value ("nameEn").toString ();
    cat.description  = obj.value ("description").toString ();
    cat.order        = obj.value ("order").toInt ();
    cat.templateCount= obj.value ("templateCount").toInt ();
    if (!cat.id.isEmpty () && !cat.name.isEmpty ()) {
      categories.append (cat);
    }
  }

  std::sort (categories.begin (), categories.end (),
             [] (const TemplateCategory& a, const TemplateCategory& b) {
               return a.order < b.order;
             });

  return categories;
}

QHash<QString, TemplateMetadataPtr>
TemplateAPI::parseTemplatesResponse (const QJsonValue& data) {
  QHash<QString, TemplateMetadataPtr> metadata;

  QJsonArray array;
  if (data.isArray ()) {
    array= data.toArray ();
  }
  else if (data.isObject ()) {
    array= data.toObject ().value ("items").toArray ();
  }
  else {
    qWarning () << "[Template] Templates data is not an object or array";
    return metadata;
  }

  for (const auto& val : array) {
    parseTemplateObject (val.toObject (), metadata);
  }

  return metadata;
}

void
TemplateAPI::parseTemplateObject (
    const QJsonObject& tmplObj, QHash<QString, TemplateMetadataPtr>& metadata) {
  TemplateMetadataPtr tmpl= QSharedPointer<TemplateMetadata>::create ();
  tmpl->id                = tmplObj.value ("templateKey").toString ();
  tmpl->name              = tmplObj.value ("name").toString ();
  tmpl->description       = tmplObj.value ("description").toString ();
  tmpl->author            = tmplObj.value ("author").toString ();
  tmpl->version           = tmplObj.value ("version").toString ();
  tmpl->license           = tmplObj.value ("license").toString ();
  tmpl->thumbnailUrl      = tmplObj.value ("thumbnailUrl").toString ();
  tmpl->fileSize= tmplObj.value ("fileSize").toVariant ().toLongLong ();
  tmpl->fileMd5 = tmplObj.value ("fileMd5").toString ();
  tmpl->language= tmplObj.value ("language").toString ();

  // category is an object: {"categoryKey", "name"}
  QJsonObject catObj= tmplObj.value ("category").toObject ();
  tmpl->category    = catObj.value ("categoryKey").toString ();

  // url → fileUrl, pdfUrl → previewUrl
  tmpl->fileUrl   = tmplObj.value ("url").toString ();
  tmpl->previewUrl= tmplObj.value ("pdfUrl").toString ();

  // createTime 优先，回退到 created_at
  QString createTime= tmplObj.value ("createTime").toString ();
  if (createTime.isEmpty ()) {
    createTime= tmplObj.value ("created_at").toString ();
  }
  tmpl->createdAt= QDateTime::fromString (createTime, Qt::ISODate);

  // updateTime 优先，回退到 updated_at，最后回退到 createdAt
  QString updateTime= tmplObj.value ("updateTime").toString ();
  if (updateTime.isEmpty ()) {
    updateTime= tmplObj.value ("updated_at").toString ();
  }
  if (!updateTime.isEmpty ()) {
    tmpl->updatedAt= QDateTime::fromString (updateTime, Qt::ISODate);
  }
  else {
    tmpl->updatedAt= tmpl->createdAt;
  }

  // tags array
  QJsonArray  tagsArray= tmplObj.value ("tags").toArray ();
  QStringList tags;
  for (const auto& tag : tagsArray) {
    tags.append (tag.toString ());
  }
  tmpl->tags= tags;

  // compatibility
  QJsonObject compatObj= tmplObj.value ("compatibility").toObject ();
  tmpl->moganMinVersion= compatObj.value ("mogan_min_version").toString ();

  // statistics
  QJsonObject statsObj= tmplObj.value ("statistics").toObject ();
  tmpl->downloadCount = statsObj.value ("downloads").toInt ();
  tmpl->rating        = statsObj.value ("rating").toDouble ();

  if (!tmpl->id.isEmpty ()) {
    metadata.insert (tmpl->id, tmpl);
  }
}

void
TemplateAPI::setupRequestHeaders (QNetworkRequest& request) {
  request.setHeader (QNetworkRequest::ContentTypeHeader, "application/json");
  request.setHeader (QNetworkRequest::UserAgentHeader,
                     "Mogan-TemplateCenter/1.0");
  request.setRawHeader ("Accept", "application/json");
}
