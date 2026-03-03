
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

QTStartupTabWidget::QTStartupTabWidget (QWidget* parent)
    : QWidget (parent), currentEntry_ (Entry::File) {}

QTStartupTabWidget::Entry
QTStartupTabWidget::current_entry () const {
  return currentEntry_;
}

void
QTStartupTabWidget::set_current_entry (Entry entry) {
  currentEntry_= entry;
}
