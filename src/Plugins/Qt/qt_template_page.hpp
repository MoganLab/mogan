
/******************************************************************************
 * MODULE     : qt_template_page.hpp
 * DESCRIPTION: Template page widget for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QT_TEMPLATE_PAGE_HPP
#define QT_TEMPLATE_PAGE_HPP

#include <QPointer>
#include <QQueue>
#include <QSharedPointer>
#include <QWidget>

class QGridLayout;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QPushButton;
class QScrollArea;
class TemplateManager;
struct TemplateMetadata;
using TemplateMetadataPtr= QSharedPointer<TemplateMetadata>;

/**
 * @brief Structure to hold pending thumbnail load request
 * Uses QPointer to automatically handle QLabel deletion
 */
struct ThumbnailRequest {
  QPointer<QLabel> label;
  QString          url;
};

/**
 * @brief Template page widget for startup tab
 *
 * Displays template categories and grid of template cards.
 * Handles template download and opening.
 */
class QTTemplatePage : public QWidget {
  Q_OBJECT

public:
  explicit QTTemplatePage (QWidget* parent= nullptr);
  ~QTTemplatePage ();

  void initialize ();

signals:
  void templateOpened (const QString& filePath);

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;
  void showEvent (QShowEvent* event) override;

private slots:
  void onTemplatesLoaded ();
  void onCategoriesLoaded ();
  void onDownloadProgress (const QString& templateId, qint64 bytesReceived,
                           qint64 bytesTotal);
  void onDownloadCompleted (const QString& templateId,
                            const QString& localPath);
  void onDownloadFailed (const QString& templateId, const QString& error);
  void onCategoryClicked ();

private:
  void     setupUI ();
  void     setupCategoryBar ();
  QWidget* createTemplateCard (const TemplateMetadataPtr& tmpl);
  void     refreshTemplateGrid (const QString& category);
  void     showTemplatePreview (const QString& templateId);
  void     downloadAndUseTemplate (const QString& templateId);
  void     loadThumbnail (QLabel* label, const QString& url);
  void     processThumbnailQueue ();

  // UI components
  QLabel*                   titleLabel_;
  QWidget*                  categoryBar_;
  QScrollArea*              scrollArea_;
  QWidget*                  gridWidget_;
  QGridLayout*              gridLayout_;
  QPointer<QProgressDialog> progressDialog_;

  // Data
  TemplateManager* templateManager_;
  QString          currentCategory_;
  QPushButton*     activeCategoryBtn_;

  // Network
  QNetworkAccessManager* networkManager_;

  // Thumbnail loading queue for concurrency control
  QQueue<ThumbnailRequest> thumbnailQueue_;
  int                      activeThumbnailRequests_         = 0;
  static constexpr int     MAX_CONCURRENT_THUMBNAIL_REQUESTS= 6;

  // Track user-cancelled downloads to avoid showing error dialogs
  bool downloadCancelledByUser_= false;
};

#endif // QT_TEMPLATE_PAGE_HPP
