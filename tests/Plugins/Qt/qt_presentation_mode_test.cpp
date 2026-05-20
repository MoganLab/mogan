
/******************************************************************************
 * MODULE     : qt_presentation_mode_test.cpp
 * DESCRIPTION: test viewport settings in presentation mode to prevent flicker
 * COPYRIGHT  : (C) 2025 Darcy Shen
 ******************************************************************************/

#include "QTMScrollView.hpp"
#include <QApplication>
#include <QPalette>
#include <QtTest/QtTest>

class TestPresentationMode : public QObject {
  Q_OBJECT

private slots:
  void test_viewport_background_in_presentation_mode ();
};

void
TestPresentationMode::test_viewport_background_in_presentation_mode () {
  QTMScrollView scrollView;
  QWidget*      viewport= scrollView.viewport ();
  QVERIFY (viewport != nullptr);

  // Simulate entering presentation mode
  viewport->setBackgroundRole (QPalette::Shadow);
  viewport->setAutoFillBackground (false);
  viewport->setAttribute (Qt::WA_OpaquePaintEvent);

  QCOMPARE (viewport->backgroundRole (), QPalette::Shadow);
  QVERIFY (!viewport->autoFillBackground ());
  QVERIFY (viewport->testAttribute (Qt::WA_OpaquePaintEvent));

  // Simulate exiting presentation mode
  viewport->setBackgroundRole (QPalette::Mid);
  viewport->setAutoFillBackground (true);
  viewport->setAttribute (Qt::WA_OpaquePaintEvent, false);

  QCOMPARE (viewport->backgroundRole (), QPalette::Mid);
  QVERIFY (viewport->autoFillBackground ());
  QVERIFY (!viewport->testAttribute (Qt::WA_OpaquePaintEvent));
}

QTEST_MAIN (TestPresentationMode)
#include "qt_presentation_mode_test.moc"
