
/******************************************************************************
 * MODULE     : should_update_menu_test.cpp
 * DESCRIPTION: 钉死 update_menus 按 buffer 类别重建各段的规则
 * COPYRIGHT  : (C) 2026
 ******************************************************************************/

#include "base.hpp"
#include "editor.hpp"
#include "url.hpp"
#include <QtTest/QtTest>

// 被测函数定义于 edit_interface.cpp
extern bool should_update_menu (int mask, url name);

static const int ALL_BITS[]= {MENU_MAIN,    ICONS_MAIN,  ICONS_MODE,
                              ICONS_FOCUS,  ICONS_EXTRA, TAB_PAGES,
                              NOTIFICATION, SIDE_TOOLS};
static const int N_BITS    = sizeof (ALL_BITS) / sizeof (ALL_BITS[0]);

class TestShouldUpdateMenu : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void test_normal_buffer ();
  void test_startup_tab ();
  void test_chat_tab ();
  void test_chat_message ();
  void test_chat_input ();
  void test_unknown_chat_sub_buffer ();
  void test_aux_search ();
  void test_aux_replace ();
};

// 普通 buffer：所有段都重建
void
TestShouldUpdateMenu::test_normal_buffer () {
  url u= url ("/tmp/should_update_menu_test.tm");
  for (int i= 0; i < N_BITS; i++)
    QVERIFY (should_update_menu (ALL_BITS[i], u));
  QVERIFY (should_update_menu (MENU_ALL, u));
  // 空掩码（未请求任何段）一律不重建
  QVERIFY (!should_update_menu (0, u));
}

// startup tab：仅重建 tab 栏
void
TestShouldUpdateMenu::test_startup_tab () {
  url u= url ("tmfs://startup-tab");
  for (int i= 0; i < N_BITS; i++) {
    if (ALL_BITS[i] == TAB_PAGES) QVERIFY (should_update_menu (TAB_PAGES, u));
    else QVERIFY (!should_update_menu (ALL_BITS[i], u));
  }
  // TAB_PAGES 在 MENU_ALL 中，整掩码不全允许时判不重建
  QVERIFY (!should_update_menu (MENU_ALL, u));
}

// chat 标签页：仅重建 tab 栏（关闭当前 tab 跳转到 chat 时，tab 栏须能移除
// 已关 tab；解耦后 tab_pages() 签名判等，不变时仅一次签名比较）
void
TestShouldUpdateMenu::test_chat_tab () {
  url u= url ("tmfs://chat-tab/B86AD167-589F-4820-88A0-388A12AAAF30");
  for (int i= 0; i < N_BITS; i++) {
    if (ALL_BITS[i] == TAB_PAGES) QVERIFY (should_update_menu (TAB_PAGES, u));
    else QVERIFY (!should_update_menu (ALL_BITS[i], u));
  }
  QVERIFY (!should_update_menu (MENU_ALL, u));
}

// chat 只读消息区：全部不重建
void
TestShouldUpdateMenu::test_chat_message () {
  url u= url ("tmfs://chat/B86AD167-589F-4820-88A0-388A12AAAF30/message");
  for (int i= 0; i < N_BITS; i++)
    QVERIFY (!should_update_menu (ALL_BITS[i], u));
  QVERIFY (!should_update_menu (MENU_ALL, u));
}

// chat 输入框：仅重建模式工具栏
void
TestShouldUpdateMenu::test_chat_input () {
  url u= url ("tmfs://chat/B86AD167-589F-4820-88A0-388A12AAAF30/input");
  for (int i= 0; i < N_BITS; i++) {
    if (ALL_BITS[i] == ICONS_MODE) QVERIFY (should_update_menu (ICONS_MODE, u));
    else QVERIFY (!should_update_menu (ALL_BITS[i], u));
  }
}

// 未识别的 chat 子 buffer：按普通 buffer 处理
void
TestShouldUpdateMenu::test_unknown_chat_sub_buffer () {
  url u= url ("tmfs://chat/B86AD167-589F-4820-88A0-388A12AAAF30/draft");
  for (int i= 0; i < N_BITS; i++)
    QVERIFY (should_update_menu (ALL_BITS[i], u));
}

// 搜索辅助缓冲区：全部不重建（嵌在搜索面板 widget 里，各菜单段与它无关）
void
TestShouldUpdateMenu::test_aux_search () {
  url u= url ("tmfs://aux/search/0123456789abcdef0123456789abcdef/1");
  for (int i= 0; i < N_BITS; i++)
    QVERIFY (!should_update_menu (ALL_BITS[i], u));
  QVERIFY (!should_update_menu (MENU_ALL, u));
}

// 替换辅助缓冲区：全部不重建（与 search 同属搜索面板 widget）
void
TestShouldUpdateMenu::test_aux_replace () {
  url u= url ("tmfs://aux/replace/0123456789abcdef0123456789abcdef/1");
  for (int i= 0; i < N_BITS; i++)
    QVERIFY (!should_update_menu (ALL_BITS[i], u));
  QVERIFY (!should_update_menu (MENU_ALL, u));
}

QTEST_MAIN (TestShouldUpdateMenu)
#include "should_update_menu_test.moc"
