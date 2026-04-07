
/******************************************************************************
 * MODULE     : qt_pdf_preview_widget.hpp
 * DESCRIPTION: PDF preview widget using MuPDF
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QT_PDF_PREVIEW_WIDGET_HPP
#define QT_PDF_PREVIEW_WIDGET_HPP

#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QSharedPointer>

/**
 * @brief PDF preview widget - reusable component for rendering PDF pages
 *
 * Features:
 * - Load PDF from URL or local file
 * - Render specific page at specified DPI
 * - Async loading with network support
 * - Error handling with fallback display
 */
class QTPdfPreviewWidget : public QLabel {
  Q_OBJECT

public:
  explicit QTPdfPreviewWidget (QWidget* parent= nullptr);
  ~QTPdfPreviewWidget ();

  // Load PDF from URL (async)
  void loadFromUrl (const QString& url, int pageNumber= 0, int dpi= 150);

  // Load PDF from local file (sync)
  bool loadFromFile (const QString& filePath, int pageNumber= 0, int dpi= 150);

  // Load PDF from QByteArray (sync)
  bool loadFromData (const QByteArray& data, int pageNumber= 0, int dpi= 150);

  // Set/get target DPI
  void setDpi (int dpi) { targetDpi_= dpi; }
  int  dpi () const { return targetDpi_; }

  // Set/get target page
  void setPageNumber (int page) { targetPage_= page; }
  int  pageNumber () const { return targetPage_; }

  // Status
  bool    isLoading () const { return isLoading_; }
  bool    hasError () const { return hasError_; }
  QString errorString () const { return errorString_; }

  // Cancel current loading
  void cancelLoading ();

  // Clear preview and show placeholder
  void clearPreview (const QString& text= QString ());

signals:
  void loadingStarted ();
  void loadingFinished (bool success);
  void error (const QString& errorMessage);

private slots:
  void onNetworkReplyFinished ();

private:
  // MuPDF rendering
  bool renderPdfPage (const QByteArray& data, int pageNumber, int dpi);

  // UI helpers
  void showLoading ();
  void showError (const QString& message);
  void setPreviewPixmap (const QPixmap& pixmap);

private:
  // Network
  QNetworkAccessManager* networkManager_;
  QNetworkReply*         currentReply_;

  // Settings
  int targetDpi_;
  int targetPage_;

  // State
  bool    isLoading_;
  bool    hasError_;
  QString errorString_;

  // Default size
  static constexpr int DEFAULT_WIDTH = 550;
  static constexpr int DEFAULT_HEIGHT= 300;
  static constexpr int DEFAULT_DPI   = 150;
};

#endif // QT_PDF_PREVIEW_WIDGET_HPP
