/******************************************************************************
 * MODULE      : VersionDialogBridge.cpp
 * DESCRIPTION : 版本弹窗 QML bridge 实现（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "VersionDialogBridge.hpp"

#include "converter.hpp"    // cork_to_utf8
#include "qt_utilities.hpp" // utf8_to_qstring

/**
 * @brief 把多行消息按 '\n' 分行并逐行转 UTF-8。
 * @details mogan string 内部是 Cork 编码，统一 cork_to_utf8 转 UTF-8 再 to
 * QString——不用 to_qstring（其 looks_utf8/looks_ascii 启发式对纯 Cork 中文
 * 不稳定）。
 */
static QStringList
message_lines (const string& message) {
  QStringList lines;
  int         start= 0;
  for (int i= 0; i < N (message); i++) {
    if (message[i] != '\n') continue;
    lines << utf8_to_qstring (cork_to_utf8 (message (start, i)));
    start= i + 1;
  }
  lines << utf8_to_qstring (cork_to_utf8 (message (start, N (message))));
  return lines;
}

VersionDialogBridge::VersionDialogBridge (QDialog* host, string title,
                                          string             message,
                                          const QStringList& button_labels)
    : m_host (host), m_title (utf8_to_qstring (cork_to_utf8 (title))),
      m_lines (message_lines (message)), m_buttonLabels (button_labels) {
  ASSERT (host != NULL, "VersionDialogBridge expects a valid QDialog host");
}

void
VersionDialogBridge::confirm () {
  m_host->done (QDialog::Accepted);
}
