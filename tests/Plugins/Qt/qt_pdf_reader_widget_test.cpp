
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget_test.cpp
 * DESCRIPTION: Tests for PDFReaderWidget
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#include "Qt/qt_pdf_reader_widget.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include "file.hpp"
#include "url.hpp"
#include <QApplication>
#include <QClipboard>
#include <QMouseEvent>
#include <QRubberBand>
#include <QScrollBar>
#include <QWheelEvent>
#include <QtTest/QtTest>

class TestPdfReaderWidget : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_creation () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    QVERIFY (widget != nullptr);
    QCOMPARE (widget->pageCount (), 0);
    QVERIFY (!widget->hasError ());
    delete widget;
  }

  void test_loadFromFile_validPdf () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    QVERIFY (is_regular (pdfUrl));

    bool result= widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QVERIFY (result);
    QCOMPARE (widget->pageCount (), 1);
    QVERIFY (!widget->hasError ());
    delete widget;
  }

  void test_loadFromFile_invalidFile () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    bool result= widget->loadFromFile ("/nonexistent/path/file.pdf");
    QVERIFY (!result);
    QVERIFY (widget->hasError ());
    delete widget;
  }

  void test_spaceKeyScrollsDown () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (200, 100);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    QScrollBar* vbar      = widget->verticalScrollBar ();
    int         initialPos= vbar->value ();
    QVERIFY (vbar->maximum () > 0);

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    vp->setFocus ();
    QTest::keyClick (vp, Qt::Key_Space);

    int newPos= vbar->value ();
    QVERIFY (newPos > initialPos);
    delete widget;
  }

  void test_wheelZoomIn () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    double initialZoom= widget->zoomFactor ();

    QWheelEvent wheelEvent (QPointF (50, 50), QPointF (50, 50), QPoint (0, 0),
                            QPoint (0, 120), Qt::NoButton, Qt::ControlModifier,
                            Qt::NoScrollPhase, false);
    QApplication::sendEvent (widget->viewport (), &wheelEvent);
    QApplication::processEvents ();

    double newZoom= widget->zoomFactor ();
    QVERIFY (newZoom > initialZoom);
    delete widget;
  }

  void test_wheelZoomOut () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    double initialZoom= widget->zoomFactor ();

    QWheelEvent wheelEvent (QPointF (50, 50), QPointF (50, 50), QPoint (0, 0),
                            QPoint (0, -120), Qt::NoButton, Qt::ControlModifier,
                            Qt::NoScrollPhase, false);
    QApplication::sendEvent (widget->viewport (), &wheelEvent);
    QApplication::processEvents ();

    double newZoom= widget->zoomFactor ();
    QVERIFY (newZoom < initialZoom);
    delete widget;
  }

  void test_currentPage () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    QCOMPARE (widget->currentPage (), 1);
    QVERIFY (!widget->canGoToPrevPage ());
    QVERIFY (!widget->canGoToNextPage ());
    delete widget;
  }

  void test_goToPage () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    widget->goToPage (1);
    QApplication::processEvents ();

    QCOMPARE (widget->currentPage (), 1);
    delete widget;
  }
};

QTEST_MAIN (TestPdfReaderWidget)
#include "qt_pdf_reader_widget_test.moc"
