
/******************************************************************************
 * MODULE     : thumbnail_loader.cpp
 * DESCRIPTION: 缩略图下载 + 本地缓存实现
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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

// 通过 sys_utils.hpp 引入 get_env
#include "sys_utils.hpp"

ThumbnailLoader::ThumbnailLoader (QObject* parent) : QObject (parent) {
  nam_= new QNetworkAccessManager (this);
  connect (nam_, &QNetworkAccessManager::finished, this,
           &ThumbnailLoader::onDownloadFinished);
}

ThumbnailLoader::~ThumbnailLoader ()= default;

QString
ThumbnailLoader::cacheDir () const {
  QString home= ImageCacheUtils::getEnvQString ("TEXMACS_HOME_PATH");
  if (home.isEmpty ())
    home= QStandardPaths::writableLocation (QStandardPaths::AppDataLocation);
  QString dir= QDir (home).filePath ("system/cache/thumbnails");
  QDir ().mkpath (dir);
  return dir;
}

QString
ThumbnailLoader::getUrl (const QString& remoteUrl) {
  return getUrlImpl (remoteUrl, true);
}

QString
ThumbnailLoader::queryUrl (const QString& remoteUrl) {
  return getUrlImpl (remoteUrl, false);
}

QString
ThumbnailLoader::getUrlImpl (const QString& remoteUrl, bool triggerDownload) {
  if (remoteUrl.isEmpty ()) return remoteUrl;

  // 已是本地资源 / qrc 资源，不处理
  if (remoteUrl.startsWith ("qrc:/") || remoteUrl.startsWith ("/") ||
      remoteUrl.startsWith (":"))
    return remoteUrl;

  // file:// URL：校验文件是否存在（可能是旧缓存残留）
  if (remoteUrl.startsWith ("file://")) {
    if (QFile::exists (remoteUrl.mid (7))) return remoteUrl;
    return QString (); // 文件已删除，无法恢复
  }

  // 已缓存的映射（file:// 或下载中的本地路径）
  auto it= urlCache_.constFind (remoteUrl);
  if (it != urlCache_.constEnd ()) return *it;

  // 生成唯一本地文件名
  QByteArray hash=
      QCryptographicHash::hash (remoteUrl.toUtf8 (), QCryptographicHash::Md5);
  QString localPath= QDir (cacheDir ()).filePath (hash.toHex () + ".png");

  // 本地文件已存在
  if (QFile::exists (localPath)) {
    urlCache_.insert (remoteUrl, "file://" + localPath);
    return "file://" + localPath;
  }

  // 不触发下载 → 直接返回原始远程 URL（QML Image 异步加载）
  if (!triggerDownload) return remoteUrl;

  // 触发异步下载
  urlCache_.insert (remoteUrl, localPath); // 标记正在下载
  QNetworkRequest req (remoteUrl);
  req.setAttribute (QNetworkRequest::RedirectPolicyAttribute,
                    QNetworkRequest::NoLessSafeRedirectPolicy);
  nam_->get (req);
  return remoteUrl;
}

void
ThumbnailLoader::onDownloadFinished (QNetworkReply* reply) {
  reply->deleteLater ();
  if (reply->error () != QNetworkReply::NoError) return;

  QString remoteUrl= reply->request ().url ().toString ();
  auto    it       = urlCache_.constFind (remoteUrl);
  if (it == urlCache_.constEnd ()) return;

  // 已是 file://（之前下载过），跳过
  if (it->startsWith ("file://")) return;

  QString localPath= *it;
  QFile   file (localPath);
  if (!file.open (QIODevice::WriteOnly)) {
    urlCache_.remove (remoteUrl);
    return;
  }
  file.write (reply->readAll ());
  file.close ();

  QString fileUrl= "file://" + localPath;
  urlCache_.insert (remoteUrl, fileUrl);
  emit ready (remoteUrl);
}
