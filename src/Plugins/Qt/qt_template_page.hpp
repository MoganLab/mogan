
/******************************************************************************
 * MODULE     : qt_template_page.hpp
 * DESCRIPTION: Template page widget for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QT_TEMPLATE_PAGE_HPP
#define QT_TEMPLATE_PAGE_HPP

#include <QSharedPointer>
#include <QWidget>

class QGridLayout;
class QLabel;
class QNetworkAccessManager;
class QProgressDialog;
class QPushButton;
class QScrollArea;
class TemplateManager;
struct TemplateMetadata;
using TemplateMetadataPtr= QSharedPointer<TemplateMetadata>;

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
  void onDownloadProgress (const QString& templateId, qint64 bytesReceived,
                           qint64 bytesTotal);
  void onDownloadCompleted (const QString& templateId,
                            const QString& localPath);
  void onDownloadFailed (const QString& templateId, const QString& error);

private:
  void     setupUI ();
  QWidget* createTemplateCard (const TemplateMetadataPtr& tmpl);
  void     refreshTemplateGrid (const QString& category);
  void     showTemplatePreview (const QString& templateId);
  void     downloadAndUseTemplate (const QString& templateId);
  void     loadThumbnail (QLabel* label, const QString& url);

  // UI components
  QLabel*          titleLabel_;
  QWidget*         categoryBar_;
  QScrollArea*     scrollArea_;
  QWidget*         gridWidget_;
  QGridLayout*     gridLayout_;
  QProgressDialog* progressDialog_;

  // Data
  TemplateManager* templateManager_;
  QString          currentCategory_;
  QPushButton*     activeCategoryBtn_;

  // Network
  QNetworkAccessManager* networkManager_;
};

#endif // QT_TEMPLATE_PAGE_HPP
