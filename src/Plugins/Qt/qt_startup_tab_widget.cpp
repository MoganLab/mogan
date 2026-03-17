
/******************************************************************************
 * MODULE     : qt_startup_tab_widget.cpp
 * DESCRIPTION: Startup tab widget skeleton for Mogan STEM
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_startup_tab_widget.hpp"

#include <QLabel>
#include <QVBoxLayout>

namespace {
const char*
entry_to_string (QTStartupTabWidget::Entry entry) {
  switch (entry) {
  case QTStartupTabWidget::Entry::File:
    return "File";
  case QTStartupTabWidget::Entry::Template:
    return "Template";
  case QTStartupTabWidget::Entry::Recent:
    return "Recent";
  case QTStartupTabWidget::Entry::Settings:
    return "Settings";
  default:
    return "Unknown";
  }
}
} // namespace

QTStartupTabWidget::QTStartupTabWidget (QWidget* parent)
    : QWidget (parent), currentEntry_ (Entry::File) {
  setMinimumSize(400, 300);
  setStyleSheet("background-color: #f0f0f0;");
  QLabel* label = new QLabel("Mogan STEM Startup Tab (File/Template/Recent/Settings)", this);
  label->setAlignment(Qt::AlignCenter);
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(label);
}

QTStartupTabWidget::Entry
QTStartupTabWidget::current_entry () const {
  return currentEntry_;
}

void
QTStartupTabWidget::set_current_entry (Entry entry) {
  currentEntry_= entry;
}
