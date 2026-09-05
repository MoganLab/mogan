/******************************************************************************
 * MODULE      : PrintToFileBridge.cpp
 * DESCRIPTION : 「打印页面选择到文件」QML 对话框 bridge 实现（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "PrintToFileBridge.hpp"

#include <QFileDialog>

QString
PrintToFileBridge::browse (const QString& current) {
  // 有路径则预填完整文件名（目录 + 建议名）；空则走系统默认目录。
  QString start= current;
  QString path=
      QFileDialog::getSaveFileName (m_host, QString (), start, m_filter);
  return path;
}
