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

void
OutlineBridge::setCurrentTarget (const QString& target) {
  if (m_currentTarget != target) {
    m_currentTarget= target;
    emit currentTargetChanged ();
  }
}

QVariantMap
OutlineBridge::convertPdfItem (const PdfOutlineItem& item, int level,
                               const QString& id) {
  QVariantMap map;
  map["id"]    = id;
  map["title"] = item.title;
  int pageOneBased= (item.page >= 0) ? item.page + 1 : -1;
  map["target"]=
      (pageOneBased >= 0) ? QString::number (pageOneBased) : QString ();
  map["page"] =
      (pageOneBased >= 0) ? QString::number (pageOneBased) : QString ();
  map["level"]= level;

  QVariantList children;
  for (int i= 0; i < item.children.size (); ++i) {
    QString childId= id + "/" + QString::number (i);
    children.append (convertPdfItem (item.children[i], level + 1, childId));
  }
  map["children"]= children;
  return map;
}

QVariantMap
OutlineBridge::convertEditorItem (const OutlineItem& item, int level,
                                  const QString& id) {
  QVariantMap map;
  map["id"]    = id;
  map["title"] = item.title;
  map["target"]= item.target;
  map["page"]  = QString ();
  map["level"] = level;

  QVariantList children;
  for (int i= 0; i < item.children.size (); ++i) {
    QString childId= id + "/" + QString::number (i);
    children.append (convertEditorItem (item.children[i], level + 1, childId));
  }
  map["children"]= children;
  return map;
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
  m_currentTarget.clear ();
  emit outlineModelChanged ();
  emit currentIdChanged ();
  emit currentTargetChanged ();
}

void
OutlineBridge::itemClicked (const QString& id, const QString& target) {
  setCurrentId (id);
  setCurrentTarget (target);
  if (!target.isEmpty ()) {
    emit outlineActivated (target);
  }
}

void
OutlineBridge::itemClicked (const QString& target) {
  itemClicked (target, target);
}

void
OutlineBridge::closeOutline () {
  emit closeRequested ();
}
