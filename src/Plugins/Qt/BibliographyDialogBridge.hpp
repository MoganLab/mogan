/******************************************************************************
 * MODULE      : BibliographyDialogBridge.hpp
 * DESCRIPTION : 「插入/修改参考文献」QML 对话框的 C++↔QML 桥。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef BIBLIOGRAPHY_DIALOG_BRIDGE_HPP
#define BIBLIOGRAPHY_DIALOG_BRIDGE_HPP

#include "boot.hpp"

#include <QDialog>
#include <QObject>
#include <QString>
#include <QVariantMap>

class BibliographyDialogBridge : public QObject {
  Q_OBJECT

public:
  explicit BibliographyDialogBridge (QDialog* host, const QString& doc_dir);

  /**
   * @brief 弹原生文件选择对话框（选择 .bib 文件）。
   * @param current 当前字段里的路径。
   * @return 用户选中的路径；取消返回空串。
   */
  Q_INVOKABLE QString browse (const QString& current);

  /**
   * @brief 检查 bib 文件并计算光栅化预览。
   * @param file 文件路径（相对或绝对）。
   * @param style 参考文献样式（如 "tm-plain"）。
   * @return QVariantMap，包含 valid(bool), status(string), hint(string),
   * preview(string)。
   */
  Q_INVOKABLE QVariantMap requestPreview (const QString& file,
                                          const QString& style);

  /**
   * @brief 将绝对路径转为相对于当前文档所在目录的路径。
   */
  Q_INVOKABLE QString toRelativePath (const QString& fullPath);

  /**
   * @brief 将相对路径转为绝对路径。
   */
  Q_INVOKABLE QString toAbsolutePath (const QString& relPath);

private:
  QDialog* m_host;
  QString  m_doc_dir;
};

#endif // defined BIBLIOGRAPHY_DIALOG_BRIDGE_HPP
