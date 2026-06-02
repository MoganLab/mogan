
/******************************************************************************
 * MODULE     : qt_pdf_toolbar.hpp
 * DESCRIPTION: Toolbar for the PDF reader, hosted in QMainWindow's toolbar area
 * COPYRIGHT  : (C) 2026 Da Shen
 *                  2026 Yifan Lu
 ******************************************************************************/

#ifndef QT_PDF_TOOLBAR_HPP
#define QT_PDF_TOOLBAR_HPP

#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>

class PDFReaderWidget;

/**
 * @brief PDF reader toolbar hosted as a QMainWindow QToolBar.
 *
 * Lives outside the central widget so it is not compressed by dock widgets
 * (e.g. the chat sidebar). All actions are forwarded to a PDFReaderWidget
 * via signal/slot connections managed by connectTo / disconnectFrom.
 */
class PdfToolBar : public QToolBar {
  Q_OBJECT

public:
  explicit PdfToolBar (const QString& title, QWidget* parent= nullptr);

  /** Connect all toolbar actions to the given PDF reader. */
  void connectTo (PDFReaderWidget* reader);
  /** Disconnect from the current reader. */
  void disconnectFrom ();

private:
  void setupWidgets ();

  // -- toolbar widgets --
  QLineEdit*   zoomCombo_;
  QToolButton* zoomDropBtn_;
  QMenu*       zoomMenu_;
  QToolButton* zoomOutBtn_;
  QToolButton* zoomInBtn_;
  QToolButton* prevPageBtn_;
  QLineEdit*   pageEdit_;
  QLabel*      pageTotalLabel_;
  QToolButton* nextPageBtn_;
  QToolButton* rectSelectBtn_;

  /** Currently connected reader (nullptr if none). */
  PDFReaderWidget* reader_= nullptr;
};

#endif // QT_PDF_TOOLBAR_HPP
