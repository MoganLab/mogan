/******************************************************************************
 * MODULE      : color_picker_bridge_test.cpp
 * DESCRIPTION : Tests for ColorPickerBridge（QML 调色板屏幕取色 bridge）。
 *               覆盖异步取色流程：overlay 打开、点击取色回传 hex、Esc 取消，
 *               以及平台门控。宿主弹窗刻意用 exec() 驱动（复刻 run_qml_dialog
 *               真实路径），用退出码证明取色过程不会意外终止 exec。
 *               会话不支持抓屏（Wayland / offscreen）时跳过交互用例；交互
 *               用例经 MOGAN_TEST_PICK_FAKE_SNAPSHOT 合成纯色快照——macOS
 *               无「屏幕录制」权限时真实抓屏为空图，overlay 路径须可离线覆盖。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/ColorPickerBridge.hpp"
#include "base.hpp"

#include <QDialog>
#include <QScreen>
#include <QTimer>
#include <QtTest/QtTest>

class TestColorPickerBridge : public QObject {
  Q_OBJECT

  // exec 模态下驱动一次取色：pickTimer 发起取色；overlay（ApplicationModal）
  // 出现后按 pressEscape 点中心取色或 Esc 取消（host 自身也是 modal，须排除）；
  // 取色回流后 host.done(42)。退出码 42 证明 exec 未因取色提前退出；
  // 5s 兜底 done(-1) 防挂死。取色结果追加到 m_results。返回 exec 退出码。
  int exec_pick_round (ColorPickerBridge& bridge, bool pressEscape) {
    QDialog host;
    m_results.clear ();

    QTimer pickTimer;
    pickTimer.setSingleShot (true);
    connect (&pickTimer, &QTimer::timeout, &bridge,
             &ColorPickerBridge::pickScreenColor);
    pickTimer.start (100);

    QTimer overlayTimer;
    connect (&overlayTimer, &QTimer::timeout, [&] {
      QWidget* o= QApplication::activeModalWidget ();
      if (o == nullptr || o == &host) return;
      overlayTimer.stop ();
      // 记录 overlay 帧几何供用例断言（xcb 下要求整屏或工作区且贴原点）。
      m_overlayFrame= o->frameGeometry ();
      if (pressEscape) QTest::keyClick (o, Qt::Key_Escape);
      else
        QTest::mouseClick (o, Qt::LeftButton, Qt::NoModifier,
                           o->rect ().center ());
    });
    overlayTimer.start (50);

    connect (&bridge, &ColorPickerBridge::screenColorPicked, &host,
             [&] (const QString& hex) {
               m_results << hex;
               host.done (42);
             });
    QTimer::singleShot (5000, &host, [&host] { host.done (-1); });

    return host.exec ();
  }

  QStringList m_results;
  QRect       m_overlayFrame;

private slots:
  void init () { init_lolly (); }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  // Wayland 会话抓不到真实屏幕（mogan 强制 xcb，经 XWayland 只能抓到黑根
  // 窗口）：canPickScreen 须看 WAYLAND_DISPLAY 而非 platformName。
  void test_canPickScreen_platform_gate () {
    ColorPickerBridge bridge;
    const bool waylandSession= QGuiApplication::platformName ().startsWith (
                                   "wayland", Qt::CaseInsensitive) ||
                               qEnvironmentVariableIsSet ("WAYLAND_DISPLAY");
    QCOMPARE (bridge.canPickScreen (), !waylandSession);
  }

  // 点击 overlay：回传合法 "#rrggbb"；exec 不被取色打断（退出码 42）；
  // xcb 下 overlay 帧几何须为整屏或工作区且贴屏幕原点（其他平台只要求可见）。
  // 用合成快照钩子：macOS 无「屏幕录制」权限时 grabWindow 返回空图，
  // 真实抓屏在 CI 与本地都无法覆盖 overlay 交互路径。
  void test_pick_click_delivers_hex () {
    ColorPickerBridge bridge;
    if (!bridge.canPickScreen ()) QSKIP ("当前会话不支持屏幕取色");
    qputenv ("MOGAN_TEST_PICK_FAKE_SNAPSHOT", "#336699");

    QCOMPARE (exec_pick_round (bridge, false), 42);
    qunsetenv ("MOGAN_TEST_PICK_FAKE_SNAPSHOT");
    QCOMPARE (m_results.size (), 1);
    QCOMPARE (m_results.first (), QStringLiteral ("#336699"));
    QScreen* scr= QGuiApplication::screenAt (m_overlayFrame.center ());
    QVERIFY (scr != nullptr);
    // 严格几何断言仅针对 xcb：KWin 曾把整屏窗口压进工作区导致快照错位，
    // 是该断言要防的回归；macOS 的 WindowServer 按自身规则摆放 Tool 窗口
    // （菜单栏/边框偏移），几何不可预期，但绘制/取色按全局坐标对齐，
    // 与窗口实际摆放无关，故只要求 overlay 非空可见。
    if (QGuiApplication::platformName () == QStringLiteral ("xcb")) {
      QVERIFY2 (
          m_overlayFrame == scr->geometry () ||
              m_overlayFrame == scr->availableGeometry (),
          qPrintable (QStringLiteral ("frame=%1 geo=%2 avail=%3")
                          .arg (QString::fromLatin1 (
                              QDebug::toString (m_overlayFrame).toUtf8 ()))
                          .arg (QString::fromLatin1 (
                              QDebug::toString (scr->geometry ()).toUtf8 ()))
                          .arg (QString::fromLatin1 (
                              QDebug::toString (scr->availableGeometry ())
                                  .toUtf8 ()))));
      QCOMPARE (m_overlayFrame.topLeft (), scr->geometry ().topLeft ());
    }
    else {
      QVERIFY2 (!m_overlayFrame.isEmpty (),
                qPrintable (QString::fromLatin1 (
                    QDebug::toString (m_overlayFrame).toUtf8 ())));
    }
  }

  // Esc 取消：回传空串；exec 同样不被打断。
  void test_pick_escape_cancels () {
    ColorPickerBridge bridge;
    if (!bridge.canPickScreen ()) QSKIP ("当前会话不支持屏幕取色");
    qputenv ("MOGAN_TEST_PICK_FAKE_SNAPSHOT", "#336699");

    QCOMPARE (exec_pick_round (bridge, true), 42);
    qunsetenv ("MOGAN_TEST_PICK_FAKE_SNAPSHOT");
    QCOMPARE (m_results.size (), 1);
    QCOMPARE (m_results.first (), QString ());
  }
};

QTEST_MAIN (TestColorPickerBridge)
#include "color_picker_bridge_test.moc"
