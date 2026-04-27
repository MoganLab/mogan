
/******************************************************************************
 * MODULE     : thumbnail_loader.cpp
 * DESCRIPTION: Shared thumbnail loading with queue, cache, and HTTP validation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "thumbnail_loader.hpp"

#include <QImage>
#include <QLabel>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimeZone>

#include "qt_utilities.hpp"
#include "thumbnail_cache.hpp"

ThumbnailLoader::ThumbnailLoader (QObject* parent) : QObject (parent) {
  networkManager_= new QNetworkAccessManager (this);
}

ThumbnailLoader::~ThumbnailLoader ()= default;

ThumbnailLoader*
ThumbnailLoader::instance () {
  static ThumbnailLoader inst;
  return &inst;
}

void
ThumbnailLoader::load (QLabel* label, const QString& url,
                       const QSize& targetSize) {
  if (!label || url.isEmpty ()) return;

  auto cached= ThumbnailCache::instance ()->getEntry (url, targetSize);

  if (cached.isValid ()) {
    QPixmap px= cached.pixmap;
    px.setDevicePixelRatio (label->devicePixelRatioF ());
    label->setPixmap (px);

    if (validatedUrls_.contains (url)) {
      return;
    }

    queue_.enqueue ({label, url, cached.etag, targetSize});
    processQueue ();
    return;
  }

  queue_.enqueue ({label, url, QString (), targetSize});
  processQueue ();
}

void
ThumbnailLoader::processQueue () {
  while (!queue_.isEmpty () && activeRequests_ < MAX_CONCURRENT) {
    ThumbnailLoadRequest req= queue_.dequeue ();

    if (req.label.isNull ()) {
      continue;
    }

    activeRequests_++;

    QNetworkRequest request (req.url);
    if (!req.cachedEtag.isEmpty ()) {
      request.setRawHeader ("If-None-Match", req.cachedEtag.toUtf8 ());
    }
    QNetworkReply* reply= networkManager_->get (request);

    connect (reply, &QNetworkReply::finished, this, [this, req, reply] () {
      activeRequests_--;

      if (req.label.isNull ()) {
        reply->deleteLater ();
        validatedUrls_.insert (req.url);
        processQueue ();
        return;
      }

      int httpStatus=
          reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
      if (httpStatus == 304) {
        validatedUrls_.insert (req.url);
        reply->deleteLater ();
        processQueue ();
        return;
      }

      if (reply->error () == QNetworkReply::NoError) {
        QByteArray data= reply->readAll ();
        QImage     image;
        if (image.loadFromData (data)) {
          qreal dpr    = req.label->devicePixelRatioF ();
          int   scaledW= qRound (req.targetSize.width () * dpr);
          int   scaledH= qRound (req.targetSize.height () * dpr);

          QImage scaled=
              image.scaled (scaledW, scaledH, Qt::KeepAspectRatioByExpanding,
                            Qt::SmoothTransformation);
          if (scaled.width () > scaledW || scaled.height () > scaledH) {
            int x = (scaled.width () - scaledW) / 2;
            int y = (scaled.height () - scaledH) / 2;
            scaled= scaled.copy (x, y, scaledW, scaledH);
          }

          QPixmap pixmap= QPixmap::fromImage (scaled);
          pixmap.setDevicePixelRatio (dpr);

          req.label->setPixmap (pixmap);

          QString   etag= QString::fromUtf8 (reply->rawHeader ("ETag"));
          QDateTime lastModified;
          QString lmStr= QString::fromUtf8 (reply->rawHeader ("Last-Modified"));
          if (!lmStr.isEmpty ()) {
            lastModified= QDateTime::fromString (lmStr, Qt::RFC2822Date);
            if (!lastModified.isValid ()) {
              lastModified= QLocale::c ().toDateTime (
                  lmStr, "ddd, dd MMM yyyy hh:mm:ss 'GMT'");
            }
            if (lastModified.isValid ()) {
              lastModified.setTimeZone (QTimeZone::utc ());
            }
          }

          ThumbnailCache::instance ()->put (req.url, req.targetSize, pixmap,
                                            etag, lastModified);
        }
      }
      if (!req.label.isNull () && req.label->pixmap ().isNull ()) {
        req.label->setText (qt_translate ("Preview"));
      }

      validatedUrls_.insert (req.url);
      reply->deleteLater ();
      processQueue ();
    });
  }
}
