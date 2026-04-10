
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
class QPushButton;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QButtonGroup;

/**
 * @brief 文档样式信息
 */
struct DocStyle {
  QString id;          // 样式ID: generic, beamer, book, exam, letter, article
  QString name;        // 显示名称
  QString description; // 描述
  bool    isDefault;   // 是否为默认样式
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
 * @brief 样式卡片部件
 */
class StyleCard : public QWidget {
  Q_OBJECT

public:
  explicit StyleCard (const DocStyle& style, QWidget* parent= nullptr);

  QString styleId () const { return styleId_; }
  void    setSelected (bool selected);
  bool    isSelected () const { return isSelected_; }

signals:
  void hovered (); // 悬停时触发（用于选中）
  void clicked (); // 单击时触发（用于打开）

protected:
  void enterEvent (QEnterEvent* event) override;
  void mousePressEvent (QMouseEvent* event) override;
  void paintEvent (QPaintEvent* event) override;

private:
  QString styleId_;
  bool    isSelected_= false;
  QLabel* iconLabel_ = nullptr;
  QLabel* nameLabel_ = nullptr;
  QLabel* badgeLabel_= nullptr; // "默认"标签
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

private:
  void onRecentDocClicked (QListWidgetItem* item);
  void onRecentDocContextMenu (const QPoint& pos);

private:
  void setupUI ();
  void setupStyleCards (QVBoxLayout* layout);
  void setupRecentDocs (QVBoxLayout* layout);
  void loadRecentDocs ();
  void saveRecentDocs ();
  void addRecentDoc (const QString& path);
  void removeRecentDoc (const QString& path);
  void createDocumentWithStyle (const QString& styleId);

  // 样式卡片相关
  QList<DocStyle>   styles_;
  QList<StyleCard*> styleCards_;
  StyleCard*        selectedCard_= nullptr;

  // 最近文档相关
  QList<RecentDoc> recentDocs_;
  QListWidget*     recentList_= nullptr;
};

#endif // QT_FILE_PAGE_HPP
