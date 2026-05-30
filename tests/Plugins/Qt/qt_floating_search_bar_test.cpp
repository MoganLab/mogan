
/******************************************************************************
 * MODULE     : qt_floating_search_bar_test.cpp
 * DESCRIPTION: Tests for QTMFloatingSearchBar widget
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_floating_search_bar.hpp"
#include "base.hpp"
#include <QLabel>
#include <QToolButton>
#include <QtTest/QtTest>

class TestFloatingSearchBar : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // === 构造 ===
  void test_constructor ();

  // === setMatchInfo ===
  void test_matchInfo_default_no_matches ();
  void test_matchInfo_with_matches ();
  void test_matchInfo_zero_matches ();

  // === setModeIcon ===
  void test_modeIcon_default_text_mode ();
  void test_modeIcon_switch_to_math ();
  void test_modeIcon_switch_back_to_text ();
};

/******************************************************************************
 * 构造
 ******************************************************************************/

void
TestFloatingSearchBar::test_constructor () {
  QTMFloatingSearchBar bar;
  QCOMPARE (bar.objectName (), QString ("floating_search_bar"));
  QVERIFY (bar.findChild<QLabel*> ("floating-search-info") != nullptr);
  QVERIFY (bar.findChild<QToolButton*> ("floating-search-mode-text") !=
           nullptr);
  QVERIFY (bar.findChild<QToolButton*> ("floating-search-prev") != nullptr);
  QVERIFY (bar.findChild<QToolButton*> ("floating-search-next") != nullptr);
  QVERIFY (bar.findChild<QToolButton*> ("floating-search-close") != nullptr);
}

/******************************************************************************
 * setMatchInfo
 ******************************************************************************/

void
TestFloatingSearchBar::test_matchInfo_default_no_matches () {
  QTMFloatingSearchBar bar;
  auto*                info= bar.findChild<QLabel*> ("floating-search-info");
  QVERIFY (info != nullptr);
  // 构造后默认应显示 "No matches"（英文环境下）
  QVERIFY (info->text ().contains ("No matches"));
}

void
TestFloatingSearchBar::test_matchInfo_with_matches () {
  QTMFloatingSearchBar bar;
  bar.setMatchInfo (3, 10);
  auto* info= bar.findChild<QLabel*> ("floating-search-info");
  QVERIFY (info != nullptr);
  QVERIFY (info->text ().contains ("3"));
  QVERIFY (info->text ().contains ("10"));
}

void
TestFloatingSearchBar::test_matchInfo_zero_matches () {
  QTMFloatingSearchBar bar;
  // 先设置为有匹配
  bar.setMatchInfo (1, 5);
  // 再清零
  bar.setMatchInfo (0, 0);
  auto* info= bar.findChild<QLabel*> ("floating-search-info");
  QVERIFY (info != nullptr);
  QVERIFY (info->text ().contains ("No matches"));
}

/******************************************************************************
 * setModeIcon
 ******************************************************************************/

void
TestFloatingSearchBar::test_modeIcon_default_text_mode () {
  QTMFloatingSearchBar bar;
  auto*                modeBtn= bar.findChild<QToolButton*> ();
  QVERIFY (modeBtn != nullptr);
  // 默认是 text mode，objectName 应为 floating-search-mode-text
  QCOMPARE (modeBtn->objectName (), QString ("floating-search-mode-text"));
}

void
TestFloatingSearchBar::test_modeIcon_switch_to_math () {
  QTMFloatingSearchBar bar;
  bar.setModeIcon (true); // math mode
  auto* modeBtn= bar.findChild<QToolButton*> ();
  QVERIFY (modeBtn != nullptr);
  QCOMPARE (modeBtn->objectName (), QString ("floating-search-mode-math"));
}

void
TestFloatingSearchBar::test_modeIcon_switch_back_to_text () {
  QTMFloatingSearchBar bar;
  bar.setModeIcon (true);  // math
  bar.setModeIcon (false); // back to text
  auto* modeBtn= bar.findChild<QToolButton*> ();
  QVERIFY (modeBtn != nullptr);
  QCOMPARE (modeBtn->objectName (), QString ("floating-search-mode-text"));
}

QTEST_MAIN (TestFloatingSearchBar)
#include "qt_floating_search_bar_test.moc"
