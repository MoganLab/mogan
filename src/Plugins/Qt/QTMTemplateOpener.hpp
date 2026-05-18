
/******************************************************************************
 * MODULE     : QTMTemplateOpener.hpp
 * DESCRIPTION: Unified template opener for HomePage and TemplatePage
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTMTEMPLATEOPENER_HPP
#define QTMTEMPLATEOPENER_HPP

#include <QObject>
#include <QPointer>

class QProgressDialog;
class QWidget;
class TemplateManager;

/**
 * @brief 统一的模板打开器
 *
 * 封装打开模板的核心逻辑：
 * - 本地模板：复制到 Documents 并立即打开
 * - 远程模板：下载（带进度对话框）→ 复制 → 打开
 *
 * 使用示例（HomePage 一键打开）：
 * @code
 * QTMTemplateOpener opener(this);
 * opener.openTemplate("elegantbook");
 * @endcode
 *
 * 使用示例（TemplatePage 预览后打开）：
 * @code
 * QTMTemplateOpener opener(this);
 * opener.openTemplate("nsfc-ysf-c");
 * @endcode
 *
 * @note openTemplate() 为同步风格：本地模板立即完成；
 * 远程模板会阻塞（通过 QProgressDialog 保持事件循环响应），
 * 直到下载完成。completed() / failed() 信号会在 openTemplate()
 * 返回前发出。
 */
class QTMTemplateOpener : public QObject {
  Q_OBJECT

public:
  explicit QTMTemplateOpener (QWidget* parent= nullptr);
  ~QTMTemplateOpener ();

  QTMTemplateOpener (const QTMTemplateOpener&)           = delete;
  QTMTemplateOpener& operator= (const QTMTemplateOpener&)= delete;

  /**
   * @brief 打开模板（本地或远程）
   *
   * 若模板在本地可用，则直接打开；
   * 否则显示进度对话框并先下载。
   *
   * @param templateId 模板 ID
   * @return 成功返回 true，失败返回 false
   */
  bool openTemplate (const QString& templateId);

  /**
   * @brief 检查模板是否在本地可用
   */
  bool isAvailableLocally (const QString& templateId);

signals:
  /**
   * @brief 下载进度更新
   */
  void downloadProgress (const QString& templateId, qint64 bytesReceived,
                         qint64 bytesTotal);

  /**
   * @brief 模板打开成功
   * @param templateId   模板 ID
   * @param documentPath Documents 中的文档路径
   */
  void completed (const QString& templateId, const QString& documentPath);

  /**
   * @brief 打开模板失败
   * @param templateId 模板 ID
   * @param error      可读错误信息（用户取消时为空）
   */
  void failed (const QString& templateId, const QString& error);

private slots:
  void onDownloadProgress (const QString& templateId, qint64 bytesReceived,
                           qint64 bytesTotal);

private:
  bool openLocalTemplate_ (const QString& templateId);
  bool startDownload_ (const QString& templateId);
  bool loadFromLocalPath_ (const QString& templateId, const QString& localPath,
                           const QString& templateName);
  void cleanupProgressDialog_ ();
  void showError_ (const QString& message);
  void resetState_ ();

  QWidget*                  parent_;
  TemplateManager*          templateManager_;
  QPointer<QProgressDialog> progressDialog_;

  QString currentTemplateId_;
  bool    downloadCancelledByUser_= false;
};

#endif // QTMTEMPLATEOPENER_HPP
