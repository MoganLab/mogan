
/******************************************************************************
 * MODULE     : qt_pdf_toolbar.cpp
 * DESCRIPTION: Toolbar for the PDF reader, hosted in QMainWindow's toolbar area
 * COPYRIGHT  : (C) 2026 Da Shen
 *                  2026 Yifan Lu
 ******************************************************************************/

#include "qt_pdf_toolbar.hpp"

#include "qt_dpi_utils.hpp"
#include "qt_pdf_reader_widget.hpp"
#include "qt_utilities.hpp"

#include <QHBoxLayout>
#include <QSizePolicy>

namespace {
// -- widget sizes (base px, scaled at runtime) --
constexpr int kButtonSize    = 28;
constexpr int kIconSize      = 16;
constexpr int kComboHeight   = 26;
constexpr int kComboWidth    = 80;
constexpr int kDropBtnWidth  = 24;
constexpr int kPageEditWidth = 50;
constexpr int kPageTotalWidth= 45;
constexpr int kComboFontSize = 14;

// -- layout margins & spacing --
constexpr int kLayoutMarginLeft  = 4;
constexpr int kLayoutMarginTop   = 2;
constexpr int kLayoutMarginRight = 4;
constexpr int kLayoutMarginBottom= 2;
constexpr int kLayoutSpacing     = 4;
} // namespace

PdfToolBar::PdfToolBar (const QString& title, QWidget* parent)
    : QToolBar (title, parent) {
  setObjectName ("pdfToolBar");
  setMovable (false);
  setContextMenuPolicy (Qt::PreventContextMenu);
  setupWidgets ();
}

void
PdfToolBar::setupWidgets () {
  // -- zoom display --
  zoomCombo_= new QLineEdit (this);
  zoomCombo_->setObjectName ("pdf-zoom-edit");
  zoomCombo_->setFixedWidth (DpiUtils::scaled (kComboWidth));
  zoomCombo_->setFixedHeight (DpiUtils::scaled (kComboHeight));
  zoomCombo_->setFrame (false);
  zoomCombo_->setStyleSheet (
      "QLineEdit { padding: 0px; margin: 0px; border: 0.5px solid #CCC; }");
  zoomCombo_->setAlignment (Qt::AlignCenter);
  zoomCombo_->setReadOnly (true);

  QFont comboFont= zoomCombo_->font ();
  comboFont.setPixelSize (DpiUtils::scaled (kComboFontSize));
  zoomCombo_->setFont (comboFont);

  // -- zoom dropdown button + menu --
  zoomDropBtn_= new QToolButton (this);
  zoomDropBtn_->setObjectName ("pdf-zoom-drop-btn");
  zoomDropBtn_->setAutoRaise (true);
  zoomDropBtn_->setFixedSize (DpiUtils::scaled (kDropBtnWidth),
                              DpiUtils::scaled (kComboHeight));
  zoomDropBtn_->setArrowType (Qt::DownArrow);
  zoomDropBtn_->setToolTip (qt_translate ("Zoom"));

  zoomMenu_= new QMenu (zoomDropBtn_);
  zoomMenu_->addAction ("Fit Width");
  zoomMenu_->addAction ("Fit Height");
  zoomMenu_->addAction ("25%");
  zoomMenu_->addAction ("33%");
  zoomMenu_->addAction ("50%");
  zoomMenu_->addAction ("75%");
  zoomMenu_->addAction ("100%");
  zoomMenu_->addAction ("125%");
  zoomMenu_->addAction ("150%");
  zoomMenu_->addAction ("200%");
  zoomMenu_->addAction ("300%");
  zoomMenu_->addAction ("400%");
  zoomMenu_->addAction ("600%");
  zoomMenu_->addAction ("800%");

  connect (zoomDropBtn_, &QToolButton::clicked, this, [=] () {
    zoomMenu_->popup (
        zoomDropBtn_->mapToGlobal (QPoint (0, zoomDropBtn_->height ())));
  });

  // -- zoom out / in --
  zoomOutBtn_= new QToolButton (this);
  zoomOutBtn_->setObjectName ("pdf-zoom-out-btn");
  zoomOutBtn_->setAutoRaise (true);
  zoomOutBtn_->setFixedSize (DpiUtils::scaled (kButtonSize),
                             DpiUtils::scaled (kButtonSize));
  zoomOutBtn_->setIconSize (
      QSize (DpiUtils::scaled (kIconSize), DpiUtils::scaled (kIconSize)));
  zoomOutBtn_->setToolTip (qt_translate ("Zoom Out"));

  zoomInBtn_= new QToolButton (this);
  zoomInBtn_->setObjectName ("pdf-zoom-in-btn");
  zoomInBtn_->setAutoRaise (true);
  zoomInBtn_->setFixedSize (DpiUtils::scaled (kButtonSize),
                            DpiUtils::scaled (kButtonSize));
  zoomInBtn_->setIconSize (
      QSize (DpiUtils::scaled (kIconSize), DpiUtils::scaled (kIconSize)));
  zoomInBtn_->setToolTip (qt_translate ("Zoom In"));

  // -- page navigation --
  prevPageBtn_= new QToolButton (this);
  prevPageBtn_->setObjectName ("pdf-prev-btn");
  prevPageBtn_->setAutoRaise (true);
  prevPageBtn_->setFixedSize (DpiUtils::scaled (kButtonSize),
                              DpiUtils::scaled (kButtonSize));
  prevPageBtn_->setIconSize (
      QSize (DpiUtils::scaled (kIconSize), DpiUtils::scaled (kIconSize)));
  prevPageBtn_->setToolTip (qt_translate ("Previous Page"));

  pageEdit_= new QLineEdit (this);
  pageEdit_->setObjectName ("pdf-page-edit");
  pageEdit_->setFixedWidth (DpiUtils::scaled (kPageEditWidth));
  pageEdit_->setFixedHeight (DpiUtils::scaled (kComboHeight));
  pageEdit_->setFrame (false);
  pageEdit_->setStyleSheet (
      "QLineEdit { padding: 0px; margin: 0px; border: 0.5px solid #CCC; }");
  pageEdit_->setAlignment (Qt::AlignCenter);
  pageEdit_->setFont (comboFont);

  pageTotalLabel_= new QLabel ("/ 0", this);
  pageTotalLabel_->setFixedWidth (DpiUtils::scaled (kPageTotalWidth));
  pageTotalLabel_->setFixedHeight (DpiUtils::scaled (kComboHeight));
  pageTotalLabel_->setStyleSheet (
      "QLabel { padding: 0px; margin: 0px; border: none; }");
  pageTotalLabel_->setAlignment (Qt::AlignCenter);

  nextPageBtn_= new QToolButton (this);
  nextPageBtn_->setObjectName ("pdf-next-btn");
  nextPageBtn_->setAutoRaise (true);
  nextPageBtn_->setFixedSize (DpiUtils::scaled (kButtonSize),
                              DpiUtils::scaled (kButtonSize));
  nextPageBtn_->setIconSize (
      QSize (DpiUtils::scaled (kIconSize), DpiUtils::scaled (kIconSize)));
  nextPageBtn_->setToolTip (qt_translate ("Next Page"));

  // -- rect select (screenshot) --
  rectSelectBtn_= new QToolButton (this);
  rectSelectBtn_->setObjectName ("pdf-screenshot-btn");
  rectSelectBtn_->setAutoRaise (true);
  rectSelectBtn_->setFixedSize (DpiUtils::scaled (kButtonSize),
                                DpiUtils::scaled (kButtonSize));
  rectSelectBtn_->setIconSize (
      QSize (DpiUtils::scaled (kIconSize), DpiUtils::scaled (kIconSize)));
  rectSelectBtn_->setCheckable (true);

  // -- layout: single container matching the original compact toolbar --
  QWidget* container= new QWidget (this);
  container->setObjectName ("pdf-reader-tool-bar");
  QHBoxLayout* layout= new QHBoxLayout (container);
  layout->setContentsMargins (kLayoutMarginLeft, kLayoutMarginTop,
                              kLayoutMarginRight, kLayoutMarginBottom);
  layout->setSpacing (kLayoutSpacing);

  QWidget*     leftWidget= new QWidget (container);
  QHBoxLayout* leftLayout= new QHBoxLayout (leftWidget);
  leftLayout->setContentsMargins (0, 0, 0, 0);
  leftLayout->setSpacing (0);
  leftLayout->addWidget (zoomCombo_, 0, Qt::AlignVCenter);
  leftLayout->addWidget (zoomDropBtn_, 0, Qt::AlignVCenter);
  leftLayout->addStretch ();
  leftWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);

  QWidget*     navWidget= new QWidget (container);
  QHBoxLayout* navLayout= new QHBoxLayout (navWidget);
  navLayout->setContentsMargins (0, 0, 0, 0);
  navLayout->setSpacing (0);
  navLayout->addWidget (zoomOutBtn_);
  navLayout->addWidget (prevPageBtn_);
  navLayout->addWidget (pageEdit_);
  navLayout->addWidget (pageTotalLabel_);
  navLayout->addWidget (nextPageBtn_);
  navLayout->addWidget (zoomInBtn_);

  QWidget*     rightWidget= new QWidget (container);
  QHBoxLayout* rightLayout= new QHBoxLayout (rightWidget);
  rightLayout->setContentsMargins (0, 0, 0, 0);
  rightLayout->addWidget (rectSelectBtn_);
  rightLayout->addStretch ();
  rightWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);

  layout->addWidget (leftWidget, 1);
  layout->addWidget (navWidget, 0);
  layout->addWidget (rightWidget, 1);

  addWidget (container);
}

void
PdfToolBar::connectTo (PDFReaderWidget* reader) {
  if (!reader) return;
  reader_= reader;

  // toolbar → reader
  connect (zoomOutBtn_, &QToolButton::clicked, reader,
           &PDFReaderWidget::zoomOut);
  connect (zoomInBtn_, &QToolButton::clicked, reader, &PDFReaderWidget::zoomIn);
  connect (prevPageBtn_, &QToolButton::clicked, reader,
           &PDFReaderWidget::onPrevPage);
  connect (nextPageBtn_, &QToolButton::clicked, reader,
           &PDFReaderWidget::onNextPage);
  connect (rectSelectBtn_, &QToolButton::toggled, reader,
           &PDFReaderWidget::setRectSelectMode);

  // zoom menu → reader
  connect (zoomMenu_, &QMenu::triggered, this, [this] (QAction* action) {
    if (!reader_) return;
    QString text= action->text ();
    if (text == "Fit Width") {
      reader_->fitWidth ();
    }
    else if (text == "Fit Height") {
      reader_->fitHeight ();
    }
    else {
      QString numStr= text;
      numStr.chop (1);
      bool   ok;
      double percent= numStr.toDouble (&ok);
      if (ok) reader_->setZoomFactor (percent / 100.0);
    }
  });

  // page edit → reader
  connect (pageEdit_, &QLineEdit::editingFinished, this, [this] () {
    if (!reader_) return;
    bool ok;
    int  page= pageEdit_->text ().toInt (&ok);
    if (ok) reader_->goToPage (page);
  });

  // reader → toolbar
  connect (reader, &PDFReaderWidget::zoomChanged, this,
           [this] (const QString& text) {
             bool blocked= zoomCombo_->blockSignals (true);
             zoomCombo_->setText (text);
             zoomCombo_->blockSignals (blocked);
           });

  connect (reader, &PDFReaderWidget::pageChanged, this,
           [this] (int current, int total) {
             pageEdit_->setText (QString::number (current));
             pageTotalLabel_->setText (QString ("/ %1").arg (total));
             prevPageBtn_->setEnabled (current > 1);
             nextPageBtn_->setEnabled (current < total);
           });

  connect (reader, &PDFReaderWidget::rectSelectModeChanged, this,
           [this] (bool checked) {
             bool blocked= rectSelectBtn_->blockSignals (true);
             rectSelectBtn_->setChecked (checked);
             rectSelectBtn_->blockSignals (blocked);
           });

  // sync initial state
  reader->updateZoomDisplay ();
  reader->updatePageNavigation ();
}

void
PdfToolBar::disconnectFrom () {
  if (!reader_) return;

  disconnect (zoomOutBtn_, nullptr, reader_, nullptr);
  disconnect (zoomInBtn_, nullptr, reader_, nullptr);
  disconnect (prevPageBtn_, nullptr, reader_, nullptr);
  disconnect (nextPageBtn_, nullptr, reader_, nullptr);
  disconnect (rectSelectBtn_, nullptr, reader_, nullptr);
  disconnect (zoomMenu_, nullptr, this, nullptr);
  disconnect (pageEdit_, nullptr, this, nullptr);
  disconnect (reader_, nullptr, this, nullptr);

  reader_= nullptr;
}
