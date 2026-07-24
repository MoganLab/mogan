
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

#include <QWidget>

class QKeyEvent;
class QQuickWidget;
class StartupBridge;

/**
 * @brief 启动页容器，使用 QQuickWidget 加载 StartupTab.qml。
 *
 * 原实现使用 QTMHomePage + QTMTemplatePage 两个 Qt Widget 页面 + 左侧
 * QPushButton 导航栏，现已替换为 QML：侧边栏、首页、模板页全部在
 * StartupTab.qml 内渲染，C++ 只提供 StartupBridge 数据/动作桥接。
 *
 * 和原接口兼容：保留 Entry 枚举和 entry_changed 信号（外部调用
 * set_current_entry 仍有效，内部告知 QML 页切换）。
 */
class QTMStartupTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class Entry { Home, Template };

  explicit QTMStartupTabWidget (QWidget* parent= nullptr);
  ~QTMStartupTabWidget ();

  Entry current_entry () const;
  void  set_current_entry (Entry entry);

  /** 供外部刷新最近文档（打开文件后调用）。 */
  void refreshRecentDocs ();
  /** 供外部添加最近文档条目。 */
  void addRecentDoc (const QString& path);

signals:
  void entry_changed (Entry entry);

protected:
  void keyPressEvent (QKeyEvent* event) override;
  void keyReleaseEvent (QKeyEvent* event) override;

private:
  void setupQml ();

  Entry          currentEntry_;
  QQuickWidget*  quickWidget_= nullptr;
  StartupBridge* bridge_     = nullptr;
};

#endif
