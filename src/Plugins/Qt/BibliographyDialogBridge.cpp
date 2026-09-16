/******************************************************************************
 * MODULE      : BibliographyDialogBridge.cpp
 * DESCRIPTION : 「插入/修改参考文献」QML 对话框 bridge 实现（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "BibliographyDialogBridge.hpp"

#include "QTMWidget.hpp"
#include "converter.hpp" // cork_to_utf8
#include "new_buffer.hpp"
#include "object_l1.hpp" // tmscm_is_tree / tmscm_to_tree
#include "preferences.hpp"
#include "qt_gui.hpp"
#include "qt_simple_widget.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp" // eval_scheme + tmscm helpers
#include "tm_window.hpp"

#include <moebius/vars.hpp>

#include <QDir>
#include <QFileDialog>
#include <QQuickWidget>

using namespace moebius;

tree
bib_preview_style () {
  tree packs (TUPLE);
  packs << "generic";
  string theme= get_preference ("gui theme", "default");
  if (theme == "liii-night" || theme == "dark") packs << "dark";
  return compound ("style", packs);
}

static QString
tmscm_to_qstring (tmscm obj) {
  return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (obj)));
}

BibliographyDialogBridge::BibliographyDialogBridge (QDialog*       host,
                                                    const QString& doc_dir,
                                                    QWidget*   previewWidget,
                                                    const url& preview_buf_url)
    : QObject (), m_host (host), m_doc_dir (doc_dir),
      m_previewWidget (previewWidget), m_preview_buf_url (preview_buf_url),
      m_placeholder (nullptr), m_isValid (false) {
  ASSERT (host != NULL,
          "BibliographyDialogBridge expects a valid QDialog host");
}

void
BibliographyDialogBridge::setPlaceholder (QQuickItem* placeholder) {
  m_placeholder= placeholder;
}

void
BibliographyDialogBridge::updatePreviewGeometry () {
  if (!m_previewWidget || !m_placeholder || !m_host) return;
  QQuickWidget* qw= m_host->findChild<QQuickWidget*> ();
  if (!qw || !qw->rootObject ()) return;
  QPointF p= m_placeholder->mapToItem (qw->rootObject (), QPointF (0, 0));
  m_previewWidget->setGeometry (QRect (
      p.toPoint (), QSize (m_placeholder->width (), m_placeholder->height ())));
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
  out["status"]= QStringLiteral ("empty");
  out["hint"]  = QString ();

  string expr= "(bib-to-tree " * qt_scheme_quote (file) * " " *
               qt_scheme_quote (style) * ")";
  tmscm res= eval_scheme (expr);
  if (!tmscm_is_list (res) || tmscm_is_null (res)) return out;

  tmscm item_status= tmscm_car (res);
  res              = tmscm_cdr (res);
  tmscm item_hint  = tmscm_is_null (res) ? tmscm_null () : tmscm_car (res);
  res              = tmscm_is_null (res) ? tmscm_null () : tmscm_cdr (res);
  tmscm item_tree  = tmscm_is_null (res) ? tmscm_null () : tmscm_car (res);

  QString status= tmscm_is_string (item_status) ? tmscm_to_qstring (item_status)
                                                : QStringLiteral ("empty");
  out["status"] = status;
  out["hint"]=
      tmscm_is_string (item_hint) ? tmscm_to_qstring (item_hint) : QString ();

  if (status == QStringLiteral ("valid") && tmscm_is_tree (item_tree)) {
    tree enriched= enrich_embedded_document (tmscm_to_tree (item_tree),
                                             bib_preview_style ());
    set_buffer_tree (m_preview_buf_url, enriched);
    the_gui->force_update ();
    m_isValid= true;
    if (m_previewWidget) {
      showPreview ();
      QTMWidget* editor= m_previewWidget->findChild<QTMWidget*> ();
      if (editor && editor->tm_widget ()) {
        editor->resize (m_previewWidget->size ());
        editor->tm_widget ()->repaint_invalid_regions ();
        QTimer::singleShot (50, this, [editor] () {
          if (editor && editor->tm_widget ()) {
            editor->tm_widget ()->repaint_invalid_regions ();
          }
        });
      }
    }
  }
  else {
    m_isValid= false;
    if (m_previewWidget) {
      m_previewWidget->hide ();
    }
  }
  return out;
}

void
BibliographyDialogBridge::showPreview () {
  updatePreviewGeometry ();
  m_previewWidget->show ();
  m_previewWidget->raise ();
}

void
BibliographyDialogBridge::setPreviewVisible (bool visible) {
  if (m_previewWidget) {
    if (visible && m_isValid) {
      showPreview ();
    }
    else {
      m_previewWidget->hide ();
    }
  }
}

QString
BibliographyDialogBridge::toRelativePath (const QString& fullPath) {
  if (fullPath.isEmpty () || m_doc_dir.isEmpty ()) return fullPath;
  if (QDir::isRelativePath (fullPath)) return fullPath;
  QDir docDir (m_doc_dir);
  return docDir.relativeFilePath (fullPath);
}
