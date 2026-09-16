/******************************************************************************
 * MODULE      : PageNumberBridge.cpp
 * DESCRIPTION : 页码设置 QML bridge 实现（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "PageNumberBridge.hpp"

#include "converter.hpp"    // cork_to_utf8
#include "qt_utilities.hpp" // qt_scheme_quote
#include "s7_tm.hpp"        // eval_scheme + tmscm helpers

#include <QVariantList>
#include <QVariantMap>

namespace {
/**
 * @brief scheme 值 → QString，类型安全且编码确定。
 */
QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (v)));
  if (tmscm_is_symbol (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_symbol (v)));
  if (tmscm_is_int (v)) return QString::number (tmscm_to_int (v));
  return QString ();
}

/**
 * @brief 将 QVariantList 规则序列化为 scheme list 字面量。
 * 形如 (("1" "3" "roman") ("4" "total" "arabic"))。
 */
string
build_rules_literal (const QVariantList& rules) {
  string out= "(";
  for (int i= 0; i < rules.size (); ++i) {
    const QVariant& item= rules[i];
    if (item.canConvert<QVariantMap> ()) {
      QVariantMap m    = item.toMap ();
      QString     start= m.value ("start").toString ();
      QString     end  = m.value ("end").toString ();
      QString     style= m.value ("style").toString ();
      out << "(" << qt_scheme_quote (start) << " " << qt_scheme_quote (end)
          << " " << qt_scheme_quote (style) << ") ";
    }
    else if (item.canConvert<QVariantList> ()) {
      QVariantList l= item.toList ();
      if (l.size () >= 3) {
        out << "(" << qt_scheme_quote (l[0].toString ()) << " "
            << qt_scheme_quote (l[1].toString ()) << " "
            << qt_scheme_quote (l[2].toString ()) << ") ";
      }
    }
  }
  out << ")";
  return out;
}
} // namespace

QVariantMap
PageNumberBridge::meta () {
  tmscm       res= eval_scheme ("(pn-qml-meta)");
  QVariantMap out;
  if (!tmscm_is_pair (res)) {
    out["total"] = 1;
    out["rules"] = QVariantList ();
    out["labels"]= QVariantMap ();
    return out;
  }
  for (tmscm cur= res; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString k= tmscm_to_qstring (tmscm_car (pair));
    tmscm   v= tmscm_cdr (pair);
    if (k == "total") {
      int t       = tmscm_to_qstring (v).toInt ();
      out["total"]= (t > 0) ? t : 1;
    }
    else if (k == "rules") {
      QVariantList rulesList;
      for (tmscm rcur= v; !tmscm_is_null (rcur); rcur= tmscm_cdr (rcur)) {
        tmscm ritem= tmscm_car (rcur);
        if (tmscm_is_list (ritem)) {
          QVariantMap ruleMap;
          tmscm       p1   = tmscm_car (ritem);
          tmscm       rest1= tmscm_cdr (ritem);
          tmscm p2= tmscm_is_pair (rest1) ? tmscm_car (rest1) : tmscm_null ();
          tmscm rest2=
              tmscm_is_pair (rest1) ? tmscm_cdr (rest1) : tmscm_null ();
          tmscm p3= tmscm_is_pair (rest2) ? tmscm_car (rest2) : tmscm_null ();
          ruleMap["start"]= tmscm_to_qstring (p1);
          ruleMap["end"]  = tmscm_to_qstring (p2);
          ruleMap["style"]= tmscm_to_qstring (p3);
          rulesList.append (ruleMap);
        }
      }
      out["rules"]= rulesList;
    }
    else if (k == "labels") {
      QVariantMap labelsMap;
      for (tmscm lcur= v; !tmscm_is_null (lcur); lcur= tmscm_cdr (lcur)) {
        tmscm lpair= tmscm_car (lcur);
        if (tmscm_is_pair (lpair)) {
          QString lk   = tmscm_to_qstring (tmscm_car (lpair));
          QString lv   = tmscm_to_qstring (tmscm_cdr (lpair));
          labelsMap[lk]= lv;
        }
      }
      out["labels"]= labelsMap;
    }
  }
  return out;
}

void
PageNumberBridge::submit (const QVariantList& rules) {
  string cmd= "(pn-qml-submit '" * build_rules_literal (rules) * ")";
  eval_scheme (cmd);
  if (m_host) m_host->close ();
}

void
PageNumberBridge::cancel () {
  if (m_host) m_host->close ();
}
