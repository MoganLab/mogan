
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget.cpp
 * DESCRIPTION: Continuous-scroll PDF reader widget with toolbar
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#include "qt_pdf_reader_widget.hpp"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGestureEvent>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QPinchGesture>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollBar>
#include <QUrl>
#include <QWheelEvent>

#ifdef Q_OS_MACOS
#include <QNativeGestureEvent>
#endif

#include "MuPDF/mupdf_renderer.hpp"
#include "qt_chat_tab_widget.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include <mupdf/fitz.h>

#include <mutex>

namespace {
constexpr float kRenderOversample= 1.5F;
constexpr float kMinRenderScale  = 0.1F;
constexpr float kMaxRenderScale  = 8.0F;

/**
 * @brief Check if the zoom modifier key is pressed.
 *
 * On macOS, the standard zoom modifier is Cmd (MetaModifier).
 * We also accept Ctrl (ControlModifier) for compatibility.
 * On other platforms, only Ctrl is accepted.
 */
bool
isZoomModifier (Qt::KeyboardModifiers modifiers) {
#ifdef Q_OS_MACOS
  return (modifiers & Qt::MetaModifier) || (modifiers & Qt::ControlModifier);
#else
  return modifiers & Qt::ControlModifier;
#endif
}
} // namespace

PDFReaderWidget::PDFReaderWidget (QWidget* parent)
    : QWidget (parent), scrollArea_ (nullptr), contentWidget_ (nullptr),
      pageLayout_ (nullptr), mainLayout_ (nullptr), rubberBand_ (nullptr),
      rectSelectMode_ (false), rectSelectDragging_ (false),
      hintLabel_ (nullptr), browseDragging_ (false), browseDragActive_ (false),
      scroller_ (nullptr), pageCount_ (0), hasError_ (false),
      targetDpi_ (DEFAULT_DPI), zoomFactor_ (1.0), pageAspectRatio_ (0.0),
      pageBaseWidthPts_ (0.0), overLink_ (false), zoomDebounceTimer_ (nullptr),
      resizeDebounceTimer_ (nullptr), gestureSafetyTimer_ (nullptr),
      inPinchGesture_ (false), blockRender_ (false), autoFitApplied_ (false),
      pinchStartZoom_ (1.0), zoomAnchorContentY_ (0.0),
      zoomAnchorViewportY_ (0.0), zoomAnchorOldZoom_ (1.0),
      hasZoomAnchor_ (false), renderCallCount_ (0) {

  mainLayout_= new QVBoxLayout (this);
  mainLayout_->setContentsMargins (0, 0, 0, 0);
  mainLayout_->setSpacing (0);

  scrollArea_= new QScrollArea (this);
  scrollArea_->setWidgetResizable (true);
  scrollArea_->setFrameShape (QFrame::NoFrame);
  scrollArea_->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

  contentWidget_= new QWidget (scrollArea_);
  contentWidget_->setAutoFillBackground (true);
  contentWidget_->setBackgroundRole (QPalette::Mid);

  pageLayout_= new QVBoxLayout (contentWidget_);
  pageLayout_->setContentsMargins (PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN,
                                   PAGE_MARGIN);
  pageLayout_->setSpacing (PAGE_MARGIN);
  pageLayout_->setAlignment (Qt::AlignHCenter);

  scrollArea_->setWidget (contentWidget_);

  // QScroller 配置（参考 Okular）
  // QScroller::scroller() installs its own eventFilter on the viewport,
  // which intercepts MouseMove events before our eventFilter can process them
  // for link hover detection. We manually drive QScroller via handleInput()
  // in our own eventFilter, so we remove QScroller's automatic eventFilter
  // to gain full control over the event flow.
  scroller_= QScroller::scroller (scrollArea_->viewport ());
  QScrollerProperties prop;
  prop.setScrollMetric (QScrollerProperties::DecelerationFactor, 0.3);
  prop.setScrollMetric (QScrollerProperties::MaximumVelocity, 1.0);
  prop.setScrollMetric (QScrollerProperties::AcceleratingFlickMaximumTime, 0.2);
  prop.setScrollMetric (QScrollerProperties::HorizontalOvershootPolicy,
                        QScrollerProperties::OvershootAlwaysOff);
  prop.setScrollMetric (QScrollerProperties::VerticalOvershootPolicy,
                        QScrollerProperties::OvershootAlwaysOff);
  prop.setScrollMetric (QScrollerProperties::DragStartDistance, 0.0);
  scroller_->setScrollerProperties (prop);

  // Remove QScroller's eventFilter — we manually forward events via
  // handleInput()
  scrollArea_->viewport ()->removeEventFilter (scroller_);

  // Make contentWidget and its children transparent to mouse events so that
  // all mouse events go directly to the viewport, where our eventFilter handles
  // link hover/click, drag-to-scroll, and rect selection.
  contentWidget_->setAttribute (Qt::WA_TransparentForMouseEvents);

  scrollArea_->viewport ()->installEventFilter (this);
  scrollArea_->viewport ()->setMouseTracking (true);
  scrollArea_->viewport ()->setCursor (Qt::OpenHandCursor);
  grabGesture (Qt::PinchGesture);

  // 保持与 QScrollArea 内部一致的步长（Okular 同款 magic value）
  scrollArea_->verticalScrollBar ()->setSingleStep (20);
  scrollArea_->horizontalScrollBar ()->setSingleStep (20);

  // 滚动条与 QScroller 同步（Okular 同款）
  auto syncScroller= [this] () {
    QScrollBar* hbar= scrollArea_->horizontalScrollBar ();
    QScrollBar* vbar= scrollArea_->verticalScrollBar ();
    scroller_->scrollTo (QPoint (hbar->value (), vbar->value ()), 0);
  };
  connect (scrollArea_->verticalScrollBar (), &QAbstractSlider::actionTriggered,
           this, syncScroller, Qt::QueuedConnection);
  connect (scrollArea_->horizontalScrollBar (),
           &QAbstractSlider::actionTriggered, this, syncScroller,
           Qt::QueuedConnection);

  mainLayout_->addWidget (scrollArea_);

  connect (scrollArea_->verticalScrollBar (), &QScrollBar::valueChanged, this,
           &PDFReaderWidget::updatePageNavigation);
  connect (scrollArea_->verticalScrollBar (), &QScrollBar::valueChanged, this,
           &PDFReaderWidget::rebuildPages);

  // 缩放防抖定时器
  zoomDebounceTimer_= new QTimer (this);
  zoomDebounceTimer_->setSingleShot (true);
  zoomDebounceTimer_->setInterval (ZOOM_DEBOUNCE_MS);
  connect (zoomDebounceTimer_, &QTimer::timeout, this,
           &PDFReaderWidget::rebuildPages);

  // Resize 防抖定时器
  resizeDebounceTimer_= new QTimer (this);
  resizeDebounceTimer_->setSingleShot (true);
  resizeDebounceTimer_->setInterval (RESIZE_DEBOUNCE_MS);
  connect (resizeDebounceTimer_, &QTimer::timeout, this,
           &PDFReaderWidget::onResizeDebounced);

  gestureSafetyTimer_= new QTimer (this);
  gestureSafetyTimer_->setSingleShot (true);
  gestureSafetyTimer_->setInterval (GESTURE_SAFETY_TIMEOUT_MS);
  connect (gestureSafetyTimer_, &QTimer::timeout, this,
           &PDFReaderWidget::finishPinchGesture);
}

PDFReaderWidget::~PDFReaderWidget () {}

void
PDFReaderWidget::updateZoomDisplay () {
  int     percent= qRound (zoomFactor_ * 100);
  QString text   = QString::number (percent) + "%";
  Q_EMIT zoomChanged (text);
}

void
PDFReaderWidget::onResizeDebounced () {
  // 当窗口离开半屏贴靠状态时，重置自动适配标志，
  // 以便下次贴靠到左/右半屏时仍能触发 Fit Width
  if (autoFitApplied_) {
    QScreen* screen= this->screen ();
    if (!screen) screen= QApplication::primaryScreen ();
    if (screen) {
      QRect screenGeo= screen->availableGeometry ();
      int   screenW  = screenGeo.width ();
      QRect winGeo   = window ()->frameGeometry ();
      int   halfWidth= screenW / 2;
      int   tolerance= qMax (20, screenW / 20);
      if (qAbs (winGeo.width () - halfWidth) > tolerance) {
        autoFitApplied_= false;
      }
    }
  }

  if (!maybeAutoFitWidth ()) {
    rebuildPages ();
  }
}

bool
PDFReaderWidget::maybeAutoFitWidth () {
  if (autoFitApplied_) return false;
  if (pdfData_.isEmpty () || pageCount_ <= 0) return false;
  if (pageBaseWidthPts_ <= 0) return false;
  if (isMaximized () || isFullScreen ()) return false;

  QScreen* screen= this->screen ();
  if (!screen) screen= QApplication::primaryScreen ();
  if (!screen) return false;

  QRect screenGeo= screen->availableGeometry ();
  int   screenW  = screenGeo.width ();
  int   screenH  = screenGeo.height ();
  QRect winGeo   = window ()->frameGeometry ();

  // 判断是否贴靠到左半屏或右半屏：
  // 1. 宽度约等于屏幕宽度的一半
  // 2. 高度约等于屏幕可用高度
  // 3. 窗口左边缘贴近屏幕左边缘（左半屏）或
  //    窗口右边缘贴近屏幕右边缘（右半屏）
  int halfWidth      = screenW / 2;
  int widthTolerance = qMax (20, screenW / 20);
  int heightTolerance= qMax (40, screenH / 20);

  if (qAbs (winGeo.width () - halfWidth) > widthTolerance) return false;
  if (qAbs (winGeo.height () - screenH) > heightTolerance) return false;

  bool snappedLeft = qAbs (winGeo.x () - screenGeo.x ()) <= 10;
  bool snappedRight= qAbs ((winGeo.x () + winGeo.width ()) -
                           (screenGeo.x () + screenGeo.width ())) <= 10;

  if (snappedLeft || snappedRight) {
    fitWidth ();
    autoFitApplied_= true;
    return true;
  }
  return false;
}

void
PDFReaderWidget::applyZoomToLabels () {
  int width= pageWidth ();
  if (width <= 0) return;

  int childCount= pageLayout_->count ();
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QLabel* label= qobject_cast<QLabel*> (item->widget ());
    if (!label) continue;
    double aspect= (i < pageAspectRatios_.size ()) ? pageAspectRatios_[i]
                                                   : pageAspectRatio_;
    if (aspect <= 0.0) aspect= 1.414;
    int height= qMax (1, qRound (width * aspect));
    label->setFixedSize (width, height);
  }
}

void
PDFReaderWidget::startPinchGesture () {
  if (inPinchGesture_) return;
  inPinchGesture_= true;
  blockRender_   = true;
  pinchStartZoom_= zoomFactor_;
  if (scroller_) scroller_->stop ();
  int childCount= pageLayout_->count ();
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QLabel* label= qobject_cast<QLabel*> (item->widget ());
    if (label) label->setScaledContents (true);
  }
}

void
PDFReaderWidget::finishPinchGesture () {
  if (!inPinchGesture_) return;
  inPinchGesture_= false;
  blockRender_   = false;

  // Sync-render the correctly-sized pixmap before turning off
  // scaledContents, so the label does not flicker from the old
  // stretched image back to a mismatched original pixmap.
  if (!pdfData_.isEmpty () && pageCount_ > 0) rebuildPages ();
  updateZoomDisplay ();

  int childCount= pageLayout_->count ();
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QLabel* label= qobject_cast<QLabel*> (item->widget ());
    if (label) label->setScaledContents (false);
  }
}

void
PDFReaderWidget::simulatePinchGesture (Qt::GestureState state,
                                       double           scaleFactor) {
  if (state == Qt::GestureStarted) {
    startPinchGesture ();
    return;
  }
  if (state == Qt::GestureUpdated) {
    double newZoom= qBound (MIN_ZOOM, pinchStartZoom_ * scaleFactor, MAX_ZOOM);
    if (qAbs (newZoom - zoomFactor_) > 0.001) {
      zoomFactor_= newZoom;
      applyZoomToLabels ();
    }
    return;
  }
  if (state == Qt::GestureFinished || state == Qt::GestureCanceled) {
    finishPinchGesture ();
  }
  int percent= qRound (zoomFactor_ * 100);
  Q_EMIT zoomChanged (QString::number (percent) + "%");
}

void
PDFReaderWidget::setZoomFactor (double factor) {
  if (pdfData_.isEmpty () || pageCount_ <= 0) {
    zoomFactor_= qBound (MIN_ZOOM, factor, MAX_ZOOM);
    updateZoomDisplay ();
    return;
  }
  // Save anchor at viewport center before zoom
  int    vpHeight= scrollArea_->viewport ()->height ();
  QPoint vpCenter (scrollArea_->viewport ()->width () / 2, vpHeight / 2);
  saveZoomAnchor (vpCenter);

  zoomFactor_= qBound (MIN_ZOOM, factor, MAX_ZOOM);
  updateZoomDisplay ();
  if (!pdfData_.isEmpty () && pageCount_ > 0) {
    zoomDebounceTimer_->start ();
  }
}

void
PDFReaderWidget::fitWidth () {
  if (pageBaseWidthPts_ <= 0) {
    setZoomFactor (1.0);
    return;
  }
  int      viewportWidth= scrollArea_->viewport ()->width () - PAGE_MARGIN * 2;
  QScreen* screen       = this->screen ();
  if (!screen) screen= QApplication::primaryScreen ();
  qreal screenDpi= screen ? screen->logicalDotsPerInch () : 96.0;
  int   baseWidth= qRound (pageBaseWidthPts_ * screenDpi / 72.0);
  if (baseWidth <= 0) return;
  setZoomFactor (static_cast<double> (viewportWidth) / baseWidth);
}

void
PDFReaderWidget::fitHeight () {
  if (pageBaseWidthPts_ <= 0 || pageAspectRatio_ <= 0) return;
  int      viewportHeight= scrollArea_->viewport ()->height ();
  QScreen* screen        = this->screen ();
  if (!screen) screen= QApplication::primaryScreen ();
  qreal screenDpi = screen ? screen->logicalDotsPerInch () : 96.0;
  int   baseWidth = qRound (pageBaseWidthPts_ * screenDpi / 72.0);
  int   baseHeight= qRound (baseWidth * pageAspectRatio_);
  if (baseHeight <= 0) return;
  setZoomFactor (static_cast<double> (viewportHeight) / baseHeight);
}

void
PDFReaderWidget::zoomIn () {
  for (int i= 0; i < ZOOM_LEVEL_COUNT; ++i) {
    if (ZOOM_LEVELS[i] > zoomFactor_ * 1.001) {
      setZoomFactor (ZOOM_LEVELS[i]);
      return;
    }
  }
  setZoomFactor (MAX_ZOOM);
}

void
PDFReaderWidget::zoomOut () {
  for (int i= ZOOM_LEVEL_COUNT - 1; i >= 0; --i) {
    if (ZOOM_LEVELS[i] < zoomFactor_ * 0.999) {
      setZoomFactor (ZOOM_LEVELS[i]);
      return;
    }
  }
  setZoomFactor (MIN_ZOOM);
}

int
PDFReaderWidget::currentPage () const {
  if (!scrollArea_ || pageCount_ <= 0) return 0;

  int scrollY= scrollArea_->verticalScrollBar ()->value ();

  int childCount= pageLayout_->count ();
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QWidget* w= item->widget ();
    if (!w) continue;
    if (w->y () + w->height () > scrollY) {
      return i + 1;
    }
  }
  return pageCount_;
}

void
PDFReaderWidget::goToPage (int page) {
  page     = qBound (1, page, pageCount_);
  int index= page - 1;

  int childCount= pageLayout_->count ();
  if (index < 0 || index >= childCount) return;

  QLayoutItem* item= pageLayout_->itemAt (index);
  if (!item) return;
  QWidget* w= item->widget ();
  if (!w) return;

  scrollArea_->verticalScrollBar ()->setValue (w->y ());
}

void
PDFReaderWidget::updatePageNavigation () {
  int current= currentPage ();
  Q_EMIT pageChanged (current, pageCount_);
}

void
PDFReaderWidget::onPrevPage () {
  int page= currentPage () - 1;
  if (page >= 1) goToPage (page);
}

void
PDFReaderWidget::onNextPage () {
  int page= currentPage () + 1;
  if (page <= pageCount_) goToPage (page);
}

bool
PDFReaderWidget::isRectSelectMode () const {
  return rectSelectMode_;
}

void
PDFReaderWidget::setRectSelectMode (bool checked) {
  rectSelectMode_= checked;
  if (scrollArea_ && scrollArea_->viewport ()) {
    QWidget* vp= scrollArea_->viewport ();
    vp->setMouseTracking (true);
    vp->setCursor (rectSelectMode_ ? Qt::CrossCursor : Qt::OpenHandCursor);
  }
  if (!rectSelectMode_ && rubberBand_) {
    rubberBand_->hide ();
    delete rubberBand_;
    rubberBand_= nullptr;
  }
  rectSelectDragging_= false;

  if (rectSelectMode_) {
    if (!hintLabel_) {
      hintLabel_= new QLabel (contentWidget_);
      hintLabel_->setObjectName ("rectSelectHint");
      hintLabel_->setStyleSheet (
          "QLabel { background-color: rgba(0, 0, 0, 180); color: white; "
          "padding: 4px 8px; border-radius: 4px; font-size: 12px; }");
    }
#ifdef Q_OS_MACOS
    QString shortcut= "Cmd+Shift+v";
#else
    QString shortcut= "Ctrl+Shift+v";
#endif
    hintLabel_->setText (
        QString ("Draw a rectangle and use %1 to magic paste!").arg (shortcut));
    hintLabel_->adjustSize ();
    hintLabel_->move (PAGE_MARGIN, PAGE_MARGIN);
    hintLabel_->show ();
  }
  else if (hintLabel_) {
    hintLabel_->hide ();
  }

  Q_EMIT rectSelectModeChanged (checked);
}

void
PDFReaderWidget::finishRectSelect (const QPoint& viewportPos) {
  if (!rubberBand_ || !contentWidget_) return;

  QRect rubberRect= rubberBand_->geometry ();
  rubberBand_->hide ();

  // 将 rubber band 的 geometry（contentWidget_ 坐标）转换为内容坐标
  QPoint contentTopLeft    = rubberRect.topLeft ();
  QPoint contentBottomRight= rubberRect.bottomRight ();

  QRect contentRect (contentTopLeft, contentBottomRight);
  if (contentRect.width () <= 0 || contentRect.height () <= 0) return;

  QLabel* label= findPageLabelAt (contentRect.center ());
  if (!label) return;

  QPixmap selected= extractSelectionPixmap (label, contentRect);
  if (selected.isNull ()) return;

  QClipboard* clipboard= QApplication::clipboard ();
  if (clipboard) {
    clipboard->setPixmap (selected);
  }

  if (hintLabel_) {
    hintLabel_->setText ("Copied to Clipboard!");
    hintLabel_->adjustSize ();
  }
}

QLabel*
PDFReaderWidget::findPageLabelAt (const QPoint& contentPos) const {
  int childCount= pageLayout_->count ();
  for (int i= 0; i < childCount; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QWidget* w= item->widget ();
    if (!w) continue;
    QLabel* label= qobject_cast<QLabel*> (w);
    if (!label) continue;
    if (label->geometry ().contains (contentPos)) {
      return label;
    }
  }
  return nullptr;
}

QPixmap
PDFReaderWidget::extractSelectionPixmap (QLabel*      label,
                                         const QRect& contentRect) const {
  if (!label) return QPixmap ();

  QPixmap pm= label->pixmap ();
  if (pm.isNull ()) return QPixmap ();

  // 计算选择区域相对于 label 的坐标
  QRect labelRect= label->geometry ();
  QRect intersect= contentRect.intersected (labelRect);
  if (intersect.isEmpty ()) return QPixmap ();

  int relX= intersect.x () - labelRect.x ();
  int relY= intersect.y () - labelRect.y ();
  int relW= intersect.width ();
  int relH= intersect.height ();

  qreal dpr = pm.devicePixelRatio ();
  int   srcX= qRound (relX * dpr);
  int   srcY= qRound (relY * dpr);
  int   srcW= qRound (relW * dpr);
  int   srcH= qRound (relH * dpr);

  QPixmap copied= pm.copy (srcX, srcY, srcW, srcH);
  copied.setDevicePixelRatio (1.0);
  return copied;
}

bool
PDFReaderWidget::canGoToPrevPage () const {
  return currentPage () > 1;
}

bool
PDFReaderWidget::canGoToNextPage () const {
  return currentPage () < pageCount_;
}

QWidget*
PDFReaderWidget::viewport () const {
  return scrollArea_ ? scrollArea_->viewport () : nullptr;
}

QScrollBar*
PDFReaderWidget::verticalScrollBar () const {
  return scrollArea_ ? scrollArea_->verticalScrollBar () : nullptr;
}

int
PDFReaderWidget::pageWidth () const {
  if (pageBaseWidthPts_ <= 0) {
    int baseWidth= scrollArea_->viewport ()->width () - PAGE_MARGIN * 2;
    return qMax (1, qRound (baseWidth * zoomFactor_));
  }

  QScreen* screen= this->screen ();
  if (!screen) screen= QApplication::primaryScreen ();
  qreal screenDpi= screen ? screen->logicalDotsPerInch () : 96.0;
  int   baseWidth= qRound (pageBaseWidthPts_ * screenDpi / 72.0);
  return qMax (1, qRound (baseWidth * zoomFactor_));
}

bool
PDFReaderWidget::renderPageToLabel (int pageNumber, QLabel* label,
                                    int targetWidth) {
  ++renderCallCount_;
#ifdef LIII_DEBUG
  cout << "renderPageToLabel page=" << pageNumber << " width=" << targetWidth
       << "\n";
#endif
  // 计算目标高度（优先使用预缓存的宽高比）
  double aspectRatio= pageAspectRatio_;
  if (pageNumber >= 0 && pageNumber < pageAspectRatios_.size ()) {
    aspectRatio= pageAspectRatios_[pageNumber];
  }
  if (aspectRatio <= 0.0) aspectRatio= 1.414;
  int targetHeight= qMax (1, qRound (targetWidth * aspectRatio));

  // 尝试从缓存读取
  PdfPageCacheKey key{pageNumber, targetWidth};
  auto            it= pageCache_.find (key);
  if (it != pageCache_.end ()) {
    QPixmap cached= it.value ();
    qreal   dpr   = devicePixelRatioF ();
    int     pxW   = qMax (1, qRound (targetWidth * dpr));
    int     pxH   = qMax (1, qRound (targetHeight * dpr));
    if (cached.width () == pxW && cached.height () == pxH) {
      label->setPixmap (cached);
      label->setFixedSize (targetWidth, targetHeight);
      return true;
    }
    // 尺寸不匹配（如 DPR 变化），移除旧缓存
    pageCache_.erase (it);
  }

  fz_context* ctx= mupdf_context ();
  if (!ctx) {
    errorString_= qt_translate ("PDF engine not available");
    hasError_   = true;
    return false;
  }

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
        }
      }
    }
    if (!handlersRegistered) {
      errorString_= qt_translate ("Failed to initialize PDF handlers");
      hasError_   = true;
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
        ctx, reinterpret_cast<const unsigned char*> (pdfData_.constData ()),
        pdfData_.size ());

    stream= fz_open_buffer (ctx, buf);
    doc   = fz_open_document_with_stream (ctx, "pdf", stream);

    if (!doc) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to open PDF document");
    }

    int totalPages= fz_count_pages (ctx, doc);
    if (totalPages <= 0) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "PDF has no pages");
    }

    if (pageNumber < 0 || pageNumber >= totalPages) {
      pageNumber= 0;
    }

    page= fz_load_page (ctx, doc, pageNumber);
    if (!page) {
      fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to load page %d", pageNumber);
    }

    fz_rect bbox      = fz_bound_page (ctx, page);
    float   pageWidth = bbox.x1 - bbox.x0;
    float   pageHeight= bbox.y1 - bbox.y0;
    aspectRatio       = pageHeight / pageWidth;
    targetHeight      = qMax (1, qRound (targetWidth * aspectRatio));

    qreal dpr      = devicePixelRatioF ();
    int   targetPxW= qMax (1, qRound (targetWidth * dpr));
    int   targetPxH= qMax (1, qRound (targetHeight * dpr));

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

    QPixmap pixmap= QPixmap::fromImage (std::move (image));
    pixmap        = pixmap.scaled (targetPxW, targetPxH, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    pixmap.setDevicePixelRatio (dpr);

    label->setPixmap (pixmap);
    label->setFixedSize (targetWidth, targetHeight);

    // 写入缓存
    pageCache_.insert (key, pixmap);
    success= true;
  }
  fz_catch (ctx) {
    qWarning () << "MuPDF error:" << fz_caught_message (ctx);
    errorString_= qt_translate ("PDF render error");
    hasError_   = true;
    success     = false;
  }

  if (pix) fz_drop_pixmap (ctx, pix);
  if (page) fz_drop_page (ctx, page);
  if (stream) fz_drop_stream (ctx, stream);
  if (buf) fz_drop_buffer (ctx, buf);
  if (doc) fz_drop_document (ctx, doc);

  return success;
}

void
PDFReaderWidget::rebuildPages () {
  if (pdfData_.isEmpty () || pageCount_ <= 0) return;

  int width= pageWidth ();
  if (width <= 0) return;

  int childCount= pageLayout_->count ();

  // 第一轮：统一设置所有 label 的目标尺寸，保证布局正确
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QLabel* label= qobject_cast<QLabel*> (item->widget ());
    if (!label) continue;
    double aspect= (i < pageAspectRatios_.size ()) ? pageAspectRatios_[i]
                                                   : pageAspectRatio_;
    if (aspect <= 0.0) aspect= 1.414;
    int height= qMax (1, qRound (width * aspect));
    label->setFixedSize (width, height);
  }

  // Force the layout to recalculate so that scroll bar ranges are correct
  contentWidget_->adjustSize ();

  // Restore scroll position anchored to the saved content point
  restoreZoomAnchor ();

  // 计算当前视口范围（考虑预加载边距）
  int scrollY       = scrollArea_->verticalScrollBar ()->value ();
  int viewportHeight= scrollArea_->viewport ()->height ();
  int minY          = scrollY - PRELOAD_MARGIN;
  int maxY          = scrollY + viewportHeight + PRELOAD_MARGIN;

  if (blockRender_) return;

  // 第二轮：只渲染可见及预加载范围内的页面
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QLabel* label= qobject_cast<QLabel*> (item->widget ());
    if (!label) continue;

    double aspect= (i < pageAspectRatios_.size ()) ? pageAspectRatios_[i]
                                                   : pageAspectRatio_;
    if (aspect <= 0.0) aspect= 1.414;
    int height= qMax (1, qRound (width * aspect));

    // 使用理论 Y 坐标判断可见性（布局 spacing = PAGE_MARGIN）
    int labelTop   = PAGE_MARGIN + i * (height + PAGE_MARGIN);
    int labelBottom= labelTop + height;

    if (labelBottom >= minY && labelTop <= maxY) {
      renderPageToLabel (i, label, width);
    }
    else {
      // 视口外：尝试用缓存的降级版本显示，避免空白跳动
      PdfPageCacheKey key{i, width};
      auto            it= pageCache_.find (key);
      if (it != pageCache_.end ()) {
        QPixmap cached= it.value ();
        qreal   dpr   = devicePixelRatioF ();
        int     pxW   = qMax (1, qRound (width * dpr));
        int     pxH   = qMax (1, qRound (height * dpr));
        if (cached.width () != pxW || cached.height () != pxH) {
          cached= cached.scaled (pxW, pxH, Qt::KeepAspectRatio,
                                 Qt::FastTransformation);
          cached.setDevicePixelRatio (dpr);
        }
        label->setPixmap (cached);
      }
      else {
        label->clear ();
      }
    }
  }
}

bool
PDFReaderWidget::loadFromFile (const QString& filePath, int dpi) {
  clear ();
  autoFitApplied_= false;
  pdfFilePath_   = filePath;

  targetDpi_= dpi;
  hasError_ = false;
  errorString_.clear ();
  pageAspectRatio_= 0.0;

  QFile file (filePath);
  if (!file.open (QIODevice::ReadOnly)) {
    errorString_=
        qt_translate ("Cannot open file: %1").arg (file.errorString ());
    hasError_= true;
    return false;
  }

  pdfData_= file.readAll ();
  file.close ();

  fz_context* ctx= mupdf_context ();
  if (!ctx) {
    errorString_= qt_translate ("PDF engine not available");
    hasError_   = true;
    return false;
  }

  static std::mutex registerMutex;
  static bool       handlersRegistered= false;
  if (!handlersRegistered) {
    std::lock_guard<std::mutex> lock (registerMutex);
    if (!handlersRegistered) {
      fz_try (ctx) {
        fz_register_document_handlers (ctx);
        handlersRegistered= true;
      }
      fz_catch (ctx) {
        errorString_= qt_translate ("Failed to initialize PDF handlers");
        hasError_   = true;
        return false;
      }
    }
  }

  fz_document* doc   = nullptr;
  fz_buffer*   buf   = nullptr;
  fz_stream*   stream= nullptr;

  fz_var (doc);
  fz_var (buf);
  fz_var (stream);

  bool opened= false;
  fz_try (ctx) {
    buf= fz_new_buffer_from_copied_data (
        ctx, reinterpret_cast<const unsigned char*> (pdfData_.constData ()),
        pdfData_.size ());

    stream= fz_open_buffer (ctx, buf);
    doc   = fz_open_document_with_stream (ctx, "pdf", stream);

    if (doc) {
      pageCount_= fz_count_pages (ctx, doc);
      opened    = (pageCount_ > 0);
      if (opened && pageCount_ > 0) {
        pageAspectRatios_.reserve (pageCount_);
        for (int i= 0; i < pageCount_; ++i) {
          fz_page* page= fz_load_page (ctx, doc, i);
          if (page) {
            fz_rect bbox  = fz_bound_page (ctx, page);
            double  aspect= (bbox.y1 - bbox.y0) / (bbox.x1 - bbox.x0);
            pageAspectRatios_.append (aspect);
            if (i == 0) {
              pageBaseWidthPts_= bbox.x1 - bbox.x0;
              pageAspectRatio_ = aspect;
            }
            fz_drop_page (ctx, page);
          }
          else {
            pageAspectRatios_.append (1.414);
          }
        }
      }
    }
  }
  fz_catch (ctx) {
    errorString_= qt_translate ("Failed to open PDF");
    hasError_   = true;
  }

  if (stream) fz_drop_stream (ctx, stream);
  if (buf) fz_drop_buffer (ctx, buf);
  if (doc) fz_drop_document (ctx, doc);

  if (!opened) {
    if (!hasError_) {
      errorString_= qt_translate ("Failed to open PDF");
      hasError_   = true;
    }
    return false;
  }

  extractPageLinks ();

  // 创建所有页面 label（先不渲染，由 rebuildPages 统一处理可见性）
  for (int i= 0; i < pageCount_; ++i) {
    QLabel* label= new QLabel (contentWidget_);
    label->setAlignment (Qt::AlignCenter);
    label->setAutoFillBackground (true);
    label->setBackgroundRole (QPalette::Base);
    label->setStyleSheet ("QLabel { border: 1px solid #cccccc; }");
    label->setAttribute (Qt::WA_TransparentForMouseEvents);
    pageLayout_->addWidget (label);
  }

  pageLayout_->addStretch (1);
  maybeAutoFitWidth ();
  rebuildPages ();
  contentWidget_->adjustSize ();
  updateZoomDisplay ();
  updatePageNavigation ();
  return true;
}

void
PDFReaderWidget::clear () {
  pdfData_.clear ();
  pdfFilePath_.clear ();
  pageCount_= 0;
  hasError_ = false;
  errorString_.clear ();
  pageAspectRatio_ = 0.0;
  pageBaseWidthPts_= 0.0;
  pageAspectRatios_.clear ();
  autoFitApplied_= false;
  clearPageLinks ();
  pageCache_.clear ();

  QLayoutItem* item;
  while ((item= pageLayout_->takeAt (0)) != nullptr) {
    if (item->widget ()) {
      delete item->widget ();
    }
    delete item;
  }

  updatePageNavigation ();
}

void
PDFReaderWidget::extractPageLinks () {
  clearPageLinks ();
  if (pdfData_.isEmpty () || pageCount_ <= 0) return;

  fz_context* ctx= mupdf_context ();
  if (!ctx) return;

  fz_document* doc   = nullptr;
  fz_buffer*   buf   = nullptr;
  fz_stream*   stream= nullptr;

  fz_var (doc);
  fz_var (buf);
  fz_var (stream);

  fz_try (ctx) {
    buf= fz_new_buffer_from_copied_data (
        ctx, reinterpret_cast<const unsigned char*> (pdfData_.constData ()),
        pdfData_.size ());
    stream= fz_open_buffer (ctx, buf);
    doc   = fz_open_document_with_stream (ctx, "pdf", stream);
    if (!doc) fz_throw (ctx, FZ_ERROR_GENERIC, "Failed to open PDF");

    pageLinks_.resize (pageCount_);
    for (int i= 0; i < pageCount_; ++i) {
      fz_page* page= fz_load_page (ctx, doc, i);
      if (!page) continue;
      fz_link* links= fz_load_links (ctx, page);
      if (links) {
        fz_rect pageBounds= fz_bound_page (ctx, page);
        float   pageW     = pageBounds.x1 - pageBounds.x0;
        float   pageH     = pageBounds.y1 - pageBounds.y0;
        for (fz_link* link= links; link; link= link->next) {
          PdfLink pl;
          pl.uri = QString::fromUtf8 (link->uri);
          pl.page= -1;
          // normalized coordinates
          // MuPDF link rects and fz_bound_page both use the same coordinate
          // space, so no Y-flip is needed — just normalize relative to the
          // page box origin.
          if (pageW > 0 && pageH > 0) {
            pl.rect= QRectF ((link->rect.x0 - pageBounds.x0) / pageW,
                             (link->rect.y0 - pageBounds.y0) / pageH,
                             (link->rect.x1 - link->rect.x0) / pageW,
                             (link->rect.y1 - link->rect.y0) / pageH);
          }
          // Resolve internal links to page numbers
          if (pl.uri.startsWith ("#") || pl.uri.startsWith ("#nameddest=") ||
              pl.uri.startsWith ("#page=")) {
            float       xp= 0, yp= 0;
            fz_location loc= fz_resolve_link (ctx, doc, link->uri, &xp, &yp);
            if (loc.page >= 0) {
              pl.page= loc.page; // 0-based page index
            }
          }
          pageLinks_[i].append (pl);
        }
        fz_drop_link (ctx, links);
      }
      fz_drop_page (ctx, page);
    }
  }
  fz_catch (ctx) {
    qWarning () << "MuPDF link extraction error:" << fz_caught_message (ctx);
  }

  if (stream) fz_drop_stream (ctx, stream);
  if (buf) fz_drop_buffer (ctx, buf);
  if (doc) fz_drop_document (ctx, doc);
}

void
PDFReaderWidget::clearPageLinks () {
  pageLinks_.clear ();
  currentLink_= PdfLink ();
  overLink_   = false;
}

PdfLink
PDFReaderWidget::linkAtPos (const QPoint& contentPos) const {
  if (pageLinks_.isEmpty ()) return PdfLink ();

  int childCount= pageLayout_->count ();
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QLabel* label= qobject_cast<QLabel*> (item->widget ());
    if (!label) continue;
    QRect labelGeom= label->geometry ();
    if (!labelGeom.contains (contentPos)) continue;

    if (i < pageLinks_.size () && !pageLinks_[i].isEmpty ()) {
      QRect  contents= label->contentsRect ();
      QPoint labelLocal (contentPos.x () - labelGeom.x (),
                         contentPos.y () - labelGeom.y ());
      double nx= static_cast<double> (labelLocal.x () - contents.x ()) /
                 qMax (1, contents.width ());
      double ny= static_cast<double> (labelLocal.y () - contents.y ()) /
                 qMax (1, contents.height ());
      for (const PdfLink& link : pageLinks_[i]) {
        if (link.rect.contains (nx, ny)) {
          return link;
        }
      }
    }
  }
  return PdfLink ();
}

void
PDFReaderWidget::handleLinkClick (const PdfLink& link) {
  if (link.uri.isEmpty ()) return;

  // Internal link with resolved page number
  if (link.page >= 0) {
    goToPage (link.page + 1); // convert 0-based to 1-based
    Q_EMIT linkClicked (link.uri);
    return;
  }

  QUrl url (link.uri);
  if (url.isValid () && !url.scheme ().isEmpty () && url.scheme () != "file") {
    QDesktopServices::openUrl (url);
    Q_EMIT linkClicked (link.uri);
  }
  else {
    Q_EMIT linkClicked (link.uri);
  }
}

void
PDFReaderWidget::updateLinkCursor (const QPoint& contentPos) {
  if (rectSelectMode_) return;

  PdfLink link= linkAtPos (contentPos);
  if (!link.uri.isEmpty ()) {
    currentLink_= link;
    overLink_   = true;
    scrollArea_->viewport ()->setCursor (Qt::PointingHandCursor);
  }
  else {
    currentLink_= PdfLink ();
    overLink_   = false;
    scrollArea_->viewport ()->setCursor (Qt::OpenHandCursor);
  }
}

void
PDFReaderWidget::saveZoomAnchor (const QPoint& viewportPos) {
  // Only save on the first zoom event of a sequence (before any zoom change).
  // Subsequent wheel events reuse the same anchor so that restoreZoomAnchor
  // correctly computes the full zoom ratio from the original state.
  if (hasZoomAnchor_) return;
  QPoint contentPos=
      contentWidget_->mapFrom (scrollArea_->viewport (), viewportPos);
  zoomAnchorContentY_ = static_cast<double> (contentPos.y ());
  zoomAnchorViewportY_= static_cast<double> (viewportPos.y ());
  zoomAnchorOldZoom_  = zoomFactor_;
  hasZoomAnchor_      = true;
}

void
PDFReaderWidget::restoreZoomAnchor () {
  if (!hasZoomAnchor_) return;
  hasZoomAnchor_= false;

  // Read the actual new page height from the label that was just resized in
  // rebuildPages.  This avoids qRound mismatches between the saved anchor
  // and the real layout geometry.
  int pageIdx= 0;
  {
    double remaining= zoomAnchorContentY_ - PAGE_MARGIN;
    for (int i= 0; i < pageCount_ && i < pageLayout_->count (); ++i) {
      QLayoutItem* item= pageLayout_->itemAt (i);
      if (!item) continue;
      int h= item->widget ()->height ();
      if (remaining >= 0) pageIdx= i;
      remaining-= (h + PAGE_MARGIN);
    }
  }

  // Compute old page top/height from the anchor content Y by finding which
  // page the anchor falls on. We use the same formula rebuildPages uses.
  QScreen* screen= this->screen ();
  if (!screen) screen= QApplication::primaryScreen ();
  qreal dpi     = screen ? screen->logicalDotsPerInch () : 96.0;
  int   baseW   = (pageBaseWidthPts_ > 0)
                      ? qRound (pageBaseWidthPts_ * dpi / 72.0)
                      : scrollArea_->viewport ()->width () - PAGE_MARGIN * 2;
  int   oldPageW= qMax (1, qRound (baseW * zoomAnchorOldZoom_));
  int   oldPageH= 0;
  {
    double aspect= (pageIdx < pageAspectRatios_.size ())
                       ? pageAspectRatios_[pageIdx]
                       : pageAspectRatio_;
    if (aspect <= 0.0) aspect= 1.414;
    oldPageH= qMax (1, qRound (oldPageW * aspect));
  }
  double oldPageTop= PAGE_MARGIN + pageIdx * (oldPageH + PAGE_MARGIN);
  double offset    = zoomAnchorContentY_ - oldPageTop;

  // Read the actual new page height from the layout (just set by rebuildPages)
  int newPageH= 1;
  if (pageIdx < pageLayout_->count ()) {
    QLayoutItem* item= pageLayout_->itemAt (pageIdx);
    if (item && item->widget ()) newPageH= item->widget ()->height ();
  }
  int newPageTop= PAGE_MARGIN + pageIdx * (newPageH + PAGE_MARGIN);

  double zoomRatio=
      (oldPageH > 0) ? static_cast<double> (newPageH) / oldPageH : 1.0;
  double      contentY     = newPageTop + offset * zoomRatio;
  int         targetScrollY= qRound (contentY - zoomAnchorViewportY_);
  QScrollBar* vbar         = scrollArea_->verticalScrollBar ();
  targetScrollY            = qBound (0, targetScrollY, vbar->maximum ());
  vbar->setValue (targetScrollY);
}

void
PDFReaderWidget::setTestLinks (int page, const QVector<PdfLink>& links) {
  if (page < 0) return;
  if (page >= pageLinks_.size ()) pageLinks_.resize (page + 1);
  pageLinks_[page]= links;
}

bool
PDFReaderWidget::isOverLink () const {
  return overLink_;
}

void
PDFReaderWidget::keyPressEvent (QKeyEvent* event) {
  if (event->key () == Qt::Key_Space) {
    QScrollBar* vbar= scrollArea_->verticalScrollBar ();
    if (vbar) {
      int scrollAmount= qRound (scrollArea_->viewport ()->height () * 0.9);
      vbar->setValue (vbar->value () + scrollAmount);
    }
    event->accept ();
    return;
  }

  if (event->key () == Qt::Key_Escape) {
    if (rectSelectDragging_) {
      rectSelectDragging_= false;
      if (rubberBand_) rubberBand_->hide ();
      event->accept ();
      return;
    }
    if (rectSelectMode_) {
      setRectSelectMode (false);
      event->accept ();
      return;
    }
  }

  // Ctrl/Cmd+J：切换 AI 聊天侧边栏
  if (event->key () == Qt::Key_J &&
      (event->modifiers () & (Qt::ControlModifier | Qt::MetaModifier))) {
    QMainWindow* mw= qobject_cast<QMainWindow*> (window ());
    if (mw) {
      QDockWidget* dock= mw->findChild<QDockWidget*> ("chatSideDock");
      if (dock) {
        if (dock->isVisible ()) {
          QTChatTabWidget* chatWidget=
              qobject_cast<QTChatTabWidget*> (dock->widget ());
          if (chatWidget) emit chatWidget->closeSidebarRequested ();
        }
        else {
          QPushButton* toggleBtn=
              mw->findChild<QPushButton*> ("chat-tab-collapse-btn");
          if (toggleBtn) toggleBtn->click ();
        }
      }
    }
    event->accept ();
    return;
  }

  if (event->key () == Qt::Key_J || event->key () == Qt::Key_K) {
    QScrollBar* vbar= scrollArea_->verticalScrollBar ();
    if (vbar) {
      int direction= (event->key () == Qt::Key_J) ? 1 : -1;
      vbar->setValue (vbar->value () + direction * vbar->singleStep ());
    }
    event->accept ();
    return;
  }

  if (isZoomModifier (event->modifiers ())) {
    switch (event->key ()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
      zoomIn ();
      event->accept ();
      return;
    case Qt::Key_Minus:
      zoomOut ();
      event->accept ();
      return;
    case Qt::Key_0:
      setZoomFactor (1.0);
      event->accept ();
      return;
    }
  }

#ifdef Q_OS_MACOS
  bool closeModifier= (event->modifiers () & Qt::MetaModifier) ||
                      (event->modifiers () & Qt::ControlModifier);
#else
  bool closeModifier= event->modifiers () & Qt::ControlModifier;
#endif
  if (closeModifier && event->key () == Qt::Key_T) {
    eval ("(new-document)");
    event->accept ();
    return;
  }
  if (closeModifier && event->key () == Qt::Key_W) {
    eval ("(safely-kill-tabpage)");
    event->accept ();
    return;
  }

  if (closeModifier) {
    int key= event->key ();
    if (key >= Qt::Key_1 && key <= Qt::Key_9) {
      int index= key - Qt::Key_1;
      eval ("(switch-to-view-index " * as_string (index) * ")");
      event->accept ();
      return;
    }
  }

  QWidget::keyPressEvent (event);
}

bool
PDFReaderWidget::event (QEvent* event) {
  if (event->type () == QEvent::Gesture) {
    QGestureEvent* gestureEvent= static_cast<QGestureEvent*> (event);
    if (QPinchGesture* pinch= qobject_cast<QPinchGesture*> (
            gestureEvent->gesture (Qt::PinchGesture))) {
      // Handle QPinchGesture on all platforms (including macOS).
      // Qt 6 maps trackpad pinch to QPinchGesture on macOS as well.
      if (pinch->state () == Qt::GestureStarted) {
        startPinchGesture ();
        gestureSafetyTimer_->start ();
        gestureEvent->accept (pinch);
        return true;
      }
      if (pinch->changeFlags () & QPinchGesture::ScaleFactorChanged) {
        double newZoom= qBound (
            MIN_ZOOM, pinchStartZoom_ * pinch->totalScaleFactor (), MAX_ZOOM);
        if (qAbs (newZoom - zoomFactor_) > 0.001) {
          zoomFactor_= newZoom;
          applyZoomToLabels ();
        }
        gestureSafetyTimer_->start ();
        gestureEvent->accept (pinch);
        return true;
      }
      if (pinch->state () == Qt::GestureFinished ||
          pinch->state () == Qt::GestureCanceled) {
        gestureSafetyTimer_->stop ();
        finishPinchGesture ();
        gestureEvent->accept (pinch);
        return true;
      }
      gestureEvent->accept (pinch);
      return true;
    }
  }
#ifdef Q_OS_MACOS
  if (event->type () == QEvent::NativeGesture) {
    QNativeGestureEvent* nativeEvent= static_cast<QNativeGestureEvent*> (event);
    Qt::NativeGestureType gestureType= nativeEvent->gestureType ();
    // If QPinchGesture is already handling the pinch, ignore native
    // gesture to avoid double-scaling.
    if (inPinchGesture_ && (gestureType == Qt::BeginNativeGesture ||
                            gestureType == Qt::EndNativeGesture ||
                            gestureType == Qt::ZoomNativeGesture)) {
      return true;
    }
    if (gestureType == Qt::BeginNativeGesture) {
      startPinchGesture ();
      gestureSafetyTimer_->start ();
      return true;
    }
    if (gestureType == Qt::EndNativeGesture) {
      gestureSafetyTimer_->stop ();
      finishPinchGesture ();
      return true;
    }
    if (gestureType == Qt::ZoomNativeGesture) {
      if (!inPinchGesture_) startPinchGesture ();
      gestureSafetyTimer_->start ();
      double delta= nativeEvent->value ();
      if (qAbs (delta) > 0.001) {
        zoomFactor_= qBound (MIN_ZOOM, zoomFactor_ * (1.0 + delta), MAX_ZOOM);
        applyZoomToLabels ();
      }
      return true;
    }
  }
#endif
  return QWidget::event (event);
}

bool
PDFReaderWidget::eventFilter (QObject* watched, QEvent* event) {
  if (watched == scrollArea_->viewport ()) {
    // Pre-compute viewport and content coordinates for mouse events.
    QPoint viewportPos, contentPos;
    bool   isMouseEvent= (event->type () == QEvent::MouseMove ||
                        event->type () == QEvent::MouseButtonPress ||
                        event->type () == QEvent::MouseButtonRelease ||
                        event->type () == QEvent::MouseButtonDblClick);
    if (isMouseEvent) {
      QMouseEvent* me= static_cast<QMouseEvent*> (event);
      viewportPos    = me->pos ();
      contentPos=
          contentWidget_->mapFrom (scrollArea_->viewport (), me->pos ());
    }
    if (event->type () == QEvent::Wheel) {
      QWheelEvent* wheelEvent= static_cast<QWheelEvent*> (event);
      if (isZoomModifier (wheelEvent->modifiers ())) {
        int delta= wheelEvent->angleDelta ().y ();
        if (delta != 0) {
          // Save anchor at cursor position before zoom
          saveZoomAnchor (wheelEvent->position ().toPoint ());
          double factor= 1.0 + static_cast<double> (delta) / 500.0;
          zoomFactor_  = qBound (MIN_ZOOM, zoomFactor_ * factor, MAX_ZOOM);
          updateZoomDisplay ();
          if (!pdfData_.isEmpty () && pageCount_ > 0) {
            zoomDebounceTimer_->start ();
          }
        }
        wheelEvent->accept ();
        return true;
      }
    }
    else if (event->type () == QEvent::KeyPress) {
      QKeyEvent* keyEvent= static_cast<QKeyEvent*> (event);
      if (keyEvent->key () == Qt::Key_Space) {
        QScrollBar* vbar= scrollArea_->verticalScrollBar ();
        if (vbar) {
          int scrollAmount= qRound (scrollArea_->viewport ()->height () * 0.9);
          vbar->setValue (vbar->value () + scrollAmount);
        }
        return true;
      }
      if (keyEvent->key () == Qt::Key_J || keyEvent->key () == Qt::Key_K) {
        QScrollBar* vbar= scrollArea_->verticalScrollBar ();
        if (vbar) {
          int direction= (keyEvent->key () == Qt::Key_J) ? 1 : -1;
          vbar->setValue (vbar->value () + direction * vbar->singleStep ());
        }
        return true;
      }
      if (keyEvent->key () == Qt::Key_Escape) {
        if (rectSelectDragging_) {
          rectSelectDragging_= false;
          if (rubberBand_) rubberBand_->hide ();
          return true;
        }
        if (rectSelectMode_) {
          setRectSelectMode (false);
          return true;
        }
      }
#ifdef Q_OS_MACOS
      bool closeModifier= (keyEvent->modifiers () & Qt::MetaModifier) ||
                          (keyEvent->modifiers () & Qt::ControlModifier);
#else
      bool closeModifier= keyEvent->modifiers () & Qt::ControlModifier;
#endif
      if (closeModifier && keyEvent->key () == Qt::Key_W) {
        eval ("(safely-kill-tabpage)");
        return true;
      }
    }
    else if (event->type () == QEvent::Resize) {
      if (!pdfData_.isEmpty () && pageCount_ > 0) {
        resizeDebounceTimer_->start ();
      }
    }
    // ============================================================
    // Context menu (right-click)
    // ============================================================
    else if (event->type () == QEvent::ContextMenu) {
      QContextMenuEvent* contextEvent= static_cast<QContextMenuEvent*> (event);
      showContextMenu (contextEvent->pos ());
      contextEvent->accept ();
      return true;
    }
    // ============================================================
    // Link hover detection (no button pressed)
    // ============================================================
    else if (!rectSelectMode_ && !browseDragging_ &&
             event->type () == QEvent::MouseMove) {
      updateLinkCursor (contentPos);
    }
    // ============================================================
    // Browse (hand) tool: default drag-to-scroll behavior
    // ============================================================
    else if (!rectSelectMode_ &&
             (event->type () == QEvent::MouseButtonPress ||
              event->type () == QEvent::MouseButtonDblClick)) {
      QMouseEvent* mouseEvent= static_cast<QMouseEvent*> (event);
      if (mouseEvent->button () == Qt::LeftButton) {
        browseDragging_    = true;
        browseDragActive_  = false;
        browseDragStartPos_= mouseEvent->globalPosition ().toPoint ();
        scroller_->handleInput (QScroller::InputPress, viewportPos,
                                mouseEvent->timestamp ());
        scrollArea_->viewport ()->setCursor (Qt::ClosedHandCursor);
        mouseEvent->accept ();
        return true;
      }
    }
    else if (!rectSelectMode_ && browseDragging_ &&
             event->type () == QEvent::MouseMove) {
      QMouseEvent* mouseEvent= static_cast<QMouseEvent*> (event);
      int          delta=
          (mouseEvent->globalPosition ().toPoint () - browseDragStartPos_)
              .manhattanLength ();
      if (!browseDragActive_ && delta > QApplication::startDragDistance ()) {
        browseDragActive_= true;
      }
      if (!browseDragActive_) {
        updateLinkCursor (contentPos);
        if (!overLink_) {
          scrollArea_->viewport ()->setCursor (Qt::ClosedHandCursor);
        }
      }
      scroller_->handleInput (QScroller::InputMove, viewportPos,
                              mouseEvent->timestamp ());
      mouseEvent->accept ();
      return true;
    }
    else if (!rectSelectMode_ && browseDragging_ &&
             event->type () == QEvent::MouseButtonRelease) {
      QMouseEvent* mouseEvent= static_cast<QMouseEvent*> (event);
      if (mouseEvent->button () == Qt::LeftButton) {
        browseDragging_= false;
        scroller_->handleInput (QScroller::InputRelease, viewportPos,
                                mouseEvent->timestamp ());
        if (!browseDragActive_ && overLink_) {
          handleLinkClick (currentLink_);
        }
        scrollArea_->viewport ()->setCursor (Qt::OpenHandCursor);
        mouseEvent->accept ();
        return true;
      }
    }
    // ============================================================
    // Rectangular selection mode
    // ============================================================
    else if (rectSelectMode_ &&
             (event->type () == QEvent::MouseButtonPress ||
              event->type () == QEvent::MouseButtonDblClick)) {
      QMouseEvent* mouseEvent= static_cast<QMouseEvent*> (event);
      if (mouseEvent->button () == Qt::LeftButton) {
        rectSelectDragging_= true;
        rectSelectStart_   = contentPos;
        if (!rubberBand_) {
          rubberBand_= new QRubberBand (QRubberBand::Rectangle, contentWidget_);
        }
        rubberBand_->setGeometry (QRect (rectSelectStart_, QSize ()));
        rubberBand_->show ();
        mouseEvent->accept ();
        return true;
      }
    }
    else if (rectSelectMode_ && rectSelectDragging_ &&
             event->type () == QEvent::MouseMove) {
      QRect rect (rectSelectStart_, contentPos);
      rect= rect.normalized ();
      rubberBand_->setGeometry (rect);
      static_cast<QMouseEvent*> (event)->accept ();
      return true;
    }
    else if (rectSelectMode_ && rectSelectDragging_ &&
             event->type () == QEvent::MouseButtonRelease) {
      QMouseEvent* mouseEvent= static_cast<QMouseEvent*> (event);
      if (mouseEvent->button () == Qt::LeftButton) {
        rectSelectDragging_= false;
        finishRectSelect (mouseEvent->pos ());
        mouseEvent->accept ();
        return true;
      }
    }
  }
  return QWidget::eventFilter (watched, event);
}

void
PDFReaderWidget::showContextMenu (const QPoint& pos) {
  if (pdfFilePath_.isEmpty ()) return;
  QMenu    menu (this);
  QAction* saveAction= menu.addAction (qt_translate ("Save as..."));
  QAction* selected  = menu.exec (scrollArea_->viewport ()->mapToGlobal (pos));
  if (selected == saveAction) {
    QString dest= QFileDialog::getSaveFileName (
        this, qt_translate ("Save PDF file"), pdfFilePath_,
        qt_translate ("PDF files (*.pdf)"));
    if (!dest.isEmpty ()) {
      QFile::copy (pdfFilePath_, dest);
    }
  }
}
