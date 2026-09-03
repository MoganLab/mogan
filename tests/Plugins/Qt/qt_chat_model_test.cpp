
/******************************************************************************
 * MODULE     : qt_chat_model_test.cpp
 * DESCRIPTION: Tests for ChatModelProvider / chat_model_parse_list
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_chat_model.hpp"
#include "base.hpp"
#include <QTemporaryFile>
#include <QtTest/QtTest>

class TestChatModel : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // === chat_model_parse_list ===
  void test_parse_full_fields ();
  void test_parse_keeps_order ();
  void test_parse_default_key ();
  void test_parse_missing_fields_use_defaults ();
  void test_parse_default_missing_falls_back ();
  void test_parse_default_not_in_list_uses_first ();
  void test_parse_unknown_dsc_color_becomes_orange ();
  void test_parse_invalid_json_returns_false ();
  void test_parse_empty_models_returns_false ();
  void test_parse_missing_key_returns_false ();

  // === ChatModelProvider::contains / find ===
  void test_contains ();
  void test_find_fallback ();

  // === BuiltinModelProvider ===
  void test_builtin_without_env ();
  void test_builtin_with_env_file ();
};

void
TestChatModel::test_parse_full_fields () {
  string               json= string (R"({
    "default": "DeepSeek",
    "models": [
      { "key": "Kimi-VLM", "name": "K3", "icon": "kimi",
        "description": "订阅优先", "dsc_color": "orange",
        "allow_thinking": true, "allow_search": true },
      { "key": "DeepSeek", "name": "R1", "icon": "deepseek",
        "description": "推理", "dsc_color": "red",
        "allow_thinking": false, "allow_search": false }
    ]
  })");
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (chat_model_parse_list (json, models, def));
  QCOMPARE (models.size (), 2);
  qcompare (models[0].key, "Kimi-VLM");
  qcompare (models[0].name, "K3");
  qcompare (models[0].icon, "kimi");
  qcompare (models[0].description, "订阅优先");
  qcompare (models[0].dscColor, "orange");
  QVERIFY (models[0].allowThinking);
  QVERIFY (models[0].allowSearch);
  qcompare (models[1].key, "DeepSeek");
  qcompare (models[1].dscColor, "red");
  QVERIFY (!models[1].allowThinking);
  QVERIFY (!models[1].allowSearch);
  qcompare (def, "DeepSeek");
}

void
TestChatModel::test_parse_keeps_order () {
  string               json= string (R"({
    "models": [
      { "key": "A" }, { "key": "B" }, { "key": "C" }
    ]
  })");
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (chat_model_parse_list (json, models, def));
  QCOMPARE (models.size (), 3);
  qcompare (models[0].key, "A");
  qcompare (models[1].key, "B");
  qcompare (models[2].key, "C");
}

void
TestChatModel::test_parse_default_key () {
  string               json= string (R"({
    "default": "B",
    "models": [ { "key": "A" }, { "key": "B" } ]
  })");
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (chat_model_parse_list (json, models, def));
  qcompare (def, "B");
}

void
TestChatModel::test_parse_missing_fields_use_defaults () {
  string               json= string (R"({
    "models": [ { "key": "Kimi-VLM" } ]
  })");
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (chat_model_parse_list (json, models, def));
  QCOMPARE (models.size (), 1);
  qcompare (models[0].name, "Kimi-VLM"); // name 缺省回落为 key
  qcompare (models[0].icon, "");
  qcompare (models[0].description, "");
  qcompare (models[0].dscColor, "orange");
  QVERIFY (models[0].allowThinking);
  QVERIFY (models[0].allowSearch);
}

void
TestChatModel::test_parse_default_missing_falls_back () {
  // 顶层 default 缺失且清单无 Kimi-VLM 时，回落为第一个模型
  string               json= string (R"({
    "models": [ { "key": "A" }, { "key": "B" } ]
  })");
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (chat_model_parse_list (json, models, def));
  qcompare (def, "A");
}

void
TestChatModel::test_parse_default_not_in_list_uses_first () {
  string               json= string (R"({
    "default": "NOT-EXIST",
    "models": [ { "key": "A" }, { "key": "B" } ]
  })");
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (chat_model_parse_list (json, models, def));
  qcompare (def, "A");
}

void
TestChatModel::test_parse_unknown_dsc_color_becomes_orange () {
  string               json= string (R"({
    "models": [ { "key": "A", "dsc_color": "blue" } ]
  })");
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (chat_model_parse_list (json, models, def));
  qcompare (models[0].dscColor, "orange");
}

void
TestChatModel::test_parse_invalid_json_returns_false () {
  QList<ChatModelInfo> models;
  ChatModelInfo        sentinel;
  sentinel.key= "sentinel";
  models.append (sentinel);
  string def= "sentinel-default";
  QVERIFY (!chat_model_parse_list (string ("not a json"), models, def));
  // 解析失败时 out 参数不变
  QCOMPARE (models.size (), 1);
  qcompare (models[0].key, "sentinel");
  qcompare (def, "sentinel-default");
}

void
TestChatModel::test_parse_empty_models_returns_false () {
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (
      !chat_model_parse_list (string (R"({ "models": [] })"), models, def));
  QVERIFY (!chat_model_parse_list (string (R"({})"), models, def));
}

void
TestChatModel::test_parse_missing_key_returns_false () {
  QList<ChatModelInfo> models;
  string               def;
  QVERIFY (!chat_model_parse_list (
      string (R"({ "models": [ { "name": "K3" } ] })"), models, def));
}

void
TestChatModel::test_contains () {
  BuiltinModelProvider provider;
  QVERIFY (provider.contains (string ("Kimi-VLM")));
  QVERIFY (!provider.contains (string ("DeepSeek")));
  QVERIFY (!provider.contains (string ("")));
}

void
TestChatModel::test_find_fallback () {
  BuiltinModelProvider provider;
  ChatModelInfo        found= provider.find (string ("Kimi-VLM"));
  qcompare (found.key, "Kimi-VLM");
  qcompare (found.name, "K3");

  // 清单外键名：兜底构造，name 回落为 key
  ChatModelInfo fallback= provider.find (string ("Ghost"));
  qcompare (fallback.key, "Ghost");
  qcompare (fallback.name, "Ghost");
  qcompare (fallback.icon, "");
  qcompare (fallback.description, "");
  qcompare (fallback.dscColor, "orange");
  QVERIFY (fallback.allowThinking);
  QVERIFY (fallback.allowSearch);
}

void
TestChatModel::test_builtin_without_env () {
  qunsetenv ("MOGAN_LLM_MODELS_FILE");
  BuiltinModelProvider provider;
  QList<ChatModelInfo> models= provider.models ();
  QCOMPARE (models.size (), 1);
  qcompare (models[0].key, "Kimi-VLM");
  qcompare (models[0].name, "K3");
  qcompare (models[0].icon, "kimi");
  qcompare (models[0].description, "");
  qcompare (models[0].dscColor, "orange");
  QVERIFY (models[0].allowThinking);
  QVERIFY (models[0].allowSearch);
  qcompare (provider.defaultModelKey (), "Kimi-VLM");
}

void
TestChatModel::test_builtin_with_env_file () {
  QTemporaryFile file;
  QVERIFY (file.open ());
  file.write (R"({
    "default": "DeepSeek",
    "models": [
      { "key": "Kimi-VLM", "name": "K3", "icon": "kimi" },
      { "key": "DeepSeek", "name": "R1", "icon": "deepseek",
        "description": "推理", "dsc_color": "red" }
    ]
  })");
  file.flush ();
  qputenv ("MOGAN_LLM_MODELS_FILE", file.fileName ().toUtf8 ());

  BuiltinModelProvider provider;
  QList<ChatModelInfo> models= provider.models ();
  QCOMPARE (models.size (), 2);
  qcompare (models[1].key, "DeepSeek");
  qcompare (provider.defaultModelKey (), "DeepSeek");

  // 文件不可读时回落内置清单
  qputenv ("MOGAN_LLM_MODELS_FILE", "/nonexistent/path/models.json");
  BuiltinModelProvider fallback;
  QCOMPARE (fallback.models ().size (), 1);
  qcompare (fallback.defaultModelKey (), "Kimi-VLM");

  qunsetenv ("MOGAN_LLM_MODELS_FILE");
}

#ifdef QTTEXMACS
QTEST_MAIN (TestChatModel)
#else
int
main () {
  return 0;
}
#endif
#include "qt_chat_model_test.moc"
