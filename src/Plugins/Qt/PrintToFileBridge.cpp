/******************************************************************************
 * MODULE      : PrintToFileBridge.cpp
 * DESCRIPTION : Native save-file access for the QML print-to-file dialog.
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file PrintToFileBridge.cpp
 *  @brief 选择打印为文件对话框的原生保存路径实现。
 */

#include "PrintToFileBridge.hpp"

#include <QFileDialog>

QString
PrintToFileBridge::chooseSaveFile (const QString& currentFileName,
                                   const QString& format,
                                   const QString& title) {
  const QString filter=
      format == "pdf" ? "PDF files (*.pdf)" : "PostScript files (*.ps)";
  const QString selected=
      QFileDialog::getSaveFileName (m_host, title, currentFileName, filter);
  if (m_host) {
    m_host->raise ();
    m_host->activateWindow ();
  }
  return selected;
}
