
/******************************************************************************
 * MODULE     : tm_locale_test.cpp
 * DESCRIPTION: Unit tests for tm_locale (get_date)
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tm_locale.hpp"
#include "base.hpp"
#include "converter.hpp"
#include "string.hpp"
#include <QtTest/QtTest>

class TestTmLocale : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_get_date_strftime_year ();
  void test_get_date_strftime_month ();
  void test_get_date_strftime_day ();
  void test_get_date_strftime_full ();
  void test_get_date_strftime_month_name ();
  void test_get_date_strftime_weekday ();
  void test_get_date_strftime_time ();

  void test_get_date_qt_year ();
  void test_get_date_qt_month ();
  void test_get_date_qt_day ();
  void test_get_date_qt_month_name ();
  void test_get_date_qt_weekday ();

  void test_get_date_default_english ();
  void test_get_date_default_british ();
  void test_get_date_default_american ();
  void test_get_date_default_german ();
  void test_get_date_default_chinese ();
  void test_get_date_default_japanese ();
  void test_get_date_default_korean ();
  void test_get_date_default_unknown_language ();

  void test_get_date_current_date_fallback ();
};

QTEST_MAIN (TestTmLocale)
#include "tm_locale_test.moc"

/******************************************************************************
 * strftime format tests
 ******************************************************************************/

void
TestTmLocale::test_get_date_strftime_year () {
  qcompare (get_date ("english", "%Y", 2024, 1, 15), "2024");
  qcompare (get_date ("english", "%Y", 2023, 12, 31), "2023");
}

void
TestTmLocale::test_get_date_strftime_month () {
  qcompare (get_date ("english", "%m", 2024, 1, 15), "01");
  qcompare (get_date ("english", "%m", 2024, 6, 25), "06");
  qcompare (get_date ("english", "%m", 2023, 12, 31), "12");
  qcompare (get_date ("english", "%m", 2024, 11, 9), "11");
}

void
TestTmLocale::test_get_date_strftime_day () {
  qcompare (get_date ("english", "%d", 2024, 1, 15), "15");
  qcompare (get_date ("english", "%d", 2024, 6, 25), "25");
  qcompare (get_date ("english", "%d", 2024, 11, 9), "09");
}

void
TestTmLocale::test_get_date_strftime_full () {
  qcompare (get_date ("english", "%Y-%m-%d", 2024, 1, 15), "2024-01-15");
  qcompare (get_date ("english", "%Y-%m-%d", 2023, 12, 31), "2023-12-31");
}

void
TestTmLocale::test_get_date_strftime_month_name () {
  // %B is locale-dependent via strftime; only verify it is supported.
  string r= get_date ("english", "%B", 2024, 1, 15);
  QVERIFY (N(r) > 0);
}

void
TestTmLocale::test_get_date_strftime_weekday () {
  // %A is locale-dependent via strftime; only verify it is supported.
  string r= get_date ("english", "%A", 2024, 1, 15);
  QVERIFY (N(r) > 0);
}

void
TestTmLocale::test_get_date_strftime_time () {
  qcompare (get_date ("english", "%H:%M", 2024, 1, 15), "00:00");
  qcompare (get_date ("english", "%H:%M:%S", 2024, 6, 25), "00:00:00");
}

/******************************************************************************
 * Qt format tests
 ******************************************************************************/

void
TestTmLocale::test_get_date_qt_year () {
  qcompare (get_date ("english", "yyyy", 2024, 1, 15), "2024");
  qcompare (get_date ("english", "yyyy", 2023, 12, 31), "2023");
}

void
TestTmLocale::test_get_date_qt_month () {
  qcompare (get_date ("english", "M", 2024, 1, 15), "1");
  qcompare (get_date ("english", "M", 2024, 6, 25), "6");
  qcompare (get_date ("english", "M", 2023, 12, 31), "12");
}

void
TestTmLocale::test_get_date_qt_day () {
  qcompare (get_date ("english", "d", 2024, 1, 15), "15");
  qcompare (get_date ("english", "d", 2024, 11, 9), "9");
  qcompare (get_date ("english", "d", 2023, 12, 31), "31");
}

void
TestTmLocale::test_get_date_qt_month_name () {
  qcompare (get_date ("english", "MMMM", 2024, 1, 15), "January");
  qcompare (get_date ("english", "MMMM", 2024, 6, 25), "June");
  qcompare (get_date ("english", "MMMM", 2023, 12, 31), "December");
}

void
TestTmLocale::test_get_date_qt_weekday () {
  qcompare (get_date ("english", "dddd", 2024, 1, 15), "Monday");
  qcompare (get_date ("english", "dddd", 2024, 6, 25), "Tuesday");
  qcompare (get_date ("english", "dddd", 2023, 12, 31), "Sunday");
}

/******************************************************************************
 * Default format tests
 ******************************************************************************/

void
TestTmLocale::test_get_date_default_english () {
  qcompare (get_date ("english", "", 2024, 1, 15), "January 15, 2024");
  qcompare (get_date ("english", "", 2024, 6, 25), "June 25, 2024");
}

void
TestTmLocale::test_get_date_default_british () {
  qcompare (get_date ("british", "", 2024, 1, 15), "January 15, 2024");
  qcompare (get_date ("british", "", 2024, 6, 25), "June 25, 2024");
}

void
TestTmLocale::test_get_date_default_american () {
  qcompare (get_date ("american", "", 2024, 1, 15), "January 15, 2024");
  qcompare (get_date ("american", "", 2024, 6, 25), "June 25, 2024");
}

void
TestTmLocale::test_get_date_default_german () {
  qcompare (get_date ("german", "", 2024, 1, 15), "15. Januar 2024");
  qcompare (get_date ("german", "", 2024, 6, 25), "25. Juni 2024");
}

void
TestTmLocale::test_get_date_default_chinese () {
  string r= herk_to_utf8 (get_date ("chinese", "", 2024, 1, 15));
  qcompare (r, "2024年1月15日");
}

void
TestTmLocale::test_get_date_default_japanese () {
  string r= herk_to_utf8 (get_date ("japanese", "", 2024, 6, 25));
  qcompare (r, "2024年6月25日");
}

void
TestTmLocale::test_get_date_default_korean () {
  string r= herk_to_utf8 (get_date ("korean", "", 2024, 1, 15));
  qcompare (r, "2024년 1월 15일");
}

void
TestTmLocale::test_get_date_default_unknown_language () {
  qcompare (get_date ("unknown_language", "", 2024, 1, 15), "15 January 2024");
  qcompare (get_date ("unknown_language", "", 2024, 6, 25), "25 June 2024");
}

void
TestTmLocale::test_get_date_current_date_fallback () {
  string r= get_date ("english", "%Y");
  QVERIFY (N(r) == 4);
  for (int i= 0; i < N(r); i++)
    QVERIFY (r[i] >= '0' && r[i] <= '9');
}
