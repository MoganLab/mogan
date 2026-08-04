/******************************************************************************
 * MODULE      : RecentDocumentsSearchBridge.hpp
 * DESCRIPTION : 最近打开文档搜索的 QML 数据桥接。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/**
 * @file RecentDocumentsSearchBridge.hpp
 * @brief 最近文档搜索对话框的只读 QML 数据桥。
 */

#ifndef RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP
#define RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP

#include <QObject>
#include <QString>
#include <QVariantList>

/**
 * @brief 向 QML 暴露最近文档模型及已翻译文案。
 *
 * @details 候选列表在创建时生成并保持只读；筛选和选择状态由 QML 本地维护，
 * 避免搜索过程修改 Scheme 的最近文档记录。
 */
class RecentDocumentsSearchBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QVariantList documents READ documents CONSTANT)
  Q_PROPERTY (QString title READ title CONSTANT)
  Q_PROPERTY (QString placeholder READ placeholder CONSTANT)
  Q_PROPERTY (QString emptyText READ emptyText CONSTANT)

public:
  /**
   * @brief 从 Scheme 的最近文档记录创建桥。
   * @param parent Qt 对象所有者。
   */
  explicit RecentDocumentsSearchBridge (QObject* parent= nullptr);

  /**
   * @brief 使用指定模型创建桥，供 QML 加载测试注入稳定数据。
   * @param documents 候选项，每项包含 `name` 和 `path`。
   * @param title 已翻译的对话框标题。
   * @param placeholder 已翻译的搜索提示。
   * @param emptyText 已翻译的空结果提示。
   * @param parent Qt 对象所有者。
   */
  RecentDocumentsSearchBridge (QVariantList documents, QString title,
                               QString placeholder, QString emptyText,
                               QObject* parent= nullptr);

  const QVariantList& documents () const { return m_documents; }
  const QString&      title () const { return m_title; }
  const QString&      placeholder () const { return m_placeholder; }
  const QString&      emptyText () const { return m_emptyText; }

private:
  /**
   * @brief 将 Scheme 最近文档列表转换为 QML 候选模型。
   * @return 以规范化路径去重后的 `{name, path}` 列表。
   * @note 路径须经 Cork 到 UTF-8 转换，避免中文文件名在 QML 中乱码。
   */
  static QVariantList recent_documents ();

  QVariantList m_documents;
  QString      m_title;
  QString      m_placeholder;
  QString      m_emptyText;
};

#endif // defined RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP
