
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
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QStyle>
#include <QTemporaryFile>
#include <QTimer>
#include <QVBoxLayout>

#include "template_manager.hpp"

// MuPDF for PDF preview
#include "MuPDF/mupdf_picture.hpp"
#include <mupdf/fitz.h>

QTTemplatePage::QTTemplatePage (QWidget* parent)
    : QWidget (parent), titleLabel_ (nullptr), categoryBar_ (nullptr),
      scrollArea_ (nullptr), gridWidget_ (nullptr), gridLayout_ (nullptr),
      progressDialog_ (nullptr), templateManager_ (nullptr),
      currentCategory_ ("university-thesis"), activeCategoryBtn_ (nullptr),
      networkManager_ (nullptr) {
  networkManager_= new QNetworkAccessManager (this);
  setupUI ();
}

QTTemplatePage::~QTTemplatePage () {}

void
QTTemplatePage::initialize () {
  templateManager_= TemplateManager::instance ();

  // Only connect signals once
  static bool signalsConnected= false;
  if (!signalsConnected) {
    connect (templateManager_, &TemplateManager::templatesLoaded, this,
             &QTTemplatePage::onTemplatesLoaded);
    connect (templateManager_, &TemplateManager::downloadProgress, this,
             &QTTemplatePage::onDownloadProgress);
    connect (templateManager_, &TemplateManager::downloadCompleted, this,
             &QTTemplatePage::onDownloadCompleted);
    connect (templateManager_, &TemplateManager::downloadFailed, this,
             &QTTemplatePage::onDownloadFailed);
    signalsConnected= true;
  }

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
QTTemplatePage::refreshTemplateGrid (const QString& category) {
  Q_UNUSED (category);

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

  // Show all templates from all categories
  QList<TemplateMetadataPtr> templates= templateManager_->templates ();

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
  QNetworkRequest request (url);
  QNetworkReply*  reply= networkManager_->get (request);

  connect (reply, &QNetworkReply::finished, [label, reply] () {
    if (reply->error () == QNetworkReply::NoError) {
      QByteArray data= reply->readAll ();
      QImage     image;
      if (image.loadFromData (data)) {
        image= image.scaled (196, 110, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
        label->setPixmap (QPixmap::fromImage (image));
        label->setStyleSheet ("border-radius: 4px;");
      }
      else {
        label->setText (tr ("Preview"));
      }
    }
    else {
      label->setText (tr ("Preview"));
    }
    reply->deleteLater ();
  });
}

void
QTTemplatePage::loadPdfPreview (QLabel* label, const QString& url) {
  // Download PDF and render first page using MuPDF
  QNetworkRequest request (url);
  QNetworkReply*  reply= networkManager_->get (request);

  connect (reply, &QNetworkReply::finished, [label, reply] () {
    if (reply->error () == QNetworkReply::NoError) {
      QByteArray pdfData= reply->readAll ();

      // Create temporary file for MuPDF
      QTemporaryFile tempFile;
      if (tempFile.open ()) {
        tempFile.write (pdfData);
        tempFile.flush ();

        // Use MuPDF to render first page
        fz_context* ctx= fz_new_context (NULL, NULL, FZ_STORE_DEFAULT);
        if (ctx) {
          fz_document* doc= NULL;
          fz_pixmap*   pix= NULL;
          fz_try (ctx) {
            doc= fz_open_document (ctx,
                                   tempFile.fileName ().toUtf8 ().constData ());
            if (doc) {
              // Render first page at 150 DPI
              float     dpi = 150;
              fz_matrix ctms= fz_scale (dpi / 72.0f, dpi / 72.0f);
              fz_page*  page= fz_load_page (ctx, doc, 0);
              fz_rect   bbox= fz_bound_page (ctx, page);
              fz_matrix ctm = fz_pre_scale (fz_translate (0, -bbox.y1),
                                            dpi / 72.0f, dpi / 72.0f);
              pix= fz_new_pixmap_from_page (ctx, page, ctm, fz_device_rgb (ctx),
                                            0);
              fz_drop_page (ctx, page);

              if (pix) {
                // Convert fz_pixmap to QImage
                int    w= fz_pixmap_width (ctx, pix);
                int    h= fz_pixmap_height (ctx, pix);
                QImage image (w, h, QImage::Format_RGB888);

                unsigned char* samples= fz_pixmap_samples (ctx, pix);
                for (int y= 0; y < h; y++) {
                  memcpy (image.scanLine (y),
                          samples + y * fz_pixmap_stride (ctx, pix), w * 3);
                }

                QPixmap pixmap= QPixmap::fromImage (image);
                pixmap        = pixmap.scaled (550, 300, Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
                label->setPixmap (pixmap);

                fz_drop_pixmap (ctx, pix);
              }
            }
          }
          fz_catch (ctx) {
            label->setText (QTTemplatePage::tr ("PDF Preview"));
          }

          if (doc) fz_drop_document (ctx, doc);
          fz_drop_context (ctx);
        }
        else {
          label->setText (QTTemplatePage::tr ("PDF Preview"));
        }
      }
      else {
        label->setText (QTTemplatePage::tr ("PDF Preview"));
      }
    }
    else {
      label->setText (QTTemplatePage::tr ("PDF Preview"));
    }
    reply->deleteLater ();
  });
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

  // Preview area (large thumbnail or placeholder)
  QLabel* previewLabel= new QLabel (dialog);
  previewLabel->setFixedSize (550, 300);
  previewLabel->setAlignment (Qt::AlignCenter);
  previewLabel->setStyleSheet (
      "background: #f5f5f5; border: 1px solid #ddd; border-radius: 8px;");

  // Load preview (PDF or image)
  if (!tmpl->previewUrl.isEmpty ()) {
    if (tmpl->previewUrl.endsWith (".pdf")) {
      // Load PDF preview using MuPDF
      loadPdfPreview (previewLabel, tmpl->previewUrl);
    }
    else {
      // Load image preview
      QNetworkRequest request (tmpl->previewUrl);
      QNetworkReply*  reply= networkManager_->get (request);
      connect (reply, &QNetworkReply::finished, [previewLabel, reply] () {
        if (reply->error () == QNetworkReply::NoError) {
          QByteArray data= reply->readAll ();
          QPixmap    pixmap;
          if (pixmap.loadFromData (data)) {
            pixmap= pixmap.scaled (550, 300, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
            previewLabel->setPixmap (pixmap);
          }
        }
        reply->deleteLater ();
      });
    }
  }
  else {
    previewLabel->setText (tr ("No Preview Available"));
  }
  layout->addWidget (previewLabel, 0, Qt::AlignCenter);

  // Buttons
  QHBoxLayout* btnLayout= new QHBoxLayout ();
  btnLayout->addStretch ();

  QPushButton* cancelBtn= new QPushButton (tr ("Cancel"), dialog);
  connect (cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
  btnLayout->addWidget (cancelBtn);

  QPushButton* useBtn= new QPushButton (tr ("Use Template"), dialog);
  useBtn->setObjectName ("template-use-btn");
  useBtn->setDefault (true);
  useBtn->setStyleSheet (
      "QPushButton { background: #4CAF50; color: white; padding: 8px 24px; "
      "border-radius: 4px; font-weight: bold; }"
      "QPushButton:hover { background: #45a049; }");
  connect (useBtn, &QPushButton::clicked, [this, dialog, templateId] () {
    dialog->accept ();
    downloadAndUseTemplate (templateId);
  });
  btnLayout->addWidget (useBtn);

  layout->addLayout (btnLayout);

  dialog->exec ();
  delete dialog;
}

void
QTTemplatePage::downloadAndUseTemplate (const QString& templateId) {
  if (!templateManager_) return;

  if (templateManager_->isTemplateAvailableLocally (templateId)) {
    QString localPath= templateManager_->localTemplatePath (templateId);
    emit    templateOpened (localPath);
  }
  else {
    // Close existing progress dialog if any
    if (progressDialog_) {
      progressDialog_->close ();
      progressDialog_->deleteLater ();
    }

    progressDialog_= new QProgressDialog (tr ("Downloading template..."),
                                          tr ("Cancel"), 0, 100, this);
    progressDialog_->setWindowModality (Qt::WindowModal);
    progressDialog_->setAutoClose (true);
    progressDialog_->show ();

    templateManager_->downloadTemplate (templateId);
  }
}

void
QTTemplatePage::onTemplatesLoaded () {
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
    progressDialog_->setMaximum (static_cast<int> (bytesTotal));
    progressDialog_->setValue (static_cast<int> (bytesReceived));
  }
}

void
QTTemplatePage::onDownloadCompleted (const QString& templateId,
                                     const QString& localPath) {
  if (progressDialog_) {
    progressDialog_->close ();
    progressDialog_->deleteLater ();
    progressDialog_= nullptr;
  }

  emit templateOpened (localPath);
}

void
QTTemplatePage::onDownloadFailed (const QString& templateId,
                                  const QString& error) {
  if (progressDialog_) {
    progressDialog_->close ();
    progressDialog_->deleteLater ();
    progressDialog_= nullptr;
  }

  QMessageBox::warning (this, tr ("Download Failed"),
                        tr ("Failed to download template: %1").arg (error));
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
