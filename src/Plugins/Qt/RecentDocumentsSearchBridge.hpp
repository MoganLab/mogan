/******************************************************************************
 * MODULE      : RecentDocumentsSearchBridge.hpp
 * DESCRIPTION : 最近打开文档搜索 QML 对话框的独立 bridge。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file RecentDocumentsSearchBridge.hpp
 *  @brief 最近文档搜索 QML 对话框的 C++↔QML bridge。
 *
 * @par 设计
 * - @b 只读数据：构造时读取 Scheme 最近文档并去重。路径保持 UTF-8 系统编码（同
 *   `qt_scheme_quote_utf8` 惯例），QML 只读候选模型和翻译文案；筛选与选择状态
 *   留在 QML 本地。
 * - @b 确认回流：open() 保存用户显式选择的路径并结束模态；closeBridge 处理取消
 *   、Esc 与窗口拖动。
 *
 * @note bridge 不挂 QObject parent，调用方在 exec 返回后删除；宿主 QDialog 只
 *       在 open() 期间借用。
 */

#ifndef RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP
#define RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP

#include <QDialog>
#include <QObject>
#include <QString>
#include <QVariantList>

/**
 * @brief 向 QML 暴露最近文档模型和确认动作。
 */
class RecentDocumentsSearchBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QVariantList documents READ documents CONSTANT)
  Q_PROPERTY (QString title READ title CONSTANT)
  Q_PROPERTY (QString placeholder READ placeholder CONSTANT)
  Q_PROPERTY (QString emptyText READ emptyText CONSTANT)

public:
  /**
   * @brief 从 Scheme 的最近文档记录创建 bridge。
   * @param host 宿主 QDialog；open() 调其 done(Accepted)，不接管其生命期。
   */
  explicit RecentDocumentsSearchBridge (QDialog* host);

  const QVariantList& documents () const { return m_documents; }
  const QString&      title () const { return m_title; }
  const QString&      placeholder () const { return m_placeholder; }
  const QString&      emptyText () const { return m_emptyText; }
  QString             selectedPath () const { return m_selectedPath; }

  /**
   * @brief 记录用户显式选中的路径并结束对话框。
   * @param path QML 当前选中候选项的原始路径。
   */
  Q_INVOKABLE void open (const QString& path);

private:
  /**
   * @brief 将 Scheme 最近文档列表转换为 QML 候选模型。
   * @return 以规范化路径去重后的 {name, path} 列表。
   * @note url->system 已返回 UTF-8 系统路径，不能经 Cork 转换（会破坏中文文件名
   *       ），同 qt_scheme_quote_utf8 的 UTF-8 通道惯例。
   */
  static QVariantList recent_documents ();

  QVariantList m_documents;
  QDialog*     m_host;
  QString      m_title;
  QString      m_placeholder;
  QString      m_emptyText;
  QString      m_selectedPath;
};

#endif // defined RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP
