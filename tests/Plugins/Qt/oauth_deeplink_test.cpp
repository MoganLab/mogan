
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

  void test_is_oauth_callback () {
    QVERIFY (oauth_deeplink::is_oauth_callback (
        "liiistem://callback?code=abc&state=inst.123"));
    // Windows 会规范化 custom scheme URL：query 前补 `/`，scheme 与 host 转小写
    QVERIFY (oauth_deeplink::is_oauth_callback (
        "liiistem://callback/?code=abc&state=inst.123"));
    QVERIFY (oauth_deeplink::is_oauth_callback (
        "liiistem://CALLBACK?code=abc&state=inst.123"));
    // 传给进程的形式由浏览器决定，scheme 大小写不敏感
    QVERIFY (oauth_deeplink::is_oauth_callback (
        "LiiiSTEM://callback?code=abc&state=inst.123"));
    // 百分号编码的 code
    QVERIFY (oauth_deeplink::is_oauth_callback (
        "liiistem://callback?code=a%2Fb%2Bc&state=inst.123"));
    // 参数顺序无关
    QVERIFY (oauth_deeplink::is_oauth_callback (
        "liiistem://callback?state=inst.123&code=abc"));

    QVERIFY (!oauth_deeplink::is_oauth_callback (
        "liiistem://callback?state=inst.123")); // 用户拒绝授权
    QVERIFY (!oauth_deeplink::is_oauth_callback (
        "liiistem://callback?code=abc")); // 缺 state，无法路由也无法防 CSRF
    QVERIFY (!oauth_deeplink::is_oauth_callback (
        "liiistem://callback?code=&state=inst.123")); // 空 code
    QVERIFY (!oauth_deeplink::is_oauth_callback (
        "liiistem://callback?error=access_denied&state=inst.123"));
    QVERIFY (!oauth_deeplink::is_oauth_callback (
        "https://callback?code=abc&state=inst.123")); // 不是自定义协议
    QVERIFY (!oauth_deeplink::is_oauth_callback ("liiistem://callback"));
    QVERIFY (!oauth_deeplink::is_oauth_callback (""));
  }

  void test_instance_id_from_url () {
    QCOMPARE (oauth_deeplink::instance_id_from_url (
                  "liiistem://callback?code=abc&state=inst7.9f3a"),
              QString ("inst7"));
    // 规范化后的形式结果一致
    QCOMPARE (oauth_deeplink::instance_id_from_url (
                  "liiistem://callback/?code=abc&state=inst7.9f3a"),
              QString ("inst7"));
    // 实例标识在第一个 `.` 之前
    QCOMPARE (oauth_deeplink::instance_id_from_url (
                  "liiistem://callback?code=abc&state=inst7.9f.3a"),
              QString ("inst7"));
    // 没有分隔符时取不出实例标识：宁可转不出去，也不要猜错实例
    QCOMPARE (oauth_deeplink::instance_id_from_url (
                  "liiistem://callback?code=abc&state=inst7"),
              QString ());
    QCOMPARE (oauth_deeplink::instance_id_from_url (
                  "liiistem://callback?code=abc&state=.9f3a"),
              QString ());
    QCOMPARE (
        oauth_deeplink::instance_id_from_url ("liiistem://callback?code=abc"),
        QString ());
    QCOMPARE (oauth_deeplink::instance_id_from_url (""), QString ());
  }

  void test_find_url () {
    QStringList args;
    args << "LiiiSTEM.exe"
         << "liiistem://callback?code=abc&state=inst.123";
    QCOMPARE (oauth_deeplink::find_url (args),
              QString ("liiistem://callback?code=abc&state=inst.123"));

    // argv[0] 是程序自身路径，不能被当成本次深链
    QStringList self;
    self << "liiistem://callback?code=abc&state=inst.123";
    QCOMPARE (oauth_deeplink::find_url (self), QString ());

    QStringList upper;
    upper << "LiiiSTEM.exe" << "LiiiSTEM://callback?code=abc&state=inst.123";
    QCOMPARE (oauth_deeplink::find_url (upper),
              QString ("LiiiSTEM://callback?code=abc&state=inst.123"));

    // Qt 自己的参数可能排在前面；取第一个命中的即可
    QStringList mixed;
    mixed << "LiiiSTEM.exe" << "-style" << "fusion"
          << "liiistem://callback?code=abc&state=inst.123";
    QCOMPARE (oauth_deeplink::find_url (mixed),
              QString ("liiistem://callback?code=abc&state=inst.123"));

    QStringList plain;
    plain << "LiiiSTEM.exe" << "doc.tm";
    QCOMPARE (oauth_deeplink::find_url (plain), QString ());
    QCOMPARE (oauth_deeplink::find_url (QStringList ()), QString ());
  }

  void test_redirect_uri () {
    QCOMPARE (oauth_deeplink::redirect_uri (), QString ("liiistem://callback"));
    // 授权请求与令牌交换两处都读它，必须稳定
    QCOMPARE (oauth_deeplink::redirect_uri (), oauth_deeplink::redirect_uri ());
  }
};

QTEST_MAIN (TestOAuthDeeplink)
#include "oauth_deeplink_test.moc"
