
/******************************************************************************
 * MODULE     : qt_pdf_preview_widget.cpp
 * DESCRIPTION: PDF preview widget with hover navigation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_pdf_preview_widget.hpp"

#include <QDebug>
#include <QFile>
#include <QHBoxLayout>
#include <QHoverEvent>
#include <QNetworkReply>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

#include <mutex>

#include "MuPDF/mupdf_renderer.hpp"
#include "pdf_preview_cache.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include <mupdf/fitz.h>

// 常量定义
namespace {
constexpr int    kMinRenderDpi             = 200;
constexpr int    kMaxRenderDpi             = 600;
constexpr int    kDefaultDpi               = 72;
constexpr int    kMargin                   = 32;
constexpr int    kMinPreviewWidth          = 400;
constexpr int    kMinPreviewHeight         = 400;
constexpr double kDefaultAspectRatio       = 1.414; // A4比例
constexpr int    kButtonOffset             = 10;
constexpr int    kPageIndicatorBottomMargin= 10;
constexpr int    kButtonBaseSize           = 28;
constexpr int    kButtonMinSize            = 24;
constexpr int    kButtonMaxSize            = 36;
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
  int scaledSize=
      qBound (kButtonMinSize,
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
  previewContainer_->setAttribute (Qt::WA_Hover, true);
  previewContainer_->setStyleSheet ("background: #fafafa; border-radius: 8px;");
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

  // 显示初始占位符
  clearPreview (qt_translate ("No Preview"));
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
  if (availWidth <= 0 || availHeight <= 0 || pageAspectRatio_ <= 0) {
    outWidth = kMinPreviewWidth;
    outHeight= kMinPreviewHeight;
    return;
  }

  double availRatio= static_cast<double> (availWidth) / availHeight;

  if (pageAspectRatio_ > availRatio) {
    // 页面比视口更宽，以宽度为准
    outWidth = availWidth;
    outHeight= static_cast<int> (outWidth / pageAspectRatio_);
  }
  else {
    // 页面比视口更高，以高度为准
    outHeight= availHeight;
    outWidth = static_cast<int> (outHeight * pageAspectRatio_);
  }

  // 保持在可用区域内，避免预览被强制放大
  outWidth = qMax (1, qMin (outWidth, availWidth));
  outHeight= qMax (1, qMin (outHeight, availHeight));
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
  if (!previewContainer_) return;

  int availWidth = qMax (1, previewContainer_->width () - kMargin);
  int availHeight= qMax (1, previewContainer_->height () - kMargin);

  int previewWidth, previewHeight;
  calculatePreviewDimensions (availWidth, availHeight, previewWidth,
                              previewHeight);

  previewLabel_->setFixedSize (previewWidth, previewHeight);
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
  hasError_       = false;
  errorString_.clear ();
  pdfData_.clear ();

  setControlsVisible (false);

  // Check cache first
  QPixmap cached= PdfPreviewCache::instance ()->get (url, currentPage_, dpi);
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
QTPdfPreviewWidget::loadFromFile (const QString& filePath, int dpi) {
  cancelLoading ();

  // Store key for caching
  currentKey_ = filePath;
  targetDpi_  = dpi;
  currentPage_= 0;
  pageCount_  = 0;
  hasError_   = false;
  errorString_.clear ();
  pdfData_.clear ();

  setControlsVisible (false);

  // Check cache first
  QPixmap cached= PdfPreviewCache::instance ()->get (filePath, currentPage_, dpi);
  if (!cached.isNull ()) {
    setPreviewPixmap (cached);
    return true;
  }

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
  targetDpi_  = dpi;
  currentPage_= 0;
  pageCount_  = 0;
  hasError_   = false;
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
  // 设置默认大小为最小预览尺寸，避免无内容时标签过大
  previewLabel_->setFixedSize (DpiUtils::scaled (kMinPreviewHeight),
                               DpiUtils::scaled (kMinPreviewWidth));
}

void
QTPdfPreviewWidget::showLoading () {
  isLoading_= true;
  previewLabel_->setText (qt_translate ("Loading..."));
  // 设置默认大小，避免文本显示过大
  previewLabel_->setFixedSize (DpiUtils::scaled (kMinPreviewHeight),
                               DpiUtils::scaled (kMinPreviewWidth));
  emit loadingStarted ();
}

void
QTPdfPreviewWidget::showError (const QString& message) {
  isLoading_= false;
  hasError_ = true;
  previewLabel_->setText (message);
  // 设置默认大小，避免错误文本显示过大
  previewLabel_->setFixedSize (DpiUtils::scaled (kMinPreviewHeight),
                               DpiUtils::scaled (kMinPreviewWidth));
  emit error (message);
  emit loadingFinished (false);
}

void
QTPdfPreviewWidget::setPreviewPixmap (const QPixmap& pixmap) {
  isLoading_= false;
  // 预览框大小由updatePreviewSize统一控制，翻页时仅替换图像避免“跳缩放”
  previewLabel_->setPixmap (pixmap);
  emit loadingFinished (true);
}

void
QTPdfPreviewWidget::goToPreviousPage () {
  if (currentPage_ > 0) {
    currentPage_--;
    renderCurrentPage ();
    updatePageControls ();
    emit pageChanged (currentPage_);
  }
}

void
QTPdfPreviewWidget::goToNextPage () {
  if (currentPage_ < pageCount_ - 1) {
    currentPage_++;
    renderCurrentPage ();
    updatePageControls ();
    emit pageChanged (currentPage_);
  }
}

void
QTPdfPreviewWidget::onNetworkReplyFinished () {
  QPointer<QNetworkReply> reply= currentReply_;
  currentReply_                = nullptr;

  if (!reply) return;

  if (reply->error () != QNetworkReply::NoError) {
    errorString_=
        qt_translate ("Download failed: %1").arg (reply->errorString ());
    showError (errorString_);
    reply->deleteLater ();
    currentLoadType_= LoadType::None;
    return;
  }

  pdfData_= reply->readAll ();
  reply->deleteLater ();

  if (pdfData_.isEmpty ()) {
    errorString_= qt_translate ("Empty PDF data received");
    showError (errorString_);
    currentLoadType_= LoadType::None;
    return;
  }

  renderCurrentPage ();
  currentLoadType_= LoadType::None;
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

  // 使用std::call_once确保文档处理器只注册一次
  static std::once_flag registerFlag;
  std::call_once (registerFlag, [ctx] () {
    fz_try (ctx) { fz_register_document_handlers (ctx); }
    fz_catch (ctx) {
      qWarning () << "Failed to register document handlers:"
                  << fz_caught_message (ctx);
    }
  });

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
    if (pageHeight > 0) {
      pageAspectRatio_= pageWidth / pageHeight;
    }

    // 计算目标尺寸
    updatePreviewSize ();
    QSize targetSize= previewLabel_->size ();

    // 根据目标尺寸计算需要的 DPI - 使用更高的DPI以保证清晰度
    float scaleX= static_cast<float> (targetSize.width ()) / pageWidth;
    float scaleY= static_cast<float> (targetSize.height ()) / pageHeight;
    float scale = qMin (scaleX, scaleY);

    // 使用DpiUtils获取屏幕缩放比例
    qreal screenScale= DpiUtils::scaleFactor (this->screen ());

    // 先按目标尺寸估算，再做过采样渲染，最后缩放到显示尺寸以提升清晰度
    int renderDpi= static_cast<int> (kDefaultDpi * scale * screenScale * 2.0);
    renderDpi    = qBound (kMinRenderDpi, renderDpi, kMaxRenderDpi);

    fz_matrix ctm= fz_scale (static_cast<float> (renderDpi) / kDefaultDpi,
                             static_cast<float> (renderDpi) / kDefaultDpi);

    pix= fz_new_pixmap_from_page (ctx, page, ctm, fz_device_rgb (ctx), 0);
    if (!pix) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to render page");
    }

    int            pixW   = fz_pixmap_width (ctx, pix);
    int            pixH   = fz_pixmap_height (ctx, pix);
    int            stride = fz_pixmap_stride (ctx, pix);
    unsigned char* samples= fz_pixmap_samples (ctx, pix);

    // 使用QImage直接引用MuPDF的像素数据（零拷贝），然后深拷贝到独立的QImage
    QImage tempImage (samples, pixW, pixH, stride, QImage::Format_RGB888);
    QImage image= tempImage.copy (); // 深拷贝，确保数据独立

    if (image.isNull ()) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to convert to image");
    }

    // 缩放到目标显示区域，避免尺寸溢出并保持页面完整可见
    QPixmap pixmap= QPixmap::fromImage (std::move (image));
    pixmap= pixmap.scaled (targetSize, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);
    setPreviewPixmap (pixmap);
    success= true;

    // Cache the rendered page for future use
    if (!currentKey_.isEmpty ()) {
      PdfPreviewCache::instance ()->put (currentKey_, currentPage_, targetDpi_,
                                         pixmap, true);
    }

    updatePageControls ();
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
QTPdfPreviewWidget::loadImageFromUrl (const QString& url,
                                      const QSize&   targetSize) {
  cancelLoading ();

  currentLoadType_= LoadType::Image;
  if (targetSize.isValid ()) {
    targetSize_= targetSize;
  }
  else {
    targetSize_= QSize (800, 600);
  }

  hasError_= false;
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
    pixmap= pixmap.scaled (displaySize.width (), displaySize.height (),
                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

  // 获取预览标签在容器中的位置
  QPoint labelPos   = previewLabel_->mapTo (previewContainer_, QPoint (0, 0));
  int    labelWidth = previewLabel_->width ();
  int    labelHeight= previewLabel_->height ();
  int    containerWidth = previewContainer_->width ();
  int    containerHeight= previewContainer_->height ();

  // 上一页按钮 - 以按钮中心与页面中心线对齐
  if (prevBtn_) {
    int btnCenterX= labelPos.x () - kButtonOffset;
    int btnCenterY= labelPos.y () + labelHeight / 2;
    int btnX= btnCenterX - prevBtn_->width () / 2;
    int btnY= btnCenterY - prevBtn_->height () / 2;
    btnX= qBound (kButtonOffset, btnX,
                  containerWidth - prevBtn_->width () - kButtonOffset);
    btnY= qBound (kButtonOffset, btnY,
                  containerHeight - prevBtn_->height () - kButtonOffset);
    prevBtn_->move (btnX, btnY);
  }

  // 下一页按钮 - 以按钮中心与页面中心线对齐
  if (nextBtn_) {
    int btnCenterX= labelPos.x () + labelWidth + kButtonOffset;
    int btnCenterY= labelPos.y () + labelHeight / 2;
    int btnX= btnCenterX - nextBtn_->width () / 2;
    int btnY= btnCenterY - nextBtn_->height () / 2;
    btnX= qBound (kButtonOffset, btnX,
                  containerWidth - nextBtn_->width () - kButtonOffset);
    btnY= qBound (kButtonOffset, btnY,
                  containerHeight - nextBtn_->height () - kButtonOffset);
    nextBtn_->move (btnX, btnY);
  }

  // 页码指示器 - 底部居中
  if (pageIndicator_ && pageCount_ > 1) {
    int indicatorX= labelPos.x () + (labelWidth - pageIndicator_->width ()) / 2;
    int indicatorY= labelPos.y () + labelHeight - pageIndicator_->height () -
                    kPageIndicatorBottomMargin;
    indicatorX= qBound (kButtonOffset, indicatorX,
                        containerWidth - pageIndicator_->width () - kButtonOffset);
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
QTPdfPreviewWidget::eventFilter (QObject* watched, QEvent* event) {
  if (watched != previewContainer_) {
    return QWidget::eventFilter (watched, event);
  }

  switch (event->type ()) {
  case QEvent::HoverEnter:
  case QEvent::Enter:
    setControlsVisible (true);
    break;
  case QEvent::HoverLeave:
  case QEvent::Leave:
    setControlsVisible (false);
    break;
  default:
    break;
  }

  return QWidget::eventFilter (watched, event);
}

void
QTPdfPreviewWidget::resizeEvent (QResizeEvent* event) {
  QWidget::resizeEvent (event);
  updatePreviewSize ();
  updateButtonPositions ();
}
