/******************************************************************************
 * MODULE      : color_picker_bridge_test.cpp
 * DESCRIPTION : Tests for ColorPickerBridge（QML 调色板屏幕取色 bridge）。
 *               覆盖异步取色流程：overlay 打开、点击取色回传 hex、Esc 取消、
 *               取色期间宿主隐藏/结束恢复，以及平台不支持时的空串兜底。
 *               平台不支持抓屏（Wayland / offscreen）时跳过交互用例。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/ColorPickerBridge.hpp"
#include "base.hpp"

#include <QRegularExpression>
#include <QSignalSpy>
#include <QtTest/QtTest>

class TestColorPickerBridge : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  // Wayland 不允许任意抓屏：canPickScreen 为 false，取色立即回空串。
  void test_canPickScreen_platform_gate () {
    ColorPickerBridge bridge;
    const bool isWayland= QGuiApplication::platformName ().startsWith (
        "wayland", Qt::CaseInsensitive);
    QCOMPARE (bridge.canPickScreen (), !isWayland);
  }

  // 点击 overlay：回传合法 "#rrggbb"，取色期间宿主隐藏、结束后恢复。
  void test_pick_click_delivers_hex () {
    ColorPickerBridge bridge (&m_host);
    if (!bridge.canPickScreen ()) QSKIP ("当前平台不支持屏幕取色");
    m_host.show ();

    QSignalSpy spy (&bridge, &ColorPickerBridge::screenColorPicked);
    bridge.pickScreenColor ();
    // offscreen 等平台 grabWindow 返回空图：立即回空串，无 overlay。
    if (spy.count () == 1 && spy.first ().first ().toString ().isEmpty ())
      QSKIP ("当前平台抓屏不可用（grabWindow 返回空图）");

    QWidget* overlay= nullptr;
    QTRY_VERIFY ((overlay= QApplication::activeModalWidget ()) != nullptr);
    QVERIFY (m_host.isHidden ());

    QTest::mouseClick (overlay, Qt::LeftButton, Qt::NoModifier,
                       overlay->rect ().center ());
    QTRY_COMPARE (spy.count (), 1);
    const QString hex= spy.first ().first ().toString ();
    QVERIFY2 (QRegularExpression ("^#[0-9a-f]{6}$").match (hex).hasMatch (),
              qPrintable (hex));
    QVERIFY (!m_host.isHidden ());
  }

  // Esc 取消：回传空串，宿主恢复显示。
  void test_pick_escape_cancels () {
    ColorPickerBridge bridge (&m_host);
    if (!bridge.canPickScreen ()) QSKIP ("当前平台不支持屏幕取色");
    m_host.show ();

    QSignalSpy spy (&bridge, &ColorPickerBridge::screenColorPicked);
    bridge.pickScreenColor ();
    if (spy.count () == 1 && spy.first ().first ().toString ().isEmpty ())
      QSKIP ("当前平台抓屏不可用（grabWindow 返回空图）");

    QWidget* overlay= nullptr;
    QTRY_VERIFY ((overlay= QApplication::activeModalWidget ()) != nullptr);

    QTest::keyClick (overlay, Qt::Key_Escape);
    QTRY_COMPARE (spy.count (), 1);
    QCOMPARE (spy.first ().first ().toString (), QString ());
    QVERIFY (!m_host.isHidden ());
  }

private:
  QWidget m_host;
};

QTEST_MAIN (TestColorPickerBridge)
#include "color_picker_bridge_test.moc"
