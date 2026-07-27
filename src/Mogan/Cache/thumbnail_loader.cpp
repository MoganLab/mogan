
/******************************************************************************
 * MODULE     : thumbnail_loader.cpp
 * DESCRIPTION: 缩略图下载 + 本地缓存 + ETag 条件请求校验
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "thumbnail_loader.hpp"

#include "image_cache_base.hpp"
#include "qt_utilities.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

#include "sys_utils.hpp"

static QString
localPathForUrl (const QString& cacheDir, const QString& remoteUrl) {
  QByteArray hash=
      QCryptographicHash::hash (remoteUrl.toUtf8 (), QCryptographicHash::Md5);
  return QDir (cacheDir).filePath (hash.toHex () + ".png");
}

ThumbnailLoader::ThumbnailLoader (QObject* parent) : QObject (parent) {
  nam_= new QNetworkAccessManager (this);
  loadMeta ();
  connect (nam_, &QNetworkAccessManager::finished, this,
           &ThumbnailLoader::onDownloadFinished);
}

ThumbnailLoader::~ThumbnailLoader ()= default;

// =========================================================================
// 缓存目录
// =========================================================================

QString
ThumbnailLoader::cacheDir () const {
  QString home= ImageCacheUtils::getEnvQString ("TEXMACS_HOME_PATH");
  if (home.isEmpty ())
    home= QStandardPaths::writableLocation (QStandardPaths::AppDataLocation);
  QString dir= QDir (home).filePath ("system/cache/thumbnails");
  QDir ().mkpath (dir);
  return dir;
}

// =========================================================================
// ETag 元数据持久化
// =========================================================================

QString
ThumbnailLoader::metaFilePath () const {
  return QDir (cacheDir ()).filePath ("thumbnails_meta.json");
}

void
ThumbnailLoader::loadMeta () {
  meta_.clear ();
  QFile file (metaFilePath ());
  if (!file.open (QIODevice::ReadOnly)) return;

  QJsonDocument doc= QJsonDocument::fromJson (file.readAll ());
  file.close ();
  if (!doc.isObject ()) return;

  // 必须先把 QJsonObject 存到局部变量再遍历：doc.object() 返回临时对象，
  // 若直接在 for 里取 begin()/end()，临时对象在该完整表达式结束时析构，
  // 迭代器随即悬空 → it.value().toObject() 触发 use-after-free 崩溃。
  QJsonObject root= doc.object ();
  for (auto it= root.begin (); it != root.end (); ++it) {
    QJsonObject   o= it.value ().toObject ();
    ThumbnailMeta m;
    m.etag        = o["etag"].toString ();
    m.lastModified= o["lastModified"].toString ();
    if (!m.etag.isEmpty () || !m.lastModified.isEmpty ())
      meta_.insert (it.key (), m);
  }
}

void
ThumbnailLoader::saveMeta () {
  QJsonObject root;
  for (auto it= meta_.constBegin (); it != meta_.constEnd (); ++it) {
    QJsonObject o;
    if (!it->etag.isEmpty ()) o["etag"]= it->etag;
    if (!it->lastModified.isEmpty ()) o["lastModified"]= it->lastModified;
    if (!o.isEmpty ()) root[it.key ()]= o;
  }

  // 原子写入：先写临时文件再 rename
  QString tmpPath= metaFilePath () + ".tmp";
  QFile   tmpFile (tmpPath);
  if (tmpFile.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    tmpFile.write (QJsonDocument (root).toJson ());
    tmpFile.close ();
    QString destPath= metaFilePath ();
    QFile::remove (destPath);
    if (!tmpFile.rename (destPath))
      qWarning () << "[ThumbnailLoader] Failed to rename meta temp file";
  }
  else {
    qWarning () << "[ThumbnailLoader] Failed to write meta temp file";
  }
}

// =========================================================================
// 公开 API
// =========================================================================

QString
ThumbnailLoader::getUrl (const QString& remoteUrl) {
  return getUrlImpl (remoteUrl, true, true);
}

QString
ThumbnailLoader::queryUrl (const QString& remoteUrl) {
  return getUrlImpl (remoteUrl, false, false);
}

// =========================================================================
// 核心逻辑
// =========================================================================

QString
ThumbnailLoader::getUrlImpl (const QString& remoteUrl, bool triggerDownload,
                             bool validateCached) {
  if (remoteUrl.isEmpty ()) return remoteUrl;

  // 已是本地资源 / qrc 资源，不处理
  if (remoteUrl.startsWith ("qrc:/") || remoteUrl.startsWith ("/") ||
      remoteUrl.startsWith (":"))
    return remoteUrl;

  // file:// URL：校验文件是否存在
  if (remoteUrl.startsWith ("file://")) {
    if (QFile::exists (remoteUrl.mid (7))) return remoteUrl;
    return QString ();
  }

  // 检查 urlCache_ 中的 file:// 映射，校验文件是否仍存在
  auto it= urlCache_.constFind (remoteUrl);
  if (it != urlCache_.constEnd ()) {
    // file:// URL 提取本地路径校验文件存在（用户可能手动删了缓存）
    if (it->startsWith ("file://") && QFile::exists (it->mid (7))) return *it;
    if (!it->startsWith ("file://")) return *it; // qrc:/ 等非 file:// 直接返回
    // file:// 但文件已不存在：清除缓存，继续走下载/查询逻辑
    urlCache_.erase (it);
  }

  // 本地文件已存在（从未缓存过，或缓存因文件丢失被清除后重新检查）
  QString localPath= localPathForUrl (cacheDir (), remoteUrl);
  if (QFile::exists (localPath)) {
    QString fileUrl= "file://" + localPath;
    urlCache_.insert (remoteUrl, fileUrl);

    // 后台 ETag 条件请求校验缓存是否过期
    if (validateCached && !validatedUrls_.contains (remoteUrl) &&
        !inFlight_.contains (remoteUrl)) {
      ThumbnailMeta meta= meta_.value (remoteUrl);
      if (!meta.etag.isEmpty () || !meta.lastModified.isEmpty ()) {
        inFlight_.insert (remoteUrl);
        QNetworkRequest req (remoteUrl);
        req.setAttribute (QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setRawHeader ("Cache-Control", "no-cache");
        if (!meta.etag.isEmpty ())
          req.setRawHeader ("If-None-Match", meta.etag.toUtf8 ());
        if (!meta.lastModified.isEmpty ())
          req.setRawHeader ("If-Modified-Since", meta.lastModified.toUtf8 ());
        nam_->get (req);
      }
      else {
        // 无 ETag 元数据（旧版下载或元数据丢失）：标记为已验证，
        // 不再发无条件请求浪费带宽
        validatedUrls_.insert (remoteUrl);
      }
    }

    return fileUrl;
  }

  // 不触发下载 → 返回原始远程 URL（QML Image 异步加载）
  if (!triggerDownload) return remoteUrl;

  // 触发全量下载
  if (inFlight_.contains (remoteUrl)) return remoteUrl;
  inFlight_.insert (remoteUrl);
  QNetworkRequest req (remoteUrl);
  req.setAttribute (QNetworkRequest::RedirectPolicyAttribute,
                    QNetworkRequest::NoLessSafeRedirectPolicy);
  nam_->get (req);
  return remoteUrl;
}

// =========================================================================
// 网络响应处理
// =========================================================================

void
ThumbnailLoader::onDownloadFinished (QNetworkReply* reply) {
  reply->deleteLater ();

  QString remoteUrl= reply->request ().url ().toString ();
  inFlight_.remove (remoteUrl);

  if (reply->error () != QNetworkReply::NoError) return;

  int httpStatus=
      reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();

  // ---- 304 Not Modified：缓存仍然新鲜 ----
  if (httpStatus == 304) {
    validatedUrls_.insert (remoteUrl);
    saveMeta ();
    return;
  }

  // ---- HTTP 错误：丢弃 ----
  if (httpStatus < 200 || httpStatus >= 300) return;

  // ---- 200 OK：保存文件 + 更新 ETag 元数据 ----
  QString localPath= localPathForUrl (cacheDir (), remoteUrl);

  QFile file (localPath);
  if (!file.open (QIODevice::WriteOnly)) return;
  file.write (reply->readAll ());
  file.close ();

  // 提取 ETag / Last-Modified
  ThumbnailMeta meta;
  meta.etag        = QString::fromUtf8 (reply->rawHeader ("ETag"));
  meta.lastModified= QString::fromUtf8 (reply->rawHeader ("Last-Modified"));
  if (!meta.etag.isEmpty () || !meta.lastModified.isEmpty ())
    meta_.insert (remoteUrl, meta);
  else meta_.remove (remoteUrl);

  validatedUrls_.insert (remoteUrl);
  saveMeta ();

  QString fileUrl= "file://" + localPath;
  urlCache_.insert (remoteUrl, fileUrl);
  emit ready (remoteUrl);
}
