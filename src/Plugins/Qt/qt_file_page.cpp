
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
#include <QStandardPaths>
#include <QStyleOption>
#include <QVBoxLayout>

#include "qt_dpi_utils.hpp"
#include "s7_tm.hpp"

// 最多显示的最近文档数量
static const int MAX_RECENT_DOCS= 10;

/******************************************************************************
 * StyleCard 实现
 ******************************************************************************/

StyleCard::StyleCard (const DocStyle& style, QWidget* parent)
    : QWidget (parent), styleId_ (style.id), isSelected_ (false) {
  // 使用 DpiUtils 计算缩放后的尺寸
  int scale   = qRound (DpiUtils::scaleFactor ());
  int width   = DpiUtils::scaled (100);
  int height  = DpiUtils::scaled (120);
  int iconSize= DpiUtils::scaled (64);

  setFixedSize (width, height);
  setCursor (Qt::PointingHandCursor);
  setFocusPolicy (Qt::NoFocus);

  QVBoxLayout* layout= new QVBoxLayout (this);
  layout->setContentsMargins (8, 8, 8, 8);
  layout->setSpacing (4);
  layout->setAlignment (Qt::AlignCenter);

  // 预览图占位（使用 QLabel 作为图标容器）
  iconLabel_= new QLabel (this);
  iconLabel_->setFixedSize (iconSize, iconSize);
  iconLabel_->setAlignment (Qt::AlignCenter);
  iconLabel_->setObjectName ("style-card-icon");
  // 显示样式ID的首字母作为占位
  iconLabel_->setText (style.id.left (1).toUpper ());
  layout->addWidget (iconLabel_, 0, Qt::AlignCenter);

  // 样式名称
  nameLabel_= new QLabel (style.name, this);
  nameLabel_->setAlignment (Qt::AlignCenter);
  nameLabel_->setObjectName ("style-card-name");
  layout->addWidget (nameLabel_, 0, Qt::AlignCenter);

  // "默认"标签
  if (style.isDefault) {
    badgeLabel_= new QLabel (tr ("Default"), this);
    badgeLabel_->setObjectName ("style-card-badge");
    badgeLabel_->setAlignment (Qt::AlignCenter);
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
  mainLayout->setContentsMargins (32, 32, 32, 32);
  mainLayout->setSpacing (24);

  // 1. 样式选择区
  setupStyleCards (mainLayout);

  // 分隔线
  QFrame* line= new QFrame (this);
  line->setFrameShape (QFrame::HLine);
  line->setObjectName ("startup-tab-separator");
  mainLayout->addWidget (line);

  // 2. 最近文档区
  setupRecentDocs (mainLayout);

  mainLayout->addStretch ();
}

void
QtFilePage::setupStyleCards (QVBoxLayout* layout) {
  // 标题
  QLabel* title= new QLabel (tr ("Document Style"), this);
  title->setObjectName ("startup-tab-section-title");
  layout->addWidget (title);

  // 样式卡片容器（水平流式布局）
  QWidget*     cardsContainer= new QWidget (this);
  QHBoxLayout* cardsLayout   = new QHBoxLayout (cardsContainer);
  cardsLayout->setContentsMargins (0, 0, 0, 0);
  cardsLayout->setSpacing (16);
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
  layout->addWidget (title);

  // 最近文档列表
  recentList_= new QListWidget (this);
  recentList_->setObjectName ("recent-docs-list");
  recentList_->setFocusPolicy (Qt::NoFocus);
  recentList_->setContextMenuPolicy (Qt::CustomContextMenu);
  recentList_->setMaximumHeight (200);

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
  QString configDir=
      QStandardPaths::writableLocation (QStandardPaths::AppConfigLocation);
  QDir ().mkpath (configDir);
  return QDir (configDir).filePath ("recent_documents.json");
}

void
QtFilePage::loadRecentDocs () {
  recentDocs_.clear ();
  recentList_->clear ();

  QString filePath= getRecentDocsFilePath ();
  QFile   file (filePath);
  if (!file.open (QIODevice::ReadOnly)) {
    return;
  }

  QByteArray    data= file.readAll ();
  QJsonDocument doc = QJsonDocument::fromJson (data);
  if (!doc.isObject ()) {
    return;
  }

  QJsonObject obj        = doc.object ();
  QJsonArray  recentArray= obj["recent_documents"].toArray ();

  for (const auto& val : recentArray) {
    QJsonObject docObj= val.toObject ();
    RecentDoc   doc;
    doc.filePath= docObj["path"].toString ();
    doc.fileName= docObj["name"].toString ();
    if (doc.fileName.isEmpty ()) {
      doc.fileName= QFileInfo (doc.filePath).fileName ();
    }
    doc.openedAt=
        QDateTime::fromString (docObj["opened_at"].toString (), Qt::ISODate);
    recentDocs_.append (doc);
  }

  refreshRecentDocs ();
}

void
QtFilePage::saveRecentDocs () {
  QJsonArray recentArray;
  for (const auto& doc : recentDocs_) {
    QJsonObject obj;
    obj["path"]     = doc.filePath;
    obj["name"]     = doc.fileName;
    obj["opened_at"]= doc.openedAt.toString (Qt::ISODate);
    recentArray.append (obj);
  }

  QJsonObject root;
  root["recent_documents"]= recentArray;

  QJsonDocument doc (root);
  QFile         file (getRecentDocsFilePath ());
  if (file.open (QIODevice::WriteOnly)) {
    file.write (doc.toJson ());
  }
}

void
QtFilePage::refreshRecentDocs () {
  recentList_->clear ();

  for (const auto& doc : recentDocs_) {
    QString text= QString ("%1\n%2").arg (doc.fileName, doc.filePath);
    auto*   item= new QListWidgetItem (text);
    item->setData (Qt::UserRole, doc.filePath);
    item->setToolTip (doc.filePath);
    recentList_->addItem (item);
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
  refreshRecentDocs ();
}

void
QtFilePage::removeRecentDoc (const QString& path) {
  for (auto it= recentDocs_.begin (); it != recentDocs_.end (); ++it) {
    if (it->filePath == path) {
      recentDocs_.erase (it);
      saveRecentDocs ();
      refreshRecentDocs ();
      return;
    }
  }
}

/******************************************************************************
 * 事件处理
 ******************************************************************************/

void
QtFilePage::onRecentDocClicked (QListWidgetItem* item) {
  if (!item) return;

  QString path= item->data (Qt::UserRole).toString ();
  if (path.isEmpty ()) return;

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
