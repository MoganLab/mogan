
/******************************************************************************
 * MODULE     : qt_chat_model_test.cpp
 * DESCRIPTION: Tests for ChatModelStore / chat_model_parse_list
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_chat_controller.hpp"
#include "Qt/qt_chat_model.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

class TestChatModel : public QObject {
  Q_OBJECT

private slots:
  void init ();
  void cleanup ();

  // === chat_model_parse_list：新格式 ===
  void test_parse_new_format_full_fields ();
  void test_parse_new_format_preserves_order ();
  void test_parse_new_format_defaults ();
  void test_parse_filters_disabled ();
  void test_parse_default_falls_back_to_first ();
  void test_parse_invalid_json ();
  void test_parse_empty_models ();
  void test_parse_models_not_array ();

  // === chat_model_parse_list：旧格式 ===
  void test_parse_old_format ();

  // === ChatModelStore ===
  void test_store_builtin_fallback ();
  void test_store_loads_home_menu ();
  void test_store_home_takes_priority ();
  void test_store_falls_back_to_path_menu ();

  // === contains / find ===
  void test_contains ();
  void test_find ();
  void test_find_missing_returns_key_fallback ();
  void test_find_empty_key ();

  // === ChatController::resolveBaseUrl（协议下发前拼接） ===
  void test_resolve_base_url_absolute ();
  void test_resolve_base_url_empty ();
  void test_resolve_base_url_relative ();
  void test_resolve_base_url_relative_empty_site ();

private:
  /// 在 rootDir 下写一份模型清单 JSON，返回是否成功
  static bool write_menu_file (const QString& rootDir, const char* content);

  bool       homeWasSet_= false; ///< TEXMACS_HOME_PATH 原本是否设置
  bool       pathWasSet_= false; ///< TEXMACS_PATH 原本是否设置
  QByteArray savedHome_;         ///< TEXMACS_HOME_PATH 原值
  QByteArray savedPath_;         ///< TEXMACS_PATH 原值
};

void
TestChatModel::init () {
  init_lolly ();
  homeWasSet_= qEnvironmentVariableIsSet ("TEXMACS_HOME_PATH");
  pathWasSet_= qEnvironmentVariableIsSet ("TEXMACS_PATH");
  savedHome_ = qgetenv ("TEXMACS_HOME_PATH");
  savedPath_ = qgetenv ("TEXMACS_PATH");
  // 每个用例从"无清单"起步：不受真实 TEXMACS_PATH/TEXMACS_HOME_PATH 干扰，
  // 需要清单的用例自行指向临时目录
  qunsetenv ("TEXMACS_HOME_PATH");
  qunsetenv ("TEXMACS_PATH");
}

void
TestChatModel::cleanup () {
  // 恢复环境变量，隔离构造的 ChatModelStore 不影响其他用例
  if (homeWasSet_) qputenv ("TEXMACS_HOME_PATH", savedHome_);
  else qunsetenv ("TEXMACS_HOME_PATH");
  if (pathWasSet_) qputenv ("TEXMACS_PATH", savedPath_);
  else qunsetenv ("TEXMACS_PATH");
}

bool
TestChatModel::write_menu_file (const QString& rootDir, const char* content) {
  QDir dir (rootDir);
  if (!dir.mkpath ("plugins/llm/data")) return false;
  QFile f (dir.filePath ("plugins/llm/data/liii_llm_menu.json"));
  if (!f.open (QIODevice::WriteOnly)) return false;
  f.write (content);
  f.close ();
  return true;
}

// === chat_model_parse_list：新格式 ===

void
TestChatModel::test_parse_new_format_full_fields () {
  const char* json=
      "{ \"default\": \"kimi-k3\","
      " \"models\": ["
      "  { \"model\": \"kimi-k3\", \"name\": \"K3\","
      "    \"base_url\": \"/api/v1/ai/siliconflow/chat\","
      "    \"default_system\": \"You are helpful.\","
      "    \"thinking\": true, \"search\": true, \"enable\": true,"
      "    \"allow_thinking\": false, \"allow_search\": false,"
      "    \"icon\": \"kimi\", \"description\": \"Vision\","
      "    \"dsc_color\": \"red\" } ] }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (chat_model_parse_list (json, models, defaultKey));
  QCOMPARE (models.size (), 1);
  ChatModelInfo m= models.first ();
  QVERIFY (m.key == string ("kimi-k3"));
  QVERIFY (m.name == string ("K3"));
  QVERIFY (m.icon == string ("kimi"));
  QVERIFY (m.description == string ("Vision"));
  QVERIFY (m.dscColor == string ("red"));
  QVERIFY (!m.allowThinking);
  QVERIFY (!m.allowSearch);
  QVERIFY (m.baseUrl == string ("/api/v1/ai/siliconflow/chat"));
  QVERIFY (m.defaultSystem == string ("You are helpful."));
  QVERIFY (defaultKey == string ("kimi-k3"));
}

void
TestChatModel::test_parse_new_format_preserves_order () {
  const char*          json= "{ \"models\": ["
                             "  { \"model\": \"deepseek-v4-pro\" },"
                             "  { \"model\": \"kimi-k3\" },"
                             "  { \"model\": \"deepseek-v4-flash\" } ] }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (chat_model_parse_list (json, models, defaultKey));
  QCOMPARE (models.size (), 3);
  QVERIFY (models[0].key == string ("deepseek-v4-pro"));
  QVERIFY (models[1].key == string ("kimi-k3"));
  QVERIFY (models[2].key == string ("deepseek-v4-flash"));
}

void
TestChatModel::test_parse_new_format_defaults () {
  const char*          json= "{ \"models\": [ { \"model\": \"kimi-k3\" } ] }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (chat_model_parse_list (json, models, defaultKey));
  QCOMPARE (models.size (), 1);
  ChatModelInfo m= models.first ();
  QVERIFY (m.name == string ("kimi-k3")); // name 缺省 = key
  QVERIFY (m.icon == string (""));
  QVERIFY (m.description == string (""));
  QVERIFY (m.dscColor == string ("orange"));
  QVERIFY (m.allowThinking);
  QVERIFY (m.allowSearch);
  QVERIFY (m.baseUrl == string (""));
  QVERIFY (m.defaultSystem == string (""));
}

void
TestChatModel::test_parse_filters_disabled () {
  // enable 为 false 与 0 两种形态均过滤；缺省 enable 视为 true
  const char*          json= "{ \"default\": \"off-bool\","
                             " \"models\": ["
                             "  { \"model\": \"off-bool\", \"enable\": false },"
                             "  { \"model\": \"off-num\", \"enable\": 0 },"
                             "  { \"model\": \"on-num\", \"enable\": 1 },"
                             "  { \"model\": \"on-default\" } ] }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (chat_model_parse_list (json, models, defaultKey));
  QCOMPARE (models.size (), 2);
  QVERIFY (models[0].key == string ("on-num"));
  QVERIFY (models[1].key == string ("on-default"));
  // default 指向被过滤条目时回退第一个条目
  QVERIFY (defaultKey == string ("on-num"));
}

void
TestChatModel::test_parse_default_falls_back_to_first () {
  const char* json=
      "{ \"default\": \"not-in-list\","
      " \"models\": [ { \"model\": \"a\" }, { \"model\": \"b\" } ] }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (chat_model_parse_list (json, models, defaultKey));
  QVERIFY (defaultKey == string ("a"));
}

void
TestChatModel::test_parse_invalid_json () {
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (!chat_model_parse_list ("{ not json", models, defaultKey));
  QVERIFY (!chat_model_parse_list ("[1, 2]", models, defaultKey)); // 顶层非对象
  QVERIFY (!chat_model_parse_list ("", models, defaultKey));
}

void
TestChatModel::test_parse_empty_models () {
  const char*          json= "{ \"default\": \"x\", \"models\": [] }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (!chat_model_parse_list (json, models, defaultKey));
}

void
TestChatModel::test_parse_models_not_array () {
  const char*          json= "{ \"models\": { \"model\": \"x\" } }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (!chat_model_parse_list (json, models, defaultKey));
}

// === chat_model_parse_list：旧格式 ===

void
TestChatModel::test_parse_old_format () {
  // 旧格式：顶层键即条目 key，名为 default 的条目指定默认 key
  const char* json= "{ \"Kimi-VLM\": { \"name\": \"K3\", \"icon\": \"kimi\","
                    "                 \"enable\": 1 },"
                    "  \"deepseek-v4\": { \"name\": \"DeepSeek\" },"
                    "  \"disabled-x\": { \"enable\": false },"
                    "  \"default\": { \"model\": \"deepseek-v4\" } }";
  QList<ChatModelInfo> models;
  string               defaultKey;
  QVERIFY (chat_model_parse_list (json, models, defaultKey));
  QCOMPARE (models.size (), 2);
  QVERIFY (models[0].key == string ("Kimi-VLM"));
  QVERIFY (models[0].name == string ("K3"));
  QVERIFY (models[0].icon == string ("kimi"));
  QVERIFY (models[1].key == string ("deepseek-v4"));
  QVERIFY (models[1].name == string ("DeepSeek"));
  QVERIFY (models[1].dscColor == string ("orange")); // 其余按缺省补齐
  QVERIFY (defaultKey == string ("deepseek-v4"));
}

// === ChatModelStore ===

void
TestChatModel::test_store_builtin_fallback () {
  // 两个路径环境变量均不指向存在文件 → 内置兜底清单
  QTemporaryDir missing;
  QVERIFY (missing.isValid ());
  qputenv ("TEXMACS_HOME_PATH", (missing.path () + "/nope-home").toUtf8 ());
  qputenv ("TEXMACS_PATH", (missing.path () + "/nope-path").toUtf8 ());

  ChatModelStore store;
  QCOMPARE (store.models ().size (), 1);
  ChatModelInfo m= store.models ().first ();
  QVERIFY (m.key == string ("Kimi-VLM"));
  QVERIFY (m.name == string ("K3"));
  QVERIFY (m.icon == string ("kimi"));
  QVERIFY (store.defaultKey () == string ("Kimi-VLM"));
}

void
TestChatModel::test_store_loads_home_menu () {
  QTemporaryDir home;
  QVERIFY (home.isValid ());
  QVERIFY (write_menu_file (home.path (),
                            "{ \"models\": [ { \"model\": \"home-model\","
                            " \"name\": \"Home\" } ] }"));
  qputenv ("TEXMACS_HOME_PATH", home.path ().toUtf8 ());
  qunsetenv ("TEXMACS_PATH");

  ChatModelStore store;
  QCOMPARE (store.models ().size (), 1);
  QVERIFY (store.defaultKey () == string ("home-model"));
}

void
TestChatModel::test_store_home_takes_priority () {
  // HOME 与 PATH 均有清单时取 HOME 的
  QTemporaryDir home, path;
  QVERIFY (home.isValid () && path.isValid ());
  QVERIFY (write_menu_file (
      home.path (), "{ \"models\": [ { \"model\": \"from-home\" } ] }"));
  QVERIFY (write_menu_file (
      path.path (), "{ \"models\": [ { \"model\": \"from-path\" } ] }"));
  qputenv ("TEXMACS_HOME_PATH", home.path ().toUtf8 ());
  qputenv ("TEXMACS_PATH", path.path ().toUtf8 ());

  ChatModelStore store;
  QCOMPARE (store.models ().size (), 1);
  QVERIFY (store.models ().first ().key == string ("from-home"));
}

void
TestChatModel::test_store_falls_back_to_path_menu () {
  // HOME 无清单时取 PATH 的
  QTemporaryDir home, path;
  QVERIFY (home.isValid () && path.isValid ());
  QVERIFY (write_menu_file (
      path.path (), "{ \"models\": [ { \"model\": \"from-path\" } ] }"));
  qputenv ("TEXMACS_HOME_PATH", home.path ().toUtf8 ());
  qputenv ("TEXMACS_PATH", path.path ().toUtf8 ());

  ChatModelStore store;
  QCOMPARE (store.models ().size (), 1);
  QVERIFY (store.models ().first ().key == string ("from-path"));
}

// === contains / find ===

void
TestChatModel::test_contains () {
  ChatModelStore store; // 兜底清单：Kimi-VLM 单条目
  QVERIFY (store.contains ("Kimi-VLM"));
  QVERIFY (!store.contains ("kimi-k3"));
  QVERIFY (!store.contains ("")); // 空键不在清单内
}

void
TestChatModel::test_find () {
  ChatModelStore store;
  ChatModelInfo  m= store.find ("Kimi-VLM");
  QVERIFY (m.key == string ("Kimi-VLM"));
  QVERIFY (m.name == string ("K3"));
  QVERIFY (m.icon == string ("kimi"));
}

void
TestChatModel::test_find_missing_returns_key_fallback () {
  ChatModelStore store;
  ChatModelInfo  m= store.find ("unknown-model");
  QVERIFY (m.key == string ("unknown-model"));
  QVERIFY (m.name == string ("unknown-model")); // 显示名缺省 = key
}

void
TestChatModel::test_find_empty_key () {
  ChatModelStore store;
  ChatModelInfo  m= store.find ("");
  QVERIFY (m.key == string (""));
  QVERIFY (m.name == string (""));
}

// === ChatController::resolveBaseUrl（协议下发前拼接） ===

void
TestChatModel::test_resolve_base_url_absolute () {
  // 以 http 开头的 base_url 视为绝对 URL，原样下发
  QVERIFY (
      ChatController::resolveBaseUrl ("https://custom.example.com/api/v1/chat",
                                      "https://liiistem.cn") ==
      string ("https://custom.example.com/api/v1/chat"));
  QVERIFY (ChatController::resolveBaseUrl ("http://insecure.example.com/chat",
                                           "https://liiistem.cn") ==
           string ("http://insecure.example.com/chat"));
}

void
TestChatModel::test_resolve_base_url_empty () {
  // 清单未提供 base_url → 空串，由子进程兜底
  QVERIFY (ChatController::resolveBaseUrl ("", "https://liiistem.cn") ==
           string (""));
}

void
TestChatModel::test_resolve_base_url_relative () {
  // 相对路径拼接当前 stem site
  QVERIFY (ChatController::resolveBaseUrl ("/api/v1/ai/siliconflow/chat",
                                           "https://liiistem.cn") ==
           string ("https://liiistem.cn/api/v1/ai/siliconflow/chat"));
}

void
TestChatModel::test_resolve_base_url_relative_empty_site () {
  // site 获取失败（account 模块缺失）时相对路径原样下传，子进程兜底拼 site
  QVERIFY (ChatController::resolveBaseUrl ("/api/v1/ai/siliconflow/chat", "") ==
           string ("/api/v1/ai/siliconflow/chat"));
}

QTEST_MAIN (TestChatModel)
#include "qt_chat_model_test.moc"
