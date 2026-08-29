/******************************************************************************
 * MODULE      : RecentDocumentsSearchBridge.cpp
 * DESCRIPTION : 最近打开文档搜索 QML bridge 的数据与确认动作实现。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "RecentDocumentsSearchBridge.hpp"

#include "qt_utilities.hpp" // qt_translate
#include "s7_tm.hpp"        // eval_scheme + tmscm helpers

#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QVariantMap>

RecentDocumentsSearchBridge::RecentDocumentsSearchBridge (QDialog* host)
    : m_documents (recent_documents ()), m_host (host),
      m_title (qt_translate ("Search recent documents")),
      m_placeholder (qt_translate ("Search")),
      m_emptyText (qt_translate ("No matching recent documents")) {
  ASSERT (host != NULL,
          "RecentDocumentsSearchBridge expects a valid QDialog host");
}

void
RecentDocumentsSearchBridge::open (const QString& path) {
  if (path.isEmpty ()) return;
  m_selectedPath= path;
  m_host->done (QDialog::Accepted);
}

QVariantList
RecentDocumentsSearchBridge::recent_documents () {
  QVariantList  documents;
  QSet<QString> seenPaths;
  tmscm         recentPaths= eval_scheme ("(recent-documents-for-qml)");
  for (tmscm cur= recentPaths; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_string (item)) continue;

    // url->system 已给出 UTF-8 系统路径；再转 Cork 会破坏非 ASCII 文件名。
    QString path= QString::fromUtf8 (as_charp (tmscm_to_string (item)));
    if (path.isEmpty ()) continue;
    QString normalizedPath= QDir::cleanPath (QDir::fromNativeSeparators (path));
#ifdef Q_OS_WIN
    normalizedPath= normalizedPath.toCaseFolded ();
#endif
    if (seenPaths.contains (normalizedPath)) continue;
    seenPaths.insert (normalizedPath);

    QVariantMap document;
    document["path"]= path;
    document["name"]= QFileInfo (path).fileName ();
    documents << document;
  }
  return documents;
}
