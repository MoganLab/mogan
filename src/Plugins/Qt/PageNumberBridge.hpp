/******************************************************************************
 * MODULE      : PageNumberBridge.hpp
 * DESCRIPTION : 页码设置 QML 对话框的 C++↔QML 桥（无状态透传）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file PageNumberBridge.hpp
 *  @brief QML 页码设置 bridge：把 QML 的请求转发到 scheme facade。
 *
 * @par 设计
 * - @b 无状态透传：页码设置是文档级的，bridge 不持有规则数据，每次 Q_INVOKABLE
 *   调用都拼 scheme 串经 eval_scheme 调 facade。
 * - @b 本地暂存 + OK 一次性提交（FormDialog / Preferences 模式）：QML
 * 打开时拉一次 meta 建本地 rules 快照、改动只改 QML 本地、OK 时 submit
 * 一次性写入、Cancel 丢弃（取消/窗口拖动复用共用 closeBridge，本桥不重复）。
 * - @b eval_scheme 调 facade：meta() 拼 `(pn-qml-meta)`、submit() 拼
 *   `(pn-qml-submit '<rules>)`。
 * - @b formatNumber：预览数字格式化走排版引擎 number 原语同源的 lolly
 *   转换函数，避免 QML 侧重复实现罗马/汉字数字。
 *
 * @note 生命期：走 run_qml_dialog（exec 阻塞模态）。bridge 不挂 parent，
 *   host destroyed 信号 deleteLater 自清。
 */

#ifndef PAGE_NUMBER_BRIDGE_HPP
#define PAGE_NUMBER_BRIDGE_HPP

#include "boot.hpp"

#include <QDialog>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class PageNumberBridge : public QObject {
  Q_OBJECT

public:
  /**
   * @brief 构造桥对象。
   * @param host 宿主 QDialog，不挂 parent。
   */
  explicit PageNumberBridge (QDialog* host) : QObject (), m_host (host) {
    ASSERT (host != NULL, "PageNumberBridge expects a valid QDialog host");
  }

  /// 一次性拉全部数据（scheme pn-qml-meta 返回：total、rules、labels）。
  Q_INVOKABLE QVariantMap meta ();
  /// 一次性提交规则列表并关窗。
  Q_INVOKABLE void submit (const QVariantList& rules);
  /// 页码数字格式化，与排版引擎 number 原语同源（lolly to_roman/to_hanzi）。
  Q_INVOKABLE QString formatNumber (int n, const QString& style);

private:
  QDialog* m_host;
};

#endif // defined PAGE_NUMBER_BRIDGE_HPP
