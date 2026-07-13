/******************************************************************************
 * MODULE      : FontSelectorBridge.cpp
 * DESCRIPTION : 字体选择器 QML bridge 实现（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "FontSelectorBridge.hpp"

#include "analyze.hpp"   // as_string(int)
#include "converter.hpp" // cork_to_utf8
#include "qt_utilities.hpp"
#include "s7_tm.hpp" // eval_scheme + tmscm helpers

#include <moebius/data/scheme.hpp> // scm_quote

#include <QVariantList>

namespace {
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
 * @brief scheme 值 → QString，类型安全且编码确定。
 * @details tmscm_to_string 内部调 s7_string，仅对 string 类型合法；对 symbol 会
 * 解引用垃圾指针（SIGBUS）。scheme assoc 的 key 常是 symbol，故按类型分流。
 * mogan string 内部是 Cork 编码，统一 cork_to_utf8 转 UTF-8 再 to QString——不用
 * to_qstring（其 looks_utf8/looks_ascii 启发式对纯 Cork 中文不稳定）。
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
 * @brief 把 specsKey 拼成 scheme 整数字面。
 */
inline string
key_token (int k) {
  return as_string (k);
}
} // namespace

//*****************************************************************************
// helpers
//*****************************************************************************

QStringList
FontSelectorBridge::evalStringList (const string& proc) {
  string expr= "(" * proc * " " * key_token (m_specsKey) * ")";
  return tmscm_to_stringlist (eval_scheme (expr));
}

QString
FontSelectorBridge::evalString (const string& proc) {
  string expr= "(" * proc * " " * key_token (m_specsKey) * ")";
  return tmscm_to_qstring (eval_scheme (expr));
}

QString
FontSelectorBridge::evalString1 (const string& proc, const string& arg) {
  // arg 是 scheme keyword 字面（":family"/":style"/":size"，ASCII 故
  // cork==ascii）。 直接拼入表达式——不能 scm_quote：那会变成 string，与 scheme
  // 侧 keyword 比较
  // (== var :family) 不等，selector-get 落入 else 返回 #f，转空 QString。
  string expr= "(" * proc * " " * key_token (m_specsKey) * " " * arg * ")";
  return tmscm_to_qstring (eval_scheme (expr));
}

QString
FontSelectorBridge::evalPreview () {
  return evalString ("font-selector-preview");
}

//*****************************************************************************
// 三栏
//*****************************************************************************

QStringList
FontSelectorBridge::requestFamilies () {
  return evalStringList ("font-selector-families");
}
QStringList
FontSelectorBridge::requestSizes () {
  return evalStringList ("font-selector-sizes");
}
QString
FontSelectorBridge::currentFamily () {
  return evalString1 ("font-selector-get", ":family");
}
QString
FontSelectorBridge::currentStyle () {
  return evalString1 ("font-selector-get", ":style");
}
QString
FontSelectorBridge::currentSize () {
  return evalString1 ("font-selector-get", ":size");
}

/**
 * @brief 联动 setter 通用实现：调 `(proc key 'arg')` 得 assoc list
 *        ((k . v) ...)，转 QVariantMap。arg 是 cork mogan string，直接
 * scm_quote 拼 scheme（确认式 utf8→cork 已在 from_qstring 完成，不再 to_qstring
 * 启发式）。
 */
static QVariantMap
eval_assoc_result (const string& proc, int key, const string& arg) {
  string expr= "(" * proc * " " * as_string (key) * " " *
               moebius::data::scm_quote (arg) * ")";
  tmscm       lst= eval_scheme (expr);
  QVariantMap out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString k= tmscm_to_qstring (tmscm_car (pair));
    tmscm   v= tmscm_cdr (pair);
    if (tmscm_is_string (v)) out.insert (k, tmscm_to_qstring (v));
    else if (tmscm_is_list (v))
      out.insert (k, QVariant::fromValue (tmscm_to_stringlist (v)));
  }
  return out;
}

QVariantMap
FontSelectorBridge::setFamily (const QString& family) {
  // set-family 返回 {styles}（filter 改 family 后 style 列表刷新）。
  return eval_assoc_result ("font-selector-set-family", m_specsKey,
                            from_qstring (family));
}
QVariantMap
FontSelectorBridge::setStyle (const QString& style) {
  // set-style 仅触发 live 写回（预览实时 widget 自动刷新），返回空 map。
  eval_scheme ("(font-selector-set-style " * key_token (m_specsKey) * " " *
               moebius::data::scm_quote (from_qstring (style)) * ")");
  return QVariantMap ();
}
QVariantMap
FontSelectorBridge::setSize (const QString& size) {
  eval_scheme ("(font-selector-set-size " * key_token (m_specsKey) * " " *
               moebius::data::scm_quote (from_qstring (size)) * ")");
  return QVariantMap ();
}

QStringList
FontSelectorBridge::requestStyles (const QString& family) {
  string expr= "(font-selector-styles " * key_token (m_specsKey) * " " *
               qt_scheme_quote (family) * ")";
  return tmscm_to_stringlist (eval_scheme (expr));
}

//*****************************************************************************
// Filter
//*****************************************************************************

/**
 * @brief filter-meta：list of (label var options value) → QVariantList of map。
 * 每项 value 可能是 string；options 是 list。
 */
static QVariantList
eval_filter_meta (int key) {
  string       expr= "(font-selector-filter-meta " * as_string (key) * ")";
  tmscm        lst = eval_scheme (expr);
  QVariantList out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_list (item) || tmscm_is_null (item)) continue;
    QVariantMap m;
    m["label"]  = tmscm_to_qstring (tmscm_car (item));
    item        = tmscm_cdr (item);
    m["var"]    = tmscm_to_qstring (tmscm_car (item));
    item        = tmscm_cdr (item);
    m["options"]= QVariant::fromValue (tmscm_to_stringlist (tmscm_car (item)));
    item        = tmscm_cdr (item);
    m["value"]  = tmscm_to_qstring (tmscm_car (item));
    out << m;
  }
  return out;
}

QVariantList
FontSelectorBridge::filterMeta () {
  return eval_filter_meta (m_specsKey);
}
QVariantMap
FontSelectorBridge::setFilter (const QString& var, const QString& val) {
  string expr= "(font-selector-set-filter " * key_token (m_specsKey) * " " *
               qt_scheme_quote (var) * " " * qt_scheme_quote (val) * ")";
  tmscm       lst= eval_scheme (expr);
  QVariantMap out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString k= tmscm_to_qstring (tmscm_car (pair));
    tmscm   v= tmscm_cdr (pair);
    if (tmscm_is_string (v)) out.insert (k, tmscm_to_qstring (v));
    else if (tmscm_is_list (v))
      out.insert (k, QVariant::fromValue (tmscm_to_stringlist (v)));
  }
  return out;
}

//*****************************************************************************
// 预览 / 样本
//*****************************************************************************

QString
FontSelectorBridge::requestPreview () {
  return evalPreview ();
}
QStringList
FontSelectorBridge::sampleKinds () {
  return evalStringList ("font-selector-sample-kinds");
}
QString
FontSelectorBridge::currentSampleKind () {
  return evalString ("font-selector-current-sample-kind");
}
QVariantMap
FontSelectorBridge::setSampleKind (const QString& kind) {
  return eval_assoc_result ("font-selector-set-sample-kind", m_specsKey,
                            from_qstring (kind));
}

//*****************************************************************************
// Advanced 定制
//*****************************************************************************

QVariantList
FontSelectorBridge::customizeMeta () {
  string expr= "(font-selector-customize-meta " * key_token (m_specsKey) * ")";
  tmscm  lst = eval_scheme (expr);
  QVariantList out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_list (item) || tmscm_is_null (item)) continue;
    QVariantMap m;
    m["group"]  = tmscm_to_qstring (tmscm_car (item));
    item        = tmscm_cdr (item);
    m["label"]  = tmscm_to_qstring (tmscm_car (item));
    item        = tmscm_cdr (item);
    m["which"]  = tmscm_to_qstring (tmscm_car (item));
    item        = tmscm_cdr (item);
    m["options"]= QVariant::fromValue (tmscm_to_stringlist (tmscm_car (item)));
    item        = tmscm_cdr (item);
    m["value"]  = tmscm_to_qstring (tmscm_car (item));
    out << m;
  }
  return out;
}
QVariantMap
FontSelectorBridge::setCustomize (const QString& which, const QString& val) {
  string expr= "(font-selector-customize-set " * key_token (m_specsKey) * " " *
               qt_scheme_quote (which) * " " * qt_scheme_quote (val) * ")";
  tmscm       lst= eval_scheme (expr);
  QVariantMap out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString k= tmscm_to_qstring (tmscm_car (pair));
    tmscm   v= tmscm_cdr (pair);
    if (tmscm_is_string (v)) out.insert (k, tmscm_to_qstring (v));
  }
  return out;
}

//*****************************************************************************
// 动作
//*****************************************************************************

QVariantMap
FontSelectorBridge::uiLabels () {
  string      expr= "(font-selector-ui-labels " * key_token (m_specsKey) * ")";
  tmscm       lst = eval_scheme (expr);
  QVariantMap out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    out.insert (tmscm_to_qstring (tmscm_car (pair)),
                tmscm_to_qstring (tmscm_cdr (pair)));
  }
  return out;
}

void
FontSelectorBridge::importFont () {
  string expr= "(font-selector-import " * key_token (m_specsKey) * ")";
  eval_scheme (expr);
}

void
FontSelectorBridge::reset () {
  string expr= "(font-selector-restore " * key_token (m_specsKey) * ")";
  eval_scheme (expr);
}

void
FontSelectorBridge::submit () {
  string expr= "(font-selector-commit " * key_token (m_specsKey) * ")";
  eval_scheme (expr);
  m_host->done (QDialog::Accepted);
}

void
FontSelectorBridge::cancel () {
  m_host->done (QDialog::Rejected);
}
