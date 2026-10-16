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
  m_currentId.clear ();
  for (int i= 0; i < outline.size (); ++i) {
    m_model.append (convertPdfItem (outline[i], 0, QString::number (i)));
  }
  m_defaultExpandedMap= computeDefaultExpandedMap (m_model);
  emit outlineModelChanged ();
  emit currentIdChanged ();
  emit defaultExpandedMapChanged ();
}

void
OutlineBridge::setOutline (const QVector<OutlineItem>& outline) {
  m_model.clear ();
  m_currentId.clear ();
  for (int i= 0; i < outline.size (); ++i) {
    m_model.append (convertEditorItem (outline[i], 0, QString::number (i)));
  }
  m_defaultExpandedMap= computeDefaultExpandedMap (m_model);
  emit outlineModelChanged ();
  emit currentIdChanged ();
  emit defaultExpandedMapChanged ();
}

void
OutlineBridge::clear () {
  m_model.clear ();
  m_currentId.clear ();
  m_defaultExpandedMap.clear ();
  emit outlineModelChanged ();
  emit currentIdChanged ();
  emit defaultExpandedMapChanged ();
}

QVariantMap
OutlineBridge::computeDefaultExpandedMap (const QVariantList& model,
                                          int threshold) {
  QVariantMap result;
  if (model.isEmpty ()) return result;

  // 阶段一：规则 1 单非叶子节点链条自动递归穿透展开
  QVariantList current= model;
  while (true) {
    QVariantList leaves;
    QVariantList nonLeaves;
    for (const QVariant& item : current) {
      QVariantMap  node    = item.toMap ();
      QVariantList children= node.value ("children").toList ();
      if (children.isEmpty ()) {
        leaves.append (item);
      }
      else {
        nonLeaves.append (item);
      }
    }
    if (leaves.isEmpty () && nonLeaves.size () == 1) {
      QVariantMap sole= nonLeaves.first ().toMap ();
      result[sole.value ("id").toString ()]= true;
      current= sole.value ("children").toList ();
    }
    else {
      break;
    }
  }

  // 阶段二：规则 2 逐层评估展开（阈值 threshold）
  int          visibleCount= result.size () + current.size ();
  QVariantList currentLayer= current;

  while (true) {
    QVariantList candidates;
    for (const QVariant& item : currentLayer) {
      QVariantMap  node    = item.toMap ();
      QVariantList children= node.value ("children").toList ();
      if (!children.isEmpty ()) {
        candidates.append (item);
      }
    }
    if (candidates.isEmpty ()) break;

    QVariantList newChildren;
    for (const QVariant& item : candidates) {
      QVariantMap node= item.toMap ();
      newChildren.append (node.value ("children").toList ());
    }
    int newCount= newChildren.size ();
    if (visibleCount + newCount <= threshold) {
      for (const QVariant& item : candidates) {
        QVariantMap node                     = item.toMap ();
        result[node.value ("id").toString ()]= true;
      }
      visibleCount+= newCount;
      currentLayer= newChildren;
    }
    else {
      break;
    }
  }

  // 为所有未展开的非叶子节点显式标记 false，防止 QML 端状态残留或歧义
  auto markUnexpanded= [&result] (auto& self,
                                  const QVariantList& nodes) -> void {
    for (const QVariant& item : nodes) {
      QVariantMap  node    = item.toMap ();
      QString      id      = node.value ("id").toString ();
      QVariantList children= node.value ("children").toList ();
      if (!children.isEmpty ()) {
        if (!result.contains (id)) {
          result[id]= false;
        }
        self (self, children);
      }
    }
  };
  markUnexpanded (markUnexpanded, model);

  return result;
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
