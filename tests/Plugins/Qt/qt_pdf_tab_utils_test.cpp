
/******************************************************************************
 * MODULE     : qt_pdf_tab_utils_test.cpp
 * DESCRIPTION: Tests for PDF tab detection utilities
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

class TestPdfTabUtils : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_is_pdf_tab_file () {
    QVERIFY (is_pdf_tab_file ("/home/test/hello.pdf"));
    QVERIFY (is_pdf_tab_file ("file:///home/test/hello.pdf"));
    QVERIFY (!is_pdf_tab_file ("/home/test/hello.tm"));
    QVERIFY (!is_pdf_tab_file ("/home/test/hello.tmu"));
    QVERIFY (!is_pdf_tab_file ("tmfs://startup-tab"));
    QVERIFY (!is_pdf_tab_file (""));
  }
};

QTEST_MAIN (TestPdfTabUtils)
#include "qt_pdf_tab_utils_test.moc"
