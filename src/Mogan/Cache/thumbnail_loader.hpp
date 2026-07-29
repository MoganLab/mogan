
/******************************************************************************
 * MODULE     : thumbnail_loader.hpp
 * DESCRIPTION: Shared thumbnail loading with queue, cache, and HTTP validation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef THUMBNAIL_LOADER_HPP
#define THUMBNAIL_LOADER_HPP

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QSize>

class QLabel;
class QNetworkAccessManager;
class QNetworkReply;

/**
 * @brief Pending thumbnail load request
 */
struct ThumbnailLoadRequest {
  QPointer<QLabel> label;
  QString          url;
  QString          cachedEtag;
  QSize            targetSize;
};

/**
 * @brief Shared thumbnail loader (singleton)
 *
 * Manages a global download queue with concurrency control,
 * ETag-based conditional HTTP requests, and ThumbnailCache integration.
 */
class ThumbnailLoader : public QObject {
  Q_OBJECT

public:
  explicit ThumbnailLoader (QObject* parent= nullptr);
  ~ThumbnailLoader ();

  static ThumbnailLoader* instance ();

  /**
   * @brief Load a thumbnail into a QLabel
   *
   * If the thumbnail is already cached, sets the pixmap immediately.
   * If the cached entry needs validation (ETag-based conditional request),
   * enqueues a background validation.  If not cached at all, downloads it.
   *
   * @param label      Target QLabel widget
   * @param url        Image URL
   * @param targetSize Desired display size (used for cache key and scaling)
   */
  void load (QLabel* label, const QString& url, const QSize& targetSize);

private:
  void processQueue ();

private:
  QNetworkAccessManager*       networkManager_= nullptr;
  QQueue<ThumbnailLoadRequest> queue_;
  int                          activeRequests_= 0;
  static constexpr int         MAX_CONCURRENT = 6;
  QSet<QString>                validatedUrls_;
};

#endif // THUMBNAIL_LOADER_HPP
