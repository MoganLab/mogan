
/******************************************************************************
 * MODULE     : qt_pdf_preview_widget_test.cpp
 * DESCRIPTION: Tests for QTPdfPreviewWidget
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#include "Qt/qt_pdf_preview_widget.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include "file.hpp"
#include "url.hpp"
#include <QtTest/QtTest>

class TestPdfPreviewWidget : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_creation () {
    QTPdfPreviewWidget* widget= new QTPdfPreviewWidget ();
    QVERIFY (widget != nullptr);
    QCOMPARE (widget->pageNumber (), 0);
    QCOMPARE (widget->pageCount (), 0);
    QVERIFY (!widget->isLoading ());
    QVERIFY (!widget->hasError ());
    delete widget;
  }

  void test_loadFromFile_validPdf () {
    QTPdfPreviewWidget* widget= new QTPdfPreviewWidget ();
    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    QVERIFY (is_regular (pdfUrl));

    bool result= widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QVERIFY (result);
    QCOMPARE (widget->pageCount (), 1);
    QCOMPARE (widget->pageNumber (), 0);
    QVERIFY (!widget->hasError ());
    delete widget;
  }

  void test_loadFromFile_invalidFile () {
    QTPdfPreviewWidget* widget= new QTPdfPreviewWidget ();
    bool result= widget->loadFromFile ("/nonexistent/path/file.pdf");
    QVERIFY (!result);
    QVERIFY (widget->hasError ());
    delete widget;
  }

  void test_clearPreview () {
    QTPdfPreviewWidget* widget= new QTPdfPreviewWidget ();
    widget->clearPreview ("Test Message");
    QVERIFY (!widget->isLoading ());
    QVERIFY (!widget->hasError ());
    delete widget;
  }
};

QTEST_MAIN (TestPdfPreviewWidget)
#include "qt_pdf_preview_widget_test.moc"
