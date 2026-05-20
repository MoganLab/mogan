
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

static QtMessageHandler defaultMessageHandler= nullptr;

static void
filterTestWarnings (QtMsgType type, const QMessageLogContext& context,
                    const QString& msg) {
  if (type == QtWarningMsg) {
    if (msg.contains ("cached device pixel ratio") ||
        msg.contains ("wayland.textinput")) {
      return;
    }
  }
  defaultMessageHandler (type, context, msg);
}

class TestPdfReaderWidget : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    defaultMessageHandler= qInstallMessageHandler (filterTestWarnings);
  }

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

    QScrollBar* vbar= widget->verticalScrollBar ();
    // Wayland 下布局/滚动条更新是异步的，轮询等待生效
    QVERIFY (QTest::qWaitFor ([&] () { return vbar->maximum () > 0; }, 1000));
    int initialPos= vbar->value ();

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

  void test_rectSelectButtonExists () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    QToolButton*     rectBtn=
        widget->findChild<QToolButton*> ("pdf-screenshot-btn");
    QVERIFY (rectBtn != nullptr);
    delete widget;
  }

  void test_rectSelectModeToggle () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    QToolButton*     rectBtn=
        widget->findChild<QToolButton*> ("pdf-screenshot-btn");
    QVERIFY (rectBtn != nullptr);

    QVERIFY (!widget->isRectSelectMode ());
    rectBtn->click ();
    QVERIFY (widget->isRectSelectMode ());
    rectBtn->click ();
    QVERIFY (!widget->isRectSelectMode ());
    delete widget;
  }

  void test_rectSelectCursor () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->show ();
    QApplication::processEvents ();

    QToolButton* rectBtn=
        widget->findChild<QToolButton*> ("pdf-screenshot-btn");
    QVERIFY (rectBtn != nullptr);

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QCOMPARE (vp->cursor ().shape (), Qt::ArrowCursor);
    rectBtn->click ();
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::CrossCursor);
    rectBtn->click ();
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::ArrowCursor);
    delete widget;
  }

  void test_rectSelectHint () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    QToolButton* rectBtn=
        widget->findChild<QToolButton*> ("pdf-screenshot-btn");
    QVERIFY (rectBtn != nullptr);

    // 进入选择模式后显示提示
    rectBtn->click ();
    QApplication::processEvents ();

    QLabel* hint= widget->findChild<QLabel*> ("rectSelectHint");
    QVERIFY (hint != nullptr);
    QVERIFY (hint->isVisible ());
    QVERIFY (hint->text ().contains ("Draw a rectangle"));

    // 模拟拖拽选择
    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    QPoint start (50, 50);
    QPoint end (150, 150);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove (vp, end);
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, end);
    QApplication::processEvents ();

    // 选择完成后提示变为 Copied to Clipboard!
    QCOMPARE (hint->text (), QString ("Copied to Clipboard!"));

    // 退出选择模式后隐藏提示
    rectBtn->click ();
    QApplication::processEvents ();
    QVERIFY (!hint->isVisible ());

    delete widget;
  }

  void test_rectSelectClipboard () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    // 清空剪贴板
    QClipboard* clipboard= QApplication::clipboard ();
    clipboard->clear ();
    QApplication::processEvents ();

    // 进入选择模式
    QToolButton* rectBtn=
        widget->findChild<QToolButton*> ("pdf-screenshot-btn");
    QVERIFY (rectBtn != nullptr);
    rectBtn->click ();
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    // 模拟拖拽选择
    QPoint start (50, 50);
    QPoint end (150, 150);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove (vp, end);
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, end);
    QApplication::processEvents ();

    // 验证剪贴板有图片
    QVERIFY (clipboard->mimeData ()->hasImage ());
    delete widget;
  }

  void test_rectSelectEscExitsMode () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    QToolButton* rectBtn=
        widget->findChild<QToolButton*> ("pdf-screenshot-btn");
    QVERIFY (rectBtn != nullptr);
    rectBtn->click ();
    QApplication::processEvents ();
    QVERIFY (widget->isRectSelectMode ());

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    vp->setFocus ();
    QTest::keyClick (vp, Qt::Key_Escape);
    QApplication::processEvents ();

    QVERIFY (!widget->isRectSelectMode ());
    QCOMPARE (vp->cursor ().shape (), Qt::ArrowCursor);
    delete widget;
  }

  void test_rectSelectEscCancelsDrag () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }

    QApplication::processEvents ();

    QToolButton* rectBtn=
        widget->findChild<QToolButton*> ("pdf-screenshot-btn");
    QVERIFY (rectBtn != nullptr);
    rectBtn->click ();
    QApplication::processEvents ();
    QVERIFY (widget->isRectSelectMode ());

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    // 开始拖拽
    QPoint start (50, 50);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QApplication::processEvents ();

    // 按下 ESC 取消拖拽
    vp->setFocus ();
    QTest::keyClick (vp, Qt::Key_Escape);
    QApplication::processEvents ();

    // 选框模式仍然开启
    QVERIFY (widget->isRectSelectMode ());
    QCOMPARE (vp->cursor ().shape (), Qt::CrossCursor);

    // 再次按下 ESC 退出选框模式
    QTest::keyClick (vp, Qt::Key_Escape);
    QApplication::processEvents ();

    QVERIFY (!widget->isRectSelectMode ());
    QCOMPARE (vp->cursor ().shape (), Qt::ArrowCursor);

    delete widget;
  }
};

QTEST_MAIN (TestPdfReaderWidget)
#include "qt_pdf_reader_widget_test.moc"
