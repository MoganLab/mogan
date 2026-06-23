
/******************************************************************************
 * MODULE     : base.cpp
 * DESCRIPTION: Implementation of base.hpp for test purpose
 * COPYRIGHT  : (C) 2022  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "file.hpp"
#include <QApplication>
#include <QWidget>
#include <QtTest/QtTest>

#ifndef KERNEL_L1
extern bool headless_mode;
#endif

void
qcompare (string actual, string expected) {
  QCOMPARE (as_charp (actual), as_charp (expected));
}

void
init_lolly () {
  lolly::init_tbox ();
  // for shared link, we need to load_string to warm up
  string s;
  load_string (url_system ("$TEXMACS_PATH/LICENSE"), s, true);
  QVERIFY (N (s) > 0);
}

void
cleanup_qt_top_level_widgets () {
  if (qApp == nullptr) return;
  const QWidgetList tops= qApp->topLevelWidgets ();
  for (QWidget* w : tops) {
    if (w->isVisible ()) w->hide ();
  }
  qApp->processEvents ();
}
