
/******************************************************************************
 * MODULE     : qt_file_page.hpp
 * DESCRIPTION: File page for startup tab with style cards and recent documents
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_FILE_PAGE_HPP
#define QT_FILE_PAGE_HPP

#include <QDateTime>
#include <QList>
#include <QString>
#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QPushButton;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QButtonGroup;
class QShowEvent;
class QResizeEvent;

/**
 * @brief 文档样式信息（支持空白文档和快捷模板）
 */
struct DocStyle {
  QString id;          // 样式ID
  QString name;        // 显示名称
  QString description; // 描述
  QString templateId;  // 对应TemplateManager中的模板ID（空表示空白文档）
};

/**
 * @brief 最近文档条目
 */
struct RecentDoc {
  QString   fileName; // 文件名
  QString   filePath; // 完整路径
  QDateTime openedAt; // 最后打开时间
};

/**
 * @brief 样式卡片部件（支持图标模式和缩略图模板模式）
 */
class StyleCard : public QWidget {
  Q_OBJECT

public:
  explicit StyleCard (const DocStyle& style, QWidget* parent= nullptr);
  ~StyleCard ();

  QString styleId () const { return styleId_; }
  QString templateId () const { return templateId_; }
  bool    isTemplate () const { return isTemplate_; }

signals:
  void clicked ();

public:
  void loadThumbnail (const QString& url);

protected:
  void mousePressEvent (QMouseEvent* event) override;
  void paintEvent (QPaintEvent* event) override;

private:
  void setupIconMode (const DocStyle& style);
  void setupThumbnailMode (const DocStyle& style);

  QString styleId_;
  QString templateId_;
  bool    isTemplate_= false;

  // Icon mode
  QLabel* iconLabel_= nullptr;
  QLabel* nameLabel_= nullptr;

  // Thumbnail mode
  QLabel* thumbnailLabel_= nullptr;
  QLabel* titleLabel_    = nullptr;
};

/**
 * @brief 文件页面 - 包含样式选择和最近文档
 */
class QtFilePage : public QWidget {
  Q_OBJECT

public:
  explicit QtFilePage (QWidget* parent= nullptr);
  ~QtFilePage ();

  void refreshRecentDocs ();
  void addRecentDoc (const QString& path);

protected:
  void showEvent (QShowEvent* event) override;
  void resizeEvent (QResizeEvent* event) override;

private:
  void onRecentDocClicked (QListWidgetItem* item);
  void onRecentDocContextMenu (const QPoint& pos);

private:
  void setupUI ();
  void setupStyleCards (QVBoxLayout* layout);
  void setupRecentDocs (QVBoxLayout* layout);
  void renderRecentDocs ();
  void loadRecentDocs ();
  void saveRecentDocs ();
  void removeRecentDoc (const QString& path);
  void clearAllRecentDocs ();
  void createDocumentWithStyle (const QString& styleId);
  void createDocumentFromTemplate (const QString& templateId);
  void refreshTemplateThumbnails ();

  // 样式卡片相关
  QList<DocStyle>   styles_;
  QList<StyleCard*> styleCards_;
  QWidget*          cardsContainer_= nullptr;
  QGridLayout*      cardsLayout_   = nullptr;
  void              rearrangeStyleCards ();

  // 最近文档相关
  QList<RecentDoc> recentDocs_;
  QListWidget*     recentList_= nullptr;
};

#endif // QT_FILE_PAGE_HPP
