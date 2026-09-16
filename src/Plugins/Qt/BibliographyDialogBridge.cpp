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
#include "object_l1.hpp"
#include "preferences.hpp"
#include "qt_gui.hpp"
#include "qt_simple_widget.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp" // eval_scheme + tmscm helpers
#include "server.hpp"
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
  if (tmscm_is_list (res) && !tmscm_is_null (res)) {
    tmscm item_status= tmscm_car (res);
    res              = tmscm_cdr (res);
    tmscm item_hint  = tmscm_is_null (res) ? tmscm_null () : tmscm_car (res);
    res              = tmscm_is_null (res) ? tmscm_null () : tmscm_cdr (res);
    tmscm item_tree  = tmscm_is_null (res) ? tmscm_null () : tmscm_car (res);

    QString status= tmscm_is_string (item_status)
                        ? tmscm_to_qstring (item_status)
                        : QStringLiteral ("empty");
    QString hint=
        tmscm_is_string (item_hint) ? tmscm_to_qstring (item_hint) : QString ();

    out["status"]= status;
    out["hint"]  = hint;

    if (!is_none (m_preview_buf_url)) {
      if (status == QStringLiteral ("valid") && tmscm_is_tree (item_tree)) {
        tree doc     = tmscm_to_tree (item_tree);
        tree sty     = bib_preview_style ();
        tree enriched= enrich_embedded_document (doc, sty);
        set_buffer_tree (m_preview_buf_url, enriched);
        texmacs_interpose_handler ();
        the_gui->force_update ();
        m_isValid= true;
        if (m_previewWidget) {
          updatePreviewGeometry ();
          m_previewWidget->show ();
          m_previewWidget->raise ();
          QTMWidget* editor= m_previewWidget->findChild<QTMWidget*> ();
          if (editor && editor->tm_widget ()) {
            editor->show ();
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
        tree sty      = bib_preview_style ();
        tree empty_doc= enrich_embedded_document (tree (DOCUMENT, ""), sty);
        if (contains (m_preview_buf_url, get_all_buffers ())) {
          set_buffer_tree (m_preview_buf_url, empty_doc);
        }
        m_isValid= false;
        if (m_previewWidget) {
          m_previewWidget->hide ();
        }
      }
    }
    return out;
  }
  return out;
}

void
BibliographyDialogBridge::setPreviewVisible (bool visible) {
  if (m_previewWidget) {
    if (visible && m_isValid) {
      updatePreviewGeometry ();
      m_previewWidget->show ();
      m_previewWidget->raise ();
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
