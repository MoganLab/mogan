/******************************************************************************
 * MODULE      : BibliographyDialogBridge.cpp
 * DESCRIPTION : 「插入/修改参考文献」QML 对话框 bridge 实现（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "BibliographyDialogBridge.hpp"

#include "converter.hpp" // cork_to_utf8
#include "qt_utilities.hpp"
#include "s7_tm.hpp" // eval_scheme + tmscm helpers

#include <QDir>
#include <QFileDialog>

static QString
tmscm_to_qstring (tmscm obj) {
  return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (obj)));
}

BibliographyDialogBridge::BibliographyDialogBridge (QDialog*       host,
                                                    const QString& doc_dir)
    : QObject (), m_host (host), m_doc_dir (doc_dir) {
  ASSERT (host != NULL,
          "BibliographyDialogBridge expects a valid QDialog host");
}

QString
BibliographyDialogBridge::browse (const QString& current) {
  QString start= current;
  if (start.isEmpty ()) {
    start= m_doc_dir.isEmpty () ? QDir::homePath () : m_doc_dir;
  }
  else if (QDir::isRelativePath (start) && !m_doc_dir.isEmpty ()) {
    start= QDir (m_doc_dir).filePath (start);
  }
  QString path= QFileDialog::getOpenFileName (
      m_host, qt_translate ("Choose BibTeX file"), start,
      QStringLiteral ("BibTeX (*.bib);;All files (*)"));
  return path;
}

QVariantMap
BibliographyDialogBridge::requestPreview (const QString& file,
                                          const QString& style) {
  QVariantMap out;
  out["status"] = QStringLiteral ("empty");
  out["hint"]   = QString ();
  out["preview"]= QString ();

  string expr= "(bibliography-preview " * qt_scheme_quote (file) * " " *
               qt_scheme_quote (style) * ")";
  tmscm res= eval_scheme (expr);
  if (tmscm_is_list (res) && !tmscm_is_null (res)) {
    tmscm item_status= tmscm_car (res);
    res              = tmscm_cdr (res);
    tmscm item_hint  = tmscm_is_null (res) ? tmscm_null () : tmscm_car (res);
    res              = tmscm_is_null (res) ? tmscm_null () : tmscm_cdr (res);
    tmscm item_img   = tmscm_is_null (res) ? tmscm_null () : tmscm_car (res);

    QString status= tmscm_is_string (item_status)
                        ? tmscm_to_qstring (item_status)
                        : QStringLiteral ("empty");
    QString hint=
        tmscm_is_string (item_hint) ? tmscm_to_qstring (item_hint) : QString ();
    QString preview=
        tmscm_is_string (item_img) ? tmscm_to_qstring (item_img) : QString ();

    out["status"] = status;
    out["hint"]   = hint;
    out["preview"]= preview;
    return out;
  }
  return out;
}

QString
BibliographyDialogBridge::toRelativePath (const QString& fullPath) {
  if (fullPath.isEmpty () || m_doc_dir.isEmpty ()) return fullPath;
  if (QDir::isRelativePath (fullPath)) return fullPath;
  QDir docDir (m_doc_dir);
  return docDir.relativeFilePath (fullPath);
}
