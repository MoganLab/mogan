/******************************************************************************
 * MODULE      : GoMenuBridge.cpp
 * DESCRIPTION : Go 菜单 QML bridge 实现
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "GoMenuBridge.hpp"

#include "qt_utilities.hpp"
#include "s7_tm.hpp"     // eval_scheme + tmscm helpers
#include "scheme.hpp"    // scheme_cmd + exec_delayed
#include "tm_server.hpp" // is_server_started

#include <QVariantList>

namespace {

QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v)) return to_qstring (tmscm_to_string (v));
  if (tmscm_is_symbol (v)) return to_qstring (tmscm_to_symbol (v));
  return QString ();
}

} // namespace

GoMenuBridge::GoMenuBridge (QWidget* host) : QObject (host), m_host (host) {
  loadMeta ();
}

GoMenuBridge::~GoMenuBridge ()= default;

void
GoMenuBridge::loadMeta () {
  m_meta.clear ();
  // 默认兜底值
  m_meta["label_recent"]= qt_translate ("Recent");
  m_meta["recent"]      = QVariantList ();

  if (!is_server_started ()) return;

  tmscm res= eval_scheme ("(go-menu-meta)");
  if (!tmscm_is_list (res)) return;

  for (tmscm cur= res; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString key= tmscm_to_qstring (tmscm_car (pair));
    tmscm   val= tmscm_cdr (pair);

    if (key == "label_recent") {
      m_meta["label_recent"]= tmscm_to_qstring (val);
    }
    else if (key == "recent" && tmscm_is_list (val)) {
      QVariantList recs;
      for (tmscm rcur= val; !tmscm_is_null (rcur); rcur= tmscm_cdr (rcur)) {
        tmscm ritem= tmscm_car (rcur);
        if (tmscm_is_list (ritem)) {
          tmscm url_scm= tmscm_car (ritem);
          ritem        = tmscm_cdr (ritem);
          tmscm title_scm=
              tmscm_is_null (ritem) ? tmscm_null () : tmscm_car (ritem);

          QVariantMap item;
          item["url"]  = tmscm_to_qstring (url_scm);
          item["title"]= tmscm_to_qstring (title_scm);
          recs.append (item);
        }
      }
      m_meta["recent"]= recs;
    }
  }
}

void
GoMenuBridge::closeMenu () {
  if (m_host) {
    m_host->close ();
  }
}

void
GoMenuBridge::loadBuffer (const QString& url) {
  closeMenu ();
  if (!is_server_started ()) return;
  exec_delayed (
      scheme_cmd ("(go-menu-load-buffer " * qt_scheme_quote_utf8 (url) * ")"));
}
