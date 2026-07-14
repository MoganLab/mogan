/******************************************************************************
 * MODULE      : ParagraphFormatBridge.hpp
 * DESCRIPTION : 段落格式 QML 对话框的 C++↔QML 桥。透传、不持段落状态：
 *               scheme 侧 paragraph-format-*
 *facade（paragraph-format-widgets.scm） 经 specsKey 句柄持有 specs，bridge 每次
 *Q_INVOKABLE 调用都带上 specsKey，拼 scheme 串经 eval_scheme 调
 *facade，把结果转 QML 可消费的类型。参考 FontSelectorBridge。 COPYRIGHT   : (C)
 *2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file ParagraphFormatBridge.hpp
 *  @brief QML 段落格式 bridge：把 QML 的请求转发到 scheme facade。
 *
 * @par 设计
 * - @b 透传无状态：段落参数真相源在文档（get-env 读、make-multi-line-with
 * 写）， bridge 除 specsKey 外不持有任何数据。
 * - @b live 写回：setPara 每次 make-multi-line-with 实时写回文档，主窗口 live
 *   重排段落（run_modal_qml_dialog 非阻塞模态保证主窗口 paint）。
 * - @b 快照撤销：Cancel/重置经打开时快照（paragraph-snapshot）写回撤销，OK
 * 落定。
 *
 * @note 生命期：host 堆分配（run_modal_qml_dialog 内 new + WA_DeleteOnClose），
 *   bridge 不挂 parent；submit/cancel 调 close() → WA_DeleteOnClose 触发 host
 *   析构 → destroyed 信号 deleteLater 掉 bridge。
 */

#ifndef PARAGRAPH_FORMAT_BRIDGE_HPP
#define PARAGRAPH_FORMAT_BRIDGE_HPP

#include "boot.hpp"

#include <QDialog>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QWindow>

class ParagraphFormatBridge : public QObject {
  Q_OBJECT

public:
  /**
   * @brief 构造桥对象。
   * @param host 宿主 QDialog（submit/cancel 调其 close()），不挂 parent。
   * @param specsKey scheme paragraph-specs-registry 的 int 句柄。
   */
  ParagraphFormatBridge (QDialog* host, int specsKey)
      : QObject (), m_host (host), m_specsKey (specsKey) {
    ASSERT (host != NULL, "ParagraphFormatBridge expects a valid QDialog host");
  }

  // ---- meta（基础/高级两 tab 的字段表）----
  /// 基础 tab 字段元数据：(label, options, var, value, editable) ×N。
  Q_INVOKABLE QVariantList basicMeta ();
  /// 高级 tab 字段元数据。
  Q_INVOKABLE QVariantList advancedMeta ();

  // ---- live 写回 ----
  /// 设单参数（make-multi-line-with 实时写回），返回写回后的新值。
  Q_INVOKABLE QString setPara (const QString& var, const QString& val);

  // ---- 动作 ----
  /// OK：paragraph-format-commit 落定，close() 关闭对话框。
  Q_INVOKABLE void submit ();
  /// Cancel：paragraph-format-cancel 经快照写回撤销，close() 关闭。
  Q_INVOKABLE void cancel ();
  /// 重置：paragraph-format-restore 快照撤销（不关窗，留在对话框继续调）。
  Q_INVOKABLE void reset ();

  /// 固定 UI 文案的翻译 + 行间距预设按钮表，QML 一次性拉取。
  Q_INVOKABLE QVariantMap uiLabels ();

  /**
   * @brief 拖动无边框窗口，委托底层 QWindow 系统级移动。
   */
  Q_INVOKABLE void startMove () {
    if (m_host && m_host->windowHandle ())
      m_host->windowHandle ()->startSystemMove ();
  }

private:
  /// 拼 `(paragraph-format-meta key which)` 求 list of assoc → QVariantList of
  /// map。
  QVariantList evalMeta (const string& which);

  QDialog* m_host;
  int      m_specsKey;
};

#endif // defined PARAGRAPH_FORMAT_BRIDGE_HPP
