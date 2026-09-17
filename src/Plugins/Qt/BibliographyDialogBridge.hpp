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
#include "tree.hpp"
#include "url.hpp"

#include <QDialog>
#include <QObject>
#include <QQuickItem>
#include <QString>
#include <QVariantMap>
#include <QWidget>

tree bib_preview_style ();

class BibliographyDialogBridge : public QObject {
  Q_OBJECT

public:
  explicit BibliographyDialogBridge (QDialog* host, const QString& doc_dir,
                                     QWidget*   previewWidget,
                                     const url& preview_buf_url);

  void setPlaceholder (QQuickItem* placeholder, QQuickItem* rootItem);

  /**
   * @brief 刷新预览 QWidget 几何对齐到 QML 占位区域。
   */
  void updatePreviewGeometry ();

  /**
   * @brief 弹原生文件选择对话框（选择 .bib 文件）。
   * @param current 当前字段里的路径。
   * @return 用户选中的路径；取消返回空串。
   */
  Q_INVOKABLE QString browse (const QString& current);

  /**
   * @brief 检查 bib 文件并更新 tmfs 缓冲区。
   * @param file 文件路径（相对或绝对）。
   * @param style 参考文献样式（如 "tm-plain"）。
   * @return QVariantMap，包含 status(string), hint(string)。
   */
  Q_INVOKABLE QVariantMap requestPreview (const QString& file,
                                          const QString& style);

  /**
   * @brief 将绝对路径转为相对于当前文档所在目录的路径。
   */
  Q_INVOKABLE QString toRelativePath (const QString& fullPath);

  /**
   * @brief 设置预览 QWidget 的可见性（如下拉框展开时临时隐藏以防遮挡）。
   */
  Q_INVOKABLE void setPreviewVisible (bool visible);

private:
  void showPreview ();

  QDialog*    m_host;
  QString     m_doc_dir;
  QWidget*    m_previewWidget;
  url         m_preview_buf_url;
  QQuickItem* m_placeholder;
  QQuickItem* m_rootItem;
  bool        m_isValid;
};

#endif // defined BIBLIOGRAPHY_DIALOG_BRIDGE_HPP
