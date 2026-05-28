
/******************************************************************************
 * MODULE     : update_menus_skip_test.cpp
 * DESCRIPTION: should_skip_menu_update 的单元测试
 * COPYRIGHT  : (C) 2026 Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "edit_interface.hpp"

class TestUpdateMenusSkip : public QObject {
  Q_OBJECT

private slots:
  void test_startup_tab_skipped ();
  void test_chat_tab_skipped ();
  void test_chat_tab_variants_skipped ();
  void test_normal_buffer_not_skipped ();
  void test_empty_buffer_not_skipped ();
};

void
TestUpdateMenusSkip::test_startup_tab_skipped () {
  QVERIFY (should_skip_menu_update (url ("tmfs://startup-tab")));
}

void
TestUpdateMenusSkip::test_chat_tab_skipped () {
  QVERIFY (should_skip_menu_update (url ("tmfs://chat-tab")));
}

void
TestUpdateMenusSkip::test_chat_tab_variants_skipped () {
  QVERIFY (should_skip_menu_update (url ("tmfs://chat-tab/1")));
  QVERIFY (should_skip_menu_update (url ("tmfs://chat-tab/session-abc")));
}

void
TestUpdateMenusSkip::test_normal_buffer_not_skipped () {
  QVERIFY (!should_skip_menu_update (url ("file:///home/user/test.tm")));
  QVERIFY (!should_skip_menu_update (url ("tmfs://view/1/default/test.tm")));
  QVERIFY (!should_skip_menu_update (url ("tmfs://startup-tab/1")));
  QVERIFY (!should_skip_menu_update (url ("tmfs://chat")));
}

void
TestUpdateMenusSkip::test_empty_buffer_not_skipped () {
  QVERIFY (!should_skip_menu_update (url ("")));
}

QTEST_MAIN (TestUpdateMenusSkip)
#include "update_menus_skip_test.moc"
