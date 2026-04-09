
/******************************************************************************
 * MODULE     : qt_template_page.cpp
 * DESCRIPTION: Template page implementation for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_template_page.hpp"

#include <QBuffer>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QStyle>
#include <QTemporaryFile>
#include <QTimer>
#include <QVBoxLayout>

#include "qt_pdf_preview_widget.hpp"
#include "template_manager.hpp"

namespace {
// 预览图片尺寸
constexpr int PREVIEW_IMAGE_WIDTH = 550;
constexpr int PREVIEW_IMAGE_HEIGHT= 300;

// 缩略图尺寸
constexpr int THUMBNAIL_WIDTH = 196;
constexpr int THUMBNAIL_HEIGHT= 110;

} // namespace

QTTemplatePage::QTTemplatePage (QWidget* parent)
    : QWidget (parent), titleLabel_ (nullptr), categoryBar_ (nullptr),
      scrollArea_ (nullptr), gridWidget_ (nullptr), gridLayout_ (nullptr),
      progressDialog_ (nullptr), templateManager_ (nullptr),
      currentCategory_ (""), activeCategoryBtn_ (nullptr),
      networkManager_ (nullptr) {
  networkManager_= new QNetworkAccessManager (this);
  setupUI ();
}

QTTemplatePage::~QTTemplatePage () {}

void
QTTemplatePage::initialize () {
  templateManager_= TemplateManager::instance ();

  // Connect signals (safe to call multiple times due to Qt's auto-connection)
  connect (templateManager_, &TemplateManager::templatesLoaded, this,
           &QTTemplatePage::onTemplatesLoaded, Qt::UniqueConnection);
  connect (templateManager_, &TemplateManager::categoriesLoaded, this,
           &QTTemplatePage::onCategoriesLoaded, Qt::UniqueConnection);
  connect (templateManager_, &TemplateManager::downloadProgress, this,
           &QTTemplatePage::onDownloadProgress, Qt::UniqueConnection);
  connect (templateManager_, &TemplateManager::downloadCompleted, this,
           &QTTemplatePage::onDownloadCompleted, Qt::UniqueConnection);
  connect (templateManager_, &TemplateManager::downloadFailed, this,
           &QTTemplatePage::onDownloadFailed, Qt::UniqueConnection);

  // Check if already initialized with data
  if (templateManager_->isInitialized () &&
      !templateManager_->templates ().isEmpty ()) {
    // Already have data, refresh immediately
    onTemplatesLoaded ();
  }
  else if (!templateManager_->isInitialized ()) {
    // Initialize asynchronously
    QTimer::singleShot (0, this,
                        [this] () { templateManager_->initialize (); });
  }
}

void
QTTemplatePage::setupUI () {
  QVBoxLayout* layout= new QVBoxLayout (this);
  layout->setContentsMargins (32, 32, 32, 32);
  layout->setSpacing (24);

  // Title
  titleLabel_= new QLabel (tr ("Template Center"), this);
  titleLabel_->setObjectName ("startup-tab-page-title");
  layout->addWidget (titleLabel_);

  // Category bar
  categoryBar_               = new QWidget (this);
  QHBoxLayout* categoryLayout= new QHBoxLayout (categoryBar_);
  categoryLayout->setContentsMargins (0, 0, 0, 0);
  categoryLayout->setSpacing (8);
  layout->addWidget (categoryBar_);

  // Scroll area for templates
  scrollArea_= new QScrollArea (this);
  scrollArea_->setWidgetResizable (true);
  scrollArea_->setFrameShape (QFrame::NoFrame);
  scrollArea_->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

  gridWidget_= new QWidget (scrollArea_);
  gridLayout_= new QGridLayout (gridWidget_);
  gridLayout_->setSpacing (20);
  gridLayout_->setContentsMargins (0, 0, 0, 0);

  scrollArea_->setWidget (gridWidget_);
  layout->addWidget (scrollArea_, 1);

  // Loading label
  QLabel* loadingLabel= new QLabel (tr ("Loading templates..."), gridWidget_);
  loadingLabel->setObjectName ("startup-tab-loading");
  loadingLabel->setAlignment (Qt::AlignCenter);
  gridLayout_->addWidget (loadingLabel, 0, 0, 1, 3);
}

void
QTTemplatePage::setupCategoryBar () {
  if (!categoryBar_) return;
  activeCategoryBtn_= nullptr;

  // Clear existing buttons
  QLayout* layout= categoryBar_->layout ();
  if (layout) {
    QLayoutItem* item;
    while ((item= layout->takeAt (0)) != nullptr) {
      if (item->widget ()) {
        delete item->widget ();
      }
      delete item;
    }
  }

  if (!templateManager_) return;

  QHBoxLayout* categoryLayout= qobject_cast<QHBoxLayout*> (layout);
  if (!categoryLayout) return;

  // Add "All" button
  QPushButton* allBtn= new QPushButton (tr ("All"), categoryBar_);
  allBtn->setObjectName ("startup-tab-category-btn");
  allBtn->setCheckable (true);
  allBtn->setChecked (currentCategory_.isEmpty ());
  allBtn->setProperty ("categoryId", QString ());
  connect (allBtn, &QPushButton::clicked, this,
           &QTTemplatePage::onCategoryClicked);
  categoryLayout->addWidget (allBtn);

  if (currentCategory_.isEmpty ()) {
    activeCategoryBtn_= allBtn;
  }

  // Add category buttons
  QList<TemplateCategory> categories= templateManager_->categories ();
  bool hasMatchedCurrentCategory    = currentCategory_.isEmpty ();
  for (const auto& cat : categories) {
    QPushButton* btn= new QPushButton (cat.name, categoryBar_);
    btn->setObjectName ("startup-tab-category-btn");
    btn->setCheckable (true);
    btn->setChecked (cat.id == currentCategory_);
    btn->setProperty ("categoryId", cat.id);
    connect (btn, &QPushButton::clicked, this,
             &QTTemplatePage::onCategoryClicked);
    categoryLayout->addWidget (btn);

    if (cat.id == currentCategory_) {
      activeCategoryBtn_       = btn;
      hasMatchedCurrentCategory= true;
    }
  }

  if (!hasMatchedCurrentCategory) {
    currentCategory_.clear ();
    allBtn->setChecked (true);
    activeCategoryBtn_= allBtn;
  }

  categoryLayout->addStretch ();
}

void
QTTemplatePage::onCategoriesLoaded () {
  setupCategoryBar ();
}

void
QTTemplatePage::onCategoryClicked () {
  QPushButton* btn= qobject_cast<QPushButton*> (sender ());
  if (!btn) return;

  // Uncheck previous button
  if (activeCategoryBtn_ && activeCategoryBtn_ != btn) {
    activeCategoryBtn_->setChecked (false);
  }

  // Check current button
  btn->setChecked (true);
  activeCategoryBtn_= btn;

  // Update current category and refresh
  currentCategory_= btn->property ("categoryId").toString ();
  refreshTemplateGrid (currentCategory_);
}

void
QTTemplatePage::refreshTemplateGrid (const QString& category) {
  // Clear existing content
  QLayoutItem* item;
  while ((item= gridLayout_->takeAt (0)) != nullptr) {
    if (item->widget ()) {
      delete item->widget ();
    }
    delete item;
  }

  if (!templateManager_ || !templateManager_->isInitialized ()) {
    QLabel* label= new QLabel (tr ("Initializing..."), gridWidget_);
    label->setAlignment (Qt::AlignCenter);
    gridLayout_->addWidget (label, 0, 0, 1, 3);
    return;
  }

  // Get templates by category or all templates
  QList<TemplateMetadataPtr> templates;
  if (category.isEmpty ()) {
    templates= templateManager_->templates ();
  }
  else {
    templates= templateManager_->templatesByCategory (category);
  }

  if (templates.isEmpty ()) {
    QLabel* label= new QLabel (tr ("No templates available."), gridWidget_);
    label->setAlignment (Qt::AlignCenter);
    gridLayout_->addWidget (label, 0, 0, 1, 3);
    return;
  }

  // Add template cards
  int row= 0, col= 0;
  for (const auto& tmpl : templates) {
    QWidget* card= createTemplateCard (tmpl);
    gridLayout_->addWidget (card, row, col);

    col++;
    if (col >= 3) {
      col= 0;
      row++;
    }
  }

  gridLayout_->setRowStretch (row + 1, 1);
}

QWidget*
QTTemplatePage::createTemplateCard (const TemplateMetadataPtr& tmpl) {
  QWidget*     card  = new QWidget (gridWidget_);
  QVBoxLayout* layout= new QVBoxLayout (card);
  layout->setContentsMargins (12, 12, 12, 12);
  layout->setSpacing (8);
  card->setObjectName ("startup-tab-template-card");
  card->setFixedSize (220, 200);
  card->setCursor (Qt::PointingHandCursor);
  card->setProperty ("templateId", tmpl->id);
  card->setToolTip (tmpl->description);

  // Thumbnail image
  QLabel* thumbnailLabel= new QLabel (card);
  thumbnailLabel->setObjectName ("startup-tab-template-thumbnail");
  thumbnailLabel->setFixedSize (196, 110);
  thumbnailLabel->setAlignment (Qt::AlignCenter);
  thumbnailLabel->setStyleSheet (
      "background: #f5f5f5; border-radius: 4px; border: 1px solid #ddd;");
  thumbnailLabel->setText (tr ("Loading..."));
  layout->addWidget (thumbnailLabel, 0, Qt::AlignHCenter);

  // Load thumbnail from URL
  if (!tmpl->thumbnailUrl.isEmpty ()) {
    loadThumbnail (thumbnailLabel, tmpl->thumbnailUrl);
  }
  else {
    thumbnailLabel->setText (tr ("No Preview"));
  }

  // Template name
  QLabel* nameLabel= new QLabel (tmpl->name, card);
  nameLabel->setObjectName ("startup-tab-template-name");
  nameLabel->setAlignment (Qt::AlignCenter);
  nameLabel->setWordWrap (true);
  nameLabel->setMaximumHeight (40);
  layout->addWidget (nameLabel);

  // Author and version
  QLabel* infoLabel=
      new QLabel (QString ("%1 · v%2").arg (tmpl->author, tmpl->version), card);
  infoLabel->setObjectName ("startup-tab-template-info");
  infoLabel->setAlignment (Qt::AlignCenter);
  infoLabel->setStyleSheet ("color: #888; font-size: 11px;");
  layout->addWidget (infoLabel);

  layout->addStretch ();

  // Install event filter to handle clicks
  card->installEventFilter (this);

  return card;
}

void
QTTemplatePage::loadThumbnail (QLabel* label, const QString& url) {
  // Add to queue and process
  thumbnailQueue_.enqueue ({label, url});
  processThumbnailQueue ();
}

void
QTTemplatePage::processThumbnailQueue () {
  // Process queued requests up to the concurrency limit
  while (!thumbnailQueue_.isEmpty () &&
         activeThumbnailRequests_ < MAX_CONCURRENT_THUMBNAIL_REQUESTS) {
    ThumbnailRequest req= thumbnailQueue_.dequeue ();

    // Check if the label is still valid (not deleted)
    // QPointer automatically becomes nullptr when QLabel is deleted
    if (req.label.isNull ()) {
      continue; // Skip invalid labels
    }

    activeThumbnailRequests_++;

    QNetworkRequest request (req.url);
    QNetworkReply*  reply= networkManager_->get (request);

    connect (reply, &QNetworkReply::finished, this, [this, req, reply] () {
      activeThumbnailRequests_--;

      // Check if label is still valid before updating
      // QPointer automatically becomes nullptr when QLabel is deleted
      if (!req.label.isNull ()) {
        if (reply->error () == QNetworkReply::NoError) {
          QByteArray data= reply->readAll ();
          QImage     image;
          if (image.loadFromData (data)) {
            image= image.scaled (THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT,
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
            req.label->setPixmap (QPixmap::fromImage (image));
            req.label->setStyleSheet ("border-radius: 4px;");
          }
          else {
            req.label->setText (tr ("Preview"));
          }
        }
        else {
          req.label->setText (tr ("Preview"));
        }
      }

      reply->deleteLater ();

      // Process next items in queue
      processThumbnailQueue ();
    });
  }
}

bool
QTTemplatePage::eventFilter (QObject* watched, QEvent* event) {
  if (event->type () == QEvent::MouseButtonRelease) {
    QWidget* card= qobject_cast<QWidget*> (watched);
    if (card && card->parent () == gridWidget_) {
      QString templateId= card->property ("templateId").toString ();
      if (!templateId.isEmpty ()) {
        showTemplatePreview (templateId);
        return true;
      }
    }
  }
  return QWidget::eventFilter (watched, event);
}

void
QTTemplatePage::showTemplatePreview (const QString& templateId) {
  if (!templateManager_) return;

  TemplateMetadataPtr tmpl= templateManager_->templateById (templateId);
  if (!tmpl) return;

  // Create preview dialog
  QDialog* dialog= new QDialog (this);
  dialog->setWindowTitle (tr ("Template Preview - %1").arg (tmpl->name));
  dialog->setMinimumSize (600, 500);

  QVBoxLayout* layout= new QVBoxLayout (dialog);
  layout->setSpacing (16);
  layout->setContentsMargins (24, 24, 24, 24);

  // Title
  QLabel* titleLabel= new QLabel (tmpl->name, dialog);
  titleLabel->setObjectName ("template-preview-title");
  titleLabel->setStyleSheet ("font-size: 18px; font-weight: bold;");
  layout->addWidget (titleLabel);

  // Description
  QLabel* descLabel= new QLabel (tmpl->description, dialog);
  descLabel->setObjectName ("template-preview-desc");
  descLabel->setWordWrap (true);
  descLabel->setStyleSheet ("color: #666;");
  layout->addWidget (descLabel);

  // Info row
  QHBoxLayout* infoLayout= new QHBoxLayout ();
  infoLayout->addWidget (new QLabel (tr ("Author: %1").arg (tmpl->author)));
  infoLayout->addWidget (new QLabel (tr ("Version: %1").arg (tmpl->version)));
  infoLayout->addStretch ();
  layout->addLayout (infoLayout);

  // Preview area using reusable PDF preview widget
  QTPdfPreviewWidget* previewWidget= new QTPdfPreviewWidget (dialog);

  // Load preview (PDF or image)
  if (!tmpl->previewUrl.isEmpty ()) {
    if (tmpl->previewUrl.endsWith (".pdf")) {
      // 使用QTPdfPreviewWidget加载PDF预览
      previewWidget->loadFromUrl (tmpl->previewUrl);
    }
    else {
      // 使用QTPdfPreviewWidget加载图片预览
      previewWidget->loadImageFromUrl (
          tmpl->previewUrl, QSize (PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT));
    }
  }
  layout->addWidget (previewWidget, 0, Qt::AlignCenter);

  // Buttons
  QHBoxLayout* btnLayout= new QHBoxLayout ();
  btnLayout->addStretch ();

  QPushButton* cancelBtn= new QPushButton (tr ("Cancel"), dialog);
  connect (cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
  btnLayout->addWidget (cancelBtn);

  QPushButton* useBtn= new QPushButton (tr ("Use Template"), dialog);
  useBtn->setObjectName ("template-use-btn");
  useBtn->setDefault (true);
  connect (useBtn, &QPushButton::clicked, [this, dialog, templateId] () {
    dialog->accept ();
    downloadAndUseTemplate (templateId);
  });
  btnLayout->addWidget (useBtn);

  layout->addLayout (btnLayout);

  dialog->exec ();
}

void
QTTemplatePage::downloadAndUseTemplate (const QString& templateId) {
  if (!templateManager_) return;

  auto cleanupProgressDialog= [this] () {
    QPointer<QProgressDialog> dialog= progressDialog_;
    progressDialog_                 = nullptr;
    if (!dialog) return;
    dialog->disconnect (this);
    dialog->hide ();
    dialog->deleteLater ();
  };

  if (templateManager_->isTemplateAvailableLocally (templateId)) {
    QString localPath= templateManager_->localTemplatePath (templateId);
    if (localPath.isEmpty ()) {
      QMessageBox::warning (this, tr ("Template Error"),
                            tr ("Local template file is missing"));
      return;
    }
    emit templateOpened (localPath);
  }
  else {
    // Track this download to distinguish user cancellation from real errors
    downloadCancelledByUser_= false;
    // Close existing progress dialog if any
    if (progressDialog_) {
      cleanupProgressDialog ();
    }

    progressDialog_= new QProgressDialog (tr ("Downloading template..."),
                                          tr ("Cancel"), 0, 100, this);
    progressDialog_->setWindowModality (Qt::WindowModal);
    progressDialog_->setAutoClose (true);

    // Connect cancel button to actually cancel the download
    connect (progressDialog_, &QProgressDialog::canceled,
             [this, templateId] () {
               // Mark as user-cancelled so onDownloadFailed won't show error
               // dialog
               downloadCancelledByUser_        = true;
               QPointer<QProgressDialog> dialog= progressDialog_;
               progressDialog_                 = nullptr;
               templateManager_->cancelDownload (templateId);
               if (dialog) {
                 dialog->disconnect (this);
                 dialog->hide ();
                 dialog->deleteLater ();
               }
             });

    progressDialog_->show ();

    templateManager_->downloadTemplate (templateId);
  }
}

void
QTTemplatePage::onTemplatesLoaded () {
  // Initialize category bar if not already done
  if (categoryBar_ && categoryBar_->layout ()->count () == 0) {
    setupCategoryBar ();
  }
  refreshTemplateGrid (currentCategory_);

  // Force layout update to ensure content is visible
  if (gridWidget_) {
    gridWidget_->update ();
    gridWidget_->adjustSize ();
  }
  if (scrollArea_) {
    scrollArea_->update ();
  }
}

void
QTTemplatePage::onDownloadProgress (const QString& templateId,
                                    qint64 bytesReceived, qint64 bytesTotal) {
  if (progressDialog_) {
    // Handle case where Content-Length is not available (bytesTotal == -1)
    if (bytesTotal < 0) {
      // Switch to indeterminate mode when total size is unknown
      progressDialog_->setRange (0, 0);
    }
    else {
      progressDialog_->setMaximum (static_cast<int> (bytesTotal));
      progressDialog_->setValue (static_cast<int> (bytesReceived));
    }
  }
}

void
QTTemplatePage::onDownloadCompleted (const QString& templateId,
                                     const QString& localPath) {
  if (progressDialog_) {
    QPointer<QProgressDialog> dialog= progressDialog_;
    progressDialog_                 = nullptr;
    dialog->disconnect (this);
    dialog->hide ();
    dialog->deleteLater ();
  }

  emit templateOpened (localPath);
}

void
QTTemplatePage::onDownloadFailed (const QString& templateId,
                                  const QString& error) {
  if (progressDialog_) {
    QPointer<QProgressDialog> dialog= progressDialog_;
    progressDialog_                 = nullptr;
    dialog->disconnect (this);
    dialog->hide ();
    dialog->deleteLater ();
  }

  // Check if this download was cancelled by the user
  // If so, don't show the error dialog
  if (!downloadCancelledByUser_) {
    QMessageBox::warning (this, tr ("Download Failed"),
                          tr ("Failed to download template: %1").arg (error));
  }
  // Reset the flag for next download
  downloadCancelledByUser_= false;
}

void
QTTemplatePage::showEvent (QShowEvent* event) {
  QWidget::showEvent (event);

  // Refresh grid when page becomes visible
  if (templateManager_ && templateManager_->isInitialized () &&
      !templateManager_->templates ().isEmpty ()) {
    refreshTemplateGrid (currentCategory_);
  }
}
