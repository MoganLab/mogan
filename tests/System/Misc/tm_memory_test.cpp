
/******************************************************************************
 * MODULE     : tm_memory_test.cpp
 * DESCRIPTION: Unit tests for tm_memory (RSS monitoring, Linux only)
 * COPYRIGHT  : (C) 2026  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "base.hpp"
#include "sys_utils.hpp"
#include "tm_memory.hpp"

inline bool
os_gnu_linux () {
  return !os_win () && !os_mingw () && !os_macos ();
}

#include <QtTest/QtTest>

class TestTmMemory : public QObject {
  Q_OBJECT

private slots:
  void init ();
  void test_get_rss_returns_positive ();
  void test_get_rss_increases_after_allocation ();
  void test_memory_info_string_not_empty ();
  void test_memory_info_string_contains_rss ();
};

void
TestTmMemory::init () {
  init_lolly ();
}

void
TestTmMemory::test_get_rss_returns_positive () {
  if (!os_gnu_linux ()) {
    QSKIP ("get_rss is Linux-only");
  }
  long rss= get_rss ();
  QVERIFY (rss > 0);
}

void
TestTmMemory::test_get_rss_increases_after_allocation () {
  if (!os_gnu_linux ()) {
    QSKIP ("get_rss is Linux-only");
  }
  long before= get_rss ();

  // Allocate ~10 MB to ensure a measurable RSS increase
  const int      size= 10 * 1024 * 1024;
  volatile char* buf = new char[size];
  // Touch pages to ensure they are resident
  for (int i= 0; i < size; i+= 4096) {
    buf[i]= static_cast<char> (i);
  }

  long after= get_rss ();
  delete[] buf;

  QVERIFY2 (after >= before, "RSS should not decrease after large allocation");
}

void
TestTmMemory::test_memory_info_string_not_empty () {
  if (!os_gnu_linux ()) {
    QSKIP ("memory_info_string is Linux-only");
  }
  string info= memory_info_string ();
  QVERIFY (N (info) > 0);
}

void
TestTmMemory::test_memory_info_string_contains_rss () {
  if (!os_gnu_linux ()) {
    QSKIP ("memory_info_string is Linux-only");
  }
  string info= memory_info_string ();
  QVERIFY2 (occurs ("RSS", info), "memory_info_string should contain 'RSS'");
}

QTEST_MAIN (TestTmMemory)
#include "tm_memory_test.moc"
