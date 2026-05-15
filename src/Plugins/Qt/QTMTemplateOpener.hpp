
/******************************************************************************
 * MODULE     : QTMTemplateOpener.hpp
 * DESCRIPTION: Unified template opener for HomePage and TemplatePage
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QTMTEMPLATEOPENER_HPP
#define QTMTEMPLATEOPENER_HPP

#include <QObject>
#include <QPointer>

class QProgressDialog;
class QWidget;
class TemplateManager;

/**
 * @brief Unified template opener
 *
 * Encapsulates the core logic for opening templates:
 * - Local template: copy to Documents and open immediately
 * - Remote template: download (with progress dialog) → copy → open
 *
 * Usage example (HomePage one-click open):
 * @code
 * QTMTemplateOpener opener(this);
 * opener.openTemplate("elegantbook");
 * @endcode
 *
 * Usage example (TemplatePage preview then open):
 * @code
 * QTMTemplateOpener opener(this);
 * opener.openTemplate("nsfc-ysf-c");
 * @endcode
 *
 * @note openTemplate() is synchronous-style: for local templates it completes
 * immediately; for remote templates it blocks (while keeping the event loop
 * responsive via QProgressDialog) until the download finishes. The completed()
 * / failed() signals are emitted before openTemplate() returns.
 */
class QTMTemplateOpener : public QObject {
  Q_OBJECT

public:
  explicit QTMTemplateOpener (QWidget* parent= nullptr);
  ~QTMTemplateOpener ();

  QTMTemplateOpener (const QTMTemplateOpener&)           = delete;
  QTMTemplateOpener& operator= (const QTMTemplateOpener&)= delete;

  /**
   * @brief Open a template (local or remote)
   *
   * If the template is available locally, open it directly.
   * Otherwise display a progress dialog and download it first.
   *
   * @param templateId Template ID
   */
  void openTemplate (const QString& templateId);

  /**
   * @brief Check whether a template is available locally
   */
  bool isAvailableLocally (const QString& templateId) const;

signals:
  /**
   * @brief Download progress update
   */
  void downloadProgress (const QString& templateId, qint64 bytesReceived,
                         qint64 bytesTotal);

  /**
   * @brief Template opened successfully
   * @param templateId   Template ID
   * @param documentPath Path of the document in Documents
   */
  void completed (const QString& templateId, const QString& documentPath);

  /**
   * @brief Failed to open template
   * @param templateId Template ID
   * @param error      Human-readable error message (empty if user cancelled)
   */
  void failed (const QString& templateId, const QString& error);

private slots:
  void onDownloadProgress (const QString& templateId, qint64 bytesReceived,
                           qint64 bytesTotal);
  void onDownloadCompleted (const QString& templateId,
                            const QString& localPath);
  void onDownloadFailed (const QString& templateId, const QString& error);

private:
  void openLocalTemplate_ (const QString& templateId);
  void startDownload_ (const QString& templateId);
  void cleanupProgressDialog_ ();
  void showError_ (const QString& message);
  void resetState_ ();

  QWidget*                  parent_;
  TemplateManager*          templateManager_;
  QPointer<QProgressDialog> progressDialog_;

  QString currentTemplateId_;
  bool    downloadCancelledByUser_= false;
};

#endif // QTMTEMPLATEOPENER_HPP
