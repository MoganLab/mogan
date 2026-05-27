
/******************************************************************************
 * MODULE     : qt_floating_search_bar.hpp
 * DESCRIPTION: A VSCode-style floating search bar widget for TeXmacs
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_FLOATING_SEARCH_BAR_HPP
#define QT_FLOATING_SEARCH_BAR_HPP

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "string.hpp"

/**
 * 轻量级悬浮搜索栏，布局类似 VSCode 的 Ctrl+F 搜索框。
 *
 * 布局: [搜索输入框] [上一个] [下一个] [关闭] [匹配计数]
 * 通过信号通知宿主执行搜索操作。
 */
class QTMFloatingSearchBar : public QWidget {
  Q_OBJECT

public:
  QTMFloatingSearchBar (QWidget* parent= nullptr);

  /// 显示搜索栏，清空输入并聚焦。
  void activate ();
  /// 获取当前搜索文本。
  QString queryText () const;
  /// 设置匹配信息标签（如 "3/12" 或 "无结果"）。
  void setMatchInfo (const QString& info);

signals:
  /// 搜索文本变化时触发（实时搜索）。
  void queryChanged (const QString& text);
  /// 用户点击下一个或按 Enter 时触发。
  void findNextRequested ();
  /// 用户点击上一个或按 Shift+Enter 时触发。
  void findPreviousRequested ();
  /// 用户关闭搜索栏时触发。
  void closeRequested ();

protected:
  void keyPressEvent (QKeyEvent* event) override;

private:
  QLineEdit* edit_;    ///< 搜索输入框
  QLabel*    infoLbl_; ///< 匹配计数标签
};

/// Scheme 胶水函数：显示 ("true"/"#t") 或隐藏悬浮搜索栏。
void qt_floating_search (string flag);

#endif // QT_FLOATING_SEARCH_BAR_HPP
