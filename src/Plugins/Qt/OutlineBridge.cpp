/******************************************************************************
 * MODULE      : OutlineBridge.cpp
 * DESCRIPTION : C++ ↔ QML bridge for OutlineSidebar (PDF & Document ToC)
 * COPYRIGHT   : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "OutlineBridge.hpp"

OutlineBridge::OutlineBridge (QObject* parent) : QObject (parent) {}

void
OutlineBridge::setCurrentId (const QString& id) {
  if (m_currentId != id) {
    m_currentId= id;
    emit currentIdChanged ();
  }
}

QVariantMap
OutlineBridge::makeNode (const QString& id, const QString& title,
                         const QString& target, const QString& page, int level,
                         const QVariantList& children) {
  QVariantMap map;
  map["id"]      = id;
  map["title"]   = title;
  map["target"]  = target;
  map["page"]    = page;
  map["level"]   = level;
  map["children"]= children;
  return map;
}

QVariantMap
OutlineBridge::convertPdfItem (const PdfOutlineItem& item, int level,
                               const QString& id) {
  // PdfOutlineItem::page 是 0-based（fz_resolve_link），跳转需要 1-based
  int     pageOneBased= (item.page >= 0) ? item.page + 1 : -1;
  QString pageStr=
      (pageOneBased >= 0) ? QString::number (pageOneBased) : QString ();

  QVariantList children;
  for (int i= 0; i < item.children.size (); ++i) {
    QString childId= id + "/" + QString::number (i);
    children.append (convertPdfItem (item.children[i], level + 1, childId));
  }
  // PDF 模式下 target 与 page 同为 1-based 页码
  return makeNode (id, item.title, pageStr, pageStr, level, children);
}

QVariantMap
OutlineBridge::convertEditorItem (const OutlineItem& item, int level,
                                  const QString& id) {
  QVariantList children;
  for (int i= 0; i < item.children.size (); ++i) {
    QString childId= id + "/" + QString::number (i);
    children.append (convertEditorItem (item.children[i], level + 1, childId));
  }
  return makeNode (id, item.title, item.target, QString (), level, children);
}

void
OutlineBridge::setOutline (const QVector<PdfOutlineItem>& outline) {
  m_model.clear ();
  for (int i= 0; i < outline.size (); ++i) {
    m_model.append (convertPdfItem (outline[i], 0, QString::number (i)));
  }
  emit outlineModelChanged ();
}

void
OutlineBridge::setOutline (const QVector<OutlineItem>& outline) {
  m_model.clear ();
  for (int i= 0; i < outline.size (); ++i) {
    m_model.append (convertEditorItem (outline[i], 0, QString::number (i)));
  }
  emit outlineModelChanged ();
}

void
OutlineBridge::clear () {
  m_model.clear ();
  m_currentId.clear ();
  emit outlineModelChanged ();
  emit currentIdChanged ();
}

void
OutlineBridge::itemClicked (const QString& id, const QString& target) {
  setCurrentId (id);
  if (!target.isEmpty ()) {
    emit outlineActivated (target);
  }
}

void
OutlineBridge::closeOutline () {
  emit closeRequested ();
}
