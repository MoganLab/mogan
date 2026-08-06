/******************************************************************************
 * MODULE     : qt_pdf_outline_widget.hpp
 * DESCRIPTION: Dockable outline sidebar widget (PDF bookmarks or document ToC)
 * COPYRIGHT  : (C) 2026
 ******************************************************************************/

#ifndef QT_PDF_OUTLINE_WIDGET_HPP
#define QT_PDF_OUTLINE_WIDGET_HPP

#include <QDockWidget>
#include <QTreeWidget>

#include "qt_pdf_reader_widget.hpp"

class OutlineWidget : public QDockWidget {
  Q_OBJECT

public:
  explicit OutlineWidget (const QString& title, QWidget* parent= nullptr);

  void setOutline (const QVector<PdfOutlineItem>& outline);
  void setOutline (const QVector<OutlineItem>& outline);
  /** 从 Scheme 获取文档大纲并填充。返回值表示是否成功加载到内容。 */
  bool loadDocumentOutline ();
  void clear ();
  bool hasContent () const;

signals:
  /** 用户点击大纲条目。target 含义由连接方解释：
   *  PDF 模式 → 页码（int 转 QString），-1 表示无效；
   *  编辑器模式 → 文档树路径（如 "0:1:2"）。 */
  void outlineActivated (const QString& target);

private:
  void buildTree (const QVector<PdfOutlineItem>& items,
                  QTreeWidgetItem*               parent);
  void buildTree (const QVector<OutlineItem>& items, QTreeWidgetItem* parent);

  QTreeWidget* tree_;
};

#endif // QT_PDF_OUTLINE_WIDGET_HPP
