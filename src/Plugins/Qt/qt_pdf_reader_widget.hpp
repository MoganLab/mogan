
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget.hpp
 * DESCRIPTION: Continuous-scroll PDF reader widget
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#ifndef QT_PDF_READER_WIDGET_HPP
#define QT_PDF_READER_WIDGET_HPP

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include <mupdf/fitz.h>

/**
 * @brief Continuous-scroll PDF reader widget
 *
 * Renders all pages vertically in a scroll area.
 * Supports mouse wheel and arrow keys for scrolling.
 */
class PDFReaderWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit PDFReaderWidget (QWidget* parent= nullptr);
  ~PDFReaderWidget ();

  bool loadFromFile (const QString& filePath, int dpi= 150);
  void clear ();

  int  pageCount () const { return pageCount_; }
  bool hasError () const { return hasError_; }

protected:
  void keyPressEvent (QKeyEvent* event) override;
  void resizeEvent (QResizeEvent* event) override;

private:
  bool renderPageToLabel (int pageNumber, QLabel* label, int targetWidth);
  void rebuildPages ();
  int  pageWidth () const;

  QWidget*     contentWidget_;
  QVBoxLayout* layout_;

  QByteArray pdfData_;
  int        pageCount_;
  bool       hasError_;
  QString    errorString_;
  int        targetDpi_;

  static constexpr int DEFAULT_DPI= 150;
  static constexpr int PAGE_MARGIN= 16;
};

#endif // QT_PDF_READER_WIDGET_HPP
