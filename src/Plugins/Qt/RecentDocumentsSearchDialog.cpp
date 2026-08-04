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
#include "sys_utils.hpp"

#include <QDialog>
#include <QQmlContext>
#include <QQuickWidget>

string
cpp_search_recent_documents_dialog () {
  string preset= get_env ("MOGAN_TEST_SEARCH_RECENT_DOCUMENTS");
  if (preset != "") return preset == "cancel" ? string ("") : preset;

  array<string>    buttons= {string ("Open"), string ("Cancel")};
  QmlDialogBridge* bridge = nullptr;
  run_qml_dialog (
      "qrc:/qml/SearchRecentDocuments.qml", "SearchRecentDocuments.qml",
      [&] (QQuickWidget* qw, QDialog& host) {
        bridge= inject_common_context (qw, host);
        RecentDocumentsSearchBridge* searchBridge=
            new RecentDocumentsSearchBridge (&host);
        qw->rootContext ()->setContextProperty ("recentSearchBridge",
                                                searchBridge);
        qw->rootContext ()->setContextProperty ("dialogButtons",
                                                translate_buttons (buttons));
      },
      520, 450);

  QString path;
  if (bridge) path= bridge->results ().value ("path").toString ();
  delete bridge;
  return from_qstring (path);
}
