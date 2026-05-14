
/******************************************************************************
 * MODULE     : qt_pdf_preview_widget.cpp
 * DESCRIPTION: PDF preview widget using MuPDF with vector rendering
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_pdf_preview_widget.hpp"

#include <QDebug>
#include <QFile>
#include <QHoverEvent>
#include <QLocale>
#include <QNetworkReply>
#include <QPushButton>
#include <QResizeEvent>
#include <QSet>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

#include <mutex>

#include "MuPDF/mupdf_renderer.hpp"
#include "pdf_file_cache.hpp"

#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include <mupdf/fitz.h>

// 会话级已验证 URL 集合（同一会话内只发一次条件请求）
static QSet<QString> s_validatedPdfUrls;

// 常量定义
namespace {
constexpr float  kRenderOversample         = 2.0F;
constexpr float  kMinRenderScale           = 0.1F;
constexpr float  kMaxRenderScale           = 8.0F;
constexpr int    kMargin                   = 0;
constexpr int    kDefaultPreviewWidth      = 600;
constexpr int    kDefaultPreviewHeight     = 600;
constexpr double kDefaultAspectRatio       = 1.414; // A4比例
constexpr int    kButtonOffset             = 10;
constexpr int    kPageIndicatorBottomMargin= 10;
constexpr int    kButtonBaseSize           = 18;
constexpr int    kButtonMinSize            = 14;
constexpr int    kButtonMaxSize            = 20;
} // namespace

QTPdfPreviewWidget::QTPdfPreviewWidget (QWidget* parent)
    : QWidget (parent), previewContainer_ (nullptr), previewLabel_ (nullptr),
      prevBtn_ (nullptr), nextBtn_ (nullptr), pageIndicator_ (nullptr),
      networkManager_ (new QNetworkAccessManager (this)),
      currentReply_ (nullptr), targetDpi_ (DEFAULT_DPI), currentPage_ (0),
      pageCount_ (0), pageAspectRatio_ (kDefaultAspectRatio),
      isLoading_ (false), hasError_ (false), currentLoadType_ (LoadType::None) {

  setupUI ();
}

QTPdfPreviewWidget::~QTPdfPreviewWidget () { cancelLoading (); }

QPushButton*
QTPdfPreviewWidget::createNavButton (const QString& text,
                                     void (QTPdfPreviewWidget::*slot) ()) {
  QPushButton* btn= new QPushButton (text, previewContainer_);
  btn->setObjectName ("pdf-preview-nav-btn");
  int scaledSize= qBound (kButtonMinSize,
                          DpiUtils::scaled (kButtonBaseSize, this->screen ()),
                          kButtonMaxSize);
  btn->setFixedSize (scaledSize, scaledSize);
  // 设置圆形边框半径，使用ID选择器确保与CSS中的选择器匹配
  int radius= scaledSize / 2;
  btn->setStyleSheet (
      QString ("QPushButton#pdf-preview-nav-btn { border-radius: %1px; }")
          .arg (radius));
  btn->setText (text);
  btn->setCursor (Qt::PointingHandCursor);
  btn->hide ();
  connect (btn, &QPushButton::clicked, this, slot);
  return btn;
}

void
QTPdfPreviewWidget::setupUI () {
  setAttribute (Qt::WA_Hover, true);

  QVBoxLayout* mainLayout= new QVBoxLayout (this);
  mainLayout->setContentsMargins (0, 0, 0, 0);
  mainLayout->setSpacing (0);

  // 预览容器（用于放置按钮和预览图）
  previewContainer_= new QWidget (this);
  previewContainer_->setObjectName ("pdf-preview-container");
  previewContainer_->setAttribute (Qt::WA_Hover, true);
  previewContainer_->setStyleSheet (
      QString ("QWidget#pdf-preview-container { border-radius: %1px; }")
          .arg (DpiUtils::scaled (8, this->screen ())));
  QVBoxLayout* containerLayout= new QVBoxLayout (previewContainer_);
  containerLayout->setContentsMargins (0, 0, 0, 0);
  containerLayout->setSpacing (0);
  containerLayout->setAlignment (Qt::AlignCenter);

  // 预览标签
  previewLabel_= new QLabel (previewContainer_);
  previewLabel_->setObjectName ("pdf-preview-label");
  previewLabel_->setAlignment (Qt::AlignCenter);

  containerLayout->addWidget (previewLabel_, 0, Qt::AlignCenter);
  mainLayout->addWidget (previewContainer_, 1, Qt::AlignCenter);

  // 创建导航按钮
  prevBtn_= createNavButton ("◀", &QTPdfPreviewWidget::goToPreviousPage);
  nextBtn_= createNavButton ("▶", &QTPdfPreviewWidget::goToNextPage);

  // 页码指示器（底部居中）
  pageIndicator_= new QLabel ("1 / 1", previewContainer_);
  pageIndicator_->setObjectName ("pdf-preview-page-indicator");
  pageIndicator_->setAlignment (Qt::AlignCenter);
  // 使用DpiUtils处理font-size、padding和border-radius
  int fontSize= DpiUtils::scaled (14, this->screen ());
  int vPadding= DpiUtils::scaled (6, this->screen ());
  int hPadding= DpiUtils::scaled (16, this->screen ());
  int radius  = DpiUtils::scaled (12, this->screen ());
  pageIndicator_->setStyleSheet (QString ("QLabel { font-size: %1px; padding: "
                                          "%2px %3px; border-radius: %4px; }")
                                     .arg (fontSize)
                                     .arg (vPadding)
                                     .arg (hPadding)
                                     .arg (radius));
  pageIndicator_->hide ();

  // 安装事件过滤器以处理鼠标悬停
  previewContainer_->installEventFilter (this);
  prevBtn_->installEventFilter (this);
  nextBtn_->installEventFilter (this);
  pageIndicator_->installEventFilter (this);
  previewLabel_->installEventFilter (this);

  // 启用鼠标跟踪以接收悬停事件
  previewContainer_->setMouseTracking (true);
  prevBtn_->setMouseTracking (true);
  nextBtn_->setMouseTracking (true);
  pageIndicator_->setMouseTracking (true);
  previewLabel_->setMouseTracking (true);

  // 默认显示 Loading，有内容时会被覆盖；无内容时由调用方设为 No Preview
  clearPreview (qt_translate ("Loading..."));
}

void
QTPdfPreviewWidget::updatePageControls () {
  pageIndicator_->setText (
      QString ("%1 / %2").arg (currentPage_ + 1).arg (pageCount_));
  pageIndicator_->adjustSize ();

  prevBtn_->setEnabled (currentPage_ > 0);
  nextBtn_->setEnabled (currentPage_ < pageCount_ - 1);

  // 更新按钮位置
  updateButtonPositions ();
}

void
QTPdfPreviewWidget::calculatePreviewDimensions (int availWidth, int availHeight,
                                                int& outWidth,
                                                int& outHeight) const {
  if (availWidth <= 0 || availHeight <= 0) {
    outWidth = kDefaultPreviewWidth;
    outHeight= kDefaultPreviewHeight;
    return;
  }
  outWidth = qMax (1, availWidth);
  outHeight= qMax (1, availHeight);
}

QSize
QTPdfPreviewWidget::calculateOptimalSize (int availWidth,
                                          int availHeight) const {
  int w, h;
  calculatePreviewDimensions (availWidth, availHeight, w, h);
  return QSize (w, h);
}

void
QTPdfPreviewWidget::updatePreviewSize () {
  if (!previewContainer_ || !previewLabel_) return;

  // Calculate available size for preview
  int availWidth = previewContainer_->width () - kMargin * 2;
  int availHeight= previewContainer_->height () - kMargin * 2;

  if (availWidth < 64 || availHeight < 64) {
    // Use default size if container is too small
    previewLabel_->setFixedSize (
        DpiUtils::scaled (kDefaultPreviewWidth, this->screen ()),
        DpiUtils::scaled (kDefaultPreviewHeight, this->screen ()));
  }
  else {
    previewLabel_->setFixedSize (availWidth, availHeight);
  }

  updateButtonPositions ();
}

void
QTPdfPreviewWidget::loadFromUrl (const QString& url, int dpi) {
  cancelLoading ();

  // Store key for caching
  currentKey_     = url;
  currentLoadType_= LoadType::PDF;
  targetDpi_      = dpi;
  currentPage_    = 0;
  pageCount_      = 0;
  pageAspectRatio_= kDefaultAspectRatio;
  hasError_       = false;
  errorString_.clear ();
  pdfData_.clear ();

  setControlsVisible (false);

  // First check if PDF file is cached locally
  PdfCacheEntry cachedEntry= PdfFileCache::instance ()->getEntry (url);
  if (cachedEntry.isValid ()) {
    // Render cached content immediately so user never sees "No Preview"
    loadFromFile (cachedEntry.filePath, dpi);
    // Restore URL key so background validation and cache updates use the
    // original URL, not the local file path.
    currentKey_= url;

    // Already validated this session: use cache directly, no network request
    if (s_validatedPdfUrls.contains (url)) {
      qDebug () << "[PDF Preview] Use cache:" << url;
      return;
    }

    // First time this session: validate with a conditional request
    qDebug () << "[PDF Preview] Validate:" << url;
    QNetworkRequest request (url);
    if (!cachedEntry.etag.isEmpty ()) {
      request.setRawHeader ("If-None-Match", cachedEntry.etag.toUtf8 ());
    }
    if (cachedEntry.lastModified.isValid ()) {
      request.setRawHeader ("If-Modified-Since",
                            cachedEntry.lastModified.toUTC ()
                                .toString (Qt::RFC2822Date)
                                .toUtf8 ());
    }
    currentReply_= networkManager_->get (request);

    connect (currentReply_, &QNetworkReply::finished, this,
             [this, cachedEntry, dpi] () {
               onConditionalReplyFinished (cachedEntry.filePath, dpi);
             });
    return;
  }

  // Show loading state
  showLoading ();

  QNetworkRequest request (url);
  currentReply_= networkManager_->get (request);

  connect (currentReply_, &QNetworkReply::finished, this,
           &QTPdfPreviewWidget::onNetworkReplyFinished);
}

bool
QTPdfPreviewWidget::loadFromFile (const QString& filePath, int dpi) {
  cancelLoading ();

  // Store key for caching
  currentKey_     = filePath;
  targetDpi_      = dpi;
  currentPage_    = 0;
  pageCount_      = 0;
  pageAspectRatio_= kDefaultAspectRatio;
  hasError_       = false;
  errorString_.clear ();
  pdfData_.clear ();

  setControlsVisible (false);

  // Read file and render
  QFile file (filePath);
  if (!file.open (QIODevice::ReadOnly)) {
    errorString_=
        qt_translate ("Cannot open file: %1").arg (file.errorString ());
    hasError_= true;
    showError (errorString_);
    emit loadingFinished (false);
    return false;
  }

  pdfData_= file.readAll ();
  file.close ();

  return renderCurrentPage ();
}

bool
QTPdfPreviewWidget::loadFromData (const QByteArray& data, int dpi) {
  cancelLoading ();

  // Clear key since we can't cache data without a persistent identifier
  currentKey_.clear ();
  targetDpi_      = dpi;
  currentPage_    = 0;
  pageCount_      = 0;
  pageAspectRatio_= kDefaultAspectRatio;
  hasError_       = false;
  errorString_.clear ();
  pdfData_= data;

  setControlsVisible (false);

  return renderCurrentPage ();
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
  previewLabel_->setPixmap (QPixmap ());
  if (text.isEmpty ()) {
    previewLabel_->setText (qt_translate ("No Preview"));
  }
  else {
    previewLabel_->setText (text);
  }
  updatePreviewSize ();
}

void
QTPdfPreviewWidget::showLoading () {
  isLoading_= true;
  previewLabel_->setText (qt_translate ("Loading..."));
  updatePreviewSize ();
  emit loadingStarted ();
}

void
QTPdfPreviewWidget::showError (const QString& message) {
  isLoading_= false;
  hasError_ = true;
  previewLabel_->setText (message);
  updatePreviewSize ();
  emit error (message);
  emit loadingFinished (false);
}

void
QTPdfPreviewWidget::setPreviewPixmap (const QPixmap& pixmap) {
  isLoading_= false;
  // 预览框大小由updatePreviewSize统一控制，翻页时仅替换图像避免”跳缩放”

  previewLabel_->setPixmap (pixmap);

  emit loadingFinished (true);
}

void
QTPdfPreviewWidget::goToPage (int page) {
  if (page < 0 || page >= pageCount_ || page == currentPage_) return;
  currentPage_= page;
  if (renderCurrentPage ()) {
    // renderPdfPage 成功时会调用 updatePageControls
    emit pageChanged (currentPage_);
  }
}

void
QTPdfPreviewWidget::goToPreviousPage () {
  goToPage (currentPage_ - 1);
}

void
QTPdfPreviewWidget::goToNextPage () {
  goToPage (currentPage_ + 1);
}

void
QTPdfPreviewWidget::onNetworkReplyFinished () {
  QPointer<QNetworkReply> reply= currentReply_;
  currentReply_                = nullptr;

  processNetworkReply (reply);
}

void
QTPdfPreviewWidget::processNetworkReply (QPointer<QNetworkReply> reply) {
  if (!reply) return;

  if (reply->error () != QNetworkReply::NoError) {
    errorString_=
        qt_translate ("Download failed: %1").arg (reply->errorString ());
    pdfData_.clear (); // 清理残留数据
    showError (errorString_);
    reply->deleteLater ();
    currentLoadType_= LoadType::None;
    return;
  }

  pdfData_= reply->readAll ();

  // Extract HTTP cache headers BEFORE deleteLater to avoid use-after-free
  QString   etag= QString::fromUtf8 (reply->rawHeader ("ETag"));
  QDateTime lastModified;
  QString   lastModStr= QString::fromUtf8 (reply->rawHeader ("Last-Modified"));
  if (!lastModStr.isEmpty ()) {
    // RFC 2822 / HTTP-date format: "Thu, 29 Jan 2026 11:32:54 GMT"
    // Use RFC2822Date with C locale to ensure consistent parsing
    lastModified= QLocale::c ().toDateTime (lastModStr,
                                            "ddd, dd MMM yyyy hh:mm:ss 'GMT'");
    if (!lastModified.isValid ()) {
      // Fallback to Qt's built-in RFC2822 parser
      lastModified= QDateTime::fromString (lastModStr, Qt::RFC2822Date);
    }
    if (lastModified.isValid ()) {
      lastModified.setTimeZone (QTimeZone::utc ());
    }
  }

  reply->deleteLater ();

  if (pdfData_.isEmpty ()) {
    errorString_= qt_translate ("Empty PDF data received");
    showError (errorString_);
    currentLoadType_= LoadType::None;
    return;
  }

  // Save to cache with HTTP metadata
  PdfFileCache::instance ()->saveToCache (currentKey_, pdfData_, etag,
                                          lastModified);
  s_validatedPdfUrls.insert (currentKey_);
  qDebug () << "[PDF Preview] Update cache:" << currentKey_;

  renderCurrentPage ();
  currentLoadType_= LoadType::None;
}

void
QTPdfPreviewWidget::onConditionalReplyFinished (const QString& cachedFilePath,
                                                int            dpi) {
  QPointer<QNetworkReply> reply= currentReply_;
  currentReply_                = nullptr;

  if (!reply) return;

  // 304 Not Modified - use cached file
  if (reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt () ==
      304) {
    qDebug () << "[PDF Preview] Cache fresh:" << currentKey_;
    s_validatedPdfUrls.insert (currentKey_);
    reply->deleteLater ();
    loadFromFile (cachedFilePath, dpi);
    return;
  }

  // Error or 200 OK - process the reply directly
  processNetworkReply (reply);
}

bool
QTPdfPreviewWidget::renderCurrentPage () {
  return renderPdfPage (pdfData_, currentPage_);
}

bool
QTPdfPreviewWidget::renderPdfPage (const QByteArray& data, int pageNumber) {
  fz_context* ctx= mupdf_context ();
  if (!ctx) {
    qWarning () << "MuPDF context not available";
    errorString_= qt_translate ("PDF engine not available");
    showError (errorString_);
    return false;
  }

  // 文档处理器仅在成功后标记完成，失败可在后续渲染时重试
  static std::mutex registerMutex;
  static bool       handlersRegistered= false;
  if (!handlersRegistered) {
    QString registerError;
    {
      std::lock_guard<std::mutex> lock (registerMutex);
      if (!handlersRegistered) {
        fz_try (ctx) {
          fz_register_document_handlers (ctx);
          handlersRegistered= true;
        }
        fz_catch (ctx) {
          registerError= QString::fromUtf8 (fz_caught_message (ctx));
          qWarning () << "Failed to register document handlers:"
                      << registerError;
        }
      }
    }
    if (!handlersRegistered) {
      errorString_= qt_translate ("Failed to initialize PDF handlers: %1")
                        .arg (registerError);
      showError (errorString_);
      return false;
    }
  }

  fz_document* doc    = nullptr;
  fz_pixmap*   pix    = nullptr;
  fz_page*     page   = nullptr;
  fz_buffer*   buf    = nullptr;
  fz_stream*   stream = nullptr;
  bool         success= false;

  fz_var (doc);
  fz_var (pix);
  fz_var (page);
  fz_var (buf);
  fz_var (stream);

  fz_try (ctx) {
    buf= fz_new_buffer_from_copied_data (
        ctx, reinterpret_cast<const unsigned char*> (data.constData ()),
        data.size ());

    stream= fz_open_buffer (ctx, buf);
    doc   = fz_open_document_with_stream (ctx, "pdf", stream);

    if (!doc) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to open PDF document");
    }

    int pageCount= fz_count_pages (ctx, doc);
    if (pageCount <= 0) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "PDF has no pages");
    }

    pageCount_= pageCount;

    if (pageNumber < 0 || pageNumber >= pageCount) {
      pageNumber= 0;
    }
    currentPage_= pageNumber;

    page= fz_load_page (ctx, doc, pageNumber);
    if (!page) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to load page %d", pageNumber);
    }

    // 获取页面边界
    fz_rect bbox= fz_bound_page (ctx, page);

    // 计算宽高比
    float pageWidth = bbox.x1 - bbox.x0;
    float pageHeight= bbox.y1 - bbox.y0;
    if (pageHeight > 0 &&
        (pageAspectRatio_ <= 0 || pageNumber == 0 || pageCount_ <= 1)) {
      pageAspectRatio_= pageWidth / pageHeight;
    }

    // 计算目标尺寸
    updatePreviewSize ();
    QSize targetSize= previewLabel_->size ();
    qreal dpr       = previewLabel_->devicePixelRatioF ();
    int   targetPxW = qMax (1, qRound (targetSize.width () * dpr));
    int   targetPxH = qMax (1, qRound (targetSize.height () * dpr));

    // 按目标尺寸计算渲染比例（参考通用MuPDF用法）
    float scaleX= static_cast<float> (targetPxW) / pageWidth;
    float scaleY= static_cast<float> (targetPxH) / pageHeight;
    float scale = qMin (scaleX, scaleY);
    float qualityScale=
        qMax (1.0F, static_cast<float> (targetDpi_) / DEFAULT_DPI);
    float renderScale=
        qBound (kMinRenderScale, scale * kRenderOversample * qualityScale,
                kMaxRenderScale);

    fz_matrix ctm= fz_scale (renderScale, renderScale);

    pix= fz_new_pixmap_from_page (ctx, page, ctm, fz_device_rgb (ctx), 0);
    if (!pix) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to render page");
    }

    int            pixW   = fz_pixmap_width (ctx, pix);
    int            pixH   = fz_pixmap_height (ctx, pix);
    int            stride = fz_pixmap_stride (ctx, pix);
    int            comps  = pix->n;
    unsigned char* samples= fz_pixmap_samples (ctx, pix);

    QImage image;
    if (comps == 3) {
      QImage tempImage (samples, pixW, pixH, stride, QImage::Format_RGB888);
      image= tempImage.copy ();
    }
    else if (comps == 4) {
      QImage tempImage (samples, pixW, pixH, stride,
                        QImage::Format_RGBA8888_Premultiplied);
      image= tempImage.copy ();
    }
    else {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Unsupported pixmap format (n=%d)",
                comps);
    }

    if (image.isNull ()) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to convert to image");
    }

    // 缩放到目标显示区域，避免尺寸溢出并保持页面完整可见
    QPixmap pixmap= QPixmap::fromImage (std::move (image));
    pixmap        = pixmap.scaled (targetPxW, targetPxH, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    pixmap.setDevicePixelRatio (dpr);
    // Show the rendered pixmap
    setPreviewPixmap (pixmap);
    success= true;

    updatePageControls ();

    // 渲染完成后，如果鼠标已经在预览区域上，显示控制按钮
    if (previewContainer_ && previewContainer_->underMouse ()) {
      setControlsVisible (true);
    }
  }
  fz_catch (ctx) {
    qWarning () << "MuPDF error:" << fz_caught_message (ctx);
    errorString_= qt_translate ("PDF render error: %1")
                      .arg (QString::fromUtf8 (fz_caught_message (ctx)));
    showError (errorString_);
    success= false;
  }

  if (pix) fz_drop_pixmap (ctx, pix);
  if (page) fz_drop_page (ctx, page);
  if (stream) fz_drop_stream (ctx, stream);
  if (buf) fz_drop_buffer (ctx, buf);
  if (doc) fz_drop_document (ctx, doc);

  return success;
}

void
QTPdfPreviewWidget::loadImageFromUrl (const QString& url) {
  cancelLoading ();

  currentLoadType_= LoadType::Image;
  hasError_       = false;
  errorString_.clear ();
  pdfData_.clear ();
  pageCount_  = 0;
  currentPage_= 0;

  setControlsVisible (false);
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
    errorString_=
        qt_translate ("Image download failed: %1").arg (reply->errorString ());
    showError (errorString_);
    reply->deleteLater ();
    currentLoadType_= LoadType::None;
    return;
  }

  QByteArray imageData= reply->readAll ();
  reply->deleteLater ();

  if (imageData.isEmpty ()) {
    errorString_= qt_translate ("Received empty image data");
    showError (errorString_);
    currentLoadType_= LoadType::None;
    return;
  }

  QPixmap pixmap;
  if (pixmap.loadFromData (imageData)) {
    updatePreviewSize ();
    QSize displaySize= previewLabel_->size ();
    qreal dpr        = previewLabel_->devicePixelRatioF ();
    int   targetPxW  = qMax (1, qRound (displaySize.width () * dpr));
    int   targetPxH  = qMax (1, qRound (displaySize.height () * dpr));
    pixmap           = pixmap.scaled (targetPxW, targetPxH, Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation);
    pixmap.setDevicePixelRatio (dpr);
    setPreviewPixmap (pixmap);
  }
  else {
    errorString_= qt_translate ("Failed to load image data");
    showError (errorString_);
  }

  currentLoadType_= LoadType::None;
}

void
QTPdfPreviewWidget::updateButtonPositions () {
  if (!previewContainer_ || !previewLabel_) return;

  auto clampPosition= [] (int containerSize, int itemSize, int preferredPos) {
    int maxPos= containerSize - itemSize - kButtonOffset;
    if (maxPos < kButtonOffset) {
      return qMax (0, (containerSize - itemSize) / 2);
    }
    return qBound (kButtonOffset, preferredPos, maxPos);
  };

  // 获取预览标签在容器中的位置
  QPoint labelPos   = previewLabel_->mapTo (previewContainer_, QPoint (0, 0));
  int    labelWidth = previewLabel_->width ();
  int    labelHeight= previewLabel_->height ();
  int    containerWidth = previewContainer_->width ();
  int    containerHeight= previewContainer_->height ();

  // 上一页按钮 - 固定覆盖在预览区左侧
  if (prevBtn_) {
    int btnX= labelPos.x () + kButtonOffset;
    int btnY= labelPos.y () + (labelHeight - prevBtn_->height ()) / 2;
    btnX    = clampPosition (containerWidth, prevBtn_->width (), btnX);
    btnY    = clampPosition (containerHeight, prevBtn_->height (), btnY);
    prevBtn_->move (btnX, btnY);
  }

  // 下一页按钮 - 固定覆盖在预览区右侧
  if (nextBtn_) {
    int btnX= labelPos.x () + labelWidth - nextBtn_->width () - kButtonOffset;
    int btnY= labelPos.y () + (labelHeight - nextBtn_->height ()) / 2;
    btnX    = clampPosition (containerWidth, nextBtn_->width (), btnX);
    btnY    = clampPosition (containerHeight, nextBtn_->height (), btnY);
    nextBtn_->move (btnX, btnY);
  }

  // 页码指示器 - 底部居中（位置计算，setControlsVisible 控制显示）
  if (pageIndicator_) {
    int indicatorX= labelPos.x () + (labelWidth - pageIndicator_->width ()) / 2;
    int indicatorY= labelPos.y () + labelHeight - pageIndicator_->height () -
                    kPageIndicatorBottomMargin;
    indicatorX=
        clampPosition (containerWidth, pageIndicator_->width (), indicatorX);
    indicatorY=
        clampPosition (containerHeight, pageIndicator_->height (), indicatorY);
    pageIndicator_->move (indicatorX, indicatorY);
  }
}

void
QTPdfPreviewWidget::setControlsVisible (bool visible) {
  // 只有多页PDF时才显示控制按钮
  bool showControls= visible && (pageCount_ > 1);

  if (prevBtn_) {
    prevBtn_->setVisible (showControls);
  }
  if (nextBtn_) {
    nextBtn_->setVisible (showControls);
  }
  if (pageIndicator_) {
    pageIndicator_->setVisible (showControls);
  }
}

bool
QTPdfPreviewWidget::mouseInWidgetHierarchy () const {
  return (previewContainer_ && previewContainer_->underMouse ()) ||
         (prevBtn_ && prevBtn_->underMouse ()) ||
         (nextBtn_ && nextBtn_->underMouse ()) ||
         (pageIndicator_ && pageIndicator_->underMouse ()) ||
         (previewLabel_ && previewLabel_->underMouse ());
}

bool
QTPdfPreviewWidget::eventFilter (QObject* watched, QEvent* event) {
  // 统一处理所有监控控件的事件
  bool isMonitoredWidget= (watched == previewContainer_) ||
                          (watched == prevBtn_) || (watched == nextBtn_) ||
                          (watched == pageIndicator_) ||
                          (watched == previewLabel_);

  if (isMonitoredWidget) {
    switch (event->type ()) {
    case QEvent::HoverEnter:
    case QEvent::Enter:
    case QEvent::HoverMove:
      setControlsVisible (true);
      break;
    case QEvent::HoverLeave:
    case QEvent::Leave:
      // 延迟检查，确保不是进入了其他相关控件
      QTimer::singleShot (50, this, [this] () {
        if (!mouseInWidgetHierarchy ()) {
          setControlsVisible (false);
        }
      });
      break;
    default:
      break;
    }
  }

  return QWidget::eventFilter (watched, event);
}

void
QTPdfPreviewWidget::resizeEvent (QResizeEvent* event) {
  QWidget::resizeEvent (event);
  updatePreviewSize ();
  updateButtonPositions ();
}
