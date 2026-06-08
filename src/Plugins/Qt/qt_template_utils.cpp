
/******************************************************************************
 * MODULE     : qt_template_utils.cpp
 * DESCRIPTION: Qt helpers for template document operations
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_template_utils.hpp"
#include "qt_floating_toast.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>

QString
qt_generate_document_save_path (const QString& templateName) {
  QString docsDir=
      QStandardPaths::writableLocation (QStandardPaths::DocumentsLocation);
  if (docsDir.isEmpty ()) {
    docsDir= QStandardPaths::writableLocation (QStandardPaths::HomeLocation);
  }
  docsDir= QDir (docsDir).filePath ("LiiiSTEM/library");

  QString baseName= templateName;
  baseName.replace (QRegularExpression ("[\\\\/:*?\"<>|]"), "_");
  if (baseName.isEmpty ()) baseName= "template";

  if (!QDir (docsDir).exists ()) QDir ().mkpath (docsDir);

  QString ext= ".tmu";
  for (int i= 0; i < 10000; ++i) {
    QString fileName=
        i == 0 ? baseName + ext
               : QString ("%1(%2)%3").arg (baseName).arg (i).arg (ext);
    QString filePath= QDir (docsDir).filePath (fileName);
    if (!QFile::exists (filePath)) return filePath;
  }
  return QString ();
}

QString
qt_copy_template_to_documents (const QString& sourcePath,
                               const QString& templateName) {
  QString savePath= qt_generate_document_save_path (templateName);
  if (!QFile::copy (sourcePath, savePath)) {
    qWarning () << "Failed to copy template from" << sourcePath << "to"
                << savePath;
    return QString ();
  }
  return savePath;
}

void
qt_load_document_path (const QString& path) {
  eval_scheme ("(load-document " * qt_scheme_quote_utf8 (path) * ")");
}

bool
qt_copy_template_and_load (QWidget* parent, const QString& sourcePath,
                           const QString& templateName) {
  QString docPath= qt_copy_template_to_documents (sourcePath, templateName);
  if (docPath.isEmpty ()) {
    QtFloatingToast::showToast (
        parent, qt_translate ("Failed to copy template to Documents"), 3000,
        QtFloatingToast::Error);
    return false;
  }
  qt_load_document_path (docPath);
  return true;
}
