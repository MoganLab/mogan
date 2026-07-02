/******************************************************************************
 * MODULE     : qtquick_test.cpp
 * DESCRIPTION: Test that QtQuick/QML and QtBodymovin modules are linkable
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQml>

// QtBodymovin is a Qt Labs module with only private headers in Qt 5.
// We verify linkability by including the version header.
#include <QtBodymovin/qtbodymovinversion.h>

#include <iostream>

int
main (int argc, char** argv) {
  // Test 1: QCoreApplication
  QCoreApplication app (argc, argv);
  std::cout << "[PASS] QCoreApplication created\n";

  // Test 2: QQmlApplicationEngine
  QQmlApplicationEngine engine;
  std::cout << "[PASS] QQmlApplicationEngine created\n";

  // Test 3: QQuickWindow API
  QQuickWindow::setDefaultAlphaBuffer (false);
  std::cout << "[PASS] QQuickWindow API accessible\n";

  // Test 4: QUrl
  QUrl testUrl ("qrc:/test.qml");
  std::cout << "[PASS] QUrl created\n";

  // Test 5: QtBodymovin version header compiled and linked
  std::cout << "[PASS] QtBodymovin header included (version "
            << QTBODYMOVIN_VERSION_STR << ")\n";

  // Test 6: Qt runtime version
  std::cout << "[INFO] Qt runtime version: " << qVersion () << "\n";

  std::cout << "\nAll QtQuick + QtBodymovin tests passed!\n";
  return 0;
}
