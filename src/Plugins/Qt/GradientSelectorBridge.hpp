/******************************************************************************
 * MODULE      : GradientSelectorBridge.hpp
 * DESCRIPTION : QML 渐变选择器对话框 Bridge。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef GRADIENT_SELECTOR_BRIDGE_H
#define GRADIENT_SELECTOR_BRIDGE_H

#include <QDialog>
#include <QImage>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class GradientSelectorBridge : public QObject {
  Q_OBJECT

  Q_PROPERTY (QString patternName READ patternName WRITE setPatternName NOTIFY
                  patternNameChanged)
  Q_PROPERTY (QString width READ width WRITE setWidth NOTIFY widthChanged)
  Q_PROPERTY (QString height READ height WRITE setHeight NOTIFY heightChanged)
  Q_PROPERTY (QString foregroundColor READ foregroundColor WRITE
                  setForegroundColor NOTIFY foregroundColorChanged)
  Q_PROPERTY (QString backgroundColor READ backgroundColor WRITE
                  setBackgroundColor NOTIFY backgroundColorChanged)
  Q_PROPERTY (QString previewUrl READ previewUrl NOTIFY previewUrlChanged)
  Q_PROPERTY (QStringList patternOptions READ patternOptions CONSTANT)
  Q_PROPERTY (QStringList patternOptionsTr READ patternOptionsTr CONSTANT)
  Q_PROPERTY (QStringList colorOptions READ colorOptions CONSTANT)
  Q_PROPERTY (QStringList colorOptionsTr READ colorOptionsTr CONSTANT)
  Q_PROPERTY (QVariantMap labels READ labels CONSTANT)

private:
  QDialog* m_host;
  QString  m_patternName;
  QString  m_width;
  QString  m_height;
  QString  m_fgColor;
  QString  m_bgColor;
  QString  m_previewUrl;
  QImage   m_source;

  void reloadSource ();
  void updatePreview ();
  void pickColor (const QString& cur,
                  void (GradientSelectorBridge::*setter) (const QString&));

public:
  explicit GradientSelectorBridge (QDialog* host, const QString& name,
                                   const QString& width, const QString& height,
                                   const QString& fg, const QString& bg,
                                   QObject* parent= nullptr);

  ~GradientSelectorBridge () override= default;

  QString          patternName () const { return m_patternName; }
  Q_INVOKABLE void setPatternName (const QString& name);

  QString          width () const { return m_width; }
  Q_INVOKABLE void setWidth (const QString& w);

  QString          height () const { return m_height; }
  Q_INVOKABLE void setHeight (const QString& h);

  QString          foregroundColor () const { return m_fgColor; }
  Q_INVOKABLE void setForegroundColor (const QString& c);

  QString          backgroundColor () const { return m_bgColor; }
  Q_INVOKABLE void setBackgroundColor (const QString& c);

  QString previewUrl () const { return m_previewUrl; }

  QStringList patternOptions () const;
  QStringList patternOptionsTr () const;
  QStringList colorOptions () const;
  QStringList colorOptionsTr () const;
  QVariantMap labels () const;

  Q_INVOKABLE void browsePatternFile ();
  Q_INVOKABLE void pickForegroundColor ();
  Q_INVOKABLE void pickBackgroundColor ();
  Q_INVOKABLE void submit ();
  Q_INVOKABLE void cancel ();

signals:
  void patternNameChanged ();
  void widthChanged ();
  void heightChanged ();
  void foregroundColorChanged ();
  void backgroundColorChanged ();
  void previewUrlChanged ();
};

#endif // defined GRADIENT_SELECTOR_BRIDGE_H
