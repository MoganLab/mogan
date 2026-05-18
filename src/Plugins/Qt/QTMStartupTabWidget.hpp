
/******************************************************************************
 * MODULE     : QTMStartupTabWidget.hpp
 * DESCRIPTION: Startup tab widget with left sidebar navigation for Mogan STEM
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTMSTARTUPTABWIDGET_HPP
#define QTMSTARTUPTABWIDGET_HPP

#include <QList>
#include <QWidget>

class QKeyEvent;
class QLabel;
class QVBoxLayout;
class QPushButton;
class QStackedWidget;
class QButtonGroup;
class QTMHomePage;
class QTMTemplatePage;
class TemplateManager;

class QTMStartupTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class Entry { Home, Template };

public:
  explicit QTMStartupTabWidget (QWidget* parent= nullptr);

  Entry current_entry () const;
  void  set_current_entry (Entry entry);

signals:
  void entry_changed (Entry entry);

private slots:
  // Application operation
  void on_app_quit ();
  void onCategoryClicked ();
  void onCategoriesLoaded ();

protected:
  void keyPressEvent (QKeyEvent* event) override;
  void keyReleaseEvent (QKeyEvent* event) override;

private:
  // 界面构建辅助函数
  void         setup_left_sidebar (QVBoxLayout* sidebarLayout);
  void         setup_right_content (QStackedWidget* stackedWidget);
  void         setupCategoryNavButtons ();
  void         clearCategoryNavButtons ();
  QPushButton* create_nav_button (const QString& text);

  // 页面创建函数
  QWidget* create_home_page ();
  QWidget* create_template_page ();

  // 导航按钮状态管理
  void set_active_nav_button (Entry entry);
  void refresh_recent_docs_on_file_entry (Entry entry);

private:
  Entry   currentEntry_;
  QString currentCategory_;

  // Navigation buttons
  QPushButton*        navHomeBtn_;
  QPushButton*        navQuitBtn_;
  QList<QPushButton*> navCategoryBtns_;
  QVBoxLayout*        categoryLayout_;

  // 互斥按钮组
  QButtonGroup* navButtonGroup_;

  QTMHomePage*     homePage_;
  QTMTemplatePage* templatePage_;
  TemplateManager* templateManager_;
};

#endif
