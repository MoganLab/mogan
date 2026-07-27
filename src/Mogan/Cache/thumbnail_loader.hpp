
/******************************************************************************
 * MODULE     : thumbnail_loader.hpp
 * DESCRIPTION: 缩略图下载 + 本地缓存 + ETag 条件请求校验
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef THUMBNAIL_LOADER_HPP
#define THUMBNAIL_LOADER_HPP

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

/** 单个缩略图的持久化校验元数据。 */
struct ThumbnailMeta {
  QString etag;
  QString lastModified;
};

/**
 * @brief 缩略图下载器：将远程 URL 下载到本地，返回 file:// 路径。
 *
 * 用法：
 *   QString url= loader->getUrl (remoteUrl);
 *   // 本地已有 → 立即返回 "file:///.../xxx.png"，后台发 If-None-Match 校验
 *   // 首次访问 → 触发异步下载，返回原 remoteUrl
 *
 * 下载完成后 emit ready (remoteUrl)，消费者据此刷新对应 UI。
 * 校验（304）不会触发 ready，仅静默更新元数据。
 *
 * 缓存策略：
 *   - 缩略图文件持久化到 system/cache/thumbnails/，以 URL MD5 命名
 *   - ETag + Last-Modified 持久化到 system/cache/thumbnails/thumbnails_meta.json
 *   - 每会话每个 URL 最多发一次条件请求（304 响应后本会话不再请求）
 */
class ThumbnailLoader : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY (ThumbnailLoader)

public:
  explicit ThumbnailLoader (QObject* parent= nullptr);
  ~ThumbnailLoader ();

  /// 获取缩略图 URL：已缓存返回 file://，否则触发异步下载。
  /// 已缓存且持有 ETag 时，后台发 If-None-Match 校验（不阻塞返回）。
  QString getUrl (const QString& remoteUrl);
  /// 只查缓存不触发下载/校验：已缓存返回 file://，否则直接返回原 URL。
  QString queryUrl (const QString& remoteUrl);

signals:
  /// remoteUrl 对应的本地 file:// 已更新（下载完成或服务端返回新版本）。
  void ready (const QString& remoteUrl);

private slots:
  void onDownloadFinished (QNetworkReply* reply);

private:
  QString cacheDir () const;
  QString metaFilePath () const;
  void    loadMeta ();
  void    saveMeta ();
  QString getUrlImpl (const QString& remoteUrl, bool triggerDownload,
                      bool validateCached);

  QNetworkAccessManager* nam_;

  /// remote URL → file:// 已缓存路径（仅在下载完成后写入）。
  QHash<QString, QString> urlCache_;

  /// 正在进行的网络请求的 remote URL（避免重复请求）。
  QSet<QString> inFlight_;

  /// 本会话已校验通过的 URL（304 或新下载后加入，避免同会话重复发条件请求）。
  QSet<QString> validatedUrls_;

  /// remote URL → ETag + Last-Modified（从 JSON 加载，下载/校验成功后更新并持久化）。
  QHash<QString, ThumbnailMeta> meta_;
};

#endif
