/******************************************************************************
 * MODULE     : converter_test.cpp
 * DESCRIPTION: Properties of characters and strings
 * COPYRIGHT  : (C) 2019 Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "converter.hpp"
#include "file.hpp"

class TestConverter : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); };
  void test_utf8_to_cork ();
  void test_cork_to_utf8 ();
};

void
TestConverter::test_utf8_to_cork () {
  qcompare (utf8_to_cork ("中"), "<#4E2D>");
  qcompare (utf8_to_cork ("“"), "\x10");
  qcompare (utf8_to_cork ("”"), "\x11");
}

void
TestConverter::test_cork_to_utf8 () {
  qcompare (cork_to_utf8 ("<#4E2D>"), "中");
  qcompare (cork_to_utf8 ("\x10"), "“");
  qcompare (cork_to_utf8 ("\x11"), "”");
}

QTEST_MAIN (TestConverter)
#include "converter_test.moc"
