/******************************************************************************
 * MODULE      : OutlineBridge.hpp
 * DESCRIPTION : C++ ↔ QML bridge for OutlineSidebar (PDF & Document ToC)
 * COPYRIGHT   : (C) 2026 Mogan STEM
 ******************************************************************************/

#ifndef OUTLINE_BRIDGE_HPP
#define OUTLINE_BRIDGE_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "qt_pdf_reader_widget.hpp"

class OutlineBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QVariantList outlineModel READ outlineModel NOTIFY outlineModelChanged)
  Q_PROPERTY (bool hasContent READ hasContent NOTIFY outlineModelChanged)
  Q_PROPERTY (QString currentId READ currentId WRITE setCurrentId NOTIFY currentIdChanged)
  Q_PROPERTY (QString currentTarget READ currentTarget WRITE setCurrentTarget NOTIFY currentTargetChanged)

public:
  explicit OutlineBridge (QObject* parent= nullptr);
  ~OutlineBridge () override= default;

  QVariantList outlineModel () const { return m_model; }
  bool         hasContent () const { return !m_model.isEmpty (); }
  QString      currentId () const { return m_currentId; }
  void         setCurrentId (const QString& id);
  QString      currentTarget () const { return m_currentTarget; }
  void         setCurrentTarget (const QString& target);

  void setOutline (const QVector<PdfOutlineItem>& outline);
  void setOutline (const QVector<OutlineItem>& outline);
  void clear ();

  Q_INVOKABLE void itemClicked (const QString& id, const QString& target);
  Q_INVOKABLE void itemClicked (const QString& target);
  Q_INVOKABLE void closeOutline ();

signals:
  void outlineModelChanged ();
  void currentIdChanged ();
  void currentTargetChanged ();
  void outlineActivated (const QString& target);
  void closeRequested ();

private:
  QVariantList m_model;
  QString      m_currentId;
  QString      m_currentTarget;

  QVariantMap convertPdfItem (const PdfOutlineItem& item, int level,
                              const QString& id);
  QVariantMap convertEditorItem (const OutlineItem& item, int level,
                                 const QString& id);
};

#endif // OUTLINE_BRIDGE_HPP
