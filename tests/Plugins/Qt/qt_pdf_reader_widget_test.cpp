
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
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPinchGesture>
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

  void test_toolBarWidgetsHeightAligned () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->show ();
    QApplication::processEvents ();

    QLineEdit* zoomEdit= widget->findChild<QLineEdit*> ("pdf-zoom-edit");
    QVERIFY (zoomEdit != nullptr);
    QLineEdit* pageEdit= widget->findChild<QLineEdit*> ("pdf-page-edit");
    QVERIFY (pageEdit != nullptr);
    QToolButton* zoomDropBtn=
        widget->findChild<QToolButton*> ("pdf-zoom-drop-btn");
    QVERIFY (zoomDropBtn != nullptr);

    QCOMPARE (zoomEdit->height (), pageEdit->height ());
    QVERIFY (zoomDropBtn->arrowType () == Qt::DownArrow);

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

    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);
    rectBtn->click ();
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::CrossCursor);
    rectBtn->click ();
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);
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
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);
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
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    delete widget;
  }

  // ============================================================
  // Browse (Hand) Tool Tests
  // ============================================================

  void test_defaultCursorIsOpenHand () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);
    delete widget;
  }

  void test_dragScrollsDown () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (200, 100);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QScrollBar* vbar= widget->verticalScrollBar ();
    QVERIFY (QTest::qWaitFor ([&] () { return vbar->maximum () > 0; }, 1000));

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    // 先将滚动条设到中间位置，以便双向验证
    int midPos= vbar->maximum () / 2;
    vbar->setValue (midPos);
    QApplication::processEvents ();
    int initialPos= vbar->value ();

    // 模拟向下拖动 30px（grab-and-pull：页面向下移动，滚动条值减小）
    QPoint start (100, 100);
    QPoint end (100, 130);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove (vp, end);
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, end);
    QApplication::processEvents ();

    // QScroller 的滚动更新是异步的，给一点时间让动画生效
    QTest::qWait (100);

    int newPos= vbar->value ();
    QVERIFY (newPos < initialPos);
    delete widget;
  }

  void test_dragCursorChangesToClosedHand () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    QPoint start (100, 100);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QApplication::processEvents ();

    // 零延迟响应：按下后立即显示 ClosedHandCursor
    QCOMPARE (vp->cursor ().shape (), Qt::ClosedHandCursor);

    QPoint beyondThreshold (
        start.x (), start.y () + QApplication::startDragDistance () + 2);
    QTest::mouseMove (vp, beyondThreshold);
    QApplication::processEvents ();

    // 拖动过程中保持 ClosedHandCursor
    QCOMPARE (vp->cursor ().shape (), Qt::ClosedHandCursor);

    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, beyondThreshold);
    QApplication::processEvents ();
    delete widget;
  }

  void test_dragKeepsClosedHandBeforeThreshold () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    QPoint start (100, 100);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::ClosedHandCursor);

    // Move a tiny distance (below drag threshold) and not over a link
    QPoint smallMove (start.x () + 1, start.y () + 1);
    QTest::mouseMove (vp, smallMove);
    QApplication::processEvents ();

    // Cursor must remain ClosedHandCursor because left button is still pressed
    QCOMPARE (vp->cursor ().shape (), Qt::ClosedHandCursor);

    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, smallMove);
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    delete widget;
  }

  void test_releaseRestoresOpenHand () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QPoint start (100, 100);
    QPoint end (100, 140);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove (vp, end);
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::ClosedHandCursor);

    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, end);
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);
    delete widget;
  }

  void test_clickDoesNotScroll () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (200, 100);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QScrollBar* vbar= widget->verticalScrollBar ();
    QVERIFY (QTest::qWaitFor ([&] () { return vbar->maximum () > 0; }, 1000));

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    int initialPos= vbar->value ();

    // 零延迟响应下，任何移动都会滚动；
    // 只有完全不动地按下释放才算单击，不触发滚动
    QPoint start (100, 100);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, start);
    QApplication::processEvents ();

    QCOMPARE (vbar->value (), initialPos);
    delete widget;
  }

  void test_rectSelectModeOverridesHand () {
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

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    // 默认小手模式
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    // 进入选择模式
    rectBtn->click ();
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::CrossCursor);

    // 在选择模式下点击不应触发小手拖动
    QScrollBar* vbar= widget->verticalScrollBar ();
    QVERIFY (QTest::qWaitFor ([&] () { return vbar->maximum () > 0; }, 1000));
    int initialPos= vbar->value ();

    QPoint start (50, 50);
    QPoint end (50, 150);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove (vp, end);
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, end);
    QApplication::processEvents ();

    // 选择模式下是 rubber band 选择，滚动条不应因小手拖动而变化
    // 但 rubber band 操作本身不滚动，所以值应保持不变
    QCOMPARE (vbar->value (), initialPos);

    // 退出选择模式后恢复小手
    rectBtn->click ();
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    delete widget;
  }

  void test_inertialScrollAfterRelease () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (200, 100);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QScrollBar* vbar= widget->verticalScrollBar ();
    QVERIFY (QTest::qWaitFor ([&] () { return vbar->maximum () > 0; }, 1000));

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    // 先将滚动条设到中间，避免触顶/底
    int midPos= vbar->maximum () / 2;
    vbar->setValue (midPos);
    QApplication::processEvents ();

    // 快速向下拖动 50px（多步模拟高速运动）
    QPoint start (50, 50);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    for (int i= 1; i <= 5; ++i) {
      QTest::mouseMove (vp, QPoint (50, 50 + i * 10));
      QApplication::processEvents ();
    }
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 100));
    int releasePos= vbar->value ();

    // 释放后等待一小段时间，惯性滚动应使值继续变化
    QTest::qWait (80);
    int afterInertia= vbar->value ();
    QVERIFY (afterInertia != releasePos);

    delete widget;
  }

  void test_inertialScrollStopsEventually () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (200, 100);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QScrollBar* vbar= widget->verticalScrollBar ();
    QVERIFY (QTest::qWaitFor ([&] () { return vbar->maximum () > 0; }, 1000));

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    int midPos= vbar->maximum () / 2;
    vbar->setValue (midPos);
    QApplication::processEvents ();

    QPoint start (50, 50);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    for (int i= 1; i <= 5; ++i) {
      QTest::mouseMove (vp, QPoint (50, 50 + i * 10));
      QApplication::processEvents ();
    }
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 100));

    // 等待足够长的时间让惯性滚动完全停止
    QTest::qWait (600);
    int stablePos= vbar->value ();

    // 再等待一帧，值应不再变化
    QTest::qWait (50);
    QCOMPARE (vbar->value (), stablePos);

    delete widget;
  }

  // ============================================================
  // Link hover and click tests
  // ============================================================

  void test_linkCursorOnHover () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    // inject a link covering the top-left area of page 0
    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.5, 0.5);
    link.uri = "#page=2";
    links.append (link);
    widget->setTestLinks (0, links);

    // move mouse into the link area
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::PointingHandCursor);
    QVERIFY (widget->isOverLink ());

    delete widget;
  }

  void test_linkCursorOffHover () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.3, 0.3);
    link.uri = "#page=2";
    links.append (link);
    widget->setTestLinks (0, links);

    // move into link area
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::PointingHandCursor);

    // move outside link area
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (300, 250), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);
    QVERIFY (!widget->isOverLink ());

    delete widget;
  }

  void test_linkClickInternalPage () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    // we need at least 2 pages to test page navigation; use a multi-page
    // PDF if available, otherwise this test will skip
    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (!is_regular (pdfUrl)) {
      delete widget;
      return;
    }
    widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    // inject an internal link at page 0
    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.5, 0.5);
    link.uri = "#page=1";
    links.append (link);
    widget->setTestLinks (0, links);

    // move into link area and click (press+release without moving)
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QVERIFY (widget->isOverLink ());

    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QApplication::processEvents ();

    // internal link should navigate to page 1 (no error / crash)
    QCOMPARE (widget->currentPage (), 1);

    delete widget;
  }

  void test_linkClickSignal () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (!is_regular (pdfUrl)) {
      delete widget;
      return;
    }
    widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QString capturedUri;
    connect (widget, &PDFReaderWidget::linkClicked,
             [&capturedUri] (const QString& uri) { capturedUri= uri; });

    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.5, 0.5);
    link.uri = "https://example.com";
    links.append (link);
    widget->setTestLinks (0, links);

    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QVERIFY (widget->isOverLink ());

    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QApplication::processEvents ();

    QCOMPARE (capturedUri, QString ("https://example.com"));

    delete widget;
  }

  void test_linkDragDoesNotTriggerClick () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (!is_regular (pdfUrl)) {
      delete widget;
      return;
    }
    widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QString capturedUri;
    connect (widget, &PDFReaderWidget::linkClicked,
             [&capturedUri] (const QString& uri) { capturedUri= uri; });

    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.5, 0.5);
    link.uri = "https://example.com";
    links.append (link);
    widget->setTestLinks (0, links);

    QPoint start (50, 50);
    QPoint end (50, 150);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, start);
    {
      QMouseEvent moveEvent (QEvent::MouseMove, end, Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, end);
    QApplication::processEvents ();

    QVERIFY (capturedUri.isEmpty ());

    delete widget;
  }

  void test_pinchZoomIn () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    double initialZoom= widget->zoomFactor ();

    widget->simulatePinchGesture (Qt::GestureStarted, 1.0);
    QApplication::processEvents ();

    widget->simulatePinchGesture (Qt::GestureUpdated, 1.5);
    QApplication::processEvents ();

    double newZoom= widget->zoomFactor ();
    QVERIFY (newZoom > initialZoom);

    widget->simulatePinchGesture (Qt::GestureFinished, 1.5);
    QApplication::processEvents ();

    delete widget;
  }

  void test_pinchZoomBlocksRender () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    int initialRenderCount= widget->renderCallCount ();

    widget->simulatePinchGesture (Qt::GestureStarted, 1.0);
    QApplication::processEvents ();

    for (int i= 0; i < 5; ++i) {
      widget->simulatePinchGesture (Qt::GestureUpdated, 1.0 + (i + 1) * 0.1);
      QApplication::processEvents ();
    }

    QCOMPARE (widget->renderCallCount (), initialRenderCount);

    widget->simulatePinchGesture (Qt::GestureFinished, 1.5);
    QApplication::processEvents ();

    QTest::qWait (250);
    QApplication::processEvents ();

    QVERIFY (widget->renderCallCount () > initialRenderCount);

    delete widget;
  }

  // Test: simulate real mouse event flow that QScroller would NOT intercept
  // because there is no InputPress before the Move.
  // This verifies that the eventFilter actually receives the hover MoveEvent
  // when sent through the normal Qt event loop (not via sendEvent shortcut).
  void test_linkHoverViaPostedEvent () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (!is_regular (pdfUrl)) {
      delete widget;
      return;
    }
    widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.5, 0.5);
    link.uri = "#page=1";
    links.append (link);
    widget->setTestLinks (0, links);

    // First verify sendEvent works (baseline)
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::PointingHandCursor);
    QVERIFY (widget->isOverLink ());

    // Move away to reset (position well outside the 0.5x0.5 link area)
    // Page label is ~794x1123; need x/794 > 0.5 or y/1123 > 0.5
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (500, 700), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    // Now use postEvent to simulate event-loop delivery (like QTest::mouseMove
    // but guaranteed to work in headless environments)
    {
      QMouseEvent* moveEvent=
          new QMouseEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                           Qt::NoButton, Qt::NoModifier);
      QCoreApplication::postEvent (vp, moveEvent);
    }
    QApplication::processEvents ();

    QCOMPARE (vp->cursor ().shape (), Qt::PointingHandCursor);
    QVERIFY (widget->isOverLink ());

    delete widget;
  }

  // Test: verify that when QScroller is in Dragging state (after Press),
  // a click (press + release without drag) on a link still triggers the
  // link click. This is the exact scenario that fails in production.
  void test_linkClickAfterQScrollerPress () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (!is_regular (pdfUrl)) {
      delete widget;
      return;
    }
    widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QString capturedUri;
    connect (widget, &PDFReaderWidget::linkClicked,
             [&capturedUri] (const QString& uri) { capturedUri= uri; });

    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.5, 0.5);
    link.uri = "https://example.com";
    links.append (link);
    widget->setTestLinks (0, links);

    // First hover to set overLink_ = true using sendEvent (known to work)
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QVERIFY (widget->isOverLink ());

    // Simulate a real click sequence through the event loop:
    // press → release without drag
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QApplication::processEvents ();
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QApplication::processEvents ();

    QCOMPARE (capturedUri, QString ("https://example.com"));

    delete widget;
  }

  // ============================================================
  // Double-click tests (TDD for mouse-left-button-not-always-working)
  // ============================================================

  void test_doubleClickStartsDrag () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (is_regular (pdfUrl)) {
      widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    }
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    // Simulate a double-click: press, release, dblclick, release
    QPoint pos (100, 100);
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, pos);
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::ClosedHandCursor);

    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, pos);
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    // Now the second click of the double-click sequence
    QTest::mouseDClick (vp, Qt::LeftButton, Qt::NoModifier, pos);
    QApplication::processEvents ();

    // After the dblclick event the cursor must be ClosedHandCursor,
    // proving the second press was handled.
    QCOMPARE (vp->cursor ().shape (), Qt::ClosedHandCursor);

    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, pos);
    QApplication::processEvents ();
    QCOMPARE (vp->cursor ().shape (), Qt::OpenHandCursor);

    delete widget;
  }

  void test_doubleClickInRectSelectMode () {
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

    QPoint pos (50, 50);

    // First click starts rubber band
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, pos);
    QApplication::processEvents ();

    QRubberBand* rb= widget->findChild<QRubberBand*> ();
    QVERIFY (rb != nullptr);
    QVERIFY (rb->isVisible ());

    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, pos);
    QApplication::processEvents ();

    // Second click of a double-click must also start rubber band.
    // QTest::mouseDClick sends dblclick+release; release hides the band,
    // so we send only the dblclick event to verify the press path.
    {
      QMouseEvent dblClickEvent (QEvent::MouseButtonDblClick, pos,
                                 Qt::LeftButton, Qt::LeftButton,
                                 Qt::NoModifier);
      QApplication::sendEvent (vp, &dblClickEvent);
    }
    QApplication::processEvents ();
    QVERIFY (rb->isVisible ());

    delete widget;
  }

  void test_doubleClickOnLinkTriggersClick () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (400, 300);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (!is_regular (pdfUrl)) {
      delete widget;
      return;
    }
    widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QApplication::processEvents ();

    QWidget* vp= widget->viewport ();
    QVERIFY (vp != nullptr);

    QString capturedUri;
    connect (widget, &PDFReaderWidget::linkClicked,
             [&capturedUri] (const QString& uri) { capturedUri= uri; });

    QVector<PdfLink> links;
    PdfLink          link;
    link.rect= QRectF (0.0, 0.0, 0.5, 0.5);
    link.uri = "https://example.com";
    links.append (link);
    widget->setTestLinks (0, links);

    // Hover to set overLink_
    {
      QMouseEvent moveEvent (QEvent::MouseMove, QPoint (50, 50), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
      QApplication::sendEvent (vp, &moveEvent);
    }
    QApplication::processEvents ();
    QVERIFY (widget->isOverLink ());

    // First click
    QTest::mousePress (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QApplication::processEvents ();
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QApplication::processEvents ();

    // Verify first click worked, then reset
    QCOMPARE (capturedUri, QString ("https://example.com"));
    capturedUri.clear ();

    // Simulate only the dblclick event (the second press of a double-click)
    {
      QMouseEvent dblClickEvent (QEvent::MouseButtonDblClick, QPoint (50, 50),
                                 Qt::LeftButton, Qt::LeftButton,
                                 Qt::NoModifier);
      QApplication::sendEvent (vp, &dblClickEvent);
    }
    QApplication::processEvents ();

    // Release after dblclick
    QTest::mouseRelease (vp, Qt::LeftButton, Qt::NoModifier, QPoint (50, 50));
    QApplication::processEvents ();

    // The dblclick-release should have triggered the link click
    QCOMPARE (capturedUri, QString ("https://example.com"));

    delete widget;
  }

  void test_autoFitWidth_whenSnappedLeftHalf () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    QScreen*         screen= QApplication::primaryScreen ();
    QRect            screenGeo=
        screen ? screen->availableGeometry () : QRect (0, 0, 1920, 1080);
    int screenW= screenGeo.width ();
    int screenH= screenGeo.height ();
    // 模拟左半屏贴靠：宽度=屏幕一半，高度=屏幕高度，左边缘对齐
    widget->resize (screenW / 2, screenH);
    widget->move (screenGeo.x (), screenGeo.y ());
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    QVERIFY (is_regular (pdfUrl));

    bool result= widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QVERIFY (result);
    QApplication::processEvents ();

    // 当窗口贴靠到左半屏时，应自动触发 Fit Width
    QVERIFY (widget->zoomFactor () != 1.0);
    delete widget;
  }

  void test_noAutoFitWidth_whenNotSnapped () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    QScreen*         screen= QApplication::primaryScreen ();
    QRect            screenGeo=
        screen ? screen->availableGeometry () : QRect (0, 0, 1920, 1080);
    int screenW= screenGeo.width ();
    int screenH= screenGeo.height ();
    // 窗口宽度远超屏幕一半，不满足半屏贴靠条件
    widget->resize (screenW * 2 / 3, screenH);
    widget->move (screenGeo.x (), screenGeo.y ());
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    QVERIFY (is_regular (pdfUrl));

    bool result= widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QVERIFY (result);
    QApplication::processEvents ();

    // 不满足半屏贴靠条件，应保持默认 100% 缩放
    QCOMPARE (widget->zoomFactor (), 1.0);
    delete widget;
  }

  void test_tiledRenderProducesFullPage () {
    PDFReaderWidget* widget= new PDFReaderWidget ();
    widget->resize (800, 600);
    widget->show ();

    url pdfUrl= url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf");
    if (!is_regular (pdfUrl)) {
      delete widget;
      return;
    }
    widget->loadFromFile (to_qstring (as_string (pdfUrl)));
    QApplication::processEvents ();
    QTest::qWait (100);

    // After loading, the page label should have a non-null pixmap
    QLabel* label= widget->findChild<QLabel*> ();
    QVERIFY (label != nullptr);
    QPixmap pm= label->pixmap ();
    QVERIFY (!pm.isNull ());

    // The pixmap should be close to the label's size (accounting for DPR)
    qreal dpr  = pm.devicePixelRatio ();
    int   pmW  = qRound (pm.width () / dpr);
    int   pmH  = qRound (pm.height () / dpr);
    QCOMPARE (pmW, label->width ());
    QCOMPARE (pmH, label->height ());

    // Check that the pixmap is not all white (content was rendered)
    QImage img= pm.toImage ();
    int    nonWhite= 0;
    for (int y= 0; y < img.height (); ++y) {
      for (int x= 0; x < img.width (); ++x) {
        if (qGray (img.pixelColor (x, y).rgb ()) < 250) { ++nonWhite; }
      }
    }
    QVERIFY2 (nonWhite > 100,
              "Rendered page appears blank — tiles may not be composited");

    // Check that non-white pixels span the full width and height
    int minX= img.width (), maxX= 0, minY= img.height (), maxY= 0;
    for (int y= 0; y < img.height (); ++y) {
      for (int x= 0; x < img.width (); ++x) {
        if (qGray (img.pixelColor (x, y).rgb ()) < 250) {
          if (x < minX) minX= x;
          if (x > maxX) maxX= x;
          if (y < minY) minY= y;
          if (y > maxY) maxY= y;
        }
      }
    }
    // Content should span at least 50% of the width and height
    double widthRatio = (double) (maxX - minX) / img.width ();
    double heightRatio= (double) (maxY - minY) / img.height ();
    QVERIFY2 (widthRatio > 0.5,
              QString ("Content only spans %1% of width").arg (widthRatio * 100)
                  .toUtf8 ()
                  .constData ());
    QVERIFY2 (heightRatio > 0.5,
              QString ("Content only spans %1% of height")
                  .arg (heightRatio * 100)
                  .toUtf8 ()
                  .constData ());

    delete widget;
  }
};

QTEST_MAIN (TestPdfReaderWidget)
#include "qt_pdf_reader_widget_test.moc"
