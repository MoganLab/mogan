/******************************************************************************
 * MODULE      : PrintToFileBridge.hpp
 * DESCRIPTION : 「打印页面选择到文件」QML 对话框的 C++↔QML 桥。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file PrintToFileBridge.hpp
 *  @brief 打印到文件 QML 对话框 bridge。
 *
 * @par 职责
 * 本对话框是一次性提交（exec 模态，OK 整包提交给 cpp_print_to_file_dialog），
 * 本身无 live 写回、无 scheme facade 往返。唯一需要 C++ 参与的交互是「文件路径」
 * 字段的 **Browse** 按钮——弹原生保存文件对话框（QFileDialog::getSaveFileName），
 * 让用户导航文件系统选存储位置后再回填 QML 字段。
 *
 * @par browse 语义
 * QML 点 Browse 时调用 browse(current)：以 current（当前字段里的路径）为初值打开
 * 原生保存对话框，返回用户选中的路径（QString）；用户取消返回空串，QML 不改字段。
 * file 过滤按对话框语义（打印目标为 PostScript 时建议 ``*.ps``/``*.pdf``）。
 *
 * @note 生命期：run_qml_dialog（exec）内桥对象在注入回调里 new、exec 结束后 delete
 *（同 cpp_form_dialog 的 bridge 处理）。不挂 QObject parent，避免随宿主析构提前
 * 释放。
 */

#ifndef PRINT_TO_FILE_BRIDGE_HPP
#define PRINT_TO_FILE_BRIDGE_HPP

#include "boot.hpp"

#include <QDialog>
#include <QObject>
#include <QString>

class PrintToFileBridge : public QObject {
  Q_OBJECT

public:
  /**
   * @brief 构造桥对象。
   * @param host 宿主 QDialog（exec 模态窗口；作原生文件对话框的 parent），不挂
   * parent。
   */
  explicit PrintToFileBridge (QDialog* host) : QObject (), m_host (host) {
    ASSERT (host != NULL, "PrintToFileBridge expects a valid QDialog host");
  }

  /**
   * @brief 弹原生保存文件对话框，返回用户选中的路径。
   * @param current 当前字段里的路径（初值，用于预填与默认目录）。
   * @return 用户选中的路径；取消返回空串。
   */
  Q_INVOKABLE QString browse (const QString& current);

private:
  QDialog* m_host;
};

#endif // defined PRINT_TO_FILE_BRIDGE_HPP
