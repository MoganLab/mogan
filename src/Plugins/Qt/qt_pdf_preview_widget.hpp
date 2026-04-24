
/******************************************************************************
 * MODULE     : qt_pdf_preview_widget.hpp
 * DESCRIPTION: PDF preview widget using MuPDF with vector rendering
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QT_PDF_PREVIEW_WIDGET_HPP
#define QT_PDF_PREVIEW_WIDGET_HPP

#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QScrollArea>
#include <QSize>
#include <QWidget>

#include <mupdf/fitz.h>

// Forward declarations
class QPushButton;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QScrollArea;
class QFrame;

/**
 * @brief PDF预览控件 - 带悬停式翻页控制和矢量渲染
 *
 * 功能特性:
 * - 从URL或本地文件加载PDF
 * - MuPDF矢量渲染（任意缩放清晰）
 * - 悬停显示左右翻页按钮（圆形）
 * - 底部居中页码指示器
 * - 自适应页面宽高比
 * - 高分DPI渲染
 * - 支持异步网络加载
 * - PDF文件本地缓存
 */
class QTPdfPreviewWidget : public QWidget {
  Q_OBJECT

public:
  explicit QTPdfPreviewWidget (QWidget* parent= nullptr);
  ~QTPdfPreviewWidget ();

  // 从URL加载PDF（异步）
  void loadFromUrl (const QString& url, int dpi= 200);

  // 从本地文件加载PDF（同步）
  bool loadFromFile (const QString& filePath, int dpi= 200);

  // 从字节数组加载PDF（同步）
  bool loadFromData (const QByteArray& data, int dpi= 200);

  // 从URL加载图片（异步）- 单页，无翻页
  void loadImageFromUrl (const QString& url);

  // 设置/获取目标DPI
  void setDpi (int dpi) { targetDpi_= dpi; }
  int  dpi () const { return targetDpi_; }

  // 当前页码
  int pageNumber () const { return currentPage_; }
  int pageCount () const { return pageCount_; }

  // 状态
  bool    isLoading () const { return isLoading_; }
  bool    hasError () const { return hasError_; }
  QString errorString () const { return errorString_; }

  // 取消当前加载
  void cancelLoading ();

  // 清除预览并显示占位符
  void clearPreview (const QString& text= QString ());

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;
  void resizeEvent (QResizeEvent* event) override;

signals:
  void loadingStarted ();
  void loadingFinished (bool success);
  void error (const QString& errorMessage);
  void pageChanged (int pageNumber);

private:
  // 加载类型枚举
  enum class LoadType { None, PDF, Image };

private slots:
  void onNetworkReplyFinished ();
  void onImageNetworkReplyFinished ();
  void onConditionalReplyFinished (const QString& cachedFilePath, int dpi);
  void goToPreviousPage ();
  void goToNextPage ();
  void goToPage (int page);

private:
  // 网络响应处理
  void processNetworkReply (QPointer<QNetworkReply> reply);

  // MuPDF渲染
  bool renderCurrentPage ();
  bool renderPdfPage (const QByteArray& data, int pageNumber);

  // UI辅助函数
  void         setupUI ();
  QPushButton* createNavButton (const QString& text,
                                void (QTPdfPreviewWidget::*slot) ());
  void         updatePageControls ();
  void         updatePreviewSize ();
  void         showLoading ();
  void         showError (const QString& message);
  void         setPreviewPixmap (const QPixmap& pixmap);

  // 计算最佳预览尺寸（保持宽高比）
  QSize calculateOptimalSize (int availWidth, int availHeight) const;
  void  calculatePreviewDimensions (int availWidth, int availHeight,
                                    int& outWidth, int& outHeight) const;

  // 控件布局与交互
  void updateButtonPositions ();
  void setControlsVisible (bool visible);
  bool mouseInWidgetHierarchy () const;

  // UI组件
  QWidget*     previewContainer_; // 预览容器（包含预览图和悬停按钮）
  QLabel*      previewLabel_;     // 预览图（仅用于图片加载，PDF用矢量渲染）
  QPushButton* prevBtn_;          // 上一页按钮（左侧）
  QPushButton* nextBtn_;          // 下一页按钮（右侧）
  QLabel*      pageIndicator_;    // 页码显示（底部居中）

  // 网络
  QNetworkAccessManager*  networkManager_;
  QPointer<QNetworkReply> currentReply_;

  // 设置
  int targetDpi_;

  // PDF数据（用于翻页）
  QByteArray pdfData_;
  int        currentPage_;
  int        pageCount_;

  // PDF页面原始尺寸（用于计算宽高比）
  double pageAspectRatio_;

  // 状态
  bool    isLoading_;
  bool    hasError_;
  QString errorString_;

  // 图片加载相关
  LoadType currentLoadType_;

  // 缓存key（URL或文件路径）
  QString currentKey_;

  // 默认尺寸
  static constexpr int DEFAULT_DPI= 200;
};

#endif // QT_PDF_PREVIEW_WIDGET_HPP
