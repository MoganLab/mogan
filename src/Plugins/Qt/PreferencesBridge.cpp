/******************************************************************************
 * MODULE      : PreferencesBridge.cpp
 * DESCRIPTION : 首选项 QML bridge 实现（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "PreferencesBridge.hpp"

#include "converter.hpp" // cork_to_utf8
#include "qt_utilities.hpp"
#include "s7_tm.hpp" // eval_scheme + tmscm helpers

#include <QVariantList>
#include <QVariantMap>

namespace {
/**
 * @brief scheme 值 → QString，类型安全且编码确定。
 * @details tmscm_to_string 内部调 s7_string，仅对 string 类型合法；对 symbol 会
 * 解引用垃圾指针（SIGBUS）。scheme assoc 的 key 常是 symbol，故按类型分流。
 * mogan string 内部是 Cork 编码，统一 cork_to_utf8 转 UTF-8 再 to QString——不用
 * to_qstring（其 looks_utf8/looks_ascii 启发式对纯 Cork 中文不稳定，见
 * FontSelectorBridge.cpp:42-48）。
 */
QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (v)));
  if (tmscm_is_symbol (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_symbol (v)));
  return QString ();
}

/**
 * @brief scheme list of string → QStringList。非 string 项跳过。
 */
QStringList
tmscm_to_stringlist (tmscm lst) {
  QStringList out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (tmscm_is_string (item))
      out << utf8_to_qstring (cork_to_utf8 (tmscm_to_string (item)));
  }
  return out;
}

/**
 * @brief scheme 值 → bool（toggle 字段的 value 统一为 "on"/"off" 字符串，但若
 * scheme 直接给 #t/#f 也兼容解析）。
 *
 * @note s7_tm.hpp 已有 tmscm_to_bool（重载——故本函数改名 preferences_bool，避免
 * 歧义：其只接受 #t/#f scheme 字面，而我们还要兼容 "on"/"off" 字符串 wire
 * 格式）。
 */
bool
preferences_bool (tmscm v) {
  if (tmscm_is_string (v)) return tmscm_to_qstring (v) == QLatin1String ("on");
  return tmscm_to_bool (v);
}

/**
 * @brief 把 scheme assoc list（((symbol . value) ...)）→ QVariantMap。
 *
 * @details bridge 的通用 assoc-list 遍历（参考 ParagraphFormatBridge::evalMeta
 * 的 解析模式）：遍历每个 (symbol . value) 对，按 symbol 名分流——
 *   options/optionsTr -> QStringList（combo 的选项列表）
 *   editable?/restart? -> bool（布尔 flag）
 *   column            -> int（双栏列号）
 *   其余              -> string（label / value / key / kind / hint / group 等）
 *
 * 字段描述符的 symbol 名取自 scheme facade 的 assoc-list 输出（见
 * preferences-widgets.scm 的
 * preferences-qml-field->descriptor——kind/key/label/value/
 * options/optionsTr/editable/restart?/radioGroup/enabledWhenKey/enabledWhenVal/
 * group/groupSpan/hint/column/layout/buttonLabel/buttonAction）。
 *
 * @note symbol 名在 scheme 侧是 'kind / 'key / 'label 等（Cork 编码的
 * symbol）—— tmscm_to_qstring 已做 cork_to_utf8，这里 QLatin1String 比较用
 * ASCII 即可（symbol 名 均为 ASCII）。
 */
QVariantMap
assoc_to_variantmap (tmscm alist) {
  QVariantMap out;
  for (tmscm cur= alist; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString k= tmscm_to_qstring (tmscm_car (pair));
    tmscm   v= tmscm_cdr (pair);
    if (k == QLatin1String ("options") && tmscm_is_list (v))
      out[k]= QVariant::fromValue (tmscm_to_stringlist (v));
    else if (k == QLatin1String ("optionsTr") && tmscm_is_list (v))
      out[k]= QVariant::fromValue (tmscm_to_stringlist (v));
    else if (k == QLatin1String ("editable")) out[k]= preferences_bool (v);
    else if (k == QLatin1String ("restart?")) out[k]= preferences_bool (v);
    else if (k == QLatin1String ("groupSpan")) out[k]= preferences_bool (v);
    else if (k == QLatin1String ("column")) out[k]= tmscm_to_int (v);
    else out[k]= tmscm_to_qstring (v);
  }
  return out;
}

/**
 * @brief scheme 字段描述符列表（assoc-list of pairs 逐项）→ QVariantList of
 * QVariantMap（每个 field 一个 map）。
 *
 * @details field-descriptor 的格式见 preferences-widgets.scm 的
 * preferences-qml-field->descriptor——每个 field 是 assoc list：
 *   ((kind . "combo") (key . "...") (label . "...") (value . "...")
 *    (options . (...)) (optionsTr . (...)) (editable? . #t) ...)
 * 本函数遍历外层 list、每个 field 走 assoc_to_variantmap 转成 QVariantMap。
 */
QVariantList
parse_field_list (tmscm fields) {
  QVariantList out;
  for (tmscm cur= fields; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm f= tmscm_car (cur);
    if (!tmscm_is_list (f)) continue;
    out << assoc_to_variantmap (f);
  }
  return out;
}

/**
 * @brief 取 tab 节点 `(key label fields ...)` 的前三项填入 out，成功时把
 * fields 之后的剩余 list（可能含 sub-tabs）写到 tail 并返回 true。
 *
 * @details 主 tab 与 sub-tab 同形（都是 key/label/fields 三元组），共用本
 * helper——parse_meta_tree 的外层 tab 与 sub-tabs 各调一次。返回 bool 而非
 * tail 本身：普通 tab 恰好 3 项（tail 为 '()），与「前 3 项不足」必须区分，
 * 否则合法的 3 项 tab 会被误 skip（返回值无法区分两者）。
 */
bool
parse_tab_node (tmscm node, QVariantMap& out, tmscm& tail) {
  if (tmscm_is_null (node)) return false;
  out["key"]= tmscm_to_qstring (tmscm_car (node));
  tmscm rest= tmscm_cdr (node);
  if (tmscm_is_null (rest)) return false;
  out["label"]= tmscm_to_qstring (tmscm_car (rest));
  rest        = tmscm_cdr (rest);
  if (tmscm_is_null (rest)) return false;
  out["fields"]= parse_field_list (tmscm_car (rest));
  tail         = tmscm_cdr (rest);
  return true;
}

/**
 * @brief scheme tab 描述符列表（preferences-qml-meta 返回的 tab 树）→ QML
 * 可消费的 QVariantMap（含 tabs 列表，每个 tab 又含 fields 列表；Convert tab
 * 额外携带 subTabs）。
 *
 * @details tab 树结构（见 preferences-widgets.scm 的 preferences-qml-meta）：
 *   每个 tab 为 (key label fields ...)——前 3 项为 tab 内部键 / 已翻译标题 /
 *   字段描述符列表；Convert tab 第 4 项起为 sub-tabs（list of (sub-key
 * sub-label sub-fields)）。
 *
 * 本函数遍历外层 tab list、每个 tab 取前 3 项构造 QVariantMap{key, label,
 * fields}（经 parse_tab_node）， 若有第 4 项且为 list 则额外加 subTabs 键。
 */
QVariantMap
parse_meta_tree (tmscm tabs) {
  QVariantList tabs_list;
  for (tmscm cur= tabs; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm tab= tmscm_car (cur);
    if (!tmscm_is_list (tab) || tmscm_is_null (tab)) continue;
    QVariantMap tab_map;
    tmscm       tail;
    if (!parse_tab_node (tab, tab_map, tail)) continue; // 前 3 项不足、已 skip
    // 第 4 项起若为 list，则为 sub-tabs（Convert tab 专属）。
    if (!tmscm_is_null (tail) && tmscm_is_list (tmscm_car (tail))) {
      QVariantList sub_list;
      for (tmscm sc= tmscm_car (tail); !tmscm_is_null (sc);
           sc      = tmscm_cdr (sc)) {
        tmscm st= tmscm_car (sc);
        if (!tmscm_is_list (st) || tmscm_is_null (st)) continue;
        QVariantMap m;
        tmscm       sub_tail;
        if (!parse_tab_node (st, m, sub_tail)) continue;
        sub_list << m;
      }
      tab_map["subTabs"]= sub_list;
    }
    tabs_list << tab_map;
  }
  QVariantMap out;
  out["tabs"]= tabs_list;
  return out;
}

/**
 * @brief 把 QVariantMap（diff，只含改动项）序列化为 scheme assoc 字面量字符串。
 *
 * @details 输出 dotted-pair 形 `(("k1" . "v1") ("k2" . "v2"))`——与
 * ParagraphFormat / FontSelector 等 sibling bridge 一致：scheme facade
 * preferences-qml-submit 经 `(cdr (assoc key ...))` 取值，dotted pair 的 cdr
 * 直接是 val 字符串（若用二元组 `(key val)`，cdr 会得到单元素 list `(val)`
 * 而非字符串，下游 setter 全部误判）。val 均为 string（toggle 已在 QML 侧
 * 序列化为 "on"/"off" 串）。key / val 经 qt_scheme_quote 转 Cork 并 quote，
 * 用户输入的任意文本（引号 / 反斜杠 / 换行等）均安全转义。
 */
string
build_assoc_literal (const QVariantMap& changed) {
  string out= "(";
  for (auto it= changed.begin (); it != changed.end (); ++it) {
    out << "(" << qt_scheme_quote (it.key ()) << " . "
        << qt_scheme_quote (it.value ().toString ()) << ")";
  }
  out << ")";
  return out;
}
} // namespace

//*****************************************************************************
// bridge methods
//*****************************************************************************

QVariantMap
PreferencesBridge::meta () {
  return eval_meta ();
}

QString
PreferencesBridge::submit (const QVariantMap& changed) {
  return eval_submit (changed);
}

void
PreferencesBridge::cancel () {
  // 本地暂存模型：Cancel 仅丢弃 QML 本地改动，scheme 侧 no-op（无 setter
  // 调用）。
  if (m_host) m_host->close ();
}

void
PreferencesBridge::callAction (const QString& name) {
  // 行内 action 按钮：透传 action 名给 scheme facade 路由调用。
  // name 是 ASCII 标识符（如 open-auto-backup-location），直接包成 scheme
  // string literal。
  string expr= string ("(preferences-qml-call-action \"") *
               from_qstring (name) * string ("\")");
  eval_scheme (expr);
}

//*****************************************************************************
// helpers
//*****************************************************************************

QVariantMap
PreferencesBridge::eval_meta () {
  tmscm lst= eval_scheme ("(preferences-qml-meta)");
  return parse_meta_tree (lst);
}

QString
PreferencesBridge::eval_submit (const QVariantMap& changed) {
  // 整个 assoc 字面量须 quote：dotted-pair 形 (("k" . "v"))
  // 出现在函数实参位置， 不 quote 会被当函数应用（car "k" 当函数 -> attempt to
  // evaluate ("k" . "v")）。
  string expr= string ("(preferences-qml-submit '") *
               build_assoc_literal (changed) * string (")");
  tmscm result= eval_scheme (expr);
  return tmscm_to_qstring (result);
}
