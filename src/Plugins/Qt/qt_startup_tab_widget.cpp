
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
  struct ButtonInfo {
    const char* text;
    Entry       entry;
  };
  ButtonInfo buttons[]= {{"File", Entry::File},
                         {"Template", Entry::Template},
                         {"Recent", Entry::Recent},
                         {"Settings", Entry::Settings}};

  QHBoxLayout* buttonLayout= new QHBoxLayout;
  for (const auto& info : buttons) {
    QPushButton* btn= new QPushButton (info.text, this);
    btn->setFocusPolicy (Qt::NoFocus);
    connect (btn, &QPushButton::clicked, this,
             [this, entry= info.entry] () { set_current_entry (entry); });
    buttonLayout->addWidget (btn);
  }
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
  label_->setText (QString ("Mogan STEM Startup Tab - Current: %1")
                       .arg (entry_to_string (currentEntry_)));
}
