
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

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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
    : QWidget (parent), currentEntry_ (Entry::File), label_ (nullptr) {
  setMinimumSize (400, 300);
  setStyleSheet ("background-color: #f0f0f0;");
  setFocusPolicy (Qt::NoFocus);
  label_= new QLabel ("Mogan STEM Startup Tab (File/Template/Recent/Settings)",
                      this);
  label_->setAlignment (Qt::AlignCenter);

  // Create buttons for each entry
  QPushButton* fileButton= new QPushButton ("File", this);
  fileButton->setFocusPolicy (Qt::NoFocus);
  QPushButton* templateButton= new QPushButton ("Template", this);
  templateButton->setFocusPolicy (Qt::NoFocus);
  QPushButton* recentButton= new QPushButton ("Recent", this);
  recentButton->setFocusPolicy (Qt::NoFocus);
  QPushButton* settingsButton= new QPushButton ("Settings", this);
  settingsButton->setFocusPolicy (Qt::NoFocus);

  // Connect button clicks to set current entry
  connect (fileButton, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::File); });
  connect (templateButton, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Template); });
  connect (recentButton, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Recent); });
  connect (settingsButton, &QPushButton::clicked, this,
           [this] () { set_current_entry (Entry::Settings); });

  // Arrange buttons horizontally
  QHBoxLayout* buttonLayout= new QHBoxLayout;
  buttonLayout->addWidget (fileButton);
  buttonLayout->addWidget (templateButton);
  buttonLayout->addWidget (recentButton);
  buttonLayout->addWidget (settingsButton);
  buttonLayout->addStretch ();

  // Main vertical layout
  QVBoxLayout* layout= new QVBoxLayout (this);
  layout->addWidget (label_);
  layout->addLayout (buttonLayout);
  layout->addStretch ();

  update_label ();
}

QTStartupTabWidget::Entry
QTStartupTabWidget::current_entry () const {
  return currentEntry_;
}

void
QTStartupTabWidget::set_current_entry (Entry entry) {
  currentEntry_= entry;
  update_label ();
}

void
QTStartupTabWidget::update_label () {
  if (label_) {
    label_->setText (QString ("Mogan STEM Startup Tab - Current: %1")
                         .arg (entry_to_string (currentEntry_)));
  }
}
