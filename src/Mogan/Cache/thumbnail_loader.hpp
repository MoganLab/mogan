
/******************************************************************************
 * MODULE     : thumbnail_loader.hpp
 * DESCRIPTION: 缩略图下载 + 本地缓存
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

class QNetworkAccessManager;
class QNetworkReply;

/**
 * @brief 缩略图下载器：将远程 URL 下载到本地，返回 file:// 路径。
 *
 * 用法：
 *   QString url= loader->getUrl (remoteUrl);
 *   // 本地已有 → "file:///.../xxx.png"
 *   // 首次访问 → 触发异步下载，返回原 remoteUrl
 *
 * 下载完成后 emit ready (remoteUrl)，消费者据此刷新对应 UI。
 */
class ThumbnailLoader : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY (ThumbnailLoader)

public:
  explicit ThumbnailLoader (QObject* parent= nullptr);
  ~ThumbnailLoader ();

  /// @return 已缓存返回 "file://..."，否则触发异步下载并返回原 URL。
  QString getUrl (const QString& remoteUrl);
  /// 只查缓存不触发下载：已缓存返回 file://，否则直接返回原 URL。
  QString queryUrl (const QString& remoteUrl);

signals:
  /// remoteUrl 对应的本地 file:// 已就绪。
  void ready (const QString& remoteUrl);

private slots:
  void onDownloadFinished (QNetworkReply* reply);

private:
  QString cacheDir () const;
  QString getUrlImpl (const QString& remoteUrl, bool triggerDownload);

  QNetworkAccessManager* nam_;
  /// remote URL → file:// 已缓存路径（仅在下载完成后写入）。
  QHash<QString, QString> urlCache_;
  /// 正在下载中的 remote URL（避免重复请求）。
  QSet<QString> inFlight_;
};

#endif
