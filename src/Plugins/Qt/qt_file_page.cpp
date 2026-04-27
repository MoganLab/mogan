
/******************************************************************************
 * MODULE     : qt_file_page.cpp
 * DESCRIPTION: File page implementation for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_file_page.hpp"
#include <QButtonGroup>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QStringList>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>

#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"
#include "sys_utils.hpp"

// 最多显示的最近文档数量
static const int MAX_RECENT_DOCS       = 50;
static const int MAX_GLOBAL_RECENT_DOCS= 100;

namespace {
constexpr int kMainMargin         = 32;  // 主内容区外边距
constexpr int kMainSpacing        = 24;  // 主纵向布局间距
constexpr int kStyleCardWidth     = 160; // 样式卡片宽度
constexpr int kStyleCardHeight    = 256; // 样式卡片高度
constexpr int kStyleIconSize      = 96;  // 样式卡片图标尺寸
constexpr int kStyleCardTopPadding= 12;  // 样式卡片顶部内边距
constexpr int kStyleCardMargin    = 8;   // 样式卡片内边距
constexpr int kStyleCardSpacing   = 4;   // 样式卡片内部控件间距
constexpr int kStyleCardsSpacing  = 16;  // 样式卡片横向间距
constexpr int kStyleCardRadius    = 8;   // 样式卡片圆角
constexpr int kStyleIconRadius    = 8;   // 样式图标圆角
constexpr int kSectionTitleFontPx = 16;  // 分区标题字号
constexpr int kStyleIconFontPx    = 48;  // 样式图标字母字号
constexpr int kStyleNameFontPx    = 14;  // 样式名称字号
constexpr int kRecentListRadius   = 8;   // Recent 列表圆角
constexpr int kRecentItemHeight   = 40;  // Recent 列表项高度
constexpr int kRecentItemRadius   = 4;   // Recent 列表项圆角
constexpr int kRecentItemMarginX  = 4;   // Recent 列表项横向边距
constexpr int kRecentItemMarginY  = 2;   // Recent 列表项纵向边距
constexpr int kRecentItemPaddingX = 8;   // Recent 列表项横向内边距
constexpr int kRecentItemPaddingY = 6;   // Recent 列表项纵向内边距
constexpr int kRecentItemSpacing  = 3;   // Recent 名称与时间标签间距
constexpr int kRecentNameFontPx   = 15;  // Recent 文件名字号
constexpr int kRecentTimeFontPx   = 11;  // Recent 时间字号
} // namespace

/******************************************************************************
 * StyleCard 实现
 ******************************************************************************/

StyleCard::StyleCard (const DocStyle& style, QWidget* parent)
    : QWidget (parent), styleId_ (style.id) {
  int width   = DpiUtils::scaled (kStyleCardWidth);
  int height  = DpiUtils::scaled (kStyleCardHeight);
  int iconSize= DpiUtils::scaled (kStyleIconSize);

  setFixedSize (width, height);
  setCursor (Qt::PointingHandCursor);
  setFocusPolicy (Qt::NoFocus);
  setToolTip (style.description);

  QVBoxLayout* layout= new QVBoxLayout (this);
  layout->setContentsMargins (DpiUtils::scaled (kStyleCardMargin),
                              DpiUtils::scaled (kStyleCardTopPadding),
                              DpiUtils::scaled (kStyleCardMargin),
                              DpiUtils::scaled (kStyleCardMargin));
  layout->setSpacing (DpiUtils::scaled (kStyleCardSpacing));

  layout->addStretch ();

  iconLabel_= new QLabel (this);
  iconLabel_->setFixedSize (iconSize, iconSize);
  iconLabel_->setAlignment (Qt::AlignCenter);
  iconLabel_->setObjectName ("style-card-icon");

  if (style.id == "generic") {
    iconLabel_->setText ("+");
  }
  else {
    iconLabel_->setText (style.id.left (1).toUpper ());
  }
  QFont iconFont= DpiUtils::scaledFont (iconLabel_->font (), kStyleIconFontPx);
  iconFont.setBold (true);
  iconLabel_->setFont (iconFont);
  layout->addWidget (iconLabel_, 0, Qt::AlignCenter);

  nameLabel_= new QLabel (style.name, this);
  nameLabel_->setAlignment (Qt::AlignCenter);
  nameLabel_->setObjectName ("style-card-name");
  DpiUtils::applyScaledFont (nameLabel_, kStyleNameFontPx);
  layout->addWidget (nameLabel_, 0, Qt::AlignCenter);

  layout->addStretch ();

  setObjectName ("style-card");
}

void
StyleCard::mousePressEvent (QMouseEvent* event) {
  if (event->button () == Qt::LeftButton) {
    emit clicked ();
  }
  QWidget::mousePressEvent (event);
}

void
StyleCard::paintEvent (QPaintEvent* event) {
  QPainter painter (this);

  QStyleOption opt;
  opt.initFrom (this);

  style ()->drawPrimitive (QStyle::PE_Widget, &opt, &painter, this);
}

/******************************************************************************
 * QtFilePage 实现
 ******************************************************************************/

QtFilePage::QtFilePage (QWidget* parent) : QWidget (parent) {
  eval_scheme ("(use-modules (startup-tab startup-tab-file))");

  styles_= {{"generic", qt_translate ("New Blank Document"),
             qt_translate ("Create a new blank document"), true}};

  setupUI ();
  loadRecentDocs ();
}

QtFilePage::~QtFilePage ()= default;

void
QtFilePage::showEvent (QShowEvent* event) {
  QWidget::showEvent (event);
  refreshRecentDocs ();
  // 初始排列卡片
  QTimer::singleShot (0, this, &QtFilePage::rearrangeStyleCards);
}

void
QtFilePage::hideEvent (QHideEvent* event) {
  QWidget::hideEvent (event);
}

void
QtFilePage::resizeEvent (QResizeEvent* event) {
  QWidget::resizeEvent (event);
  // 只在宽度变化时重排，避免不必要的计算
  if (event->oldSize ().width () != event->size ().width ()) {
    rearrangeStyleCards ();
  }
}

void
QtFilePage::setupUI () {
  QVBoxLayout* mainLayout= new QVBoxLayout (this);
  mainLayout->setContentsMargins (
      DpiUtils::scaled (kMainMargin), DpiUtils::scaled (kMainMargin),
      DpiUtils::scaled (kMainMargin), DpiUtils::scaled (kMainMargin));
  mainLayout->setSpacing (DpiUtils::scaled (kMainSpacing));

  // 1. 样式选择区
  setupStyleCards (mainLayout);

  // 分隔线
  QFrame* line= new QFrame (this);
  line->setFrameShape (QFrame::HLine);
  line->setObjectName ("startup-tab-separator");
  mainLayout->addWidget (line);

  // 2. 最近文档区
  setupRecentDocs (mainLayout);
}

void
QtFilePage::rearrangeStyleCards () {
  if (!cardsLayout_ || styleCards_.isEmpty ()) return;

  // 清除当前布局中的卡片
  for (auto* card : styleCards_) {
    cardsLayout_->removeWidget (card);
  }

  // 计算每行可以容纳的卡片数
  int cardWidth= DpiUtils::scaled (kStyleCardWidth);
  int spacing  = DpiUtils::scaled (kStyleCardsSpacing);
  int availableWidth=
      cardsContainer_->width () - DpiUtils::scaled (kMainMargin * 2);
  int cardsPerRow= qMax (1, (availableWidth + spacing) / (cardWidth + spacing));

  // 重新排列卡片
  for (int i= 0; i < styleCards_.size (); ++i) {
    int row= i / cardsPerRow;
    int col= i % cardsPerRow;
    cardsLayout_->addWidget (styleCards_[i], row, col,
                             Qt::AlignLeft | Qt::AlignTop);
  }
}

void
QtFilePage::setupStyleCards (QVBoxLayout* layout) {
  // 标题
  QLabel* title= new QLabel (qt_translate ("Document Style"), this);
  title->setObjectName ("startup-tab-section-title");
  DpiUtils::applyScaledFont (title, kSectionTitleFontPx);
  layout->addWidget (title);

  // 样式卡片容器（响应式网格布局）
  cardsContainer_= new QWidget (this);
  cardsLayout_   = new QGridLayout (cardsContainer_);
  cardsLayout_->setContentsMargins (0, 0, 0, 0);
  cardsLayout_->setSpacing (DpiUtils::scaled (kStyleCardsSpacing));
  cardsLayout_->setAlignment (Qt::AlignLeft | Qt::AlignTop);

  for (const auto& style : styles_) {
    StyleCard* card= new StyleCard (style, cardsContainer_);
    styleCards_.append (card);

    connect (card, &StyleCard::clicked, this,
             [this, card] () { createDocumentWithStyle (card->styleId ()); });
  }

  layout->addWidget (cardsContainer_);
}

void
QtFilePage::setupRecentDocs (QVBoxLayout* layout) {
  // 标题
  QLabel* title= new QLabel (qt_translate ("Recent Documents"), this);
  title->setObjectName ("startup-tab-section-title");
  DpiUtils::applyScaledFont (title, kSectionTitleFontPx);
  layout->addWidget (title);

  // 最近文档列表
  recentList_= new QListWidget (this);
  recentList_->setObjectName ("recent-docs-list");
  recentList_->setStyleSheet (
      QString ("QListWidget#recent-docs-list { border-radius: %1px; }\n"
               "QListWidget#recent-docs-list::item {\n"
               "  min-height: %2px;\n"
               "  border-radius: %3px;\n"
               "  margin: %4px %5px;\n"
               "}")
          .arg (DpiUtils::scaled (kRecentListRadius))
          .arg (DpiUtils::scaled (kRecentItemHeight))
          .arg (DpiUtils::scaled (kRecentItemRadius))
          .arg (DpiUtils::scaled (kRecentItemMarginY))
          .arg (DpiUtils::scaled (kRecentItemMarginX)));
  recentList_->setFocusPolicy (Qt::NoFocus);
  recentList_->setContextMenuPolicy (Qt::CustomContextMenu);
  recentList_->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

  connect (recentList_, &QListWidget::itemClicked, this,
           [this] (QListWidgetItem* item) {
             if (item) onRecentDocClicked (item);
           });
  connect (recentList_, &QListWidget::customContextMenuRequested, this,
           &QtFilePage::onRecentDocContextMenu);

  layout->addWidget (recentList_);
}

/******************************************************************************
 * 最近文档管理
 ******************************************************************************/

static QString
getRecentDocsFilePath () {
  string  homePath = get_env ("TEXMACS_HOME_PATH");
  QString configDir= to_qstring (homePath * "/system");
  QDir ().mkpath (configDir);
  return QDir (configDir).filePath ("recent-files.json");
}

static QStringList
getRecentDocPathsFromScheme () {
  QStringList paths;
  tmscm       result= eval_scheme ("(startup-tab-get-recent-docs)");
  if (!tmscm_is_list (result)) return paths;

  for (tmscm cur= result; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_string (item)) continue;
    paths.append (QString::fromUtf8 (as_charp (tmscm_to_string (item))));
  }
  return paths;
}

static QDateTime
readRecentOpenedAt (const QJsonObject& docObj, bool hasFilesField) {
  if (hasFilesField) {
    qint64    lastOpen= static_cast<qint64> (docObj["last_open"].toDouble ());
    QDateTime openedAt= QDateTime::fromSecsSinceEpoch (lastOpen);
    return openedAt.isValid () ? openedAt : QDateTime::currentDateTime ();
  }

  QDateTime openedAt=
      QDateTime::fromString (docObj["opened_at"].toString (), Qt::ISODate);
  return openedAt.isValid () ? openedAt : QDateTime::currentDateTime ();
}

static void
populateRecentDocsFromPaths (QList<RecentDoc>&  recentDocs,
                             const QStringList& recentPaths) {
  for (const QString& path : recentPaths) {
    if (path.isEmpty ()) continue;
    RecentDoc recentDoc;
    recentDoc.filePath= path;
    recentDoc.fileName= QFileInfo (path).fileName ();
    recentDoc.openedAt= QDateTime::currentDateTime ();
    recentDocs.append (recentDoc);
    if (recentDocs.size () >= MAX_RECENT_DOCS) break;
  }
}

void
QtFilePage::loadRecentDocs () {
  recentDocs_.clear ();
  recentList_->clear ();

  QStringList recentPaths= getRecentDocPathsFromScheme ();
  for (QString& path : recentPaths) {
    path= QDir::fromNativeSeparators (path);
  }
  recentPaths.removeDuplicates ();

  QString filePath= getRecentDocsFilePath ();
  QFile   file (filePath);
  if (!file.open (QIODevice::ReadOnly)) {
    populateRecentDocsFromPaths (recentDocs_, recentPaths);
    renderRecentDocs ();
    return;
  }

  QByteArray    data= file.readAll ();
  QJsonDocument doc = QJsonDocument::fromJson (data);
  if (!doc.isObject ()) {
    populateRecentDocsFromPaths (recentDocs_, recentPaths);
    renderRecentDocs ();
    return;
  }

  QJsonObject                 obj          = doc.object ();
  const bool                  hasFilesField= obj.contains ("files");
  const QJsonArray            recentArray  = hasFilesField
                                                 ? obj["files"].toArray ()
                                                 : obj["recent_documents"].toArray ();
  QHash<QString, QJsonObject> recentByPath;
  for (const auto& val : recentArray) {
    const QJsonObject docObj= val.toObject ();
    const QString path= QDir::fromNativeSeparators (docObj["path"].toString ());
    if (!path.isEmpty ()) recentByPath.insert (path, docObj);
  }

  for (const QString& path : recentPaths) {
    if (path.isEmpty ()) continue;

    RecentDoc recentDoc;
    recentDoc.filePath= path;
    if (recentByPath.contains (path)) {
      const QJsonObject& docObj= recentByPath[path];
      recentDoc.fileName       = docObj["name"].toString ();
      recentDoc.openedAt       = readRecentOpenedAt (docObj, hasFilesField);
    }
    else {
      recentDoc.openedAt= QDateTime::currentDateTime ();
    }
    if (recentDoc.fileName.isEmpty ()) {
      recentDoc.fileName= QFileInfo (path).fileName ();
    }

    recentDocs_.append (recentDoc);
    if (recentDocs_.size () >= MAX_RECENT_DOCS) break;
  }

  renderRecentDocs ();
}

void
QtFilePage::saveRecentDocs () {
  QFile       file (getRecentDocsFilePath ());
  QJsonObject root;
  if (file.open (QIODevice::ReadOnly)) {
    QJsonDocument existingDoc= QJsonDocument::fromJson (file.readAll ());
    if (existingDoc.isObject ()) {
      root= existingDoc.object ();
    }
    file.close ();
  }
  if (root.isEmpty ()) {
    root["meta"] = QJsonObject{{"version", 1}, {"total", 0}};
    root["files"]= QJsonArray ();
  }

  QJsonObject meta = root["meta"].toObject ();
  QJsonArray  files= root["files"].toArray ();

  QHash<QString, QJsonObject> existingByPath;
  for (const auto& val : files) {
    QJsonObject obj = val.toObject ();
    QString     path= QDir::fromNativeSeparators (obj["path"].toString ());
    if (!path.isEmpty ()) existingByPath.insert (path, obj);
  }

  QJsonArray updatedFiles;
  for (const auto& doc : recentDocs_) {
    QJsonObject obj  = existingByPath.value (doc.filePath);
    obj["path"]      = doc.filePath;
    obj["name"]      = doc.fileName;
    obj["last_open"] = static_cast<double> (doc.openedAt.toSecsSinceEpoch ());
    obj["show"]      = true;
    obj["open_count"]= qMax (1, obj["open_count"].toInt () + 1);
    updatedFiles.append (obj);
    existingByPath.remove (doc.filePath);
  }

  for (auto it= existingByPath.constBegin (); it != existingByPath.constEnd ();
       ++it) {
    QJsonObject obj= it.value ();
    obj["show"]    = false;
    updatedFiles.append (obj);
  }

  while (updatedFiles.size () > MAX_GLOBAL_RECENT_DOCS) {
    updatedFiles.removeLast ();
  }

  meta["version"]= meta.value ("version").toInt (1);
  meta["total"]  = updatedFiles.size ();
  root["meta"]   = meta;
  root["files"]  = updatedFiles;

  if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate)) return;
  file.write (QJsonDocument (root).toJson ());
  file.close ();
}

void
QtFilePage::refreshRecentDocs () {
  loadRecentDocs ();
}

void
QtFilePage::renderRecentDocs () {
  while (recentList_->count () > 0) {
    QListWidgetItem* item  = recentList_->takeItem (0);
    QWidget*         widget= recentList_->itemWidget (item);
    if (widget) {
      recentList_->removeItemWidget (item);
      delete widget;
    }
    delete item;
  }

  for (const auto& doc : recentDocs_) {
    auto* item= new QListWidgetItem ();
    item->setData (Qt::UserRole, doc.filePath);
    item->setToolTip (doc.filePath);
    item->setSizeHint (QSize (0, DpiUtils::scaled (kRecentItemHeight)));
    recentList_->addItem (item);

    auto* rowWidget= new QWidget (recentList_);
    rowWidget->setObjectName ("startup-tab-recent-item");
    rowWidget->setAttribute (Qt::WA_TransparentForMouseEvents);
    auto* rowLayout= new QHBoxLayout (rowWidget);
    rowLayout->setContentsMargins (DpiUtils::scaled (kRecentItemPaddingX),
                                   DpiUtils::scaled (kRecentItemPaddingY),
                                   DpiUtils::scaled (kRecentItemPaddingX),
                                   DpiUtils::scaled (kRecentItemPaddingY));
    rowLayout->setSpacing (DpiUtils::scaled (kRecentItemSpacing));

    auto* nameLabel= new QLabel (doc.fileName, rowWidget);
    nameLabel->setObjectName ("startup-tab-recent-name");
    QFont nameFont=
        DpiUtils::scaledFont (nameLabel->font (), kRecentNameFontPx);
    nameFont.setBold (true);
    nameLabel->setFont (nameFont);

    auto* timeLabel= new QLabel (qt_translate ("Last opened") + ": " +
                                     doc.openedAt.toString ("yyyy-MM-dd hh:mm"),
                                 rowWidget);
    timeLabel->setObjectName ("startup-tab-recent-time");
    DpiUtils::applyScaledFont (timeLabel, kRecentTimeFontPx);
    timeLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);

    rowLayout->addWidget (nameLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget (timeLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    recentList_->setItemWidget (item, rowWidget);
  }

  if (recentDocs_.isEmpty ()) {
    auto* item= new QListWidgetItem (qt_translate ("No recent documents"));
    item->setFlags (item->flags () & ~Qt::ItemIsEnabled);
    recentList_->addItem (item);
  }
}

void
QtFilePage::addRecentDoc (const QString& path) {
  QString normPath= QDir::fromNativeSeparators (path);
  QString fileName= QFileInfo (normPath).fileName ();

  // 检查是否已存在
  for (auto it= recentDocs_.begin (); it != recentDocs_.end (); ++it) {
    if (it->filePath == normPath) {
      it->openedAt= QDateTime::currentDateTime ();
      // 移到最前面
      RecentDoc doc= *it;
      recentDocs_.erase (it);
      recentDocs_.prepend (doc);
      saveRecentDocs ();
      eval_scheme ("(startup-tab-add-recent-doc " *
                   qt_scheme_quote_utf8 (normPath) * ")");
      refreshRecentDocs ();
      return;
    }
  }

  // 添加新条目
  RecentDoc doc;
  doc.filePath= normPath;
  doc.fileName= fileName;
  doc.openedAt= QDateTime::currentDateTime ();
  recentDocs_.prepend (doc);

  // 限制数量
  while (recentDocs_.size () > MAX_RECENT_DOCS) {
    recentDocs_.removeLast ();
  }

  saveRecentDocs ();
  eval_scheme ("(startup-tab-add-recent-doc " *
               qt_scheme_quote_utf8 (normPath) * ")");
  refreshRecentDocs ();
}

void
QtFilePage::removeRecentDoc (const QString& path) {
  QString normPath= QDir::fromNativeSeparators (path);
  for (auto it= recentDocs_.begin (); it != recentDocs_.end (); ++it) {
    if (it->filePath == normPath) {
      recentDocs_.erase (it);
      break;
    }
  }
  saveRecentDocs ();

  eval_scheme ("(startup-tab-clear-recent-doc " *
               qt_scheme_quote_utf8 (normPath) * ")");
  refreshRecentDocs ();
}

void
QtFilePage::clearAllRecentDocs () {
  eval_scheme ("(startup-tab-clear-all-recent)");
  recentDocs_.clear ();
  saveRecentDocs ();
  refreshRecentDocs ();
}

/******************************************************************************
 * 事件处理
 ******************************************************************************/

void
QtFilePage::onRecentDocClicked (QListWidgetItem* item) {
  if (!item) return;

  QString path= item->data (Qt::UserRole).toString ();
  if (path.isEmpty ()) return;

  addRecentDoc (path);

  eval_scheme ("(load-document " * qt_scheme_quote_utf8 (path) * ")");
}

void
QtFilePage::onRecentDocContextMenu (const QPoint& pos) {
  QListWidgetItem* item= recentList_->itemAt (pos);
  if (!item) return;

  QString path= item->data (Qt::UserRole).toString ();
  if (path.isEmpty ()) return;

  QMenu    menu (this);
  QAction* removeAction= menu.addAction (qt_translate ("Remove from list"));
  QAction* clearAction = menu.addAction (qt_translate ("Clear list"));

  QAction* selected= menu.exec (recentList_->mapToGlobal (pos));
  if (selected == removeAction) {
    removeRecentDoc (path);
  }
  else if (selected == clearAction) {
    clearAllRecentDocs ();
  }
}

void
QtFilePage::createDocumentWithStyle (const QString& styleId) {
  // 验证 styleId 是预定义的合法值，防止注入攻击
  static const QStringList validStyles= {"generic"};
  if (!validStyles.contains (styleId)) {
    qWarning () << "Invalid style ID:" << styleId;
    return;
  }

  eval_scheme ("(new-document-with-style " * qt_scheme_quote (styleId) * ")");
}
