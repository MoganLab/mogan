/******************************************************************************
 * MODULE      : color_picker_bridge_test.cpp
 * DESCRIPTION : Tests for ColorPickerBridge（QML 调色板屏幕取色 bridge）。
 *               覆盖异步取色流程：overlay 打开、点击取色回传 hex、Esc 取消，
 *               以及平台门控。宿主弹窗刻意用 exec() 驱动（复刻 run_qml_dialog
 *               真实路径），用退出码证明取色过程不会意外终止 exec。
 *               会话不支持抓屏（Wayland / offscreen）时跳过交互用例。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/ColorPickerBridge.hpp"
#include "base.hpp"

#include <QDialog>
#include <QRegularExpression>
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
      // 记录 overlay 帧几何：允许整屏或被 WM 约束到工作区（KWin 会压缩
      // 整屏请求），但必须贴着屏幕原点——错位会让快照与真实桌面对不上。
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
  // overlay 帧几何为整屏或工作区（WM 可约束），但须贴屏幕原点。
  void test_pick_click_delivers_hex () {
    ColorPickerBridge bridge;
    if (!bridge.canPickScreen ()) QSKIP ("当前会话不支持屏幕取色");

    QCOMPARE (exec_pick_round (bridge, false), 42);
    QCOMPARE (m_results.size (), 1);
    const QString hex= m_results.first ();
    if (hex.isEmpty ()) QSKIP ("当前平台抓屏不可用（grabWindow 返回空图）");
    QVERIFY2 (QRegularExpression ("^#[0-9a-f]{6}$").match (hex).hasMatch (),
              qPrintable (hex));
    QScreen* scr= QGuiApplication::screenAt (m_overlayFrame.center ());
    QVERIFY (scr != nullptr);
    QVERIFY (m_overlayFrame == scr->geometry () ||
             m_overlayFrame == scr->availableGeometry ());
    QCOMPARE (m_overlayFrame.topLeft (), scr->geometry ().topLeft ());
  }

  // Esc 取消：回传空串；exec 同样不被打断。
  void test_pick_escape_cancels () {
    ColorPickerBridge bridge;
    if (!bridge.canPickScreen ()) QSKIP ("当前会话不支持屏幕取色");

    QCOMPARE (exec_pick_round (bridge, true), 42);
    QCOMPARE (m_results.size (), 1);
    QCOMPARE (m_results.first (), QString ());
  }
};

QTEST_MAIN (TestColorPickerBridge)
#include "color_picker_bridge_test.moc"
