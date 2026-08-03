/******************************************************************************
 * MODULE      : RecentDocumentsSearchDialog.cpp
 * DESCRIPTION : 最近打开文档搜索 QML 对话框的实现。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "QTMQmlDialog.hpp"
#include "QTMQmlDialogBridge.hpp"
#include "QTMQmlDialogInternal.hpp"
#include "RecentDocumentsSearchBridge.hpp"

#include "qt_utilities.hpp"
#include "s7_tm.hpp"
#include "sys_utils.hpp"

#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QQmlContext>
#include <QQuickWidget>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

string
cpp_search_recent_documents_dialog () {
  string preset= get_env ("MOGAN_TEST_SEARCH_RECENT_DOCUMENTS");
  if (preset != "") return preset == "cancel" ? string ("") : preset;

  QVariantList  recentDocuments;
  QSet<QString> seenPaths;
  tmscm         recentPaths= eval_scheme ("(recent-documents-for-qml)");
  for (tmscm cur= recentPaths; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_string (item)) continue;
    QString path= QString::fromUtf8 (as_charp (tmscm_to_string (item)));
    if (path.isEmpty ()) continue;
    QString normalizedPath= QDir::cleanPath (QDir::fromNativeSeparators (path));
    if (seenPaths.contains (normalizedPath)) continue;
    seenPaths.insert (normalizedPath);
    QVariantMap document;
    document["path"]= path;
    document["name"]= QFileInfo (path).fileName ();
    recentDocuments << document;
  }

  array<string>    buttons= {string ("Open"), string ("Cancel")};
  QmlDialogBridge* bridge = nullptr;
  run_qml_dialog (
      "qrc:/qml/SearchRecentDocuments.qml", "SearchRecentDocuments.qml",
      [&] (QQuickWidget* qw, QDialog& host) {
        bridge= inject_common_context (qw, host);
        RecentDocumentsSearchBridge* searchBridge=
            new RecentDocumentsSearchBridge (
                recentDocuments, qt_translate ("Search recent documents"),
                qt_translate ("Search"),
                qt_translate ("No matching recent documents"),
                translate_buttons (buttons), &host);
        qw->rootContext ()->setContextProperty ("recentSearchBridge",
                                                searchBridge);
      },
      520, 450);

  QString path;
  if (bridge) path= bridge->results ().value ("path").toString ();
  delete bridge;
  return from_qstring (path);
}
