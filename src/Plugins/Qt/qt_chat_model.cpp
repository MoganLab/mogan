
/******************************************************************************
 * MODULE     : qt_chat_model.cpp
 * DESCRIPTION: LLM 聊天模型清单：值类型、来源抽象与内置实现
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_chat_model.hpp"
#include "qt_utilities.hpp"
#include "sys_utils.hpp" // get_env

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

/******************************************************************************
 * ChatModelProvider
 ******************************************************************************/

bool
ChatModelProvider::contains (const string& key) const {
  QList<ChatModelInfo> all= models ();
  for (int i= 0; i < all.size (); ++i) {
    if (all[i].key == key) return true;
  }
  return false;
}

ChatModelInfo
ChatModelProvider::find (const string& key) const {
  QList<ChatModelInfo> all= models ();
  for (int i= 0; i < all.size (); ++i) {
    if (all[i].key == key) return all[i];
  }
  // 清单外键名兜底：保证菜单/会话仍能展示一个可读的条目
  ChatModelInfo fallback;
  fallback.key          = key;
  fallback.name         = key;
  fallback.icon         = "";
  fallback.description  = "";
  fallback.dscColor     = "orange";
  fallback.allowThinking= true;
  fallback.allowSearch  = true;
  return fallback;
}

/******************************************************************************
 * JSON 解析
 ******************************************************************************/

bool
chat_model_parse_list (const string& jsonText, QList<ChatModelInfo>& outModels,
                       string& outDefaultKey) {
  QJsonParseError err;
  QJsonDocument   doc=
      QJsonDocument::fromJson (utf8_to_qstring (jsonText).toUtf8 (), &err);
  if (err.error != QJsonParseError::NoError || !doc.isObject ()) return false;

  QJsonArray arr= doc.object ().value ("models").toArray ();
  if (arr.isEmpty ()) return false;

  QList<ChatModelInfo> models;
  for (const QJsonValue& v : arr) {
    if (!v.isObject ()) return false;
    QJsonObject   o= v.toObject ();
    ChatModelInfo info;
    info.key= from_qstring_utf8 (o.value ("key").toString ());
    // key 是协议与 manifest 的唯一标识，缺失则整条清单不可用
    if (is_empty (info.key)) return false;
    QString name    = o.value ("name").toString ();
    info.name       = name.isEmpty () ? info.key : from_qstring_utf8 (name);
    info.icon       = from_qstring_utf8 (o.value ("icon").toString ());
    info.description= from_qstring_utf8 (o.value ("description").toString ());
    string color    = from_qstring_utf8 (o.value ("dsc_color").toString ());
    info.dscColor   = (color == "red" || color == "orange") ? color : "orange";
    info.allowThinking= o.value ("allow_thinking").toBool (true);
    info.allowSearch  = o.value ("allow_search").toBool (true);
    models.append (info);
  }

  string def= from_qstring_utf8 (doc.object ().value ("default").toString ());
  if (is_empty (def)) def= "Kimi-VLM";
  bool inList= false;
  for (int i= 0; i < models.size (); ++i) {
    if (models[i].key == def) inList= true;
  }
  if (!inList) def= models.first ().key;

  outModels    = models;
  outDefaultKey= def;
  return true;
}

/******************************************************************************
 * BuiltinModelProvider
 ******************************************************************************/

BuiltinModelProvider::BuiltinModelProvider () {
  // 调试通道：环境变量指向 JSON 文件时从文件读清单，便于本地联调多条目 UI
  string path= get_env ("MOGAN_LLM_MODELS_FILE");
  if (!is_empty (path)) {
    QFile file (to_qstring (path));
    if (file.open (QIODevice::ReadOnly)) {
      string content= from_qstring_utf8 (QString::fromUtf8 (file.readAll ()));
      QList<ChatModelInfo> fileModels;
      string               fileDefault;
      if (chat_model_parse_list (content, fileModels, fileDefault)) {
        models_    = fileModels;
        defaultKey_= fileDefault;
        return;
      }
    }
  }

  ChatModelInfo kimi;
  kimi.key          = "Kimi-VLM";
  kimi.name         = "K3";
  kimi.icon         = "kimi";
  kimi.description  = "";
  kimi.dscColor     = "orange";
  kimi.allowThinking= true;
  kimi.allowSearch  = true;
  models_.append (kimi);
  defaultKey_= "Kimi-VLM";
}

QList<ChatModelInfo>
BuiltinModelProvider::models () const {
  return models_;
}

string
BuiltinModelProvider::defaultModelKey () const {
  return defaultKey_;
}
