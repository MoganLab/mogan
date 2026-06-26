/******************************************************************************
 * MODULE     : qt_tabpage_rebuild_test.cpp
 * DESCRIPTION: 验证 tab 增删/切换不触发整条标签栏重建的性能回归测试
 * 计数器仅在 LIII_DEBUG 下存在，release 构建相应断言被编译期跳过。
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/QTMTabPage.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QAction>
#include <QApplication>
#include <QList>
#include <QtTest/QtTest>

namespace {
// 构造 carrier 列表：每个 url 一个 QTMTabPage，包成 QTMTabPageAction，
// 模拟 SLOT_TAB_PAGES 喂给 replaceTabPages 的输入。
QList<QAction*>*
makeCarrierList (const QList<QString>& urls) {
  auto* list= new QList<QAction*> ();
  for (const auto& u : urls) {
    auto* title   = new QAction (u);
    auto* closeBtn= new QAction ("Close");
    auto* tab= new QTMTabPage (url (from_qstring (u)), title, closeBtn, false);
    list->append (new QTMTabPageAction (tab));
  }
  return list;
}
} // namespace

class TestQTMTabPageRebuild : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  // 仅切换 active => updateActiveTab 不摄取/移除任何 widget。
  void test_activeSwitchDoesNotRebuild () {
    QWidget             host;
    QTMTabPageContainer container (&host);
    container.setRowHeight (32);
    host.resize (800, 40);
    host.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&host));

    QList<QAction*>* tabs= makeCarrierList ({"tmfs://view/1", "tmfs://view/2"});
    container.replaceTabPages (tabs);

#ifdef LIII_DEBUG
    int added0  = container.debug_added_count;
    int removed0= container.debug_removed_count;
    int active0 = container.debug_active_count;
#else
    QSKIP ("计数器仅在 LIII_DEBUG 构建可用");
#endif

    // 来回切 active 10 次：这正是性能问题的原始场景。
    for (int i= 0; i < 5; ++i) {
      container.updateActiveTab (url ("tmfs://view/2"));
      container.updateActiveTab (url ("tmfs://view/1"));
    }

#ifdef LIII_DEBUG
    QCOMPARE (container.debug_added_count, added0);        // 未摄取
    QCOMPARE (container.debug_removed_count, removed0);    // 未移除
    QCOMPARE (container.debug_active_count - active0, 10); // 轻量命中 10 次
#endif
  }

  // 加一个 tab => 仅摄取 1 个、复用其余。
  void test_addTabIsIncremental () {
    QWidget             host;
    QTMTabPageContainer container (&host);
    container.setRowHeight (32);
    host.resize (800, 40);
    host.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&host));

    container.replaceTabPages (
        makeCarrierList ({"tmfs://view/1", "tmfs://view/2"}));
#ifdef LIII_DEBUG
    QCOMPARE (container.debug_added_count, 2); // 首次全部摄取
    QCOMPARE (container.debug_removed_count, 0);
#else
    QSKIP ("计数器仅在 LIII_DEBUG 构建可用");
#endif

    container.replaceTabPages (
        makeCarrierList ({"tmfs://view/1", "tmfs://view/2", "tmfs://view/3"}));
#ifdef LIII_DEBUG
    QCOMPARE (container.debug_added_count, 3);   // 仅 +1
    QCOMPARE (container.debug_removed_count, 0); // 未移除
#endif
  }

  // 删一个 tab => 仅 deleteLater 1 个、复用其余。
  void test_removeTabIsIncremental () {
    QWidget             host;
    QTMTabPageContainer container (&host);
    container.setRowHeight (32);
    host.resize (800, 40);
    host.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&host));

    container.replaceTabPages (
        makeCarrierList ({"tmfs://view/1", "tmfs://view/2", "tmfs://view/3"}));

    container.replaceTabPages (makeCarrierList ({"tmfs://view/1"}));
#ifdef LIII_DEBUG
    QCOMPARE (container.debug_removed_count, 2); // 仅移除多余的两个
    QCOMPARE (container.debug_added_count, 3);   // 首次摄取的不变，无新增
#else
    QSKIP ("计数器仅在 LIII_DEBUG 构建可用");
#endif
  }

  // 集合不变、仅顺序变化 => 不摄取、不移除。
  void test_reorderDoesNotRebuild () {
    QWidget             host;
    QTMTabPageContainer container (&host);
    container.setRowHeight (32);
    host.resize (800, 40);
    host.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&host));

    container.replaceTabPages (
        makeCarrierList ({"tmfs://view/1", "tmfs://view/2", "tmfs://view/3"}));
#ifdef LIII_DEBUG
    int added0  = container.debug_added_count;
    int removed0= container.debug_removed_count;
#else
    QSKIP ("计数器仅在 LIII_DEBUG 构建可用");
#endif

    container.replaceTabPages (
        makeCarrierList ({"tmfs://view/3", "tmfs://view/1", "tmfs://view/2"}));
#ifdef LIII_DEBUG
    QCOMPARE (container.debug_added_count, added0);
    QCOMPARE (container.debug_removed_count, removed0);
#endif
  }
};

QTEST_MAIN (TestQTMTabPageRebuild)
#include "qt_tabpage_rebuild_test.moc"
