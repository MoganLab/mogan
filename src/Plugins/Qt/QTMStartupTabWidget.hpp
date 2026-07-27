
/******************************************************************************
 * MODULE     : QTMStartupTabWidget.hpp
 * DESCRIPTION: Startup tab container — hosts QQuickWidget loading StartupTab.qml
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTMSTARTUPTABWIDGET_HPP
#define QTMSTARTUPTABWIDGET_HPP

#include <QWidget>

class QKeyEvent;
class QQuickWidget;
class StartupBridge;

/**
 * @brief 启动页容器，内嵌 QQuickWidget 渲染 StartupTab.qml。
 *
 * 原实现使用 QTMHomePage + QTMTemplatePage 两个 widget 页面 + QPushButton
 * 导航栏，现已替换为 QML：侧边栏、首页、模板页全部在 QML 内渲染，
 * C++ 只通过 StartupBridge 提供数据与动作。
 */
class QTMStartupTabWidget : public QWidget {
  Q_OBJECT

public:
  explicit QTMStartupTabWidget (QWidget* parent= nullptr);
  ~QTMStartupTabWidget ();

  void refreshRecentDocs ();
  void addRecentDoc (const QString& path);
  void refreshTheme ();

protected:
  void keyPressEvent (QKeyEvent* event) override;
  void keyReleaseEvent (QKeyEvent* event) override;
  void showEvent (QShowEvent* event) override;

private:
  QQuickWidget*  quickWidget_= nullptr;
  StartupBridge* bridge_     = nullptr;
};

#endif
