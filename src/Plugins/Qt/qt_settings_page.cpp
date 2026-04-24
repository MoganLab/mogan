/******************************************************************************
 * MODULE     : qt_settings_page.cpp
 * DESCRIPTION: Settings page rendered entirely from Scheme
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "qt_settings_page.hpp"

#include <QTabWidget>
#include <QVBoxLayout>

#include "qt_dpi_utils.hpp"
#include "qt_widget.hpp"
#include "s7_tm.hpp"
#include "scheme.hpp"
#include "widget.hpp"

#include "QTMMenuHelper.hpp"
#include "object.hpp"

// External declaration from tm_window.cpp
widget make_menu_widget (object menu);

namespace {
constexpr int kTabPaneTop     = 0;
constexpr int kTabBorderRadius= 12;
constexpr int kTabPaddingY    = 6;
constexpr int kTabPaddingX    = 14;
constexpr int kTabMarginY     = 4;
constexpr int kTabMarginX     = 2;
} // namespace

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

      // Find the inner QTabWidget and apply unified cross-platform styling
      // to avoid platform-native differences (Windows/macOS/Linux)
      QTabWidget* tabWidget= qw->findChild<QTabWidget*> ();
      if (tabWidget) {
        tabWidget->setObjectName ("settings-tab-widget");
        // Structural stylesheet: pill-shaped category buttons.
        // Colors are controlled by the theme CSS (liii.css, etc.)
        // so that light/dark themes work correctly.
        tabWidget->setStyleSheet (
            QString ("QTabWidget#settings-tab-widget::pane {"
                     "  top: %1px;"
                     "}"
                     "QTabWidget#settings-tab-widget QTabBar::tab {"
                     "  border-radius: %2px;"
                     "  padding: %3px %4px;"
                     "  margin: %5px %6px;"
                     "}")
                .arg (DpiUtils::scaled (kTabPaneTop))
                .arg (DpiUtils::scaled (kTabBorderRadius))
                .arg (DpiUtils::scaled (kTabPaddingY))
                .arg (DpiUtils::scaled (kTabPaddingX))
                .arg (DpiUtils::scaled (kTabMarginY))
                .arg (DpiUtils::scaled (kTabMarginX)));
      }
    }
  }
}

QTSettingsPage::~QTSettingsPage ()= default;
