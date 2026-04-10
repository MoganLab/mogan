
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
#include <QStyleOption>
#include <QVBoxLayout>

#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"
#include "sys_utils.hpp"

// 最多显示的最近文档数量
static const int MAX_RECENT_DOCS       = 10;
static const int MAX_GLOBAL_RECENT_DOCS= 25;

namespace {
constexpr int kMainMargin         = 32;  // 主内容区外边距
constexpr int kMainSpacing        = 24;  // 主纵向布局间距
constexpr int kStyleCardWidth     = 100; // 样式卡片宽度
constexpr int kStyleCardHeight    = 120; // 样式卡片高度
constexpr int kStyleIconSize      = 64;  // 样式卡片图标尺寸
constexpr int kStyleCardTopPadding= 12;  // 样式卡片顶部内边距
constexpr int kStyleCardMargin    = 8;   // 样式卡片内边距
constexpr int kStyleCardSpacing   = 4;   // 样式卡片内部控件间距
constexpr int kStyleCardsSpacing  = 16;  // 样式卡片横向间距
constexpr int kSectionTitleFontPx = 16;  // 分区标题字号
constexpr int kStyleIconFontPx    = 18;  // 样式图标字母字号
constexpr int kStyleNameFontPx    = 12;  // 样式名称字号
constexpr int kStyleBadgeFontPx   = 10;  // Default 徽标字号
constexpr int kRecentItemHeight   = 64;  // Recent 列表项高度
constexpr int kRecentItemPaddingX = 8;   // Recent 列表项横向内边距
constexpr int kRecentItemPaddingY = 6;   // Recent 列表项纵向内边距
constexpr int kRecentItemSpacing  = 3;   // Recent 名称与路径行间距
constexpr int kRecentNameFontPx   = 15;  // Recent 文件名字号
constexpr int kRecentPathFontPx   = 11;  // Recent 路径字号
} // namespace

/******************************************************************************
 * StyleCard 实现
 ******************************************************************************/

StyleCard::StyleCard (const DocStyle& style, QWidget* parent)
    : QWidget (parent), styleId_ (style.id), isSelected_ (false) {
  int width   = DpiUtils::scaled (kStyleCardWidth);
  int height  = DpiUtils::scaled (kStyleCardHeight);
  int iconSize= DpiUtils::scaled (kStyleIconSize);

  setFixedSize (width, height);
  setCursor (Qt::PointingHandCursor);
  setFocusPolicy (Qt::NoFocus);

  QVBoxLayout* layout= new QVBoxLayout (this);
  layout->setContentsMargins (DpiUtils::scaled (kStyleCardMargin),
                              DpiUtils::scaled (kStyleCardTopPadding),
                              DpiUtils::scaled (kStyleCardMargin),
                              DpiUtils::scaled (kStyleCardMargin));
  layout->setSpacing (DpiUtils::scaled (kStyleCardSpacing));
  layout->setAlignment (Qt::AlignTop);

  // 预览图占位（使用 QLabel 作为图标容器）
  iconLabel_= new QLabel (this);
  iconLabel_->setFixedSize (iconSize, iconSize);
  iconLabel_->setAlignment (Qt::AlignCenter);
  iconLabel_->setObjectName ("style-card-icon");
  // 显示样式ID的首字母作为占位
  iconLabel_->setText (style.id.left (1).toUpper ());
  QFont iconFont= iconLabel_->font ();
  iconFont.setBold (true);
  iconFont.setPixelSize (DpiUtils::scaled (kStyleIconFontPx));
  iconLabel_->setFont (iconFont);
  layout->addWidget (iconLabel_, 0, Qt::AlignCenter);

  // 样式名称
  nameLabel_= new QLabel (style.name, this);
  nameLabel_->setAlignment (Qt::AlignCenter);
  nameLabel_->setObjectName ("style-card-name");
  QFont nameFont= nameLabel_->font ();
  nameFont.setPixelSize (DpiUtils::scaled (kStyleNameFontPx));
  nameLabel_->setFont (nameFont);
  layout->addWidget (nameLabel_, 0, Qt::AlignCenter);

  // "默认"标签
  if (style.isDefault) {
    badgeLabel_= new QLabel (tr ("Default"), this);
    badgeLabel_->setObjectName ("style-card-badge");
    badgeLabel_->setAlignment (Qt::AlignCenter);
    QFont badgeFont= badgeLabel_->font ();
    badgeFont.setPixelSize (DpiUtils::scaled (kStyleBadgeFontPx));
    badgeLabel_->setFont (badgeFont);
    layout->addWidget (badgeLabel_, 0, Qt::AlignCenter);
  }

  setObjectName ("style-card");
}

void
StyleCard::setSelected (bool selected) {
  if (isSelected_ != selected) {
    isSelected_= selected;
    setProperty ("selected", selected);
    style ()->unpolish (this);
    style ()->polish (this);
    update ();
  }
}

void
StyleCard::enterEvent (QEnterEvent* event) {
  // 悬停时选中
  emit hovered ();
  QWidget::enterEvent (event);
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
  painter.setRenderHint (QPainter::Antialiasing);

  // 绘制背景
  QStyleOption opt;
  opt.initFrom (this);
  style ()->drawPrimitive (QStyle::PE_Widget, &opt, &painter, this);

  // 绘制选中边框
  if (isSelected_) {
    painter.setPen (QPen (opt.palette.highlight (), 2));
    painter.setBrush (Qt::NoBrush);
    painter.drawRoundedRect (rect ().adjusted (1, 1, -1, -1), 6, 6);
  }
}

/******************************************************************************
 * QtFilePage 实现
 ******************************************************************************/

QtFilePage::QtFilePage (QWidget* parent) : QWidget (parent) {
  eval_scheme ("(use-modules (startup-tab startup-tab-file))");

  // 初始化样式列表
  styles_= {{"generic", tr ("Generic"), tr ("General purpose document"), true},
            {"beamer", tr ("Beamer"), tr ("Presentation slides"), false},
            {"book", tr ("Book"), tr ("Book format"), false},
            {"exam", tr ("Exam"), tr ("Examination paper"), false},
            {"letter", tr ("Letter"), tr ("Letter format"), false},
            {"article", tr ("Article"), tr ("Article format"), false}};

  setupUI ();
  loadRecentDocs ();
}

QtFilePage::~QtFilePage ()= default;

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
QtFilePage::setupStyleCards (QVBoxLayout* layout) {
  // 标题
  QLabel* title= new QLabel (tr ("Document Style"), this);
  title->setObjectName ("startup-tab-section-title");
  QFont titleFont= title->font ();
  titleFont.setPixelSize (DpiUtils::scaled (kSectionTitleFontPx));
  title->setFont (titleFont);
  layout->addWidget (title);

  // 样式卡片容器（水平流式布局）
  QWidget*     cardsContainer= new QWidget (this);
  QHBoxLayout* cardsLayout   = new QHBoxLayout (cardsContainer);
  cardsLayout->setContentsMargins (0, 0, 0, 0);
  cardsLayout->setSpacing (DpiUtils::scaled (kStyleCardsSpacing));
  cardsLayout->setAlignment (Qt::AlignLeft);

  for (const auto& style : styles_) {
    StyleCard* card= new StyleCard (style, cardsContainer);
    styleCards_.append (card);
    cardsLayout->addWidget (card);

    connect (card, &StyleCard::hovered, this, [this, card] () {
      // 悬停时选中
      if (selectedCard_ != card) {
        if (selectedCard_) {
          selectedCard_->setSelected (false);
        }
        selectedCard_= card;
        card->setSelected (true);
      }
    });

    connect (card, &StyleCard::clicked, this, [this, card] () {
      // 单击：创建文档
      createDocumentWithStyle (card->styleId ());
    });

    // 默认选中 Generic
    if (style.isDefault) {
      card->setSelected (true);
      selectedCard_= card;
    }
  }

  cardsLayout->addStretch ();
  layout->addWidget (cardsContainer);
}

void
QtFilePage::setupRecentDocs (QVBoxLayout* layout) {
  // 标题
  QLabel* title= new QLabel (tr ("Recent Documents"), this);
  title->setObjectName ("startup-tab-section-title");
  QFont titleFont= title->font ();
  titleFont.setPixelSize (DpiUtils::scaled (kSectionTitleFontPx));
  title->setFont (titleFont);
  layout->addWidget (title);

  // 最近文档列表
  recentList_= new QListWidget (this);
  recentList_->setObjectName ("recent-docs-list");
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

void
QtFilePage::loadRecentDocs () {
  recentDocs_.clear ();
  recentList_->clear ();

  QString filePath= getRecentDocsFilePath ();
  QFile   file (filePath);
  if (!file.open (QIODevice::ReadOnly)) {
    renderRecentDocs ();
    return;
  }

  QByteArray    data= file.readAll ();
  QJsonDocument doc = QJsonDocument::fromJson (data);
  if (!doc.isObject ()) {
    renderRecentDocs ();
    return;
  }

  QJsonObject obj= doc.object ();
  QJsonArray  recentArray;
  if (obj.contains ("files")) {
    recentArray= obj["files"].toArray ();
  }
  else {
    recentArray= obj["recent_documents"].toArray ();
  }

  for (const auto& val : recentArray) {
    QJsonObject docObj= val.toObject ();
    if (obj.contains ("files") && !docObj["show"].toBool (false)) continue;

    RecentDoc doc;
    doc.filePath= docObj["path"].toString ();
    doc.fileName= docObj["name"].toString ();
    if (doc.fileName.isEmpty ()) {
      doc.fileName= QFileInfo (doc.filePath).fileName ();
    }
    if (obj.contains ("files")) {
      qint64 lastOpen= static_cast<qint64> (docObj["last_open"].toDouble ());
      doc.openedAt   = QDateTime::fromSecsSinceEpoch (lastOpen);
      if (!doc.openedAt.isValid ()) {
        doc.openedAt= QDateTime::currentDateTime ();
      }
    }
    else {
      doc.openedAt=
          QDateTime::fromString (docObj["opened_at"].toString (), Qt::ISODate);
      if (!doc.openedAt.isValid ()) {
        doc.openedAt= QDateTime::currentDateTime ();
      }
    }
    recentDocs_.append (doc);
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
    QString     path= obj["path"].toString ();
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
  recentList_->clear ();

  for (const auto& doc : recentDocs_) {
    auto* item= new QListWidgetItem ();
    item->setData (Qt::UserRole, doc.filePath);
    item->setToolTip (doc.filePath);
    item->setSizeHint (QSize (0, DpiUtils::scaled (kRecentItemHeight)));
    recentList_->addItem (item);

    auto* rowWidget= new QWidget (recentList_);
    rowWidget->setObjectName ("startup-tab-recent-item");
    auto* rowLayout= new QVBoxLayout (rowWidget);
    rowLayout->setContentsMargins (DpiUtils::scaled (kRecentItemPaddingX),
                                   DpiUtils::scaled (kRecentItemPaddingY),
                                   DpiUtils::scaled (kRecentItemPaddingX),
                                   DpiUtils::scaled (kRecentItemPaddingY));
    rowLayout->setSpacing (DpiUtils::scaled (kRecentItemSpacing));

    auto* nameLabel= new QLabel (doc.fileName, rowWidget);
    nameLabel->setObjectName ("startup-tab-recent-name");
    QFont nameFont= nameLabel->font ();
    nameFont.setBold (true);
    nameFont.setPixelSize (DpiUtils::scaled (kRecentNameFontPx));
    nameLabel->setFont (nameFont);

    auto* pathLabel= new QLabel (doc.filePath, rowWidget);
    pathLabel->setObjectName ("startup-tab-recent-path");
    QFont pathFont= pathLabel->font ();
    pathFont.setPixelSize (DpiUtils::scaled (kRecentPathFontPx));
    pathLabel->setFont (pathFont);

    rowLayout->addWidget (nameLabel);
    rowLayout->addWidget (pathLabel);
    recentList_->setItemWidget (item, rowWidget);
  }

  if (recentDocs_.isEmpty ()) {
    auto* item= new QListWidgetItem (tr ("No recent documents"));
    item->setFlags (item->flags () & ~Qt::ItemIsEnabled);
    recentList_->addItem (item);
  }
}

void
QtFilePage::addRecentDoc (const QString& path) {
  QString fileName= QFileInfo (path).fileName ();

  // 检查是否已存在
  for (auto it= recentDocs_.begin (); it != recentDocs_.end (); ++it) {
    if (it->filePath == path) {
      it->openedAt= QDateTime::currentDateTime ();
      // 移到最前面
      RecentDoc doc= *it;
      recentDocs_.erase (it);
      recentDocs_.prepend (doc);
      saveRecentDocs ();
      QString escapedPath= path;
      escapedPath.replace ("\\", "\\\\");
      escapedPath.replace ("\"", "\\\"");
      QString schemeCmd=
          QString ("(startup-tab-add-recent-doc \"%1\")").arg (escapedPath);
      eval_scheme (schemeCmd.toUtf8 ().constData ());
      refreshRecentDocs ();
      return;
    }
  }

  // 添加新条目
  RecentDoc doc;
  doc.filePath= path;
  doc.fileName= fileName;
  doc.openedAt= QDateTime::currentDateTime ();
  recentDocs_.prepend (doc);

  // 限制数量
  while (recentDocs_.size () > MAX_RECENT_DOCS) {
    recentDocs_.removeLast ();
  }

  saveRecentDocs ();
  QString escapedPath= path;
  escapedPath.replace ("\\", "\\\\");
  escapedPath.replace ("\"", "\\\"");
  QString schemeCmd=
      QString ("(startup-tab-add-recent-doc \"%1\")").arg (escapedPath);
  eval_scheme (schemeCmd.toUtf8 ().constData ());
  refreshRecentDocs ();
}

void
QtFilePage::removeRecentDoc (const QString& path) {
  for (auto it= recentDocs_.begin (); it != recentDocs_.end (); ++it) {
    if (it->filePath == path) {
      recentDocs_.erase (it);
      break;
    }
  }
  saveRecentDocs ();

  QString escapedPath= path;
  escapedPath.replace ("\\", "\\\\");
  escapedPath.replace ("\"", "\\\"");
  QString schemeCmd=
      QString ("(startup-tab-clear-recent-doc \"%1\")").arg (escapedPath);
  eval_scheme (schemeCmd.toUtf8 ().constData ());
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

  // 转义路径中的双引号和反斜杠，防止 Scheme 注入
  QString escapedPath= path;
  escapedPath.replace ("\\", "\\\\"); // 先替换反斜杠
  escapedPath.replace ("\"", "\\\""); // 再替换双引符

  QString schemeCmd= QString ("(load-document \"%1\")").arg (escapedPath);
  eval_scheme (schemeCmd.toUtf8 ().constData ());
}

void
QtFilePage::onRecentDocContextMenu (const QPoint& pos) {
  QListWidgetItem* item= recentList_->itemAt (pos);
  if (!item) return;

  QString path= item->data (Qt::UserRole).toString ();
  if (path.isEmpty ()) return;

  QMenu    menu (this);
  QAction* removeAction= menu.addAction (tr ("Remove from list"));

  if (menu.exec (recentList_->mapToGlobal (pos)) == removeAction) {
    removeRecentDoc (path);
  }
}

void
QtFilePage::createDocumentWithStyle (const QString& styleId) {
  // 验证 styleId 是预定义的合法值，防止注入攻击
  static const QStringList validStyles= {"generic", "beamer", "book",
                                         "exam",    "letter", "article"};
  if (!validStyles.contains (styleId)) {
    qWarning () << "Invalid style ID:" << styleId;
    return;
  }

  // 调用 Scheme 函数创建指定样式的文档
  QString schemeCmd= QString ("(new-document-with-style \"%1\")").arg (styleId);
  eval_scheme (schemeCmd.toUtf8 ().constData ());
}
