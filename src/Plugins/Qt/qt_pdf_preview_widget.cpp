
/******************************************************************************
 * MODULE     : qt_pdf_preview_widget.cpp
 * DESCRIPTION: PDF preview widget implementation using MuPDF
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_pdf_preview_widget.hpp"

#include <QDebug>
#include <QFile>
#include <QNetworkReply>
#include <QPainter>
#include <QVBoxLayout>

#include <atomic>
#include <mutex>

#include "MuPDF/mupdf_renderer.hpp"
#include <mupdf/fitz.h>

QTPdfPreviewWidget::QTPdfPreviewWidget (QWidget* parent)
    : QLabel (parent), networkManager_ (nullptr), currentReply_ (nullptr),
      targetDpi_ (DEFAULT_DPI), targetPage_ (0), isLoading_ (false),
      hasError_ (false) {

  networkManager_= new QNetworkAccessManager (this);

  // Setup label appearance
  setFixedSize (DEFAULT_WIDTH, DEFAULT_HEIGHT);
  setAlignment (Qt::AlignCenter);
  setStyleSheet (
      "background: #f5f5f5; border: 1px solid #ddd; border-radius: 8px;");

  // Show initial placeholder
  clearPreview (tr ("No Preview Available"));
}

QTPdfPreviewWidget::~QTPdfPreviewWidget () { cancelLoading (); }

void
QTPdfPreviewWidget::loadFromUrl (const QString& url, int pageNumber, int dpi) {
  cancelLoading ();

  targetPage_= pageNumber;
  targetDpi_ = dpi;
  hasError_  = false;
  errorString_.clear ();

  showLoading ();

  QNetworkRequest request (url);
  currentReply_= networkManager_->get (request);

  connect (currentReply_, &QNetworkReply::finished, this,
           &QTPdfPreviewWidget::onNetworkReplyFinished);
}

bool
QTPdfPreviewWidget::loadFromFile (const QString& filePath, int pageNumber,
                                  int dpi) {
  cancelLoading ();

  targetPage_= pageNumber;
  targetDpi_ = dpi;
  hasError_  = false;
  errorString_.clear ();

  QFile file (filePath);
  if (!file.open (QIODevice::ReadOnly)) {
    errorString_= tr ("Cannot open file: %1").arg (file.errorString ());
    hasError_   = true;
    showError (errorString_);
    emit loadingFinished (false);
    return false;
  }

  QByteArray data= file.readAll ();
  file.close ();

  return renderPdfPage (data, targetPage_, targetDpi_);
}

bool
QTPdfPreviewWidget::loadFromData (const QByteArray& data, int pageNumber,
                                  int dpi) {
  cancelLoading ();

  targetPage_= pageNumber;
  targetDpi_ = dpi;
  hasError_  = false;
  errorString_.clear ();

  return renderPdfPage (data, targetPage_, targetDpi_);
}

void
QTPdfPreviewWidget::cancelLoading () {
  if (currentReply_) {
    disconnect (currentReply_, nullptr, this, nullptr);
    currentReply_->abort ();
    currentReply_->deleteLater ();
    currentReply_= nullptr;
  }
  isLoading_= false;
}

void
QTPdfPreviewWidget::clearPreview (const QString& text) {
  setPixmap (QPixmap ());
  if (text.isEmpty ()) {
    setText (tr ("No Preview Available"));
  }
  else {
    setText (text);
  }
}

void
QTPdfPreviewWidget::showLoading () {
  isLoading_= true;
  setText (tr ("Loading PDF..."));
  emit loadingStarted ();
}

void
QTPdfPreviewWidget::showError (const QString& message) {
  isLoading_= false;
  hasError_ = true;
  setText (message);
  emit error (message);
  emit loadingFinished (false);
}

void
QTPdfPreviewWidget::setPreviewPixmap (const QPixmap& pixmap) {
  isLoading_= false;
  setPixmap (pixmap);
  emit loadingFinished (true);
}

void
QTPdfPreviewWidget::onNetworkReplyFinished () {
  QNetworkReply* reply= currentReply_;
  currentReply_       = nullptr;

  if (!reply) return;

  if (reply->error () != QNetworkReply::NoError) {
    errorString_= tr ("Download failed: %1").arg (reply->errorString ());
    showError (errorString_);
    reply->deleteLater ();
    return;
  }

  QByteArray pdfData= reply->readAll ();
  reply->deleteLater ();

  if (pdfData.isEmpty ()) {
    errorString_= tr ("Empty PDF data received");
    showError (errorString_);
    return;
  }

  renderPdfPage (pdfData, targetPage_, targetDpi_);
}

bool
QTPdfPreviewWidget::renderPdfPage (const QByteArray& data, int pageNumber,
                                   int dpi) {
  // Get MuPDF context
  fz_context* ctx= mupdf_context ();
  if (!ctx) {
    qWarning () << "MuPDF context not available";
    errorString_= tr ("PDF engine not available");
    showError (errorString_);
    return false;
  }

  // Register document handlers (needed to open PDF files)
  // Note: handlersRegistered is a function-local static, which is thread-safe in C++11+
  static std::atomic<bool> handlersRegistered{false};
  static std::mutex        handlerMutex;

  if (!handlersRegistered.load (std::memory_order_acquire)) {
    std::lock_guard<std::mutex> lock (handlerMutex);
    if (!handlersRegistered.load (std::memory_order_relaxed)) {
      bool success= true;
      fz_try (ctx) {
        fz_register_document_handlers (ctx);
      }
      fz_catch (ctx) {
        qWarning () << "Failed to register document handlers:"
                    << fz_caught_message (ctx);
        success= false;
      }
      // Only set to true if registration succeeded
      // If it fails, we don't want to prevent future retries
      if (success) {
        handlersRegistered.store (true, std::memory_order_release);
      }
    }
  }

  fz_document* doc    = nullptr;
  fz_pixmap*   pix    = nullptr;
  fz_buffer*   buf    = nullptr;
  fz_stream*   stream = nullptr;
  bool         success= false;

  // Protect variables for exception handling
  fz_var (doc);
  fz_var (pix);
  fz_var (buf);
  fz_var (stream);

  fz_try (ctx) {
    // Create buffer from QByteArray
    buf= fz_new_buffer_from_copied_data (
        ctx, reinterpret_cast<const unsigned char*> (data.constData ()),
        data.size ());

    // Create stream from buffer
    stream= fz_open_buffer (ctx, buf);

    // Open PDF document from stream
    doc= fz_open_document_with_stream (ctx, "pdf", stream);

    if (!doc) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to open PDF document");
    }

    // Check page count
    int pageCount= fz_count_pages (ctx, doc);
    if (pageCount <= 0) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "PDF has no pages");
    }

    // Validate page number
    if (pageNumber < 0 || pageNumber >= pageCount) {
      pageNumber= 0;
    }

    // Get page
    fz_page* page= fz_load_page (ctx, doc, pageNumber);
    if (!page) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to load page %d", pageNumber);
    }

    // Get page bounds
    fz_rect bbox= fz_bound_page (ctx, page);

    // Calculate transform matrix for target DPI
    float     scale= static_cast<float> (dpi) / 72.0f;
    fz_matrix ctm  = fz_scale (scale, scale);

    // Render page with RGB color space
    pix= fz_new_pixmap_from_page (ctx, page, ctm, fz_device_rgb (ctx), 0);
    if (!pix) {
      fz_drop_page (ctx, page);
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to render page");
    }

    // Convert RGB pixmap to QImage
    int            pixW   = fz_pixmap_width (ctx, pix);
    int            pixH   = fz_pixmap_height (ctx, pix);
    int            stride = fz_pixmap_stride (ctx, pix);
    unsigned char* samples= fz_pixmap_samples (ctx, pix);

    // Create QImage from RGB data
    QImage image (pixW, pixH, QImage::Format_RGB888);
    for (int y= 0; y < pixH; y++) {
      unsigned char* src= samples + y * stride;
      unsigned char* dst= image.scanLine (y);
      memcpy (dst, src, pixW * 3);
    }

    if (image.isNull ()) {
      fz_drop_pixmap (ctx, pix);
      fz_drop_page (ctx, page);
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to convert to image");
    }

    // Scale to widget size while maintaining aspect ratio
    QPixmap pixmap= QPixmap::fromImage (image);
    pixmap= pixmap.scaled (DEFAULT_WIDTH, DEFAULT_HEIGHT, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);

    setPreviewPixmap (pixmap);
    success= true;

    // Cleanup
    fz_drop_pixmap (ctx, pix);
    fz_drop_page (ctx, page);
  }
  fz_catch (ctx) {
    qWarning () << "MuPDF error:" << fz_caught_message (ctx);
    errorString_= tr ("PDF render error: %1")
                      .arg (QString::fromUtf8 (fz_caught_message (ctx)));
    showError (errorString_);
    success= false;
  }

  // Cleanup resources
  if (stream) fz_drop_stream (ctx, stream);
  if (buf) fz_drop_buffer (ctx, buf);
  if (doc) fz_drop_document (ctx, doc);

  return success;
}
