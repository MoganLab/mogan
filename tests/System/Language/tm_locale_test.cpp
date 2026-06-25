
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
#include <QDate>
#include <QtTest/QtTest>

class TestTmLocale : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_get_date_strftime_year ();
  void test_get_date_strftime_month ();
  void test_get_date_strftime_day ();
  void test_get_date_strftime_full ();

  void test_get_date_qt_year ();
  void test_get_date_qt_month ();
  void test_get_date_qt_day ();

  void test_get_date_default_english ();
  void test_get_date_default_british ();
  void test_get_date_default_american ();
  void test_get_date_default_german ();
  void test_get_date_default_chinese ();
  void test_get_date_default_japanese ();
  void test_get_date_default_korean ();
  void test_get_date_default_taiwanese ();
  void test_get_date_default_unknown_language ();
};

QTEST_MAIN (TestTmLocale)
#include "tm_locale_test.moc"

/******************************************************************************
 * Helpers
 ******************************************************************************/

static bool
starts_with (string s, string prefix) {
  int n= N(s), m= N(prefix);
  if (m > n) return false;
  for (int i= 0; i < m; i++)
    if (s[i] != prefix[i]) return false;
  return true;
}

static bool
ends_with (string s, string suffix) {
  int n= N(s), m= N(suffix);
  if (m > n) return false;
  for (int i= 0; i < m; i++)
    if (s[n - m + i] != suffix[i]) return false;
  return true;
}

static bool
is_digit_string (string s) {
  if (N(s) == 0) return false;
  for (int i= 0; i < N(s); i++)
    if (s[i] < '0' || s[i] > '9') return false;
  return true;
}

static string
zero_padded (int n, int width) {
  string r= as_string (n);
  while (N(r) < width)
    r= "0" * r;
  return r;
}

static void
qcompare_range (string actual, int lo, int hi) {
  QVERIFY (N(actual) > 0);
  QVERIFY (is_digit_string (actual));
  int value= 0;
  for (int i= 0; i < N(actual); i++)
    value= value * 10 + (actual[i] - '0');
  QVERIFY (value >= lo);
  QVERIFY (value <= hi);
}

/******************************************************************************
 * strftime format tests
 ******************************************************************************/

void
TestTmLocale::test_get_date_strftime_year () {
  QDate before= QDate::currentDate ();
  string r    = get_date ("english", "%Y");
  QDate after = QDate::currentDate ();
  if (before.year () == after.year ()) {
    qcompare (r, as_string (before.year ()));
  }
  else {
    QVERIFY (N(r) == 4);
    QVERIFY (is_digit_string (r));
  }
}

void
TestTmLocale::test_get_date_strftime_month () {
  QDate before= QDate::currentDate ();
  string r    = get_date ("english", "%m");
  QDate after = QDate::currentDate ();
  if (before == after) {
    qcompare (r, zero_padded (before.month (), 2));
  }
  else {
    qcompare_range (r, 1, 12);
  }
}

void
TestTmLocale::test_get_date_strftime_day () {
  QDate before= QDate::currentDate ();
  string r    = get_date ("english", "%d");
  QDate after = QDate::currentDate ();
  if (before == after) {
    qcompare (r, zero_padded (before.day (), 2));
  }
  else {
    qcompare_range (r, 1, 31);
  }
}

void
TestTmLocale::test_get_date_strftime_full () {
  QDate before= QDate::currentDate ();
  string r    = get_date ("english", "%Y-%m-%d");
  QDate after = QDate::currentDate ();
  if (before == after) {
    string expected= as_string (before.year ()) * "-" *
                     zero_padded (before.month (), 2) * "-" *
                     zero_padded (before.day (), 2);
    qcompare (r, expected);
  }
  else {
    QVERIFY (N(r) == 10);
    QVERIFY (r[4] == '-' && r[7] == '-');
  }
}

/******************************************************************************
 * Qt format tests
 ******************************************************************************/

void
TestTmLocale::test_get_date_qt_year () {
  QDate before= QDate::currentDate ();
  string r    = get_date ("english", "yyyy");
  QDate after = QDate::currentDate ();
  if (before.year () == after.year ()) {
    qcompare (r, as_string (before.year ()));
  }
  else {
    QVERIFY (N(r) == 4);
    QVERIFY (is_digit_string (r));
  }
}

void
TestTmLocale::test_get_date_qt_month () {
  QDate before= QDate::currentDate ();
  string r    = get_date ("english", "M");
  QDate after = QDate::currentDate ();
  if (before == after) {
    qcompare (r, as_string (before.month ()));
  }
  else {
    qcompare_range (r, 1, 12);
  }
}

void
TestTmLocale::test_get_date_qt_day () {
  QDate before= QDate::currentDate ();
  string r    = get_date ("english", "d");
  QDate after = QDate::currentDate ();
  if (before == after) {
    qcompare (r, as_string (before.day ()));
  }
  else {
    qcompare_range (r, 1, 31);
  }
}

/******************************************************************************
 * Default format tests
 ******************************************************************************/

void
TestTmLocale::test_get_date_default_english () {
  string r= get_date ("english", "");
  QVERIFY (N(r) > 0);
  QDate today= QDate::currentDate ();
  QVERIFY (contains (r, as_string (today.year ())));
  QVERIFY (contains (r, as_string (today.day ())));
}

void
TestTmLocale::test_get_date_default_british () {
  string r= get_date ("british", "");
  QVERIFY (N(r) > 0);
  QDate today= QDate::currentDate ();
  QVERIFY (contains (r, as_string (today.year ())));
}

void
TestTmLocale::test_get_date_default_american () {
  string r= get_date ("american", "");
  QVERIFY (N(r) > 0);
  QDate today= QDate::currentDate ();
  QVERIFY (contains (r, as_string (today.year ())));
}

void
TestTmLocale::test_get_date_default_german () {
  string r= get_date ("german", "");
  QVERIFY (N(r) > 0);
  QDate today= QDate::currentDate ();
  QVERIFY (contains (r, as_string (today.year ())));
}

void
TestTmLocale::test_get_date_default_chinese () {
  string r      = get_date ("chinese", "");
  string utf8_r = herk_to_utf8 (r);
  QDate  today  = QDate::currentDate ();
  QVERIFY (starts_with (utf8_r, as_string (today.year ())));
  QVERIFY (ends_with (utf8_r, "日"));
  QVERIFY (contains (utf8_r, "年"));
  QVERIFY (contains (utf8_r, "月"));
}

void
TestTmLocale::test_get_date_default_japanese () {
  string r      = get_date ("japanese", "");
  string utf8_r = herk_to_utf8 (r);
  QDate  today  = QDate::currentDate ();
  QVERIFY (starts_with (utf8_r, as_string (today.year ())));
  QVERIFY (ends_with (utf8_r, "日"));
  QVERIFY (contains (utf8_r, "年"));
  QVERIFY (contains (utf8_r, "月"));
}

void
TestTmLocale::test_get_date_default_korean () {
  string r      = get_date ("korean", "");
  string utf8_r = herk_to_utf8 (r);
  QDate  today  = QDate::currentDate ();
  QVERIFY (starts_with (utf8_r, as_string (today.year ())));
  QVERIFY (ends_with (utf8_r, "일"));
  QVERIFY (contains (utf8_r, "년 "));
  QVERIFY (contains (utf8_r, "월 "));
}

void
TestTmLocale::test_get_date_default_taiwanese () {
  string r      = get_date ("taiwanese", "");
  string utf8_r = herk_to_utf8 (r);
  QDate  today  = QDate::currentDate ();
  QVERIFY (starts_with (utf8_r, as_string (today.year ())));
  QVERIFY (ends_with (utf8_r, "日"));
  QVERIFY (contains (utf8_r, "年"));
  QVERIFY (contains (utf8_r, "月"));
}

void
TestTmLocale::test_get_date_default_unknown_language () {
  string r= get_date ("unknown_language", "");
  QVERIFY (N(r) > 0);
  QDate today= QDate::currentDate ();
  QVERIFY (contains (r, as_string (today.year ())));
}
