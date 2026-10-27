/******************************************************************************
 * MODULE     : qt_attachment_converter.hpp
 * DESCRIPTION: 文档类附件的无头自转换（→ PDF）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_ATTACHMENT_CONVERTER_HPP
#define QT_ATTACHMENT_CONVERTER_HPP

#include <QObject>
#include <QString>

class QProcess;

/**
 * @brief 把 mogan 文档（.tmu/.tm/.stem）用宿主自身的无头能力导出为 PDF。
 *
 * 附件管线只认 PDF 等既有格式，文档格式直接送入会提取失败。选择文档类
 * 附件时先经本类转成 PDF，再以该 PDF 走既有 ingest，插件侧零改动。
 *
 * 转换通过 QProcess 启动宿主自身二进制，参数为 -c <输入> <输出.pdf>：
 * init_texmacs.cpp 对 -c 自动开启 headless（research.cpp），转换分支末尾
 * 自动追加 (quit-TeXmacs)，故无需另传 -q，以进程退出判定完成。
 *
 * 临时产物放在附件插件缓存目录同级的 convert/<uuid>/ 下，仅供 ingest
 * 使用；宿主在附件插件 ready 或 error 终态删除，不长期保留。
 */
class AttachmentConverter : public QObject {
  Q_OBJECT

public:
  explicit AttachmentConverter (QObject* parent= nullptr) : QObject (parent) {}

  /**
   * @brief 发起一次转换。
   *
   * @param sourcePath 待转换文档的绝对路径
   *
   * 成功与失败都以异步回调通知，避免阻塞 UI：转换期间附件按钮保持处理中
   * 动画。二者只触发其一。
   */
  void start (const QString& sourcePath);

signals:
  /// 转换成功，pdfPath 为临时 PDF 的绝对路径（调用方负责 ingest）
  void converted (const QString& pdfPath);
  /// 转换失败（超时、退出码非零、产物缺失等），reason 供 toast 展示
  void failed (const QString& reason);

private:
  /**
   * @brief 释放内部 QProcess（成功与失败收口的共用一步）。
   *
   * settled_ 已挡住所有后续信号，deleteLater 抑制未投递事件，无需
   * 额外 disconnect；调用后 process_ 为 nullptr。
   */
  void settleProcess ();

  /**
   * @brief 统一失败收口：清理临时目录、发 failed 信号，只生效一次。
   *
   * 超时、errorOccurred、finished 三条路径可能先后触发（超时 kill 后进程
   * 仍会发 finished），故用 settled_ 保证信号只发一次、临时目录只删一次。
   *
   * @param tempDir 本次转换的临时目录
   * @param reason  失败原因，供调用方 toast 展示
   */
  void finishWithFailure (const QString& tempDir, const QString& reason);

  QProcess* process_= nullptr;
  bool      settled_= false;
};

#endif // QT_ATTACHMENT_CONVERTER_HPP