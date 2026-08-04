/******************************************************************************
 * MODULE      : VersionDialogBridge.hpp
 * DESCRIPTION : 版本弹窗 QML 对话框的 C++↔QML 桥（只读数据 + 确认）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file VersionDialogBridge.hpp
 *  @brief QML 版本弹窗 bridge：向 QML 暴露标题、文本行、按钮文案并回传确认。
 *
 * @par 设计
 * - @b 只读数据：标题与多行正文由 Scheme 侧翻译好（Cork 编码），构造时在
 *   VersionDialogBridge.cpp 内经 cork_to_utf8 转 UTF-8 再 to QString，QML 侧为
 *   CONSTANT 只读属性，不持有弹窗状态。
 * - @b 确认回流：confirm() 以 QDialog::Accepted 结束 exec 模态（退出码 1），由
 *   cpp_version_dialog 映射为「确定」。窗口拖动 / ESC 取消等外壳行为不在本桥——
 *   DialogShell 经独立的 closeBridge（QmlDialogBridge）承担，二者解耦。
 *
 * @note 生命期：bridge 不挂 parent，由调用方（cpp_version_dialog）在 exec 后
 *       delete；host destroyed 信号 deleteLater 自清。
 */

#ifndef VERSION_DIALOG_BRIDGE_HPP
#define VERSION_DIALOG_BRIDGE_HPP

#include "boot.hpp"
#include "string.hpp"

#include <QDialog>
#include <QObject>
#include <QString>
#include <QStringList>

class VersionDialogBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QString title READ title CONSTANT)
  Q_PROPERTY (QStringList lines READ lines CONSTANT)
  Q_PROPERTY (QStringList buttonLabels READ buttonLabels CONSTANT)

public:
  /**
   * @brief 构造桥对象。
   * @param host 宿主 QDialog（confirm 调其 done(Accepted)），不挂 parent。
   * @param title 弹窗标题（Cork 编码，构造时转 UTF-8）。
   * @param message 多行正文，按 '\n' 分行并逐行转 UTF-8。
   * @param button_labels 已翻译的按钮文案（经 translate_buttons）。
   */
  VersionDialogBridge (QDialog* host, string title, string message,
                       const QStringList& button_labels);

  QString     title () const { return m_title; }
  QStringList lines () const { return m_lines; }
  QStringList buttonLabels () const { return m_buttonLabels; }

  Q_INVOKABLE void confirm ();

private:
  QDialog*    m_host;
  QString     m_title;
  QStringList m_lines;
  QStringList m_buttonLabels;
};

#endif // defined VERSION_DIALOG_BRIDGE_HPP
