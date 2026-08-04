/******************************************************************************
 * MODULE      : PrintToFileBridge.hpp
 * DESCRIPTION : Native save-file access for the QML print-to-file dialog.
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file PrintToFileBridge.hpp
 *  @brief 选择打印为文件对话框的专用 QML bridge。
 *
 *  @par 设计
 *  通用 QmlDialogBridge 只维护模态对话框的提交与取消；原生文件选择属于打印
 *  领域交互，由本类隔离，避免其它对话框获得无关 API。
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
  explicit PrintToFileBridge (QDialog* host) : QObject (), m_host (host) {
    ASSERT (host != NULL, "PrintToFileBridge expects a valid QDialog host");
  }

  /**
   * @brief 打开系统保存文件对话框。
   * @param currentFileName 当前建议的输出路径。
   * @param format 输出格式标识，支持 pdf 与 postscript。
   * @param title 原生对话框标题。
   * @return 用户选择的系统路径；取消时返回空字符串。
   * @note 返回后重新激活宿主窗口，避免原生对话框关闭后 QML 模态窗落到后台。
   */
  Q_INVOKABLE QString chooseSaveFile (const QString& currentFileName,
                                      const QString& format,
                                      const QString& title);

private:
  QDialog* m_host;
};

#endif // defined PRINT_TO_FILE_BRIDGE_HPP
