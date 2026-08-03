/******************************************************************************
 * MODULE      : RecentDocumentsSearchBridge.cpp
 * DESCRIPTION : 最近打开文档搜索的 QML 数据桥接实现。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "RecentDocumentsSearchBridge.hpp"

RecentDocumentsSearchBridge::RecentDocumentsSearchBridge (
    QVariantList documents, QString title, QString placeholder,
    QString emptyText, QStringList buttonLabels, QObject* parent)
    : QObject (parent), m_documents (documents), m_title (title),
      m_placeholder (placeholder), m_emptyText (emptyText),
      m_buttonLabels (buttonLabels) {}
