/******************************************************************************
 * MODULE      : GoMenuBridge.cpp
 * DESCRIPTION : Go 菜单 QML bridge 实现
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "GoMenuBridge.hpp"

#include "converter.hpp" // cork_to_utf8
#include "qt_utilities.hpp"
#include "s7_tm.hpp"     // eval_scheme + tmscm helpers
#include "tm_server.hpp" // is_server_started

#include <QTimer>
#include <QVariantList>

namespace {

QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (v)));
  if (tmscm_is_symbol (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_symbol (v)));
  return QString ();
}

bool
tmscm_to_cpp_bool (tmscm v) {
  if (tmscm_is_bool (v)) return tmscm_to_bool (v);
  if (tmscm_is_string (v)) {
    QString s= tmscm_to_qstring (v);
    return s == "true" || s == "#t";
  }
  return false;
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
  m_meta["can_back"]     = false;
  m_meta["can_forward"]  = false;
  m_meta["label_back"]   = qt_translate ("Back");
  m_meta["label_forward"]= qt_translate ("Forward");
  m_meta["label_save"]   = qt_translate ("Save position");
  m_meta["label_buffers"]= qt_translate ("Open documents");
  m_meta["label_recent"] = qt_translate ("Recent");
  m_meta["buffers"]      = QVariantList ();
  m_meta["recent"]       = QVariantList ();

  if (!is_server_started ()) return;

  tmscm res= eval_scheme ("(go-menu-meta)");
  if (!tmscm_is_list (res)) return;

  for (tmscm cur= res; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString key= tmscm_to_qstring (tmscm_car (pair));
    tmscm   val= tmscm_cdr (pair);

    if (key == "can_back") {
      m_meta["can_back"]= tmscm_to_cpp_bool (val);
    }
    else if (key == "can_forward") {
      m_meta["can_forward"]= tmscm_to_cpp_bool (val);
    }
    else if (key == "label_back") {
      m_meta["label_back"]= tmscm_to_qstring (val);
    }
    else if (key == "label_forward") {
      m_meta["label_forward"]= tmscm_to_qstring (val);
    }
    else if (key == "label_save") {
      m_meta["label_save"]= tmscm_to_qstring (val);
    }
    else if (key == "label_buffers") {
      m_meta["label_buffers"]= tmscm_to_qstring (val);
    }
    else if (key == "label_recent") {
      m_meta["label_recent"]= tmscm_to_qstring (val);
    }
    else if (key == "buffers" && tmscm_is_list (val)) {
      QVariantList bufs;
      for (tmscm bcur= val; !tmscm_is_null (bcur); bcur= tmscm_cdr (bcur)) {
        tmscm bitem= tmscm_car (bcur);
        if (tmscm_is_list (bitem)) {
          tmscm url_scm= tmscm_car (bitem);
          bitem        = tmscm_cdr (bitem);
          tmscm title_scm=
              tmscm_is_null (bitem) ? tmscm_null () : tmscm_car (bitem);
          bitem= tmscm_is_null (bitem) ? tmscm_null () : tmscm_cdr (bitem);
          tmscm curr_scm=
              tmscm_is_null (bitem) ? tmscm_null () : tmscm_car (bitem);

          QVariantMap item;
          item["url"]    = tmscm_to_qstring (url_scm);
          item["title"]  = tmscm_to_qstring (title_scm);
          item["current"]= tmscm_to_cpp_bool (curr_scm);
          bufs.append (item);
        }
      }
      m_meta["buffers"]= bufs;
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
GoMenuBridge::goBack () {
  closeMenu ();
  if (!is_server_started ()) return;
  QTimer::singleShot (0, [] () { eval_scheme ("(cursor-history-backward)"); });
}

void
GoMenuBridge::goForward () {
  closeMenu ();
  if (!is_server_started ()) return;
  QTimer::singleShot (0, [] () { eval_scheme ("(cursor-history-forward)"); });
}

void
GoMenuBridge::savePosition () {
  closeMenu ();
  if (!is_server_started ()) return;
  QTimer::singleShot (
      0, [] () { eval_scheme ("(cursor-history-add (cursor-path))"); });
}

void
GoMenuBridge::switchToBuffer (const QString& url) {
  closeMenu ();
  if (!is_server_started ()) return;
  string expr= "(go-menu-switch-to-buffer " * qt_scheme_quote_utf8 (url) * ")";
  QTimer::singleShot (0, [expr] () { eval_scheme (expr); });
}

void
GoMenuBridge::loadBuffer (const QString& url) {
  closeMenu ();
  if (!is_server_started ()) return;
  string expr= "(go-menu-load-buffer " * qt_scheme_quote_utf8 (url) * ")";
  QTimer::singleShot (0, [expr] () { eval_scheme (expr); });
}
