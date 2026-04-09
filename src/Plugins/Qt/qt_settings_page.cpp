/******************************************************************************
 * MODULE     : qt_settings_page.cpp
 * DESCRIPTION: Settings page rendered entirely from Scheme
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_settings_page.hpp"

#include <QTabWidget>
#include <QVBoxLayout>

#include "qt_widget.hpp"
#include "s7_tm.hpp"
#include "scheme.hpp"
#include "widget.hpp"

#include "QTMMenuHelper.hpp"
#include "object.hpp"

// External declaration from tm_window.cpp
widget make_menu_widget (object menu);

QTSettingsPage::QTSettingsPage (QWidget* parent) : QWidget (parent) {
  QVBoxLayout* layout= new QVBoxLayout (this);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSpacing (0);

  // Mark this window to disable tab auto-resize
  this->setProperty ("tm_no_tab_auto_resize", true);

  // Load settings UI from Scheme
  eval_scheme ("(use-modules (startup-tab startup-tab-settings))");

  // Get widget definition and render it
  object widget_def= eval ("(startup-settings-widget)");
  widget w         = make_menu_widget (widget_def);

  // Embed the widget
  if (!is_nil (w)) {
    QWidget* qw= concrete (w)->as_qwidget ();
    if (qw) {
      qw->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
      layout->addWidget (qw, 1);
    }
  }
}

QTSettingsPage::~QTSettingsPage ()= default;
