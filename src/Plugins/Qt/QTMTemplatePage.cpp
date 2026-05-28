
/******************************************************************************
 * MODULE     : QTMTemplatePage.cpp
 * DESCRIPTION: Template page implementation for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMTemplatePage.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QShowEvent>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include "QTMTemplateOpener.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_floating_toast.hpp"
#include "qt_pdf_preview_widget.hpp"
#include "qt_utilities.hpp"
#include "template_manager.hpp"
#include "thumbnail_loader.hpp"

namespace {
// 预览图片尺寸（增大预览区域）
constexpr int PREVIEW_IMAGE_WIDTH= 600;

// 缩略图尺寸（使用2x尺寸以便在高分屏上显示清晰）
constexpr int THUMBNAIL_WIDTH = 160;
constexpr int THUMBNAIL_HEIGHT= 227;

constexpr int kPageMargin          = 16;  // 页面边距（减小边白）
constexpr int kPageSpacing         = 24;  // 页面主布局间距
constexpr int kGridSpacing         = 16;  // 模板网格间距
constexpr int kCardWidth           = 176; // 模板卡片宽度
constexpr int kCardHeight          = 243; // 模板卡片高度（仅缩略图区域）
constexpr int kCardMargin          = 8;   // 卡片内边距
constexpr int kCardSpacing         = 5;   // 卡片内部间距
constexpr int kNameLabelMaxWidth   = 176; // 模板名称最大宽度
constexpr int kNameLabelMaxHeight  = 40;  // 模板名称最大高度
constexpr int kPreviewDialogMinW   = 700; // 预览弹窗最小宽度
constexpr int kPreviewDialogMinH   = 800; // 预览弹窗最小高度
constexpr int kPreviewLayoutSpacing= 16;  // 预览弹窗布局间距
constexpr int kPreviewLayoutMargin = 24;  // 预览弹窗布局边距
constexpr int kSectionTitleFontPx  = 16;  // 分区标题字号
constexpr int kLoadingFontPx       = 14;  // Loading 文案字号
constexpr int kTemplateNameFontPx  = 11;  // 模板名称字号
constexpr int kPreviewTitleFontPx  = 18;  // 预览标题字号
constexpr int kPreviewDescFontPx   = 14;  // 预览描述字号
constexpr int kUseButtonFontPx     = 13;  // Use Template 按钮字号
constexpr int kInfoFontPx          = 10;  // 模板信息字号
constexpr int kUseButtonRadiusPx   = 4;   // Use Template 按钮圆角
constexpr int kUseButtonPadYPx     = 8;   // Use Template 按钮纵向内边距
constexpr int kUseButtonPadXPx     = 24;  // Use Template 按钮横向内边距
constexpr int kGridMarginYPx       = 5;   // 网格布局上下边距
constexpr int kGridMarginXPx       = 10;  // 网格布局左右边距
constexpr int kCardRadiusPx        = 8;   // 模板卡片圆角

} // namespace

QTMTemplatePage::QTMTemplatePage (QWidget* parent)
    : QWidget (parent), titleLabel_ (nullptr), scrollArea_ (nullptr),
      gridWidget_ (nullptr), gridLayout_ (nullptr), templateManager_ (nullptr),
      resizeDebounceTimer_ (nullptr) {

  resizeDebounceTimer_= new QTimer (this);
  resizeDebounceTimer_->setSingleShot (true);
  resizeDebounceTimer_->setInterval (200);
  connect (resizeDebounceTimer_, &QTimer::timeout, this, [this] () {
    if (templateManager_ && templateManager_->isInitialized ()) {
      int newColumnCount= calculateColumnCount ();
      if (newColumnCount != currentColumnCount_) {
        refreshTemplateGrid ();
      }
    }
  });

  setupUI ();
}

QTMTemplatePage::~QTMTemplatePage () {}

void
QTMTemplatePage::initialize () {
  templateManager_= TemplateManager::instance ();

  connect (templateManager_, &TemplateManager::templatesLoaded, this,
           &QTMTemplatePage::onTemplatesLoaded, Qt::UniqueConnection);

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
QTMTemplatePage::setCategory (const QString& categoryId,
                              const QString& displayName) {
  if (currentCategory_ != categoryId) {
    currentCategory_ = categoryId;
    gridNeedsRefresh_= true;
    if (isVisible ()) {
      refreshTemplateGrid ();
    }
  }
  if (titleLabel_ && !displayName.isEmpty ()) {
    titleLabel_->setText (displayName);
  }
}

void
QTMTemplatePage::refreshGrid () {
  gridNeedsRefresh_= true;
  refreshTemplateGrid ();
}

void
QTMTemplatePage::setupUI () {
  QVBoxLayout* layout= new QVBoxLayout (this);
  layout->setContentsMargins (
      DpiUtils::scaled (kPageMargin), DpiUtils::scaled (kPageMargin),
      DpiUtils::scaled (kPageMargin), DpiUtils::scaled (kPageMargin));
  layout->setSpacing (DpiUtils::scaled (kPageSpacing));

  titleLabel_= new QLabel (qt_translate ("Template Center"), this);
  titleLabel_->setObjectName ("startup-tab-section-title");
  DpiUtils::applyScaledFont (titleLabel_, kSectionTitleFontPx);
  layout->addWidget (titleLabel_);

  // Scroll area for templates
  scrollArea_= new QScrollArea (this);
  scrollArea_->setWidgetResizable (true);
  scrollArea_->setFrameShape (QFrame::NoFrame);
  scrollArea_->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

  gridWidget_= new QWidget (scrollArea_);
  gridWidget_->setObjectName ("startup-tab-grid");
  gridLayout_= new QGridLayout (gridWidget_);
  gridLayout_->setSpacing (DpiUtils::scaled (kGridSpacing));
  gridLayout_->setContentsMargins (
      DpiUtils::scaled (kGridMarginXPx), DpiUtils::scaled (kGridMarginYPx),
      DpiUtils::scaled (kGridMarginXPx), DpiUtils::scaled (kGridMarginYPx));

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

int
QTMTemplatePage::calculateColumnCount () const {
  if (!scrollArea_) return 4;

  int availableWidth= scrollArea_->viewport ()->width ();
  int cardWidth     = DpiUtils::scaled (kCardWidth);
  int spacing       = DpiUtils::scaled (kGridSpacing);
  int cardSpace     = cardWidth + spacing;

  // Viewport not yet properly laid out (default QWidget size is small),
  // return a sensible default instead of 1 column
  if (availableWidth < cardSpace && availableWidth < cardWidth * 2) return 4;

  int columns= (availableWidth + spacing) / cardSpace;
  return qBound (1, columns, 9);
}

void
QTMTemplatePage::refreshTemplateGrid () {
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
  if (currentCategory_.isEmpty ()) {
    templates= templateManager_->templates ();
  }
  else {
    templates= templateManager_->templatesByCategory (currentCategory_);
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
QTMTemplatePage::createTemplateCard (const TemplateMetadataPtr& tmpl) {
  // 外层容器
  QWidget*     item      = new QWidget (gridWidget_);
  QVBoxLayout* itemLayout= new QVBoxLayout (item);
  itemLayout->setContentsMargins (0, 0, 0, 0);
  itemLayout->setSpacing (DpiUtils::scaled (kCardSpacing));
  item->setObjectName ("startup-tab-template-item");
  item->setToolTip (tmpl->description);

  // 缩略图卡片
  QFrame*      card  = new QFrame (item);
  QVBoxLayout* layout= new QVBoxLayout (card);
  layout->setContentsMargins (
      DpiUtils::scaled (kCardMargin), DpiUtils::scaled (kCardMargin),
      DpiUtils::scaled (kCardMargin), DpiUtils::scaled (kCardMargin));
  layout->setSpacing (0);
  card->setObjectName ("startup-tab-template-card");
  card->setFixedSize (DpiUtils::scaled (kCardWidth),
                      DpiUtils::scaled (kCardHeight));
  card->setProperty ("templateId", tmpl->id);
  card->setCursor (Qt::PointingHandCursor);
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
  thumbnailLabel->setText (qt_translate ("Loading..."));
  layout->addWidget (thumbnailLabel, 0, Qt::AlignHCenter);

  // Load thumbnail from URL
  if (!tmpl->thumbnailUrl.isEmpty ()) {
    QSize targetSize (DpiUtils::scaled (THUMBNAIL_WIDTH),
                      DpiUtils::scaled (THUMBNAIL_HEIGHT));
    ThumbnailLoader::instance ()->load (thumbnailLabel, tmpl->thumbnailUrl,
                                        targetSize);
  }
  else {
    thumbnailLabel->setText (qt_translate ("No Preview"));
  }

  itemLayout->addWidget (card, 0, Qt::AlignHCenter);

  // Template name
  QLabel* nameLabel= new QLabel (tmpl->name, item);
  nameLabel->setObjectName ("startup-tab-template-name");
  nameLabel->setAlignment (Qt::AlignCenter);
  nameLabel->setWordWrap (true);
  nameLabel->setFixedWidth (DpiUtils::scaled (kNameLabelMaxWidth));
  DpiUtils::applyScaledFont (nameLabel, kTemplateNameFontPx);
  // 手动计算换行后的实际高度，避免 QLabel sizeHint 不准确导致截断
  {
    QFontMetrics fm (nameLabel->font ());
    QRect        textRect= fm.boundingRect (
        QRect (0, 0, DpiUtils::scaled (kNameLabelMaxWidth), INT_MAX),
        Qt::AlignCenter | Qt::TextWordWrap, tmpl->name);
    int neededHeight= textRect.height () + 4;
    nameLabel->setFixedHeight (
        qMin (neededHeight, DpiUtils::scaled (kNameLabelMaxHeight)));
  }
  nameLabel->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  itemLayout->addWidget (nameLabel, 0, Qt::AlignHCenter);

  // Author and version
  QLabel* infoLabel=
      new QLabel (QString ("%1 · v%2").arg (tmpl->author, tmpl->version), item);
  infoLabel->setObjectName ("startup-tab-template-info");
  infoLabel->setAlignment (Qt::AlignCenter);
  DpiUtils::applyScaledFont (infoLabel, kInfoFontPx);
  itemLayout->addWidget (infoLabel);

  itemLayout->addStretch ();

  // Install event filter on card only
  card->installEventFilter (this);

  return item;
}

bool
QTMTemplatePage::eventFilter (QObject* watched, QEvent* event) {
  if (event->type () == QEvent::MouseButtonRelease) {
    QWidget* card= qobject_cast<QWidget*> (watched);
    if (card && card->objectName () == "startup-tab-template-card") {
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
QTMTemplatePage::showTemplatePreview (const QString& templateId) {
  if (!templateManager_) return;

  TemplateMetadataPtr tmpl= templateManager_->templateById (templateId);
  if (!tmpl) return;

  // Create preview dialog
  QDialog* dialog= new QDialog (this);
  dialog->setWindowTitle (
      qt_translate ("Template Preview - %1").arg (tmpl->name));

  // 根据屏幕可用区域限制对话框尺寸，防止高分屏下溢出
  QScreen* screen= this->screen ();
  if (!screen) screen= QGuiApplication::primaryScreen ();
  QRect availGeo= screen ? screen->availableGeometry () : QRect ();
  int   maxDlgH = availGeo.height () > 0 ? qRound (availGeo.height () * 0.9)
                                         : DpiUtils::scaled (kPreviewDialogMinH);

  // 预览区尺寸由对话框高度上限决定（1:1 正方形）
  int basePreviewSize= DpiUtils::scaled (PREVIEW_IMAGE_WIDTH);
  int maxPreviewSize = qRound (maxDlgH * 0.7);
  int previewSize    = qMin (basePreviewSize, maxPreviewSize);

  // 对话框最大宽度收紧：仅比预览框宽一点（边距 + 少量余量）
  int marginW = DpiUtils::scaled (kPreviewLayoutMargin) * 2;
  int spacingW= DpiUtils::scaled (kPreviewLayoutSpacing);
  int maxDlgW = previewSize + marginW + spacingW;
  if (availGeo.width () > 0) {
    maxDlgW= qMin (maxDlgW, qRound (availGeo.width () * 0.9));
  }

  int minW= qMin (DpiUtils::scaled (kPreviewDialogMinW), maxDlgW);
  int minH= qMin (DpiUtils::scaled (kPreviewDialogMinH), maxDlgH);
  dialog->setMinimumSize (minW, minH);
  dialog->setMaximumSize (maxDlgW, maxDlgH);
  dialog->resize (minW, minH);

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
  previewWidget->setFixedSize (previewSize, previewSize);

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
    QTMTemplateOpener opener (this);
    opener.openTemplate (templateId);
  });
  btnLayout->addWidget (useBtn);

  layout->addLayout (btnLayout);

  dialog->exec ();
  delete dialog;
}

void
QTMTemplatePage::onTemplatesLoaded () {
  gridNeedsRefresh_= true;
  refreshTemplateGrid ();
}

void
QTMTemplatePage::showEvent (QShowEvent* event) {
  QWidget::showEvent (event);

  // Refresh grid when page becomes visible. If onTemplatesLoaded already
  // refreshed while the widget had no proper size, recalculate now that
  // the viewport has its final width to avoid showing the wrong column count.
  if (gridNeedsRefresh_) {
    refreshTemplateGrid ();
  }
  else if (templateManager_ && templateManager_->isInitialized () &&
           !templateManager_->templates ().isEmpty ()) {
    int newColumnCount= calculateColumnCount ();
    if (newColumnCount != currentColumnCount_) {
      refreshTemplateGrid ();
    }
  }
}

void
QTMTemplatePage::resizeEvent (QResizeEvent* event) {
  QWidget::resizeEvent (event);

  // Debounce resize to avoid frequent grid rebuilds during window dragging
  if (resizeDebounceTimer_) {
    resizeDebounceTimer_->start ();
  }
}
