
/******************************************************************************
 * MODULE     : qt_template_page.cpp
 * DESCRIPTION: Template page implementation for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_template_page.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSet>
#include <QShowEvent>
#include <QStyle>
#include <QTemporaryFile>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

#include "qt_dpi_utils.hpp"
#include "qt_pdf_preview_widget.hpp"
#include "qt_utilities.hpp"
#include "template_manager.hpp"
#include "thumbnail_cache.hpp"

namespace {
// 预览图片尺寸（增大预览区域）
constexpr int PREVIEW_IMAGE_WIDTH= 600;

// 缩略图尺寸（使用2x尺寸以便在高分屏上显示清晰）
constexpr int THUMBNAIL_WIDTH = 240;
constexpr int THUMBNAIL_HEIGHT= 135;

constexpr int kPageMargin          = 16;  // 页面边距（减小边白）
constexpr int kPageSpacing         = 24;  // 页面主布局间距
constexpr int kCategorySpacing     = 8;   // 分类按钮间距
constexpr int kGridSpacing         = 20;  // 模板网格间距
constexpr int kCardWidth           = 264; // 模板卡片宽度
constexpr int kCardHeight          = 220; // 模板卡片高度
constexpr int kCardMargin          = 12;  // 卡片内边距
constexpr int kCardSpacing         = 8;   // 卡片内部间距
constexpr int kNameLabelMaxHeight  = 40;  // 模板名称最大高度
constexpr int kPreviewDialogMinW   = 700; // 预览弹窗最小宽度
constexpr int kPreviewDialogMinH   = 800; // 预览弹窗最小高度
constexpr int kPreviewLayoutSpacing= 16;  // 预览弹窗布局间距
constexpr int kPreviewLayoutMargin = 24;  // 预览弹窗布局边距
constexpr int kPageTitleFontPx     = 24;  // 页面标题字号
constexpr int kLoadingFontPx       = 14;  // Loading 文案字号
constexpr int kTemplateNameFontPx  = 14;  // 模板名称字号
constexpr int kPreviewTitleFontPx  = 18;  // 预览标题字号
constexpr int kPreviewDescFontPx   = 14;  // 预览描述字号
constexpr int kUseButtonFontPx     = 13;  // Use Template 按钮字号
constexpr int kInfoFontPx          = 11;  // 模板信息字号
constexpr int kThumbRadiusPx       = 4;   // 缩略图圆角
constexpr int kThumbBorderWidthPx  = 1;   // 缩略图边框宽度
constexpr int kUseButtonRadiusPx   = 4;   // Use Template 按钮圆角
constexpr int kUseButtonPadYPx     = 8;   // Use Template 按钮纵向内边距
constexpr int kUseButtonPadXPx     = 24;  // Use Template 按钮横向内边距
constexpr int kGridMarginPx        = 30;  // 网格布局上下边距
constexpr int kCategoryBtnRadiusPx = 12;  // 分类按钮圆角
constexpr int kCategoryBtnPadYPx   = 6;   // 分类按钮纵向内边距
constexpr int kCategoryBtnPadXPx   = 14;  // 分类按钮横向内边距
constexpr int kCardRadiusPx        = 8;   // 模板卡片圆角

void
applyThumbnailFrameStyle (QLabel* label) {
  if (!label) return;
  label->setStyleSheet (
      QString ("QLabel#startup-tab-template-thumbnail { border-radius: %1px; }")
          .arg (DpiUtils::scaled (kThumbRadiusPx)));
}

} // namespace

QTTemplatePage::QTTemplatePage (QWidget* parent)
    : QWidget (parent), titleLabel_ (nullptr), categoryBar_ (nullptr),
      scrollArea_ (nullptr), gridWidget_ (nullptr), gridLayout_ (nullptr),
      progressDialog_ (nullptr), templateManager_ (nullptr),
      currentCategory_ (""), activeCategoryBtn_ (nullptr),
      networkManager_ (nullptr), resizeDebounceTimer_ (nullptr) {
  networkManager_= new QNetworkAccessManager (this);

  resizeDebounceTimer_= new QTimer (this);
  resizeDebounceTimer_->setSingleShot (true);
  resizeDebounceTimer_->setInterval (200);
  connect (resizeDebounceTimer_, &QTimer::timeout, this, [this] () {
    if (templateManager_ && templateManager_->isInitialized ()) {
      int newColumnCount= calculateColumnCount ();
      if (newColumnCount != currentColumnCount_) {
        refreshTemplateGrid (currentCategory_);
      }
    }
  });

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
  layout->setContentsMargins (
      DpiUtils::scaled (kPageMargin), DpiUtils::scaled (kPageMargin),
      DpiUtils::scaled (kPageMargin), DpiUtils::scaled (kPageMargin));
  layout->setSpacing (DpiUtils::scaled (kPageSpacing));

  // Title
  titleLabel_= new QLabel (qt_translate ("Template Center"), this);
  titleLabel_->setObjectName ("startup-tab-page-title");
  DpiUtils::applyScaledFont (titleLabel_, kPageTitleFontPx);
  layout->addWidget (titleLabel_);

  // Category bar
  categoryBar_               = new QWidget (this);
  QHBoxLayout* categoryLayout= new QHBoxLayout (categoryBar_);
  categoryLayout->setContentsMargins (0, 0, 0, 0);
  categoryLayout->setSpacing (DpiUtils::scaled (kCategorySpacing));
  layout->addWidget (categoryBar_);

  // Scroll area for templates
  scrollArea_= new QScrollArea (this);
  scrollArea_->setWidgetResizable (true);
  scrollArea_->setFrameShape (QFrame::NoFrame);
  scrollArea_->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

  gridWidget_= new QWidget (scrollArea_);
  gridLayout_= new QGridLayout (gridWidget_);
  gridLayout_->setSpacing (DpiUtils::scaled (kGridSpacing));
  gridLayout_->setContentsMargins (0, DpiUtils::scaled (kGridMarginPx), 0,
                                   DpiUtils::scaled (kGridMarginPx));

  scrollArea_->setWidget (gridWidget_);
  layout->addWidget (scrollArea_, 1);

  // Loading label
  QLabel* loadingLabel=
      new QLabel (qt_translate ("Loading templates..."), gridWidget_);
  loadingLabel->setObjectName ("startup-tab-loading");
  loadingLabel->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (loadingLabel, kLoadingFontPx);
  gridLayout_->addWidget (loadingLabel, 0, 0, 1, 1);
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

  // Helper: apply category button style
  auto styleCategoryBtn= [] (QPushButton* btn) {
    btn->setStyleSheet (QString ("QPushButton#startup-tab-category-btn {"
                                 "  border-radius: %1px;"
                                 "  padding: %2px %3px;"
                                 "}")
                            .arg (DpiUtils::scaled (kCategoryBtnRadiusPx))
                            .arg (DpiUtils::scaled (kCategoryBtnPadYPx))
                            .arg (DpiUtils::scaled (kCategoryBtnPadXPx)));
    btn->setCursor (Qt::PointingHandCursor);
  };

  // Add "All" button
  QPushButton* allBtn= new QPushButton (qt_translate ("All"), categoryBar_);
  allBtn->setObjectName ("startup-tab-category-btn");
  allBtn->setCheckable (true);
  allBtn->setChecked (currentCategory_.isEmpty ());
  allBtn->setProperty ("categoryId", QString ());
  styleCategoryBtn (allBtn);
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
    QPushButton* btn=
        new QPushButton (qt_translate (from_qstring (cat.name)), categoryBar_);
    btn->setObjectName ("startup-tab-category-btn");
    btn->setCheckable (true);
    btn->setChecked (cat.id == currentCategory_);
    btn->setProperty ("categoryId", cat.id);
    styleCategoryBtn (btn);
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

int
QTTemplatePage::calculateColumnCount () const {
  if (!scrollArea_) return 4;

  int availableWidth= scrollArea_->viewport ()->width ();
  int cardWidth     = DpiUtils::scaled (kCardWidth);
  int spacing       = DpiUtils::scaled (kGridSpacing);
  int cardSpace     = cardWidth + spacing;

  // Viewport not yet properly laid out (default QWidget size is small),
  // return a sensible default instead of 1 column
  if (availableWidth < cardSpace && availableWidth < cardWidth * 2) return 4;

  int columns= (availableWidth + spacing) / cardSpace;
  return qBound (1, columns, 6);
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

  // Calculate columns first so placeholder labels span the right width
  currentColumnCount_= calculateColumnCount ();

  if (!templateManager_ || !templateManager_->isInitialized ()) {
    QLabel* label= new QLabel (qt_translate ("Initializing..."), gridWidget_);
    label->setAlignment (Qt::AlignCenter);
    gridLayout_->addWidget (label, 0, 0, 1, currentColumnCount_);
    gridNeedsRefresh_= false;
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
    QLabel* label=
        new QLabel (qt_translate ("No templates available."), gridWidget_);
    label->setAlignment (Qt::AlignCenter);
    gridLayout_->addWidget (label, 0, 0, 1, currentColumnCount_);
    gridNeedsRefresh_= false;
    return;
  }

  // Add template cards
  int row= 0, col= 0;
  for (const auto& tmpl : templates) {
    QWidget* card= createTemplateCard (tmpl);
    gridLayout_->addWidget (card, row, col);

    col++;
    if (col >= currentColumnCount_) {
      col= 0;
      row++;
    }
  }

  gridLayout_->setRowStretch (row + 1, 1);

  gridNeedsRefresh_= false;
}

QWidget*
QTTemplatePage::createTemplateCard (const TemplateMetadataPtr& tmpl) {
  QFrame*      card  = new QFrame (gridWidget_);
  QVBoxLayout* layout= new QVBoxLayout (card);
  layout->setContentsMargins (
      DpiUtils::scaled (kCardMargin), DpiUtils::scaled (kCardMargin),
      DpiUtils::scaled (kCardMargin), DpiUtils::scaled (kCardMargin));
  layout->setSpacing (DpiUtils::scaled (kCardSpacing));
  card->setObjectName ("startup-tab-template-card");
  card->setFixedSize (DpiUtils::scaled (kCardWidth),
                      DpiUtils::scaled (kCardHeight));
  card->setCursor (Qt::PointingHandCursor);
  card->setProperty ("templateId", tmpl->id);
  card->setToolTip (tmpl->description);
  card->setFrameShape (QFrame::StyledPanel);
  card->setStyleSheet (QString ("QFrame#startup-tab-template-card {"
                                "  border-radius: %1px;"
                                "}")
                           .arg (DpiUtils::scaled (kCardRadiusPx)));

  // Thumbnail image
  QLabel* thumbnailLabel= new QLabel (card);
  thumbnailLabel->setObjectName ("startup-tab-template-thumbnail");
  thumbnailLabel->setFixedSize (DpiUtils::scaled (THUMBNAIL_WIDTH),
                                DpiUtils::scaled (THUMBNAIL_HEIGHT));
  thumbnailLabel->setAlignment (Qt::AlignCenter);
  thumbnailLabel->setProperty ("thumbnailLoaded", false);
  applyThumbnailFrameStyle (thumbnailLabel);
  thumbnailLabel->setText (qt_translate ("Loading..."));
  layout->addWidget (thumbnailLabel, 0, Qt::AlignHCenter);

  // Load thumbnail from URL
  if (!tmpl->thumbnailUrl.isEmpty ()) {
    loadThumbnail (thumbnailLabel, tmpl->thumbnailUrl);
  }
  else {
    thumbnailLabel->setText (qt_translate ("No Preview"));
  }

  // Template name
  QLabel* nameLabel= new QLabel (tmpl->name, card);
  nameLabel->setObjectName ("startup-tab-template-name");
  nameLabel->setAlignment (Qt::AlignCenter);
  nameLabel->setWordWrap (true);
  nameLabel->setMaximumHeight (DpiUtils::scaled (kNameLabelMaxHeight));
  DpiUtils::applyScaledFont (nameLabel, kTemplateNameFontPx);
  layout->addWidget (nameLabel);

  // Author and version
  QLabel* infoLabel=
      new QLabel (QString ("%1 · v%2").arg (tmpl->author, tmpl->version), card);
  infoLabel->setObjectName ("startup-tab-template-info");
  infoLabel->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (infoLabel, kInfoFontPx);
  layout->addWidget (infoLabel);

  layout->addStretch ();

  // Install event filter to handle clicks
  card->installEventFilter (this);

  return card;
}

void
QTTemplatePage::loadThumbnail (QLabel* label, const QString& url) {
  QSize targetSize (DpiUtils::scaled (THUMBNAIL_WIDTH),
                    DpiUtils::scaled (THUMBNAIL_HEIGHT));

  ThumbnailCache::ThumbnailCacheEntry cached=
      ThumbnailCache::instance ()->getEntry (url, targetSize);

  if (cached.isValid ()) {
    // Always display cached pixmap immediately (avoid showing "Loading...")
    QPixmap px= cached.pixmap;
    px.setDevicePixelRatio (label->devicePixelRatioF ());
    label->setPixmap (px);
    label->setProperty ("thumbnailLoaded", true);
    applyThumbnailFrameStyle (label);

    // Already validated this session: nothing more to do
    if (validatedUrls_.contains (url)) {
      qDebug () << "[TemplatePage] Use cache:" << url;
      return;
    }

    // First time this session: validate in background
    qDebug () << "[TemplatePage] Validate:" << url;
    thumbnailQueue_.enqueue ({label, url, cached.etag});
    processThumbnailQueue ();
    return;
  }

  qDebug () << "[TemplatePage] Download:" << url;
  thumbnailQueue_.enqueue ({label, url, QString ()});
  processThumbnailQueue ();
}

void
QTTemplatePage::processThumbnailQueue () {
  while (!thumbnailQueue_.isEmpty () &&
         activeThumbnailRequests_ < MAX_CONCURRENT_THUMBNAIL_REQUESTS) {
    ThumbnailRequest req= thumbnailQueue_.dequeue ();

    if (req.label.isNull ()) {
      continue;
    }

    activeThumbnailRequests_++;

    QNetworkRequest request (req.url);
    if (!req.cachedEtag.isEmpty ()) {
      request.setRawHeader ("If-None-Match", req.cachedEtag.toUtf8 ());
    }
    QNetworkReply* reply= networkManager_->get (request);

    connect (reply, &QNetworkReply::finished, this, [this, req, reply] () {
      activeThumbnailRequests_--;

      if (req.label.isNull ()) {
        reply->deleteLater ();
        validatedUrls_.insert (req.url);
        processThumbnailQueue ();
        return;
      }

      // 304 Not Modified - cached image is still valid
      int httpStatus=
          reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
      if (httpStatus == 304) {
        qDebug () << "[TemplatePage] Cache fresh:" << req.url;
        validatedUrls_.insert (req.url);
        reply->deleteLater ();
        processThumbnailQueue ();
        return;
      }

      if (reply->error () == QNetworkReply::NoError) {
        QByteArray data= reply->readAll ();
        QImage     image;
        if (image.loadFromData (data)) {
          qreal  dpr         = req.label->devicePixelRatioF ();
          int    targetWidth = DpiUtils::scaled (THUMBNAIL_WIDTH);
          int    targetHeight= DpiUtils::scaled (THUMBNAIL_HEIGHT);
          int    scaledW     = qRound (targetWidth * dpr);
          int    scaledH     = qRound (targetHeight * dpr);
          QImage scaled=
              image.scaled (scaledW, scaledH, Qt::KeepAspectRatioByExpanding,
                            Qt::SmoothTransformation);
          if (scaled.width () > scaledW || scaled.height () > scaledH) {
            int x = (scaled.width () - scaledW) / 2;
            int y = 0;
            scaled= scaled.copy (x, y, scaledW, scaledH);
          }
          QPixmap pixmap= QPixmap::fromImage (scaled);
          pixmap.setDevicePixelRatio (dpr);

          req.label->setPixmap (pixmap);
          req.label->setProperty ("thumbnailLoaded", true);
          applyThumbnailFrameStyle (req.label);
          req.label->style ()->unpolish (req.label);
          req.label->style ()->polish (req.label);

          // Extract HTTP cache headers and save to cache
          QString   etag= QString::fromUtf8 (reply->rawHeader ("ETag"));
          QDateTime lastModified;
          QString lmStr= QString::fromUtf8 (reply->rawHeader ("Last-Modified"));
          if (!lmStr.isEmpty ()) {
            lastModified= QDateTime::fromString (lmStr, Qt::RFC2822Date);
            if (!lastModified.isValid ()) {
              lastModified= QLocale::c ().toDateTime (
                  lmStr, "ddd, dd MMM yyyy hh:mm:ss 'GMT'");
            }
            if (lastModified.isValid ()) {
              lastModified.setTimeZone (QTimeZone::utc ());
            }
          }

          QSize targetSize (targetWidth, targetHeight);
          ThumbnailCache::instance ()->put (req.url, targetSize, pixmap, etag,
                                            lastModified);
          qDebug () << "[TemplatePage] Update cache:" << req.url;
        }
        else {
          req.label->setText (qt_translate ("Preview"));
        }
      }
      else {
        // Only show placeholder if there was no cached pixmap to preserve
        if (req.label->pixmap ().isNull ()) {
          req.label->setText (qt_translate ("Preview"));
        }
      }

      validatedUrls_.insert (req.url);
      reply->deleteLater ();
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
  dialog->setWindowTitle (
      qt_translate ("Template Preview - %1").arg (tmpl->name));
  dialog->setMinimumSize (DpiUtils::scaled (kPreviewDialogMinW),
                          DpiUtils::scaled (kPreviewDialogMinH));
  dialog->resize (DpiUtils::scaled (kPreviewDialogMinW),
                  DpiUtils::scaled (kPreviewDialogMinH));

  QVBoxLayout* layout= new QVBoxLayout (dialog);
  layout->setSpacing (DpiUtils::scaled (kPreviewLayoutSpacing));
  layout->setContentsMargins (DpiUtils::scaled (kPreviewLayoutMargin),
                              DpiUtils::scaled (kPreviewLayoutMargin),
                              DpiUtils::scaled (kPreviewLayoutMargin),
                              DpiUtils::scaled (kPreviewLayoutMargin));

  // Title
  QLabel* titleLabel= new QLabel (tmpl->name, dialog);
  titleLabel->setObjectName ("template-preview-title");
  QFont titleFont=
      DpiUtils::scaledFont (titleLabel->font (), kPreviewTitleFontPx);
  titleFont.setBold (true);
  titleLabel->setFont (titleFont);
  layout->addWidget (titleLabel);

  // Description
  QLabel* descLabel= new QLabel (tmpl->description, dialog);
  descLabel->setObjectName ("template-preview-desc");
  descLabel->setWordWrap (true);
  DpiUtils::applyScaledFont (descLabel, kPreviewDescFontPx);
  layout->addWidget (descLabel);

  // Info row
  QHBoxLayout* infoLayout= new QHBoxLayout ();
  infoLayout->addWidget (
      new QLabel (qt_translate ("Author: %1").arg (tmpl->author)));
  infoLayout->addWidget (
      new QLabel (qt_translate ("Version: %1").arg (tmpl->version)));
  infoLayout->addStretch ();
  layout->addLayout (infoLayout);

  // Preview area using reusable PDF preview widget
  QTPdfPreviewWidget* previewWidget= new QTPdfPreviewWidget (dialog);
  // 设置固定尺寸，确保无内容时也有足够显示区域
  previewWidget->setFixedSize (DpiUtils::scaled (PREVIEW_IMAGE_WIDTH),
                               DpiUtils::scaled (PREVIEW_IMAGE_WIDTH));

  // Load PDF preview
  if (!tmpl->previewUrl.isEmpty ()) {
    previewWidget->loadFromUrl (tmpl->previewUrl);
  }
  else {
    previewWidget->clearPreview (qt_translate ("No Preview"));
  }
  layout->addWidget (previewWidget, 0, Qt::AlignCenter);

  // Buttons
  QHBoxLayout* btnLayout= new QHBoxLayout ();
  btnLayout->addStretch ();

  QPushButton* cancelBtn= new QPushButton (qt_translate ("Cancel"), dialog);
  cancelBtn->setObjectName ("template-cancel-btn");
  DpiUtils::applyScaledFont (cancelBtn, kUseButtonFontPx);
  cancelBtn->setCursor (Qt::PointingHandCursor);
  cancelBtn->setStyleSheet (QString ("QPushButton#template-cancel-btn {"
                                     "  padding: %1px %2px;"
                                     "  border-radius: %3px;"
                                     "}")
                                .arg (DpiUtils::scaled (kUseButtonPadYPx))
                                .arg (DpiUtils::scaled (kUseButtonPadXPx))
                                .arg (DpiUtils::scaled (kUseButtonRadiusPx)));
  connect (cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
  btnLayout->addWidget (cancelBtn);

  QPushButton* useBtn= new QPushButton (qt_translate ("Use Template"), dialog);
  useBtn->setObjectName ("template-use-btn");
  DpiUtils::applyScaledFont (useBtn, kUseButtonFontPx);
  useBtn->setCursor (Qt::PointingHandCursor);
  useBtn->setStyleSheet (QString ("QPushButton#template-use-btn {"
                                  "  padding: %1px %2px;"
                                  "  border-radius: %3px;"
                                  "}")
                             .arg (DpiUtils::scaled (kUseButtonPadYPx))
                             .arg (DpiUtils::scaled (kUseButtonPadXPx))
                             .arg (DpiUtils::scaled (kUseButtonRadiusPx)));
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
      QMessageBox::warning (this, qt_translate ("Template Error"),
                            qt_translate ("Local template file is missing"));
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

    progressDialog_=
        new QProgressDialog (qt_translate ("Downloading template..."),
                             qt_translate ("Cancel"), 0, 100, this);
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
  gridNeedsRefresh_= true;
  refreshTemplateGrid (currentCategory_);
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
    QMessageBox::warning (
        this, qt_translate ("Download Failed"),
        qt_translate ("Failed to download template: %1").arg (error));
  }
  // Reset the flag for next download
  downloadCancelledByUser_= false;
}

void
QTTemplatePage::showEvent (QShowEvent* event) {
  QWidget::showEvent (event);

  // Refresh grid when page becomes visible (avoid duplicate if already
  // refreshed by onTemplatesLoaded)
  if (gridNeedsRefresh_ && templateManager_ &&
      templateManager_->isInitialized () &&
      !templateManager_->templates ().isEmpty ()) {
    refreshTemplateGrid (currentCategory_);
  }
}

void
QTTemplatePage::resizeEvent (QResizeEvent* event) {
  QWidget::resizeEvent (event);

  // Debounce resize to avoid frequent grid rebuilds during window dragging
  if (resizeDebounceTimer_) {
    resizeDebounceTimer_->start ();
  }
}
