
/******************************************************************************
 * MODULE     : qt_startup_tab_widget.hpp
 * DESCRIPTION: Startup tab widget with left sidebar navigation for Mogan STEM
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
class QVBoxLayout;
class QPushButton;
class QStackedWidget;
class QButtonGroup;

class QTStartupTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class Entry { File, Template, Recent, Settings };

public:
  explicit QTStartupTabWidget (QWidget* parent= nullptr);

  Entry current_entry () const;
  void  set_current_entry (Entry entry);

signals:
  void entry_changed (Entry entry);

private slots:
  // File operations
  void on_file_new ();
  void on_file_open ();

  // Application operation
  void on_app_quit ();

private:
  // 界面构建辅助函数
  void         setup_left_sidebar (QVBoxLayout* sidebarLayout);
  void         setup_right_content (QStackedWidget* stackedWidget);
  QPushButton* create_nav_button (const QString& text);

  // 页面创建函数
  QWidget* create_file_page ();
  QWidget* create_template_page ();
  QWidget* create_recent_page ();
  QWidget* create_settings_page ();

  // 导航按钮状态管理
  void set_active_nav_button (Entry entry);

private:
  Entry currentEntry_;

  // Navigation buttons
  QPushButton* navFileBtn_;
  QPushButton* navTemplateBtn_;
  QPushButton* navRecentBtn_;
  QPushButton* navSettingsBtn_;
  QPushButton* navQuitBtn_;

  // 互斥按钮组
  QButtonGroup* navButtonGroup_;
};

#endif
