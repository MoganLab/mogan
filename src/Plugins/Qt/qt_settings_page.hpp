/******************************************************************************
 * MODULE     : qt_settings_page.hpp
 * DESCRIPTION: Settings page rendered entirely from Scheme
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QT_SETTINGS_PAGE_HPP
#define QT_SETTINGS_PAGE_HPP

#include <QWidget>

/**
 * @brief Settings page that renders UI entirely from Scheme
 *
 * All UI structure is defined in Scheme (startup-tab-settings.scm)
 * and rendered via make_menu_widget.
 */
class QTSettingsPage : public QWidget {
  Q_OBJECT

public:
  explicit QTSettingsPage (QWidget* parent= nullptr);
  ~QTSettingsPage ();
};

#endif // QT_SETTINGS_PAGE_HPP
