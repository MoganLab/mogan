/******************************************************************************
 * MODULE      : RecentDocumentsSearchBridge.cpp
 * DESCRIPTION : 最近打开文档搜索的 QML 数据桥接实现。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "RecentDocumentsSearchBridge.hpp"

#include "converter.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QVariantMap>

RecentDocumentsSearchBridge::RecentDocumentsSearchBridge (QObject* parent)
    : RecentDocumentsSearchBridge (
          recent_documents (), qt_translate ("Search recent documents"),
          qt_translate ("Search"),
          qt_translate ("No matching recent documents"), parent) {}

RecentDocumentsSearchBridge::RecentDocumentsSearchBridge (
    QVariantList documents, QString title, QString placeholder,
    QString emptyText, QObject* parent)
    : QObject (parent), m_documents (documents), m_title (title),
      m_placeholder (placeholder), m_emptyText (emptyText) {}

QVariantList
RecentDocumentsSearchBridge::recent_documents () {
  QVariantList  documents;
  QSet<QString> seenPaths;
  tmscm         recentPaths= eval_scheme ("(recent-documents-for-qml)");
  for (tmscm cur= recentPaths; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_string (item)) continue;

    QString path= utf8_to_qstring (cork_to_utf8 (tmscm_to_string (item)));
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
