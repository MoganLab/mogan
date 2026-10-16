/******************************************************************************
 * MODULE     : qt_outline_widget_test.cpp
 * DESCRIPTION: Tests for OutlineWidget & OutlineBridge (PDF & Document ToC)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "base.hpp"
#include "qt_pdf_outline_widget.hpp"
#include "qt_pdf_toolbar.hpp"
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

  static QVariantMap makeTestNode (const QString& id, const QString& title,
                                   const QVariantList& children= {}) {
    QVariantMap map;
    map["id"]      = id;
    map["title"]   = title;
    map["children"]= children;
    return map;
  }

  void test_defaultExpandedMap_empty () {
    QVariantMap map= OutlineBridge::computeDefaultExpandedMap ({});
    QVERIFY (map.isEmpty ());
  }

  void test_defaultExpandedMap_rule1_single_child () {
    // 6027_01: Part I -> Chapter 1, Chapter 2
    QVariantList chaps;
    chaps.append (makeTestNode ("0/0", "Chapter 1"));
    chaps.append (makeTestNode ("0/1", "Chapter 2"));
    QVariantList root;
    root.append (makeTestNode ("0", "Part I", chaps));

    QVariantMap map= OutlineBridge::computeDefaultExpandedMap (root);
    QCOMPARE (map.value ("0").toBool (), true);
  }

  void test_defaultExpandedMap_rule1_chain () {
    // 6027_02: Part I -> Chapter 1 -> Section 1, Section 2
    QVariantList secs;
    secs.append (makeTestNode ("0/0/0", "Section 1"));
    secs.append (makeTestNode ("0/0/1", "Section 2"));
    QVariantList chaps;
    chaps.append (makeTestNode ("0/0", "Chapter 1", secs));
    QVariantList root;
    root.append (makeTestNode ("0", "Part I", chaps));

    QVariantMap map= OutlineBridge::computeDefaultExpandedMap (root);
    QCOMPARE (map.value ("0").toBool (), true);
    QCOMPARE (map.value ("0/0").toBool (), true);
  }

  void test_defaultExpandedMap_rule1_sibling_leaf () {
    // 6027_03: Chapter 1 (with 30 sections) and Chapter* (leaf)
    QVariantList secs;
    for (int i= 0; i < 30; ++i) {
      secs.append (
          makeTestNode (QString ("0/%1").arg (i), QString ("Sec %1").arg (i)));
    }
    QVariantList root;
    root.append (makeTestNode ("0", "Chapter 1", secs));
    root.append (makeTestNode ("1", "Appendix"));

    QVariantMap map= OutlineBridge::computeDefaultExpandedMap (root);
    // Sibling leaf blocks Rule 1; 30 sections exceed threshold for Rule 2
    QCOMPARE (map.value ("0").toBool (), false);
  }

  void test_defaultExpandedMap_rule2_large_book () {
    // 6027_04: 10 Chapters, each with 5 Sections (total 60 > 30)
    QVariantList root;
    for (int c= 0; c < 10; ++c) {
      QVariantList secs;
      for (int s= 0; s < 5; ++s) {
        secs.append (makeTestNode (QString ("%1/%2").arg (c).arg (s), "Sec"));
      }
      root.append (makeTestNode (QString::number (c),
                                 QString ("Chapter %1").arg (c), secs));
    }

    QVariantMap map= OutlineBridge::computeDefaultExpandedMap (root);
    for (int c= 0; c < 10; ++c) {
      QCOMPARE (map.value (QString::number (c)).toBool (), false);
    }
  }

  void test_defaultExpandedMap_rule2_small_article () {
    // 6027_05: 3 Chapters, each with 2 Sections (total 9 <= 30)
    QVariantList root;
    for (int c= 0; c < 3; ++c) {
      QVariantList secs;
      for (int s= 0; s < 2; ++s) {
        secs.append (makeTestNode (QString ("%1/%2").arg (c).arg (s), "Sec"));
      }
      root.append (makeTestNode (QString::number (c),
                                 QString ("Chapter %1").arg (c), secs));
    }

    QVariantMap map= OutlineBridge::computeDefaultExpandedMap (root);
    for (int c= 0; c < 3; ++c) {
      QCOMPARE (map.value (QString::number (c)).toBool (), true);
    }
  }

  void test_defaultExpandedMap_rule2_boundary_30_and_31 () {
    // 6027_06: 5 Chapters, each 5 Sections = 30 <= 30 -> all expand
    QVariantList root30;
    for (int c= 0; c < 5; ++c) {
      QVariantList secs;
      for (int s= 0; s < 5; ++s) {
        secs.append (makeTestNode (QString ("%1/%2").arg (c).arg (s), "Sec"));
      }
      root30.append (makeTestNode (QString::number (c),
                                   QString ("Chapter %1").arg (c), secs));
    }
    QVariantMap map30= OutlineBridge::computeDefaultExpandedMap (root30);
    for (int c= 0; c < 5; ++c) {
      QCOMPARE (map30.value (QString::number (c)).toBool (), true);
    }

    // 6027_07: 5 Chapters, 4 with 5, 1 with 6 = 31 > 30 -> all collapsed
    QVariantList root31;
    for (int c= 0; c < 5; ++c) {
      QVariantList secs;
      int          count= (c == 4) ? 6 : 5;
      for (int s= 0; s < count; ++s) {
        secs.append (makeTestNode (QString ("%1/%2").arg (c).arg (s), "Sec"));
      }
      root31.append (makeTestNode (QString::number (c),
                                   QString ("Chapter %1").arg (c), secs));
    }
    QVariantMap map31= OutlineBridge::computeDefaultExpandedMap (root31);
    for (int c= 0; c < 5; ++c) {
      QCOMPARE (map31.value (QString::number (c)).toBool (), false);
    }
  }

  void test_defaultExpandedMap_rule2_deep_three_levels () {
    // 6027_08: 2 Chapters, each 2 Sections, each 2 Subsections = 14 <= 30
    QVariantList root;
    for (int c= 0; c < 2; ++c) {
      QVariantList secs;
      for (int s= 0; s < 2; ++s) {
        QVariantList subs;
        for (int sub= 0; sub < 2; ++sub) {
          subs.append (makeTestNode (
              QString ("%1/%2/%3").arg (c).arg (s).arg (sub), "Sub"));
        }
        secs.append (
            makeTestNode (QString ("%1/%2").arg (c).arg (s), "Sec", subs));
      }
      root.append (makeTestNode (QString::number (c),
                                 QString ("Chapter %1").arg (c), secs));
    }

    QVariantMap map= OutlineBridge::computeDefaultExpandedMap (root);
    for (int c= 0; c < 2; ++c) {
      QCOMPARE (map.value (QString::number (c)).toBool (), true);
      for (int s= 0; s < 2; ++s) {
        QCOMPARE (map.value (QString ("%1/%2").arg (c).arg (s)).toBool (),
                  true);
      }
    }
  }

  void test_bridge_defaultExpandedMap_and_activeId_reset () {
    OutlineBridge bridge;
    QCOMPARE (bridge.currentId (), QString ());

    QVector<OutlineItem> items;
    OutlineItem          chap;
    chap.title = "Chapter 1";
    chap.target= "0";
    OutlineItem sec;
    sec.title = "Section 1.1";
    sec.target= "0:0";
    chap.children.append (sec);
    items.append (chap);

    bridge.setOutline (items);
    QCOMPARE (bridge.currentId (), QString ());
    // Rule 1 single child penetration: Chapter 1 is expanded
    QCOMPARE (bridge.defaultExpandedMap ().value ("0").toBool (), true);

    // Active item set on click
    bridge.itemClicked ("0", "0");
    QCOMPARE (bridge.currentId (), QString ("0"));

    // Reloading outline resets currentId and recomputes defaultExpandedMap
    bridge.setOutline (items);
    QCOMPARE (bridge.currentId (), QString ());
    QCOMPARE (bridge.defaultExpandedMap ().value ("0").toBool (), true);

    // Clear clears both
    bridge.clear ();
    QCOMPARE (bridge.currentId (), QString ());
    QVERIFY (bridge.defaultExpandedMap ().isEmpty ());
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
