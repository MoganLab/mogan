/******************************************************************************
 * MODULE     : qt_pdf_outline_widget.hpp
 * DESCRIPTION: Dockable outline sidebar widget (PDF bookmarks or document ToC)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#ifndef QT_PDF_OUTLINE_WIDGET_HPP
#define QT_PDF_OUTLINE_WIDGET_HPP

#include <QDockWidget>
#include <QQuickWidget>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSize>
#include <QVector>

#include "OutlineBridge.hpp"
#include "qt_pdf_reader_widget.hpp"

class QTimer;

class OutlineWidget : public QDockWidget {
  Q_OBJECT

public:
  explicit OutlineWidget (const QString& title, QWidget* parent= nullptr);
  ~OutlineWidget () override= default;

  void setOutline (const QVector<PdfOutlineItem>& outline);
  void setOutline (const QVector<OutlineItem>& outline);
  /** 从 Scheme 获取文档大纲并填充。返回值表示是否成功加载到内容。 */
  bool loadDocumentOutline ();
  void clear ();
  bool hasContent () const;

  OutlineBridge* bridge () const { return bridge_; }
  QQuickWidget*  quickWidget () const { return quick_; }

  QSize sizeHint () const override;

signals:
  /** 用户点击大纲条目。target 含义由连接方解释：
   *  PDF 模式 → 页码（int 转 QString），-1 表示无效；
   *  编辑器模式 → 文档树路径（如 "0:1:2"）。 */
  void outlineActivated (const QString& target);

protected:
  void resizeEvent (QResizeEvent* event) override;
  void showEvent (QShowEvent* event) override;

private:
  OutlineBridge* bridge_;
  QQuickWidget*  quick_;
  // 宽度偏好防抖：拖动分隔条期间不写盘，停顿后落盘一次
  QTimer* widthSaveTimer_;
  int     pendingWidth_= 0;
  // QML 源延迟到首次显示再加载，避免主窗口启动时无谓解析
  bool qmlLoaded_= false;
};

#endif // QT_PDF_OUTLINE_WIDGET_HPP
