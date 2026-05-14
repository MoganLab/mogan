
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget.cpp
 * DESCRIPTION: Continuous-scroll PDF reader widget
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#include "qt_pdf_reader_widget.hpp"

#include <QDebug>
#include <QFile>
#include <QKeyEvent>
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
    : QScrollArea (parent), contentWidget_ (nullptr), layout_ (nullptr),
      pageCount_ (0), hasError_ (false), targetDpi_ (DEFAULT_DPI),
      zoomFactor_ (1.0) {

  setWidgetResizable (true);
  setFrameShape (QFrame::NoFrame);
  setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

  contentWidget_= new QWidget (this);
  contentWidget_->setAutoFillBackground (true);
  contentWidget_->setBackgroundRole (QPalette::Mid);

  layout_= new QVBoxLayout (contentWidget_);
  layout_->setContentsMargins (PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN,
                               PAGE_MARGIN);
  layout_->setSpacing (PAGE_MARGIN);
  layout_->setAlignment (Qt::AlignHCenter);

  setWidget (contentWidget_);
}

PDFReaderWidget::~PDFReaderWidget () {}

int
PDFReaderWidget::pageWidth () const {
  int baseWidth= viewport ()->width () - PAGE_MARGIN * 2;
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

  int childCount= layout_->count ();
  for (int i= 0; i < childCount && i < pageCount_; ++i) {
    QLayoutItem* item= layout_->itemAt (i);
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

  targetDpi_= dpi;
  hasError_ = false;
  errorString_.clear ();

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
    layout_->addWidget (label);
  }

  layout_->addStretch (1);
  contentWidget_->adjustSize ();
  return true;
}

void
PDFReaderWidget::clear () {
  pdfData_.clear ();
  pageCount_= 0;
  hasError_ = false;
  errorString_.clear ();

  QLayoutItem* item;
  while ((item= layout_->takeAt (0)) != nullptr) {
    if (item->widget ()) {
      delete item->widget ();
    }
    delete item;
  }
}

void
PDFReaderWidget::resizeEvent (QResizeEvent* event) {
  QScrollArea::resizeEvent (event);
  if (!pdfData_.isEmpty () && pageCount_ > 0) {
    rebuildPages ();
  }
}

void
PDFReaderWidget::keyPressEvent (QKeyEvent* event) {
  if (event->key () == Qt::Key_Space) {
    QScrollBar* vbar= verticalScrollBar ();
    if (vbar) {
      int scrollAmount= qRound (viewport ()->height () * 0.9);
      vbar->setValue (vbar->value () + scrollAmount);
    }
    return;
  }
  QScrollArea::keyPressEvent (event);
}

void
PDFReaderWidget::wheelEvent (QWheelEvent* event) {
  if (event->modifiers () & Qt::ControlModifier) {
    int delta= event->angleDelta ().y ();
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
    }
    event->accept ();
    return;
  }
  QScrollArea::wheelEvent (event);
}
