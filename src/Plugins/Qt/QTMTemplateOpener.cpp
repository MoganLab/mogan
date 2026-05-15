
/******************************************************************************
 * MODULE     : QTMTemplateOpener.cpp
 * DESCRIPTION: Unified template opener implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
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
QTMTemplateOpener::isAvailableLocally (const QString& templateId) const {
  return templateManager_ &&
         templateManager_->isTemplateAvailableLocally (templateId);
}

void
QTMTemplateOpener::openTemplate (const QString& templateId) {
  resetState_ ();
  currentTemplateId_= templateId;

  if (isAvailableLocally (templateId)) {
    openLocalTemplate_ (templateId);
    return;
  }

  startDownload_ (templateId);
}

void
QTMTemplateOpener::openLocalTemplate_ (const QString& templateId) {
  if (!templateManager_) {
    emit failed (templateId, qt_translate ("Template manager not available"));
    return;
  }

  auto meta= templateManager_->templateById (templateId);
  if (!meta) {
    showError_ (qt_translate ("Template metadata not found"));
    emit failed (templateId, qt_translate ("Template metadata not found"));
    return;
  }

  QString localPath= templateManager_->localTemplatePath (templateId);
  if (localPath.isEmpty ()) {
    showError_ (qt_translate ("Local template file is missing"));
    emit failed (templateId, qt_translate ("Local template file is missing"));
    return;
  }

  QString docPath= qt_copy_template_to_documents (localPath, meta->name);
  if (docPath.isEmpty ()) {
    showError_ (qt_translate ("Failed to copy template to Documents"));
    emit failed (templateId,
                 qt_translate ("Failed to copy template to Documents"));
    return;
  }

  qt_load_document_path (docPath);
  emit completed (templateId, docPath);
}

void
QTMTemplateOpener::startDownload_ (const QString& templateId) {
  if (!templateManager_) {
    emit failed (templateId, qt_translate ("Template manager not available"));
    return;
  }

  cleanupProgressDialog_ ();

  progressDialog_=
      new QProgressDialog (qt_translate ("Downloading template..."),
                           qt_translate ("Cancel"), 0, 100, parent_);
  progressDialog_->setWindowModality (Qt::WindowModal);
  progressDialog_->setAutoClose (true);

  connect (progressDialog_, &QProgressDialog::canceled, [this, templateId] () {
    downloadCancelledByUser_= true;
    if (templateManager_) {
      templateManager_->cancelDownload (templateId);
    }
  });

  connect (templateManager_, &TemplateManager::downloadProgress, this,
           &QTMTemplateOpener::onDownloadProgress);
  connect (templateManager_, &TemplateManager::downloadCompleted, this,
           &QTMTemplateOpener::onDownloadCompleted);
  connect (templateManager_, &TemplateManager::downloadFailed, this,
           &QTMTemplateOpener::onDownloadFailed);

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
    return;
  }

  auto meta= templateManager_->templateById (templateId);
  if (!meta) {
    showError_ (qt_translate ("Template metadata not found"));
    emit failed (templateId, qt_translate ("Template metadata not found"));
    return;
  }

  QString docPath= qt_copy_template_to_documents (localPath, meta->name);
  if (docPath.isEmpty ()) {
    showError_ (qt_translate ("Failed to copy template to Documents"));
    emit failed (templateId,
                 qt_translate ("Failed to copy template to Documents"));
    return;
  }

  qt_load_document_path (docPath);
  emit completed (templateId, docPath);
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
QTMTemplateOpener::onDownloadCompleted (const QString& templateId,
                                        const QString& /*localPath*/) {
  if (templateId != currentTemplateId_) return;
  // Synchronous path: completion logic is handled in startDownload_
  // after downloadTemplateSync returns.
}

void
QTMTemplateOpener::onDownloadFailed (const QString& templateId,
                                     const QString& /*error*/) {
  if (templateId != currentTemplateId_) return;
  // Synchronous path: error logic is handled in startDownload_
  // after downloadTemplateSync returns.
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
    disconnect (templateManager_, &TemplateManager::downloadCompleted, this,
                &QTMTemplateOpener::onDownloadCompleted);
    disconnect (templateManager_, &TemplateManager::downloadFailed, this,
                &QTMTemplateOpener::onDownloadFailed);
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
