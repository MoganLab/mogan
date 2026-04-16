
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
#include "pdf_preview_cache.hpp"
#include <mupdf/fitz.h>

QTPdfPreviewWidget::QTPdfPreviewWidget (QWidget* parent)
    : QLabel (parent), networkManager_ (nullptr), currentReply_ (nullptr),
      targetDpi_ (DEFAULT_DPI), targetPage_ (0), isLoading_ (false),
      hasError_ (false), currentLoadType_ (LoadType::None),
      targetSize_ (DEFAULT_WIDTH, DEFAULT_HEIGHT) {

  networkManager_= new QNetworkAccessManager (this);

  // 设置标签外观
  setFixedSize (DEFAULT_WIDTH, DEFAULT_HEIGHT);
  setAlignment (Qt::AlignCenter);
  setStyleSheet (
      "background: #f5f5f5; border: 1px solid #ddd; border-radius: 8px;");

  // 显示初始占位符
  clearPreview (tr ("No Preview Available"));
}

QTPdfPreviewWidget::~QTPdfPreviewWidget () { cancelLoading (); }

void
QTPdfPreviewWidget::loadFromUrl (const QString& url, int pageNumber, int dpi) {
  cancelLoading ();

  // Store key for caching
  currentKey_     = url;
  currentLoadType_= LoadType::PDF;
  targetPage_     = pageNumber;
  targetDpi_      = dpi;
  hasError_       = false;
  errorString_.clear ();

  // Check cache first
  QPixmap cached= PdfPreviewCache::instance ()->get (url, pageNumber, dpi);
  if (!cached.isNull ()) {
    setPreviewPixmap (cached);
    return;
  }

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

  // Store key for caching
  currentKey_= filePath;
  targetPage_= pageNumber;
  targetDpi_ = dpi;
  hasError_  = false;
  errorString_.clear ();

  // Check cache first
  QPixmap cached= PdfPreviewCache::instance ()->get (filePath, pageNumber, dpi);
  if (!cached.isNull ()) {
    setPreviewPixmap (cached);
    return true;
  }

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

  // Clear key since we can't cache data without a persistent identifier
  currentKey_.clear ();
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
  isLoading_      = false;
  currentLoadType_= LoadType::None;
  currentKey_.clear ();
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
  QPointer<QNetworkReply> reply= currentReply_;
  currentReply_                = nullptr;

  if (!reply) return;

  if (reply->error () != QNetworkReply::NoError) {
    errorString_= tr ("Download failed: %1").arg (reply->errorString ());
    showError (errorString_);
    reply->deleteLater ();
    currentLoadType_= LoadType::None;
    return;
  }

  QByteArray pdfData= reply->readAll ();
  reply->deleteLater ();

  if (pdfData.isEmpty ()) {
    errorString_= tr ("Empty PDF data received");
    showError (errorString_);
    currentLoadType_= LoadType::None;
    return;
  }

  renderPdfPage (pdfData, targetPage_, targetDpi_);
  currentLoadType_= LoadType::None;
}

bool
QTPdfPreviewWidget::renderPdfPage (const QByteArray& data, int pageNumber,
                                   int dpi) {
  // 获取MuPDF上下文
  fz_context* ctx= mupdf_context ();
  if (!ctx) {
    qWarning () << "MuPDF context not available";
    errorString_= tr ("PDF engine not available");
    showError (errorString_);
    return false;
  }

  // 注册文档处理器（用于打开PDF文件）
  // 注意：handlersRegistered是函数局部静态变量，线程安全
  // 在C++11及以上版本中
  static std::atomic<bool> handlersRegistered{false};
  static std::mutex        handlerMutex;

  if (!handlersRegistered.load (std::memory_order_acquire)) {
    std::lock_guard<std::mutex> lock (handlerMutex);
    if (!handlersRegistered.load (std::memory_order_relaxed)) {
      bool success= true;
      fz_try (ctx) { fz_register_document_handlers (ctx); }
      fz_catch (ctx) {
        qWarning () << "Failed to register document handlers:"
                    << fz_caught_message (ctx);
        success= false;
      }
      // 仅在注册成功时设置为true
      // 如果失败，我们不希望阻止后续重试
      if (success) {
        handlersRegistered.store (true, std::memory_order_release);
      }
    }
  }

  fz_document* doc    = nullptr;
  fz_pixmap*   pix    = nullptr;
  fz_page*     page   = nullptr;
  fz_buffer*   buf    = nullptr;
  fz_stream*   stream = nullptr;
  bool         success= false;

  // 为异常处理保护变量
  fz_var (doc);
  fz_var (pix);
  fz_var (page);
  fz_var (buf);
  fz_var (stream);

  fz_try (ctx) {
    // 从QByteArray创建缓冲区
    buf= fz_new_buffer_from_copied_data (
        ctx, reinterpret_cast<const unsigned char*> (data.constData ()),
        data.size ());

    // 从缓冲区创建流
    stream= fz_open_buffer (ctx, buf);

    // 从流打开PDF文档
    doc= fz_open_document_with_stream (ctx, "pdf", stream);

    if (!doc) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to open PDF document");
    }

    // 检查页数
    int pageCount= fz_count_pages (ctx, doc);
    if (pageCount <= 0) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "PDF has no pages");
    }

    // 验证页码
    if (pageNumber < 0 || pageNumber >= pageCount) {
      pageNumber= 0;
    }

    // 获取页面
    page= fz_load_page (ctx, doc, pageNumber);
    if (!page) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to load page %d", pageNumber);
    }

    // 获取页面边界
    fz_rect bbox= fz_bound_page (ctx, page);

    // 为目标DPI计算变换矩阵
    float     scale= static_cast<float> (dpi) / 72.0f;
    fz_matrix ctm  = fz_scale (scale, scale);

    // 使用RGB色彩空间渲染页面
    pix= fz_new_pixmap_from_page (ctx, page, ctm, fz_device_rgb (ctx), 0);
    if (!pix) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to render page");
    }

    // 将RGB pixmap转换为QImage
    int            pixW   = fz_pixmap_width (ctx, pix);
    int            pixH   = fz_pixmap_height (ctx, pix);
    int            stride = fz_pixmap_stride (ctx, pix);
    unsigned char* samples= fz_pixmap_samples (ctx, pix);

    // 从RGB数据创建QImage
    QImage image (pixW, pixH, QImage::Format_RGB888);
    for (int y= 0; y < pixH; y++) {
      unsigned char* src= samples + y * stride;
      unsigned char* dst= image.scanLine (y);
      memcpy (dst, src, pixW * 3);
    }

    if (image.isNull ()) {
      fz_drop_pixmap (ctx, pix);
      pix= nullptr;
      fz_drop_page (ctx, page);
      page= nullptr;
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to convert to image");
    }

    // 缩放到控件尺寸，同时保持宽高比
    QPixmap pixmap= QPixmap::fromImage (image);
    pixmap= pixmap.scaled (DEFAULT_WIDTH, DEFAULT_HEIGHT, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);

    setPreviewPixmap (pixmap);
    success= true;

    // Cache the rendered page for future use
    if (!currentKey_.isEmpty ()) {
      PdfPreviewCache::instance ()->put (currentKey_, pageNumber, dpi, pixmap,
                                         true);
    }

    // 清理
  }
  fz_catch (ctx) {
    qWarning () << "MuPDF error:" << fz_caught_message (ctx);
    errorString_= tr ("PDF render error: %1")
                      .arg (QString::fromUtf8 (fz_caught_message (ctx)));
    showError (errorString_);
    success= false;
  }

  // 清理资源
  if (pix) fz_drop_pixmap (ctx, pix);
  if (page) fz_drop_page (ctx, page);
  if (stream) fz_drop_stream (ctx, stream);
  if (buf) fz_drop_buffer (ctx, buf);
  if (doc) fz_drop_document (ctx, doc);

  return success;
}

void
QTPdfPreviewWidget::loadImageFromUrl (const QString& url,
                                      const QSize&   targetSize) {
  cancelLoading ();

  // 设置加载类型和目标尺寸
  currentLoadType_= LoadType::Image;
  if (targetSize.isValid ()) {
    targetSize_= targetSize;
  }
  else {
    targetSize_= QSize (DEFAULT_WIDTH, DEFAULT_HEIGHT);
  }

  hasError_= false;
  errorString_.clear ();

  showLoading ();

  QNetworkRequest request (url);
  currentReply_= networkManager_->get (request);

  connect (currentReply_, &QNetworkReply::finished, this,
           &QTPdfPreviewWidget::onImageNetworkReplyFinished);
}

void
QTPdfPreviewWidget::onImageNetworkReplyFinished () {
  QPointer<QNetworkReply> reply= currentReply_;
  currentReply_                = nullptr;

  if (!reply) return;

  if (reply->error () != QNetworkReply::NoError) {
    errorString_= tr ("Image download failed: %1").arg (reply->errorString ());
    showError (errorString_);
    reply->deleteLater ();
    currentLoadType_= LoadType::None;
    return;
  }

  QByteArray imageData= reply->readAll ();
  reply->deleteLater ();

  if (imageData.isEmpty ()) {
    errorString_= tr ("Received empty image data");
    showError (errorString_);
    currentLoadType_= LoadType::None;
    return;
  }

  // 加载图片数据
  QPixmap pixmap;
  if (pixmap.loadFromData (imageData)) {
    // 缩放图片到目标尺寸，保持宽高比
    pixmap= pixmap.scaled (targetSize_.width (), targetSize_.height (),
                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setPreviewPixmap (pixmap);
  }
  else {
    errorString_= tr ("Failed to load image data");
    showError (errorString_);
  }

  // 重置加载类型
  currentLoadType_= LoadType::None;
}
