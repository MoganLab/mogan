
/******************************************************************************
 * MODULE     : QTMTemplateOpener.cpp
 * DESCRIPTION: Unified template opener implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMTemplateOpener.hpp"
#include "qt_floating_toast.hpp"
#include "qt_template_utils.hpp"
#include "qt_utilities.hpp"
#include "template_manager.hpp"

#include <QProgressDialog>

QTMTemplateOpener::QTMTemplateOpener (QWidget* parent)
    : QObject (parent), parent_ (parent),
      templateManager_ (TemplateManager::instance ()) {}

QTMTemplateOpener::~QTMTemplateOpener () { cleanupProgressDialog_ (); }

bool
QTMTemplateOpener::isAvailableLocally (const QString& templateId) {
  return templateManager_ && templateManager_->verifyLocalTemplate (templateId);
}

bool
QTMTemplateOpener::openTemplate (const QString& templateId) {
  resetState_ ();
  currentTemplateId_= templateId;

  if (isAvailableLocally (templateId)) {
    return openLocalTemplate_ (templateId);
  }

  return startDownload_ (templateId);
}

bool
QTMTemplateOpener::openLocalTemplate_ (const QString& templateId) {
  if (!templateManager_) {
    showError_ (qt_translate ("Template manager not available"));
    emit failed (templateId, qt_translate ("Template manager not available"));
    return false;
  }

  auto meta= templateManager_->templateById (templateId);
  if (!meta) {
    showError_ (qt_translate ("Template metadata not found"));
    emit failed (templateId, qt_translate ("Template metadata not found"));
    return false;
  }

  QString localPath= templateManager_->localTemplatePath (templateId);
  if (localPath.isEmpty ()) {
    showError_ (qt_translate ("Local template file is missing"));
    emit failed (templateId, qt_translate ("Local template file is missing"));
    return false;
  }

  return loadFromLocalPath_ (templateId, localPath, meta->name);
}

bool
QTMTemplateOpener::startDownload_ (const QString& templateId) {
  if (!templateManager_) {
    showError_ (qt_translate ("Template manager not available"));
    emit failed (templateId, qt_translate ("Template manager not available"));
    return false;
  }

  cleanupProgressDialog_ ();

  progressDialog_=
      new QProgressDialog (qt_translate ("Downloading template..."),
                           qt_translate ("Cancel"), 0, 100, parent_);
  progressDialog_->setWindowModality (Qt::WindowModal);

  connect (progressDialog_, &QProgressDialog::canceled, [this, templateId] () {
    downloadCancelledByUser_= true;
    if (templateManager_) {
      templateManager_->cancelDownload (templateId);
    }
  });

  connect (templateManager_, &TemplateManager::downloadProgress, this,
           &QTMTemplateOpener::onDownloadProgress);

  progressDialog_->show ();

  QString errorMsg;
  QString localPath=
      templateManager_->downloadTemplateSync (templateId, 30000, &errorMsg);

  cleanupProgressDialog_ ();

  if (localPath.isEmpty ()) {
    if (!downloadCancelledByUser_) {
      QString msg=
          errorMsg.isEmpty () ? qt_translate ("Download failed") : errorMsg;
      showError_ (msg);
      emit failed (templateId, msg);
    }
    else {
      emit failed (templateId, QString ());
    }
    return false;
  }

  auto meta= templateManager_->templateById (templateId);
  if (!meta) {
    showError_ (qt_translate ("Template metadata not found"));
    emit failed (templateId, qt_translate ("Template metadata not found"));
    return false;
  }

  return loadFromLocalPath_ (templateId, localPath, meta->name);
}

bool
QTMTemplateOpener::loadFromLocalPath_ (const QString& templateId,
                                       const QString& localPath,
                                       const QString& templateName) {
  QString docPath= qt_copy_template_to_documents (localPath, templateName);
  if (docPath.isEmpty ()) {
    showError_ (qt_translate ("Failed to copy template to Documents"));
    emit failed (templateId,
                 qt_translate ("Failed to copy template to Documents"));
    return false;
  }

  qt_load_document_path (docPath);
  emit completed (templateId, docPath);
  return true;
}

void
QTMTemplateOpener::onDownloadProgress (const QString& templateId,
                                       qint64         bytesReceived,
                                       qint64         bytesTotal) {
  if (templateId != currentTemplateId_) return;
  if (!progressDialog_) return;

  if (bytesTotal < 0) {
    progressDialog_->setRange (0, 0);
  }
  else {
    progressDialog_->setMaximum (static_cast<int> (bytesTotal));
    progressDialog_->setValue (static_cast<int> (bytesReceived));
  }

  emit downloadProgress (templateId, bytesReceived, bytesTotal);
}

void
QTMTemplateOpener::cleanupProgressDialog_ () {
  if (progressDialog_) {
    progressDialog_->hide ();
    progressDialog_->deleteLater ();
    progressDialog_= nullptr;
  }

  if (templateManager_) {
    disconnect (templateManager_, &TemplateManager::downloadProgress, this,
                &QTMTemplateOpener::onDownloadProgress);
  }
}

void
QTMTemplateOpener::showError_ (const QString& message) {
  QtFloatingToast::showToast (parent_, message, 3000, QtFloatingToast::Error);
}

void
QTMTemplateOpener::resetState_ () {
  currentTemplateId_.clear ();
  downloadCancelledByUser_= false;
  cleanupProgressDialog_ ();
}
