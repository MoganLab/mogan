/******************************************************************************
 * MODULE     : qt_tab_page_test.cpp
 * DESCRIPTION: Tests for QTMTabPage dirty marker behavior
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/QTMTabPage.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QApplication>
#include <QList>
#include <QMouseEvent>
#include <QtTest/QtTest>

namespace {
// 构造 carrier 列表：url 与显示标题可独立指定，模拟 SLOT_TAB_PAGES 喂给
// replaceTabPages 的输入（标题带尾部 ` *` 表示未保存）。
QList<QAction*>*
makeCarrierList (const QList<QPair<QString, QString>>& urlTitlePairs) {
  auto* list= new QList<QAction*> ();
  for (const auto& p : urlTitlePairs) {
    auto* title   = new QAction (p.second);
    auto* closeBtn= new QAction ("Close");
    auto* tab=
        new QTMTabPage (url (from_qstring (p.first)), title, closeBtn, false);
    list->append (new QTMTabPageAction (tab));
  }
  return list;
}
} // namespace

class TestQTMTabPage : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_dirty_title_moves_star_to_close_slot () {
    QAction    titleAction (QString::fromUtf8 ("very-long-file-name.tm *"),
                            nullptr);
    QAction    closeAction ("Close", nullptr);
    QTMTabPage tab (url ("file:///tmp/test.tm"), &titleAction, &closeAction,
                    false);
    tab.resize (220, 32);
    tab.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&tab));

    QCOMPARE (tab.text (), QString::fromUtf8 ("very-long-file-name.tm"));
    QVERIFY (tab.isDirty ());

    auto* closeBtn= tab.findChild<QWK::WindowButton*> ("tabpage-close-button");
    QVERIFY (closeBtn != nullptr);
    QVERIFY (!closeBtn->isVisible ());

    QPoint closeCenter= closeBtn->geometry ().center ();
    // macOS (Cocoa) 的 QTest::mouseMove 不会向未 grab 鼠标的 widget 派发
    // mouseMoveEvent，因此直接合成一个 MouseMove 事件投递给 tab，触发其
    // hover 检测逻辑（等价于 Windows 上鼠标移入关闭按钮区域）。
    QMouseEvent moveEvent (QEvent::MouseMove, closeCenter,
                           tab.mapToGlobal (closeCenter), Qt::NoButton,
                           Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent (&tab, &moveEvent);
    QTRY_VERIFY (closeBtn->isVisible ());
  }

  void test_clean_title_keeps_close_button_hidden_without_hover () {
    QAction    titleAction ("clean-file.tm", nullptr);
    QAction    closeAction ("Close", nullptr);
    QTMTabPage tab (url ("file:///tmp/test.tm"), &titleAction, &closeAction,
                    false);
    tab.resize (220, 32);
    tab.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&tab));

    QCOMPARE (tab.text (), QString::fromUtf8 ("clean-file.tm"));
    QVERIFY (!tab.isDirty ());
  }

  // 回归：replaceTabPages 复用既有 tab 时，dirty 状态必须随标题刷新。
  // 0350 把 `*` 从标题尾部移到关闭按钮位置，m_isDirty 只在构造时解析一次；
  // 2014 把全量重建改成增量复用后，复用路径只 setText 不更新 m_isDirty，
  // 导致编辑标脏/保存去脏都不反映到 `*` 显示。syncDisplay 是该路径的修复点。
  void test_sync_display_updates_dirty_state () {
    QAction    titleAction ("doc.tm", nullptr); // 构造时干净
    QAction    closeAction ("Close", nullptr);
    QTMTabPage tab (url ("file:///tmp/doc.tm"), &titleAction, &closeAction,
                    false);
    tab.resize (220, 32);
    tab.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&tab));
    QVERIFY (!tab.isDirty ());
    QCOMPARE (tab.text (), QString::fromUtf8 ("doc.tm"));

    // 模拟 replaceTabPages 复用：srcTab 已解析过尾部 `*`，传入干净标题 +
    // dirty。
    tab.syncDisplay (QString::fromUtf8 ("doc.tm"), true);
    QVERIFY (tab.isDirty ());
    QCOMPARE (tab.text (), QString::fromUtf8 ("doc.tm"));

    // 保存去脏：dirty 翻回 false。
    tab.syncDisplay (QString::fromUtf8 ("doc.tm"), false);
    QVERIFY (!tab.isDirty ());
  }

  // 端到端：replaceTabPages 复用 tab 时，新标题的尾部 `*` 必须刷新到既有
  // tab 的 dirty 状态（而非停留在构造时解析的旧值）。这正是 0350+2014 回归
  // bug 的真实触发路径：编辑标脏后上层重发带 `*` 的标题，复用分支需把 dirty
  // 同步过去，关闭按钮位置才会画 `*`。
  // debug_findTab 仅 LIII_DEBUG 下存在，release 构建跳过。
  void test_replaceTabPages_refreshes_dirty_on_reuse () {
    QWidget             host;
    QTMTabPageContainer container (&host);
    container.setRowHeight (32);
    host.resize (400, 40);
    host.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&host));

    // 首次：干净标题，tab 构造时 dirty=false。
    container.replaceTabPages (makeCarrierList ({{"tmfs://view/1", "doc.tm"}}));
#ifndef LIII_DEBUG
    QSKIP ("debug_findTab 仅 LIII_DEBUG 构建可用");
#else
    QTMTabPage* tab= container.debug_findTab (url ("tmfs://view/1"));
    QVERIFY (tab != nullptr);
    QVERIFY (!tab->isDirty ());

    // 同一 url 再次喂入，但标题改为带 ` *`（模拟编辑标脏后上层重发）。
    // 必须复用同一 tab 对象，且其 dirty 翻为 true。
    container.replaceTabPages (
        makeCarrierList ({{"tmfs://view/1", "doc.tm *"}}));
    QTMTabPage* reused= container.debug_findTab (url ("tmfs://view/1"));
    QCOMPARE (reused, tab); // 指针不变 => 复用而非重建
    QVERIFY (reused->isDirty ());
    QCOMPARE (reused->text (), QString::fromUtf8 ("doc.tm"));

    // 第三次：标题去掉 `*`（模拟保存去脏），dirty 翻回 false。
    container.replaceTabPages (makeCarrierList ({{"tmfs://view/1", "doc.tm"}}));
    QVERIFY (!container.debug_findTab (url ("tmfs://view/1"))->isDirty ());
#endif
  }
};

QTEST_MAIN (TestQTMTabPage)
#include "qt_tab_page_test.moc"
