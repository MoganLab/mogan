/******************************************************************************
 * MODULE      : VersionDialogBridge.hpp
 * DESCRIPTION : Data and actions exposed to the Version QML dialog.
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef VERSION_DIALOG_BRIDGE_HPP
#define VERSION_DIALOG_BRIDGE_HPP

#include "QTMQmlDialogBridge.hpp"

#include <QString>
#include <QStringList>

class VersionDialogBridge : public QmlDialogBridge {
  Q_OBJECT
  Q_PROPERTY (QString title READ title CONSTANT)
  Q_PROPERTY (QStringList lines READ lines CONSTANT)
  Q_PROPERTY (QStringList buttonLabels READ buttonLabels CONSTANT)

public:
  VersionDialogBridge (QDialog* host, const QString& title,
                       const QStringList& lines,
                       const QStringList& button_labels)
      : QmlDialogBridge (host), m_title (title), m_lines (lines),
        m_buttonLabels (button_labels) {}

  QString     title () const { return m_title; }
  QStringList lines () const { return m_lines; }
  QStringList buttonLabels () const { return m_buttonLabels; }

  Q_INVOKABLE void confirm () { choose (QDialog::Accepted); }

private:
  QString     m_title;
  QStringList m_lines;
  QStringList m_buttonLabels;
};

#endif // defined VERSION_DIALOG_BRIDGE_HPP
