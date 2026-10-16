/******************************************************************************
 * MODULE     : qt_outline_widget_test.cpp
 * DESCRIPTION: Tests for OutlineWidget & OutlineBridge (PDF & Document ToC)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "base.hpp"
#include "preferences.hpp"
#include "qt_pdf_outline_widget.hpp"
#include "qt_pdf_toolbar.hpp"
#include <QQuickItem>
#include <QSignalSpy>
#include <QtTest/QtTest>

class TestOutlineWidget : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    set_preference ("outline sidebar", "on");
  }
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

  void test_pdf_toolbar_outline_toggle_creation () {
    PdfToolBar*  toolBar= new PdfToolBar ("pdf toolbar");
    QToolButton* btn=
        toolBar->findChild<QToolButton*> ("pdf-toggle-outline-btn");
    QVERIFY (btn != nullptr);
    QVERIFY (btn->isCheckable ());
    QVERIFY (!btn->isChecked ());
    delete toolBar;
  }

  void test_pdf_toolbar_outline_toggle_api () {
    PdfToolBar*  toolBar= new PdfToolBar ("pdf toolbar");
    QToolButton* btn=
        toolBar->findChild<QToolButton*> ("pdf-toggle-outline-btn");
    QVERIFY (btn != nullptr);

    toolBar->setOutlineChecked (true);
    QVERIFY (btn->isChecked ());

    toolBar->setOutlineChecked (false);
    QVERIFY (!btn->isChecked ());

    toolBar->setOutlineEnabled (false);
    QVERIFY (!btn->isEnabled ());

    toolBar->setOutlineEnabled (true);
    QVERIFY (btn->isEnabled ());

    delete toolBar;
  }

  void test_pdf_toolbar_outline_toggle_signal () {
    PdfToolBar*  toolBar= new PdfToolBar ("pdf toolbar");
    QToolButton* btn=
        toolBar->findChild<QToolButton*> ("pdf-toggle-outline-btn");
    QVERIFY (btn != nullptr);

    QSignalSpy spy (toolBar, &PdfToolBar::toggleOutlineClicked);
    btn->click ();
    QCOMPARE (spy.count (), 1);

    delete toolBar;
  }

  void test_scrollbar_visibility_and_interaction () {
    OutlineWidget* widget= new OutlineWidget ("目录");
    widget->resize (300, 400);
    widget->show ();
    QTest::qWait (50);

    QQuickWidget* qw= widget->quickWidget ();
    QVERIFY (qw != nullptr);
    QQuickItem* rootItem= qw->rootObject ();
    QVERIFY (rootItem != nullptr);

    QQuickItem* scrollbar= rootItem->findChild<QQuickItem*> ("scrollbar");
    QVERIFY (scrollbar != nullptr);

    // 1. 少量大纲未超出视口：滚动条隐藏
    QVector<OutlineItem> shortOutline;
    OutlineItem          i1;
    i1.title = "Chapter 1";
    i1.target= "0:1";
    OutlineItem i2;
    i2.title = "Chapter 2";
    i2.target= "0:2";
    shortOutline.append (i1);
    shortOutline.append (i2);
    widget->setOutline (shortOutline);
    QTest::qWait (50);

    QVERIFY (!scrollbar->isVisible ());

    // 2. 超出视口的大量大纲项：滚动条按需显示
    QVector<OutlineItem> longOutline;
    for (int i= 0; i < 30; ++i) {
      OutlineItem item;
      item.title = QString ("Chapter %1").arg (i + 1);
      item.target= QString ("0:%1").arg (i);
      longOutline.append (item);
    }
    widget->setOutline (longOutline);
    QTest::qWait (100);

    QQuickItem* list= rootItem->findChild<QQuickItem*> ("outlineList");
    QVERIFY (list != nullptr);

    QVERIFY (scrollbar->isVisible ());

    QQuickItem* thumb= rootItem->findChild<QQuickItem*> ("scrollbarThumb");
    QVERIFY (thumb != nullptr);
    qreal thumbH= thumb->property ("height").toReal ();
    QVERIFY (thumbH >= 24.0); // 最小高度保护

    qreal initialY= thumb->property ("y").toReal ();
    QCOMPARE (initialY, 0.0); // 顶部贴顶

    // 3. 模拟滚动列表，滑块位置同步联动
    list->setProperty ("contentY", 100.0);
    QTest::qWait (50);

    qreal scrolledY= thumb->property ("y").toReal ();
    QVERIFY (scrolledY > initialY);

    // 4. 测试翻页操作（点击滑块下方/上方）
    qreal yBeforePageDown= list->property ("contentY").toReal ();
    QMetaObject::invokeMethod (scrollbar, "pageDown");
    QTest::qWait (50);
    qreal yAfterPageDown= list->property ("contentY").toReal ();
    QVERIFY (yAfterPageDown > yBeforePageDown);

    QMetaObject::invokeMethod (scrollbar, "pageUp");
    QTest::qWait (50);
    qreal yAfterPageUp= list->property ("contentY").toReal ();
    QVERIFY (yAfterPageUp < yAfterPageDown);

    // 5. 测试滑块拖拽换算联动
    list->setProperty ("contentY", 0.0);
    QTest::qWait (50);
    QCOMPARE (thumb->property ("y").toReal (), 0.0);

    // 模拟拖动滑块 20 像素
    QMetaObject::invokeMethod (scrollbar, "scrollByThumbDelta",
                               Q_ARG (QVariant, 20.0), Q_ARG (QVariant, 0.0));
    QTest::qWait (50);
    QVERIFY (list->property ("contentY").toReal () > 0.0);
    QVERIFY (thumb->property ("y").toReal () > 0.0);

    // 6. 测试搜索过滤：匹配条目减少至视口内时，滚动条自动隐藏
    rootItem->setProperty ("searchQuery", "Chapter 10");
    QTest::qWait (100);
    QVERIFY (!scrollbar->isVisible ());

    // 清空搜索后滚动条恢复显示
    rootItem->setProperty ("searchQuery", "");
    QTest::qWait (100);
    QVERIFY (scrollbar->isVisible ());

    // 7. 全部收起/折叠或清空后自动隐藏
    widget->clear ();
    QTest::qWait (50);
    QVERIFY (!scrollbar->isVisible ());

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
