/******************************************************************************
 * MODULE     : qt_outline_widget_test.cpp
 * DESCRIPTION: Tests for OutlineWidget & OutlineBridge (PDF & Document ToC)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "base.hpp"
#include "qt_pdf_outline_widget.hpp"
#include <QSignalSpy>
#include <QtTest/QtTest>

class TestOutlineWidget : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_creation () {
    OutlineWidget* widget= new OutlineWidget ("目录");
    QVERIFY (widget != nullptr);
    QVERIFY (widget->bridge () != nullptr);
    QVERIFY (!widget->hasContent ());
    delete widget;
  }

  void test_setOutline_pdf () {
    OutlineWidget* widget= new OutlineWidget ("目录");

    QVector<PdfOutlineItem> outline;
    PdfOutlineItem          c1;
    c1.title= "Chapter 1";
    c1.page = 0; // 0-based -> 1-based page 1

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

    QVariantList model= widget->bridge ()->outlineModel ();
    QCOMPARE (model.size (), 2);

    QVariantMap top0= model.at (0).toMap ();
    QCOMPARE (top0.value ("title").toString (), QString ("Chapter 1"));
    QCOMPARE (top0.value ("page").toString (), QString ("1"));
    QCOMPARE (top0.value ("target").toString (), QString ("1"));

    QVariantList children0= top0.value ("children").toList ();
    QCOMPARE (children0.size (), 1);
    QVariantMap child0= children0.at (0).toMap ();
    QCOMPARE (child0.value ("title").toString (), QString ("Section 1.1"));
    QCOMPARE (child0.value ("page").toString (), QString ("2"));

    QVariantMap top1= model.at (1).toMap ();
    QCOMPARE (top1.value ("title").toString (), QString ("Chapter 2"));
    QCOMPARE (top1.value ("page").toString (), QString ("6"));

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

    QVariantList model= widget->bridge ()->outlineModel ();
    QCOMPARE (model.size (), 2);

    QVariantMap item0= model.at (0).toMap ();
    QCOMPARE (item0.value ("title").toString (), QString ("1 相关文档"));
    QCOMPARE (item0.value ("target").toString (), QString ("0:1"));

    QVariantList subList= item0.value ("children").toList ();
    QCOMPARE (subList.size (), 1);
    QVariantMap subItem= subList.at (0).toMap ();
    QCOMPARE (subItem.value ("title").toString (), QString ("1.1 内部规范"));
    QCOMPARE (subItem.value ("target").toString (), QString ("0:1:0"));

    delete widget;
  }

  void test_outlineActivated_signal () {
    OutlineWidget* widget= new OutlineWidget ("目录");

    QVector<OutlineItem> outline;
    OutlineItem          item;
    item.title = "Test Chapter";
    item.target= "0:3:1";
    outline.append (item);

    widget->setOutline (outline);

    QSignalSpy spy (widget, &OutlineWidget::outlineActivated);

    widget->bridge ()->itemClicked ("0", "0:3:1");

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

  void test_closeRequested () {
    OutlineWidget* widget= new OutlineWidget ("目录");
    widget->show ();
    QVERIFY (widget->isVisible ());

    widget->bridge ()->closeOutline ();
    QVERIFY (!widget->isVisible ());

    delete widget;
  }
};

#ifdef QTTEXMACS
QTEST_MAIN (TestOutlineWidget)
#else
int
main () {
  return 0;
}
#endif

#include "qt_outline_widget_test.moc"
