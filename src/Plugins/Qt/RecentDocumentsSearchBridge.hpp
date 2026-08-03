/******************************************************************************
 * MODULE      : RecentDocumentsSearchBridge.hpp
 * DESCRIPTION : 最近打开文档搜索的 QML 数据桥接。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP
#define RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class RecentDocumentsSearchBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QVariantList documents READ documents CONSTANT)
  Q_PROPERTY (QString title READ title CONSTANT)
  Q_PROPERTY (QString placeholder READ placeholder CONSTANT)
  Q_PROPERTY (QString emptyText READ emptyText CONSTANT)
  Q_PROPERTY (QStringList buttonLabels READ buttonLabels CONSTANT)

public:
  RecentDocumentsSearchBridge (QVariantList documents, QString title,
                               QString placeholder, QString emptyText,
                               QStringList buttonLabels,
                               QObject*    parent= nullptr);

  const QVariantList& documents () const { return m_documents; }
  const QString&      title () const { return m_title; }
  const QString&      placeholder () const { return m_placeholder; }
  const QString&      emptyText () const { return m_emptyText; }
  const QStringList&  buttonLabels () const { return m_buttonLabels; }

private:
  QVariantList m_documents;
  QString      m_title;
  QString      m_placeholder;
  QString      m_emptyText;
  QStringList  m_buttonLabels;
};

#endif // defined RECENT_DOCUMENTS_SEARCH_BRIDGE_HPP
