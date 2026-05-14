
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget.cpp
 * DESCRIPTION: Continuous-scroll PDF reader widget with toolbar
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#include "qt_pdf_reader_widget.hpp"

#include <QDebug>
#include <QFile>
#include <QKeyEvent>
#include <QLineEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include "MuPDF/mupdf_renderer.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include <mupdf/fitz.h>

#include <mutex>

namespace {
constexpr float kRenderOversample= 1.5F;
constexpr float kMinRenderScale  = 0.1F;
constexpr float kMaxRenderScale  = 8.0F;
} // namespace

PDFReaderWidget::PDFReaderWidget (QWidget* parent)
    : QWidget (parent), scrollArea_ (nullptr), contentWidget_ (nullptr),
      pageLayout_ (nullptr), mainLayout_ (nullptr), toolBar_ (nullptr),
      zoomCombo_ (nullptr), prevPageBtn_ (nullptr), pageEdit_ (nullptr),
      pageTotalLabel_ (nullptr), nextPageBtn_ (nullptr), pageCount_ (0),
      hasError_ (false), targetDpi_ (DEFAULT_DPI), zoomFactor_ (1.0),
      pageAspectRatio_ (0.0) {

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
  scrollArea_->viewport ()->installEventFilter (this);

  mainLayout_->addWidget (scrollArea_);

  setupToolBar ();

  connect (scrollArea_->verticalScrollBar (), &QScrollBar::valueChanged, this,
           &PDFReaderWidget::updatePageNavigation);
}

PDFReaderWidget::~PDFReaderWidget () {}

void
PDFReaderWidget::setupToolBar () {
  toolBar_= new QToolBar (this);
  toolBar_->setMovable (false);

  zoomCombo_= new QComboBox (toolBar_);
  zoomCombo_->setEditable (true);
  zoomCombo_->lineEdit ()->setReadOnly (true);
  zoomCombo_->lineEdit ()->setAlignment (Qt::AlignCenter);
  zoomCombo_->setMinimumWidth (80);

  zoomCombo_->addItem ("Fit Width");
  zoomCombo_->addItem ("Fit Height");
  zoomCombo_->addItem ("50%");
  zoomCombo_->addItem ("75%");
  zoomCombo_->addItem ("100%");
  zoomCombo_->addItem ("125%");
  zoomCombo_->addItem ("150%");
  zoomCombo_->addItem ("200%");
  zoomCombo_->addItem ("300%");
  zoomCombo_->addItem ("400%");

  connect (zoomCombo_, QOverload<int>::of (&QComboBox::currentIndexChanged),
           this, &PDFReaderWidget::onZoomChanged);

  toolBar_->addWidget (zoomCombo_);
  toolBar_->addSeparator ();

  prevPageBtn_= new QPushButton ("<", toolBar_);
  prevPageBtn_->setFixedWidth (30);
  connect (prevPageBtn_, &QPushButton::clicked, this,
           &PDFReaderWidget::onPrevPage);

  pageEdit_= new QLineEdit (toolBar_);
  pageEdit_->setFixedWidth (40);
  pageEdit_->setAlignment (Qt::AlignCenter);
  connect (pageEdit_, &QLineEdit::editingFinished, this,
           &PDFReaderWidget::onPageEditingFinished);

  pageTotalLabel_= new QLabel ("of 0", toolBar_);

  nextPageBtn_= new QPushButton (">", toolBar_);
  nextPageBtn_->setFixedWidth (30);
  connect (nextPageBtn_, &QPushButton::clicked, this,
           &PDFReaderWidget::onNextPage);

  toolBar_->addWidget (prevPageBtn_);
  toolBar_->addWidget (pageEdit_);
  toolBar_->addWidget (pageTotalLabel_);
  toolBar_->addWidget (nextPageBtn_);

  mainLayout_->insertWidget (0, toolBar_);
}

void
PDFReaderWidget::updateZoomDisplay () {
  if (!zoomCombo_) return;

  int    percent= qRound (zoomFactor_ * 100);
  QString text  = QString::number (percent) + "%";

  disconnect (zoomCombo_,
              QOverload<int>::of (&QComboBox::currentIndexChanged), this,
              &PDFReaderWidget::onZoomChanged);

  if (qFuzzyCompare (zoomFactor_, 1.0)) {
    int idx= zoomCombo_->findText ("Fit Width");
    if (idx >= 0) zoomCombo_->setCurrentIndex (idx);
  }
  else {
    int idx= zoomCombo_->findText (text);
    if (idx >= 0)
      zoomCombo_->setCurrentIndex (idx);
    else
      zoomCombo_->setCurrentText (text);
  }

  connect (zoomCombo_, QOverload<int>::of (&QComboBox::currentIndexChanged),
           this, &PDFReaderWidget::onZoomChanged);
}

void
PDFReaderWidget::onZoomChanged (int index) {
  if (index < 0) return;
  QString text= zoomCombo_->itemText (index);
  if (text == "Fit Width") {
    fitWidth ();
  }
  else if (text == "Fit Height") {
    fitHeight ();
  }
  else {
    QString numStr= text;
    numStr.chop (1);
    bool   ok;
    double percent= numStr.toDouble (&ok);
    if (ok) setZoomFactor (percent / 100.0);
  }
}

void
PDFReaderWidget::setZoomFactor (double factor) {
  zoomFactor_= qBound (MIN_ZOOM, factor, MAX_ZOOM);
  if (!pdfData_.isEmpty () && pageCount_ > 0) {
    rebuildPages ();
  }
  updateZoomDisplay ();
}

void
PDFReaderWidget::fitWidth () {
  setZoomFactor (1.0);
}

void
PDFReaderWidget::fitHeight () {
  if (pageAspectRatio_ <= 0) return;
  int baseWidth     = scrollArea_->viewport ()->width () - PAGE_MARGIN * 2;
  int viewportHeight= scrollArea_->viewport ()->height ();
  if (baseWidth <= 0) return;
  double targetZoom=
      static_cast<double> (viewportHeight) / (baseWidth * pageAspectRatio_);
  setZoomFactor (targetZoom);
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
  page= qBound (1, page, pageCount_);
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
  if (!pageEdit_ || !pageTotalLabel_ || !prevPageBtn_ || !nextPageBtn_) return;

  int current= currentPage ();
  pageEdit_->setText (QString::number (current));
  pageTotalLabel_->setText (QString ("of %1").arg (pageCount_));

  prevPageBtn_->setEnabled (current > 1);
  nextPageBtn_->setEnabled (current < pageCount_);
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

void
PDFReaderWidget::onPageEditingFinished () {
  bool ok;
  int  page= pageEdit_->text ().toInt (&ok);
  if (ok) goToPage (page);
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
  int baseWidth= scrollArea_->viewport ()->width () - PAGE_MARGIN * 2;
  return qMax (1, qRound (baseWidth * zoomFactor_));
}

bool
PDFReaderWidget::renderPageToLabel (int pageNumber, QLabel* label,
                                    int targetWidth) {
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

    fz_rect bbox       = fz_bound_page (ctx, page);
    float   pageWidth  = bbox.x1 - bbox.x0;
    float   pageHeight = bbox.y1 - bbox.y0;
    float   aspectRatio= pageHeight / pageWidth;

    int targetHeight= qMax (1, qRound (targetWidth * aspectRatio));

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
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= pageLayout_->itemAt (i);
    if (!item) continue;
    QLabel* label= qobject_cast<QLabel*> (item->widget ());
    if (label) {
      renderPageToLabel (i, label, width);
    }
  }
}

bool
PDFReaderWidget::loadFromFile (const QString& filePath, int dpi) {
  clear ();

  targetDpi_       = dpi;
  hasError_        = false;
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
        fz_page* page= fz_load_page (ctx, doc, 0);
        if (page) {
          fz_rect bbox    = fz_bound_page (ctx, page);
          pageAspectRatio_= (bbox.y1 - bbox.y0) / (bbox.x1 - bbox.x0);
          fz_drop_page (ctx, page);
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

  int width= pageWidth ();
  for (int i= 0; i < pageCount_; ++i) {
    QLabel* label= new QLabel (contentWidget_);
    label->setAlignment (Qt::AlignCenter);
    label->setAutoFillBackground (true);
    label->setBackgroundRole (QPalette::Base);
    label->setStyleSheet ("QLabel { border: 1px solid #cccccc; }");
    renderPageToLabel (i, label, width);
    pageLayout_->addWidget (label);
  }

  pageLayout_->addStretch (1);
  contentWidget_->adjustSize ();
  updateZoomDisplay ();
  updatePageNavigation ();
  return true;
}

void
PDFReaderWidget::clear () {
  pdfData_.clear ();
  pageCount_= 0;
  hasError_ = false;
  errorString_.clear ();
  pageAspectRatio_= 0.0;

  QLayoutItem* item;
  while ((item= pageLayout_->takeAt (0)) != nullptr) {
    if (item->widget ()) {
      delete item->widget ();
    }
    delete item;
  }

  updatePageNavigation ();
}

bool
PDFReaderWidget::eventFilter (QObject* watched, QEvent* event) {
  if (watched == scrollArea_->viewport ()) {
    if (event->type () == QEvent::Wheel) {
      QWheelEvent* wheelEvent= static_cast<QWheelEvent*> (event);
      if (wheelEvent->modifiers () & Qt::ControlModifier) {
        int delta= wheelEvent->angleDelta ().y ();
        if (delta != 0) {
          if (delta > 0) {
            zoomFactor_= qMin (zoomFactor_ + ZOOM_STEP, MAX_ZOOM);
          }
          else {
            zoomFactor_= qMax (zoomFactor_ - ZOOM_STEP, MIN_ZOOM);
          }
          if (!pdfData_.isEmpty () && pageCount_ > 0) {
            rebuildPages ();
          }
          updateZoomDisplay ();
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
          int scrollAmount=
              qRound (scrollArea_->viewport ()->height () * 0.9);
          vbar->setValue (vbar->value () + scrollAmount);
        }
        return true;
      }
    }
    else if (event->type () == QEvent::Resize) {
      if (!pdfData_.isEmpty () && pageCount_ > 0) {
        rebuildPages ();
      }
    }
  }
  return QWidget::eventFilter (watched, event);
}
