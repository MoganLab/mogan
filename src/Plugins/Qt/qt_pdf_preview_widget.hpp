
/******************************************************************************
 * MODULE     : qt_pdf_preview_widget.hpp
 * DESCRIPTION: PDF preview widget using MuPDF
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QT_PDF_PREVIEW_WIDGET_HPP
#define QT_PDF_PREVIEW_WIDGET_HPP

#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QSharedPointer>
#include <QSize>

/**
 * @brief PDF预览控件 - 可重用的PDF页面渲染组件
 *
 * 功能特性:
 * - 从URL或本地文件加载PDF
 * - 渲染指定页面和DPI
 * - 支持异步网络加载
 * - 错误处理与后备显示
 */
class QTPdfPreviewWidget : public QLabel {
  Q_OBJECT

public:
  explicit QTPdfPreviewWidget (QWidget* parent= nullptr);
  ~QTPdfPreviewWidget ();

  // 从URL加载PDF（异步）
  void loadFromUrl (const QString& url, int pageNumber= 0, int dpi= 150);

  // 从本地文件加载PDF（同步）
  bool loadFromFile (const QString& filePath, int pageNumber= 0, int dpi= 150);

  // 从字节数组加载PDF（同步）
  bool loadFromData (const QByteArray& data, int pageNumber= 0, int dpi= 150);

  // 从URL加载图片（异步）
  void loadImageFromUrl (const QString& url, const QSize& targetSize= QSize ());

  // 设置/获取目标DPI
  void setDpi (int dpi) { targetDpi_= dpi; }
  int  dpi () const { return targetDpi_; }

  // 设置/获取目标页码
  void setPageNumber (int page) { targetPage_= page; }
  int  pageNumber () const { return targetPage_; }

  // 状态
  bool    isLoading () const { return isLoading_; }
  bool    hasError () const { return hasError_; }
  QString errorString () const { return errorString_; }

  // 取消当前加载
  void cancelLoading ();

  // 清除预览并显示占位符
  void clearPreview (const QString& text= QString ());

signals:
  void loadingStarted ();
  void loadingFinished (bool success);
  void error (const QString& errorMessage);

private:
  // 加载类型枚举
  enum class LoadType { None, PDF, Image };

private slots:
  void onNetworkReplyFinished ();
  void onImageNetworkReplyFinished ();

private:
  // MuPDF渲染
  bool renderPdfPage (const QByteArray& data, int pageNumber, int dpi);

  // UI辅助函数
  void showLoading ();
  void showError (const QString& message);
  void setPreviewPixmap (const QPixmap& pixmap);

private:
  // 网络
  QNetworkAccessManager*  networkManager_;
  QPointer<QNetworkReply> currentReply_;

  // 设置
  int targetDpi_;
  int targetPage_;

  // 状态
  bool    isLoading_;
  bool    hasError_;
  QString errorString_;

  // 图片加载相关
  LoadType currentLoadType_;
  QSize    targetSize_;

  // 缓存key（URL或文件路径）
  QString currentKey_;

  // 默认尺寸
  static constexpr int DEFAULT_WIDTH = 550;
  static constexpr int DEFAULT_HEIGHT= 300;
  static constexpr int DEFAULT_DPI   = 150;
};

#endif // QT_PDF_PREVIEW_WIDGET_HPP
