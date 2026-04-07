
/******************************************************************************
 * MODULE     : qt_template_page.cpp
 * DESCRIPTION: Template page implementation for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_template_page.hpp"

#include <QDebug>
#include <QEvent>
#include <QGridLayout>
#include <QShowEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include "template_manager.hpp"

QTTemplatePage::QTTemplatePage(QWidget* parent)
    : QWidget(parent),
      titleLabel_(nullptr),
      categoryBar_(nullptr),
      scrollArea_(nullptr),
      gridWidget_(nullptr),
      gridLayout_(nullptr),
      progressDialog_(nullptr),
      templateManager_(nullptr),
      currentCategory_("thesis"),
      activeCategoryBtn_(nullptr) {
    setupUI();
}

QTTemplatePage::~QTTemplatePage() {
}

void QTTemplatePage::initialize() {
    templateManager_ = TemplateManager::instance();

    connect(templateManager_, &TemplateManager::templatesLoaded, this,
            &QTTemplatePage::onTemplatesLoaded);
    connect(templateManager_, &TemplateManager::downloadProgress, this,
            &QTTemplatePage::onDownloadProgress);
    connect(templateManager_, &TemplateManager::downloadCompleted, this,
            &QTTemplatePage::onDownloadCompleted);
    connect(templateManager_, &TemplateManager::downloadFailed, this,
            &QTTemplatePage::onDownloadFailed);

    // Check if already initialized with data
    if (templateManager_->isInitialized() &&
        !templateManager_->templates().isEmpty()) {
        // Already have data, refresh immediately
        onTemplatesLoaded();
    } else {
        // Initialize asynchronously
        QTimer::singleShot(0, this, [this]() {
            templateManager_->initialize();
        });
    }
}

void QTTemplatePage::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(24);

    // Title
    titleLabel_ = new QLabel("Template Center", this);
    titleLabel_->setObjectName("startup-tab-page-title");
    layout->addWidget(titleLabel_);

    // Category bar
    createCategoryButtons();
    layout->addWidget(categoryBar_);

    // Scroll area for templates
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    gridWidget_ = new QWidget(scrollArea_);
    gridLayout_ = new QGridLayout(gridWidget_);
    gridLayout_->setSpacing(20);
    gridLayout_->setContentsMargins(0, 0, 0, 0);

    scrollArea_->setWidget(gridWidget_);
    layout->addWidget(scrollArea_, 1);

    // Loading label
    QLabel* loadingLabel = new QLabel("Loading templates...", gridWidget_);
    loadingLabel->setObjectName("startup-tab-loading");
    loadingLabel->setAlignment(Qt::AlignCenter);
    gridLayout_->addWidget(loadingLabel, 0, 0, 1, 3);
}

void QTTemplatePage::createCategoryButtons() {
    categoryBar_ = new QWidget(this);
    QHBoxLayout* categoryLayout = new QHBoxLayout(categoryBar_);
    categoryLayout->setSpacing(16);
    categoryLayout->setContentsMargins(0, 0, 0, 0);

    QStringList categories = {"Thesis", "Lab Report", "Math Modeling"};
    QStringList categoryIds = {"thesis", "lab-report", "math-modeling"};

    for (int i = 0; i < categories.size(); ++i) {
        QPushButton* catBtn = new QPushButton(categories[i], categoryBar_);
        catBtn->setObjectName(i == 0 ? "startup-tab-category-btn-active"
                                     : "startup-tab-category-btn");
        catBtn->setCheckable(true);
        catBtn->setChecked(i == 0);
        catBtn->setFocusPolicy(Qt::NoFocus);
        catBtn->setCursor(Qt::PointingHandCursor);
        catBtn->setProperty("categoryId", categoryIds[i]);
        categoryLayout->addWidget(catBtn);

        if (i == 0) {
            activeCategoryBtn_ = catBtn;
        }

        connect(catBtn, &QPushButton::clicked, this,
                &QTTemplatePage::onCategoryClicked);
    }

    categoryLayout->addStretch();
}

void QTTemplatePage::onCategoryClicked() {
    QPushButton* clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn || clickedBtn == activeCategoryBtn_) return;

    // Update button styles
    if (activeCategoryBtn_) {
        activeCategoryBtn_->setObjectName("startup-tab-category-btn");
        activeCategoryBtn_->setChecked(false);
        activeCategoryBtn_->style()->unpolish(activeCategoryBtn_);
        activeCategoryBtn_->style()->polish(activeCategoryBtn_);
    }

    clickedBtn->setObjectName("startup-tab-category-btn-active");
    clickedBtn->setChecked(true);
    clickedBtn->style()->unpolish(clickedBtn);
    clickedBtn->style()->polish(clickedBtn);

    activeCategoryBtn_ = clickedBtn;

    // Refresh grid
    QString catId = clickedBtn->property("categoryId").toString();
    refreshTemplateGrid(catId);
}

void QTTemplatePage::refreshTemplateGrid(const QString& category) {
    currentCategory_ = category;

    // Clear existing content
    QLayoutItem* item;
    while ((item = gridLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    if (!templateManager_ || !templateManager_->isInitialized()) {
        QLabel* label = new QLabel("Initializing...", gridWidget_);
        label->setAlignment(Qt::AlignCenter);
        gridLayout_->addWidget(label, 0, 0, 1, 3);
        return;
    }

    QList<TemplateMetadataPtr> templates =
        templateManager_->templatesByCategory(category);

    if (templates.isEmpty()) {
        QLabel* label = new QLabel("No templates available.", gridWidget_);
        label->setAlignment(Qt::AlignCenter);
        gridLayout_->addWidget(label, 0, 0, 1, 3);
        return;
    }

    // Add template cards
    int row = 0, col = 0;
    for (const auto& tmpl : templates) {
        QWidget* card = createTemplateCard(tmpl);
        gridLayout_->addWidget(card, row, col);

        col++;
        if (col >= 3) {
            col = 0;
            row++;
        }
    }

    gridLayout_->setRowStretch(row + 1, 1);
}

QWidget* QTTemplatePage::createTemplateCard(const TemplateMetadataPtr& tmpl) {
    QWidget* card = new QWidget(gridWidget_);
    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    card->setObjectName("startup-tab-template-card");
    card->setFixedSize(200, 180);
    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("templateId", tmpl->id);

    // Preview placeholder
    QLabel* preview = new QLabel(card);
    preview->setObjectName("startup-tab-template-preview");
    preview->setFixedSize(176, 100);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet("background: #f0f0f0; border-radius: 4px;");
    preview->setText("Preview");
    layout->addWidget(preview, 0, Qt::AlignHCenter);

    // Template name
    QLabel* nameLabel = new QLabel(tmpl->name, card);
    nameLabel->setObjectName("startup-tab-template-name");
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setWordWrap(true);
    layout->addWidget(nameLabel);

    // Version
    QLabel* versionLabel = new QLabel(tmpl->version, card);
    versionLabel->setObjectName("startup-tab-template-version");
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet("color: #888; font-size: 11px;");
    layout->addWidget(versionLabel);

    layout->addStretch();

    // Install event filter to handle clicks
    card->installEventFilter(this);

    return card;
}

bool QTTemplatePage::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        QWidget* card = qobject_cast<QWidget*>(watched);
        if (card && card->parent() == gridWidget_) {
            QString templateId = card->property("templateId").toString();
            if (!templateId.isEmpty()) {
                downloadTemplate(templateId);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void QTTemplatePage::downloadTemplate(const QString& templateId) {
    if (!templateManager_) return;

    if (templateManager_->isTemplateAvailableLocally(templateId)) {
        QString localPath = templateManager_->localTemplatePath(templateId);
        emit templateOpened(localPath);
    } else {
        progressDialog_ = new QProgressDialog("Downloading template...", "Cancel",
                                              0, 100, this);
        progressDialog_->setWindowModality(Qt::WindowModal);
        progressDialog_->setAutoClose(true);
        progressDialog_->show();

        templateManager_->downloadTemplate(templateId);
    }
}

void QTTemplatePage::onTemplatesLoaded() {
    refreshTemplateGrid(currentCategory_);

    // Force layout update to ensure content is visible
    if (gridWidget_) {
        gridWidget_->update();
        gridWidget_->adjustSize();
    }
    if (scrollArea_) {
        scrollArea_->update();
    }
}

void QTTemplatePage::onDownloadProgress(const QString& templateId,
                                        qint64 bytesReceived,
                                        qint64 bytesTotal) {
    if (progressDialog_) {
        progressDialog_->setMaximum(static_cast<int>(bytesTotal));
        progressDialog_->setValue(static_cast<int>(bytesReceived));
    }
}

void QTTemplatePage::onDownloadCompleted(const QString& templateId,
                                         const QString& localPath) {
    if (progressDialog_) {
        progressDialog_->close();
        progressDialog_->deleteLater();
        progressDialog_ = nullptr;
    }

    emit templateOpened(localPath);
}

void QTTemplatePage::onDownloadFailed(const QString& templateId,
                                      const QString& error) {
    if (progressDialog_) {
        progressDialog_->close();
        progressDialog_->deleteLater();
        progressDialog_ = nullptr;
    }

    QMessageBox::warning(this, "Download Failed",
                         QString("Failed to download template: %1").arg(error));
}

void QTTemplatePage::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // Refresh grid when page becomes visible
    if (templateManager_ && templateManager_->isInitialized() &&
        !templateManager_->templates().isEmpty()) {
        refreshTemplateGrid(currentCategory_);
    }
}
