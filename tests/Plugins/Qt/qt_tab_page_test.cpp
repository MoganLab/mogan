/******************************************************************************
 * MODULE     : qt_tab_page_test.cpp
 * DESCRIPTION: Tests for QTMTabPage dirty marker behavior
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/QTMTabPage.hpp"
#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include <QApplication>
#include <QCursor>
#include <QList>
#include <QMouseEvent>
#include <QScreen>
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

// 断言「未悬停」状态前调用。QT_QPA_PLATFORM=offscreen（Debian CI）下虚拟
// 光标恒在 (0,0)，顶层窗口也映射在 (0,0)，show() 的合成 Enter 事件会把
// m_hoverOnTab 置 true，导致未悬停断言随环境翻转；macOS runner 的窗口默认
// 位置同样可能压在真实光标下。把窗口挪到不含光标的屏幕角落并补一个 Leave
// 事件，使 hover 状态归零且后续不再受真实/合成光标影响。
void
moveAwayFromCursor (QWidget& w) {
  const QRect         avail= w.screen ()->availableGeometry ();
  const QList<QPoint> corners{
      {avail.left () + 16, avail.top () + 16},
      {avail.right () - w.width () - 16, avail.top () + 16},
      {avail.left () + 16, avail.bottom () - w.height () - 16},
      {avail.right () - w.width () - 16, avail.bottom () - w.height () - 16}};
  for (const QPoint& c : corners)
    if (!QRect (c, w.size ()).contains (QCursor::pos ())) {
      w.move (c);
      break;
    }
  QEvent leave (QEvent::Leave);
  QApplication::sendEvent (&w, &leave);
}
} // namespace

class TestQTMTabPage : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  // 回归（1266）：脏标签悬停任意位置（含远离关闭按钮的文本区）即显示 ×，
  // × 取代关闭按钮位置的脏圆点；离开后 × 隐藏，回落为脏圆点。
  void test_dirty_title_shows_close_button_on_hover () {
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
    moveAwayFromCursor (tab);
    QVERIFY (!closeBtn->isVisible ());

    // macOS (Cocoa) 的 QTest::mouseMove 不会向未 grab 鼠标的 widget 派发
    // 事件，因此直接合成 Enter/Leave 事件投递给 tab，触发其 hover 检测
    // 逻辑（等价于 Windows 上鼠标移入/移出标签页）。
    QPoint      textArea (20, tab.height () / 2);
    QEnterEvent enterEvent (QPointF (textArea), QPointF (textArea),
                            tab.mapToGlobal (textArea));
    QApplication::sendEvent (&tab, &enterEvent);
    QTRY_VERIFY (closeBtn->isVisible ());

    QEvent leaveEvent (QEvent::Leave);
    QApplication::sendEvent (&tab, &leaveEvent);
    QTRY_VERIFY (!closeBtn->isVisible ());
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

  // 回归（0910）：updateActiveTab -> setChecked(true) 经
  // QToolButton::checkStateSet 把勾选状态写回 defaultAction，ActionChanged
  // 同步派发回按钮后 QToolButton 重新执行 setDefaultAction，用 action 文本
  // （若仍带尾部 ` *`）覆盖按钮文本——文本里的 `*` 与关闭按钮位置的脏标记
  // `*` 叠加显示成两个星号。修复：剥离 `*` 时把干净标题同步回 action 文本。
  void test_checked_state_sync_keeps_title_clean () {
    QAction    titleAction ("no_name_1.tmu *", nullptr);
    QAction    closeAction ("Close", nullptr);
    QTMTabPage tab (url ("tmfs://view/9"), &titleAction, &closeAction, false);
    tab.resize (220, 32);
    tab.show ();
    QVERIFY (QTest::qWaitForWindowExposed (&tab));
    QVERIFY (tab.isDirty ());
    QCOMPARE (tab.text (), QString::fromUtf8 ("no_name_1.tmu"));

    tab.setChecked (true); // 模拟 updateActiveTab 激活该标签页
    QVERIFY (tab.isDirty ());
    QCOMPARE (tab.text (), QString::fromUtf8 ("no_name_1.tmu"));
    QCOMPARE (titleAction.text (), QString::fromUtf8 ("no_name_1.tmu"));
    // 活动 + 脏 + 未悬停：关闭按钮隐藏，显示的是脏圆点而非 ×（1266）。
    auto* closeBtn= tab.findChild<QWK::WindowButton*> ("tabpage-close-button");
    QVERIFY (closeBtn != nullptr);
    moveAwayFromCursor (tab);
    QVERIFY (!closeBtn->isVisible ());

    tab.setChecked (false); // 切走后再切回，回写路径重复触发仍需保持干净
    tab.setChecked (true);
    QVERIFY (tab.isDirty ());
    QVERIFY (!tab.text ().endsWith (QLatin1Char ('*')));
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
