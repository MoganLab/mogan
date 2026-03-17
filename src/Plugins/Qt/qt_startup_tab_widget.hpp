
/******************************************************************************
 * MODULE     : qt_startup_tab_widget.hpp
 * DESCRIPTION: Startup tab widget skeleton for Mogan STEM
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_STARTUP_TAB_WIDGET_HPP
#define QT_STARTUP_TAB_WIDGET_HPP

#include <QWidget>

class QLabel;

class QTStartupTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class Entry { File, Template, Recent, Settings };

public:
  explicit QTStartupTabWidget (QWidget* parent= nullptr);

  Entry current_entry () const;
  void  set_current_entry (Entry entry);

private slots:
  void update_label ();

private:
  Entry   currentEntry_;
  QLabel* label_;
};

#endif
