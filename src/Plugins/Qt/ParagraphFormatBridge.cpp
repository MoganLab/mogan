/******************************************************************************
 * MODULE      : ParagraphFormatBridge.cpp
 * DESCRIPTION : 段落格式 QML bridge 实现（见配套 .hpp）。参考
 *FontSelectorBridge。 COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "ParagraphFormatBridge.hpp"

#include "analyze.hpp"   // as_string(int)
#include "converter.hpp" // cork_to_utf8
#include "qt_utilities.hpp"
#include "s7_tm.hpp" // eval_scheme + tmscm helpers

namespace {
/**
 * @brief scheme 值 → QString，类型安全且编码确定（symbol/string 分流）。
 * @details tmscm_to_string 仅对 string 合法；scheme assoc 的 key 常是 symbol。
 * mogan string 内部 Cork 编码，统一 cork_to_utf8 转 UTF-8。
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
 * @brief 把 specsKey 拼成 scheme 整数字面。
 */
inline string
key_token (int k) {
  return as_string (k);
}
} // namespace

//*****************************************************************************
// meta：基础/高级字段表
//*****************************************************************************

/**
 * @brief paragraph-format-meta 返回 list of assoc（每项 ((label . x)(options .
 * y) (var . z)(value . w)(editable . b))），转 QVariantList of QVariantMap。
 */
QVariantList
ParagraphFormatBridge::evalMeta (const string& which) {
  // which 是 ASCII 字面（"basic"/"advanced"），裸拼入表达式并 quote 成 scheme
  // string。
  string expr= "(paragraph-format-meta " * key_token (m_specsKey) * " \"" *
               which * "\")";
  tmscm        lst= eval_scheme (expr);
  QVariantList out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_list (item)) continue;
    QVariantMap m;
    // item 是 assoc list，遍历各 (key . val) 对。
    for (tmscm p= item; !tmscm_is_null (p); p= tmscm_cdr (p)) {
      tmscm pair= tmscm_car (p);
      if (!tmscm_is_pair (pair)) continue;
      QString k= tmscm_to_qstring (tmscm_car (pair));
      tmscm   v= tmscm_cdr (pair);
      if (k == "options")
        m["options"]= QVariant::fromValue (tmscm_to_stringlist (v));
      else if (k == "editable") m["editable"]= tmscm_to_bool (v);
      else m[k]= tmscm_to_qstring (v);
    }
    out << m;
  }
  return out;
}

QVariantList
ParagraphFormatBridge::basicMeta () {
  return evalMeta ("basic");
}

QVariantList
ParagraphFormatBridge::advancedMeta () {
  return evalMeta ("advanced");
}

//*****************************************************************************
// live 写回
//*****************************************************************************

QString
ParagraphFormatBridge::setPara (const QString& var, const QString& val) {
  string expr= "(paragraph-format-set " * key_token (m_specsKey) * " " *
               qt_scheme_quote (var) * " " * qt_scheme_quote (val) * ")";
  return tmscm_to_qstring (eval_scheme (expr));
}

//*****************************************************************************
// 动作
//*****************************************************************************

void
ParagraphFormatBridge::submit () {
  eval_scheme ("(paragraph-format-commit " * key_token (m_specsKey) * ")");
  m_host->close ();
}

void
ParagraphFormatBridge::cancel () {
  eval_scheme ("(paragraph-format-cancel " * key_token (m_specsKey) * ")");
  m_host->close ();
}

void
ParagraphFormatBridge::reset () {
  eval_scheme ("(paragraph-format-revert " * key_token (m_specsKey) * ")");
}

//*****************************************************************************
// UI 标签 + 行间距预设
//*****************************************************************************

/**
 * @brief paragraph-format-ui-labels 返回 assoc：
 *   ((basic . s)(advanced . s)(reset . s)(ok . s)(cancel . s)
 *    (sepPresets . (((label . s)(val . s)) ...)))
 * sepPresets 的值是 list，转 QVariantList of QVariantMap；其余 string。
 */
QVariantMap
ParagraphFormatBridge::uiLabels () {
  string      expr= "(paragraph-format-ui-labels)";
  tmscm       lst = eval_scheme (expr);
  QVariantMap out;
  for (tmscm cur= lst; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString k= tmscm_to_qstring (tmscm_car (pair));
    tmscm   v= tmscm_cdr (pair);
    if (k == "sepPresets" && tmscm_is_list (v)) {
      QVariantList presets;
      for (tmscm pc= v; !tmscm_is_null (pc); pc= tmscm_cdr (pc)) {
        tmscm       item= tmscm_car (pc);
        QVariantMap pm;
        for (tmscm p= item; !tmscm_is_null (p); p= tmscm_cdr (p)) {
          tmscm pp= tmscm_car (p);
          if (!tmscm_is_pair (pp)) continue;
          pm[tmscm_to_qstring (tmscm_car (pp))]=
              tmscm_to_qstring (tmscm_cdr (pp));
        }
        presets << pm;
      }
      out.insert ("sepPresets", presets);
    }
    else out.insert (k, tmscm_to_qstring (v));
  }
  return out;
}
