
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget.hpp
 * DESCRIPTION: Continuous-scroll PDF reader widget with toolbar
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#ifndef QT_PDF_READER_WIDGET_HPP
#define QT_PDF_READER_WIDGET_HPP

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <mupdf/fitz.h>

/**
 * @brief Continuous-scroll PDF reader widget with toolbar
 *
 * Renders all pages vertically in a scroll area.
 * Supports mouse wheel zoom, Fit Width/Height, and page navigation.
 */
class PDFReaderWidget : public QWidget {
  Q_OBJECT

public:
  explicit PDFReaderWidget (QWidget* parent= nullptr);
  ~PDFReaderWidget ();

  bool loadFromFile (const QString& filePath, int dpi= 150);
  void clear ();

  int    pageCount () const { return pageCount_; }
  bool   hasError () const { return hasError_; }
  double zoomFactor () const { return zoomFactor_; }
  void   setZoomFactor (double factor);
  void   fitWidth ();
  void   fitHeight ();

  int  currentPage () const;
  void goToPage (int page);

  bool canGoToPrevPage () const;
  bool canGoToNextPage () const;

  QWidget*    viewport () const;
  QScrollBar* verticalScrollBar () const;

private slots:
  void onZoomChanged (int index);
  void onPrevPage ();
  void onNextPage ();
  void onPageEditingFinished ();
  void updatePageNavigation ();

  void keyPressEvent (QKeyEvent* event) override;

private:
  bool renderPageToLabel (int pageNumber, QLabel* label, int targetWidth);
  void rebuildPages ();
  int  pageWidth () const;
  void setupToolBar ();
  void updateZoomDisplay ();

  bool eventFilter (QObject* watched, QEvent* event) override;

  QScrollArea* scrollArea_;
  QWidget*     contentWidget_;
  QVBoxLayout* pageLayout_;
  QVBoxLayout* mainLayout_;

  QToolBar*    toolBar_;
  QComboBox*   zoomCombo_;
  QPushButton* prevPageBtn_;
  QLineEdit*   pageEdit_;
  QLabel*      pageTotalLabel_;
  QPushButton* nextPageBtn_;

  QByteArray pdfData_;
  int        pageCount_;
  bool       hasError_;
  QString    errorString_;
  int        targetDpi_;
  double     zoomFactor_;
  double     pageAspectRatio_;
  double     pageBaseWidthPts_;

  static constexpr int    DEFAULT_DPI= 150;
  static constexpr int    PAGE_MARGIN= 16;
  static constexpr double MIN_ZOOM   = 0.1;
  static constexpr double MAX_ZOOM   = 5.0;
  static constexpr double ZOOM_STEP  = 0.1;
};

#endif // QT_PDF_READER_WIDGET_HPP
