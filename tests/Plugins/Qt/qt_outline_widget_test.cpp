/******************************************************************************
 * MODULE     : qt_outline_widget_test.cpp
 * DESCRIPTION: Tests for OutlineWidget (PDF bookmarks & document ToC)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "qt_pdf_outline_widget.hpp"
#include "base.hpp"
#include <QSignalSpy>
#include <QtTest/QtTest>

class TestOutlineWidget : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () { init_lolly (); }

  void test_creation () {
    OutlineWidget* widget= new OutlineWidget ("目录");
    QVERIFY (widget != nullptr);
    QVERIFY (widget->treeWidget () != nullptr);
    QVERIFY (!widget->hasContent ());
    delete widget;
  }

  void test_setOutline_pdf () {
    OutlineWidget* widget= new OutlineWidget ("目录");

    QVector<PdfOutlineItem> outline;
    PdfOutlineItem          c1;
    c1.title= "Chapter 1";
    c1.page = 0; // 0-based in PdfOutlineItem, should become 1 in widget

    PdfOutlineItem s1;
    s1.title= "Section 1.1";
    s1.page = 1; // 0-based -> 2
    c1.children.append (s1);

    PdfOutlineItem c2;
    c2.title= "Chapter 2";
    c2.page = 5; // 0-based -> 6

    outline.append (c1);
    outline.append (c2);

    widget->setOutline (outline);
    QVERIFY (widget->hasContent ());
    QCOMPARE (widget->treeWidget ()->topLevelItemCount (), 2);

    QTreeWidgetItem* top0= widget->treeWidget ()->topLevelItem (0);
    QCOMPARE (top0->text (0), QString ("Chapter 1"));
    QCOMPARE (top0->data (0, Qt::UserRole).toString (), QString ("1"));
    QCOMPARE (top0->childCount (), 1);

    QTreeWidgetItem* child0= top0->child (0);
    QCOMPARE (child0->text (0), QString ("Section 1.1"));
    QCOMPARE (child0->data (0, Qt::UserRole).toString (), QString ("2"));

    QTreeWidgetItem* top1= widget->treeWidget ()->topLevelItem (1);
    QCOMPARE (top1->text (0), QString ("Chapter 2"));
    QCOMPARE (top1->data (0, Qt::UserRole).toString (), QString ("6"));

    delete widget;
  }

  void test_setOutline_editor_nested () {
    OutlineWidget* widget= new OutlineWidget ("目录");

    QVector<OutlineItem> outline;
    OutlineItem          sec1;
    sec1.title = "1 相关文档";
    sec1.target= "0:1";

    OutlineItem sub1;
    sub1.title = "1.1 内部规范";
    sub1.target= "0:1:0";
    sec1.children.append (sub1);

    OutlineItem sec2;
    sec2.title = "2 如何验收";
    sec2.target= "0:2";

    outline.append (sec1);
    outline.append (sec2);

    widget->setOutline (outline);
    QVERIFY (widget->hasContent ());
    QCOMPARE (widget->treeWidget ()->topLevelItemCount (), 2);

    QTreeWidgetItem* item0= widget->treeWidget ()->topLevelItem (0);
    QCOMPARE (item0->text (0), QString ("1 相关文档"));
    QCOMPARE (item0->data (0, Qt::UserRole).toString (), QString ("0:1"));
    QCOMPARE (item0->childCount (), 1);

    QTreeWidgetItem* subItem= item0->child (0);
    QCOMPARE (subItem->text (0), QString ("1.1 内部规范"));
    QCOMPARE (subItem->data (0, Qt::UserRole).toString (), QString ("0:1:0"));

    delete widget;
  }

  void test_outlineActivated_signal () {
    OutlineWidget* widget= new OutlineWidget ("目录");
    widget->resize (300, 400);
    widget->show ();

    QVector<OutlineItem> outline;
    OutlineItem          item;
    item.title = "Test Chapter";
    item.target= "0:3:1";
    outline.append (item);

    widget->setOutline (outline);

    QSignalSpy spy (widget, &OutlineWidget::outlineActivated);

    QTreeWidgetItem* treeItem= widget->treeWidget ()->topLevelItem (0);
    QVERIFY (treeItem != nullptr);

    // Click item
    QRect itemRect= widget->treeWidget ()->visualItemRect (treeItem);
    QTest::mouseClick (widget->treeWidget ()->viewport (), Qt::LeftButton,
                       Qt::NoModifier, itemRect.center ());

    QCOMPARE (spy.count (), 1);
    QCOMPARE (spy.takeFirst ().at (0).toString (), QString ("0:3:1"));

    delete widget;
  }

  void test_clear_and_hasContent () {
    OutlineWidget* widget= new OutlineWidget ("目录");
    QVERIFY (!widget->hasContent ());

    QVector<OutlineItem> outline;
    OutlineItem          item;
    item.title = "Item";
    item.target= "0";
    outline.append (item);
    widget->setOutline (outline);
    QVERIFY (widget->hasContent ());

    widget->clear ();
    QVERIFY (!widget->hasContent ());
    QVERIFY (!widget->isVisible ());

    delete widget;
  }
};

QTEST_MAIN (TestOutlineWidget)
#include "qt_outline_widget_test.moc"
