
/******************************************************************************
 * MODULE     : qt_chat_model.cpp
 * DESCRIPTION: LLM 聊天的模型清单数据类型与加载器
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_chat_model.hpp"

#include "file.hpp"
#include "qt_utilities.hpp"
#include "sys_utils.hpp"
#include "tm_url.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace moebius;

namespace {

/**
 * @brief 读取 JSON 对象的字符串字段。
 * @return 字段值；缺失或非字符串时返回缺省值 fallback
 */
string
json_string_field (const QJsonObject& obj, const char* key,
                   const string& fallback) {
  QJsonValue v= obj.value (QLatin1String (key));
  if (!v.isString ()) return fallback;
  return from_qstring_utf8 (v.toString ());
}

/**
 * @brief 读取 JSON 对象的布尔字段（接受 bool 或 0/1 数字）。
 * @return 字段值；缺失或类型不符时返回缺省值 fallback
 */
bool
json_bool_field (const QJsonObject& obj, const char* key, bool fallback) {
  QJsonValue v= obj.value (QLatin1String (key));
  if (v.isBool ()) return v.toBool ();
  if (v.isDouble ()) return v.toDouble () != 0;
  return fallback;
}

/**
 * @brief 从 JSON 条目构造 ChatModelInfo（缺省规则见任务契约）。
 * @param key 条目键（新格式 = model 字段；旧格式 = 顶层键）
 */
ChatModelInfo
info_from_entry (const string& key, const QJsonObject& entry) {
  ChatModelInfo info;
  info.key          = key;
  info.name         = json_string_field (entry, "name", key);
  info.icon         = json_string_field (entry, "icon", "");
  info.description  = json_string_field (entry, "description", "");
  info.dscColor     = json_string_field (entry, "dsc_color", "orange");
  info.allowThinking= json_bool_field (entry, "allow_thinking", true);
  info.allowSearch  = json_bool_field (entry, "allow_search", true);
  info.baseUrl      = json_string_field (entry, "base_url", "");
  info.defaultSystem= json_string_field (entry, "default_system", "");
  return info;
}

/**
 * @brief 解析默认 key：显式 default 在清单内用之，否则取第一个条目。
 */
string
resolve_default_key (const QList<ChatModelInfo>& models,
                     const string&               explicitDefault) {
  // string::operator== 非 const，按值迭代才能比较（lolly 既有行为）
  for (ChatModelInfo info : models) {
    if (info.key == explicitDefault) return explicitDefault;
  }
  return models.isEmpty () ? string () : models.first ().key;
}

/// 清单文件在安装树中的相对路径（HOME/PATH 根目录之后的部分）
constexpr const char* kMenuRelPath= "plugins/llm/data/liii_llm_menu.json";

} // namespace

bool
chat_model_parse_list (const string& jsonText, QList<ChatModelInfo>& outModels,
                       string& outDefaultKey) {
  string text=
      jsonText; // string::begin() 非 const，拷贝后使用（lolly 既有行为）
  QJsonParseError parseError;
  QJsonDocument   doc= QJsonDocument::fromJson (
      QByteArray (text.begin (), N (text)), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject ())
    return false;

  QJsonObject          root= doc.object ();
  QList<ChatModelInfo> models;
  string               explicitDefault;

  if (root.contains (QLatin1String ("models"))) {
    // 新格式：models 必须是数组（数组保序，即菜单展示顺序）
    QJsonValue modelsValue= root.value (QLatin1String ("models"));
    if (!modelsValue.isArray ()) return false;
    for (const QJsonValue& v : modelsValue.toArray ()) {
      if (!v.isObject ()) continue;
      QJsonObject entry= v.toObject ();
      string      key  = json_string_field (entry, "model", "");
      if (is_empty (key)) continue; // 无 model 字段的条目无效，跳过
      if (!json_bool_field (entry, "enable", true)) continue;
      models.append (info_from_entry (key, entry));
    }
    explicitDefault= json_string_field (root, "default", "");
  }
  else {
    // 旧格式：顶层键即条目 key；名为 default 的条目指定默认 key
    for (const QString& k : root.keys ()) {
      QJsonValue v= root.value (k);
      if (!v.isObject ()) continue;
      QJsonObject entry= v.toObject ();
      if (k == QLatin1String ("default")) {
        explicitDefault= json_string_field (entry, "model", "");
        continue;
      }
      if (!json_bool_field (entry, "enable", true)) continue;
      models.append (info_from_entry (from_qstring_utf8 (k), entry));
    }
  }

  if (models.isEmpty ()) return false;
  outModels    = models;
  outDefaultKey= resolve_default_key (models, explicitDefault);
  return true;
}

ChatModelStore::ChatModelStore () {
  const char* roots[]= {"TEXMACS_HOME_PATH", "TEXMACS_PATH"};
  for (const char* envVar : roots) {
    string root= get_env (envVar);
    if (is_empty (root)) continue;
    url    u= url_system (root, kMenuRelPath);
    string content;
    // load_string 对缺失文件也会打日志，先挡一层
    if (!exists (u) || load_string (u, content, false)) continue;
    if (chat_model_parse_list (content, models_, defaultKey_)) return;
  }

  // 兜底清单：与 main 既有行为一致（模型名 Kimi-VLM），无清单文件时
  // 菜单仅此一项且选中，发送/持久化/恢复不受影响
  ChatModelInfo fallback;
  fallback.key = "Kimi-VLM";
  fallback.name= "K3";
  fallback.icon= "kimi";
  models_.append (fallback);
  defaultKey_= fallback.key;
}

bool
ChatModelStore::contains (const string& key) const {
  for (ChatModelInfo info : models_) {
    if (info.key == key) return true;
  }
  return false;
}

ChatModelInfo
ChatModelStore::find (const string& key) const {
  for (ChatModelInfo info : models_) {
    if (info.key == key) return info;
  }
  ChatModelInfo fallback; // 找不到：以 key 兜底构造的 Info
  fallback.key = key;
  fallback.name= key;
  return fallback;
}
