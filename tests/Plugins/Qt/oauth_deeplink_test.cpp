
/******************************************************************************
 * MODULE     : oauth_deeplink_test.cpp
 * DESCRIPTION: Tests for liiistem:// deep link parsing
 * COPYRIGHT  : (C) 2026  MoonL79
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/oauth_deeplink.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

// 只覆盖与本平台无关的解析部分。注册表补写与实例转发都要碰真实系统状态
// （HKCU、本地 socket），不适合放进单元测试
class TestOAuthDeeplink : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_is_wake () {
    QVERIFY (oauth_deeplink::is_wake ("liiistem://wake?instance=inst7"));
    // Windows 会规范化 custom scheme URL：query 前补 `/`，scheme 与 host 转小写
    QVERIFY (oauth_deeplink::is_wake ("liiistem://wake/?instance=inst7"));
    QVERIFY (oauth_deeplink::is_wake ("liiistem://WAKE?instance=inst7"));
    // 传给进程的形式由浏览器决定，scheme 大小写不敏感
    QVERIFY (oauth_deeplink::is_wake ("LiiiSTEM://wake?instance=inst7"));
    // 参数顺序无关：页面可能自行拼上别的参数
    QVERIFY (
        oauth_deeplink::is_wake ("liiistem://wake?from=growth&instance=inst7"));

    // 实例标识用于路由，缺了就没法把窗口交回发起登录的实例，不算一次唤醒
    QVERIFY (!oauth_deeplink::is_wake ("liiistem://wake?instance="));
    QVERIFY (!oauth_deeplink::is_wake ("liiistem://wake"));
    QVERIFY (!oauth_deeplink::is_wake ("https://wake?instance=inst7"));
    QVERIFY (!oauth_deeplink::is_wake (""));
  }

  void test_instance_id_from_url () {
    QCOMPARE (
        oauth_deeplink::instance_id_from_url ("liiistem://wake?instance=inst7"),
        QString ("inst7"));
    // 规范化后的形式结果一致
    QCOMPARE (oauth_deeplink::instance_id_from_url (
                  "liiistem://wake/?instance=inst7"),
              QString ("inst7"));
    // 参数顺序无关
    QCOMPARE (oauth_deeplink::instance_id_from_url (
                  "liiistem://wake?from=growth&instance=inst7"),
              QString ("inst7"));
    // 取不出实例标识时宁可转不出去，也不要猜错实例
    QCOMPARE (oauth_deeplink::instance_id_from_url ("liiistem://wake"),
              QString ());
    QCOMPARE (oauth_deeplink::instance_id_from_url (""), QString ());
  }

  // macOS 上这条判定决定「置前」还是「退出」，两个方向都要锁住
  void test_is_wake_for () {
    QVERIFY (oauth_deeplink::is_wake_for ("liiistem://wake?instance=me", "me"));
    // 规范化形态与大小写不影响判定（与 is_wake 同源）
    QVERIFY (
        oauth_deeplink::is_wake_for ("LiiiSTEM://wake/?instance=me", "me"));
    QVERIFY (oauth_deeplink::is_wake_for (
        "liiistem://wake?from=growth&instance=me", "me"));

    // 指向别的实例：不能当成给本进程的，否则会把别人的唤醒当自己的用
    QVERIFY (
        !oauth_deeplink::is_wake_for ("liiistem://wake?instance=other", "me"));
    // 还没登记自己的标识时不能猜：不知道自己是哪个实例 ≠ 就是给本进程的
    QVERIFY (!oauth_deeplink::is_wake_for ("liiistem://wake?instance=me", ""));
    QVERIFY (!oauth_deeplink::is_wake_for ("", "me"));
    QVERIFY (!oauth_deeplink::is_wake_for ("https://wake?instance=me", "me"));
    QVERIFY (!oauth_deeplink::is_wake_for ("liiistem://wake", "me"));
  }

  void test_find_url () {
    QStringList args;
    args << "LiiiSTEM.exe" << "liiistem://wake?instance=inst7";
    QCOMPARE (oauth_deeplink::find_url (args),
              QString ("liiistem://wake?instance=inst7"));

    // argv[0] 是程序自身路径，不能被当成本次深链
    QStringList self;
    self << "liiistem://wake?instance=inst7";
    QCOMPARE (oauth_deeplink::find_url (self), QString ());

    QStringList upper;
    upper << "LiiiSTEM.exe" << "LiiiSTEM://wake?instance=inst7";
    QCOMPARE (oauth_deeplink::find_url (upper),
              QString ("LiiiSTEM://wake?instance=inst7"));

    // Qt 自己的参数可能排在前面；取第一个命中的即可
    QStringList mixed;
    mixed << "LiiiSTEM.exe" << "-style" << "fusion"
          << "liiistem://wake?instance=inst7";
    QCOMPARE (oauth_deeplink::find_url (mixed),
              QString ("liiistem://wake?instance=inst7"));

    QStringList plain;
    plain << "LiiiSTEM.exe" << "doc.tm";
    QCOMPARE (oauth_deeplink::find_url (plain), QString ());
    QCOMPARE (oauth_deeplink::find_url (QStringList ()), QString ());
  }
};

QTEST_MAIN (TestOAuthDeeplink)
#include "oauth_deeplink_test.moc"
