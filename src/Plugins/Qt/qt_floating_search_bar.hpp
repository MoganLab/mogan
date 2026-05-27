
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

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "string.hpp"

/**
 * 悬浮搜索栏容器。
 *
 * 布局:
 *   上层: [TeXmacs 输入区] [上一个] [下一个] [关闭]
 *   下层: [TeXmacs 输入区] [匹配计数]
 * 输入区是嵌入的 texmacs_input_widget，绑定到 search-buffer，
 * 搜索逻辑与底部搜索面板完全一致。
 */
class QTMFloatingSearchBar : public QWidget {
  Q_OBJECT

public:
  QTMFloatingSearchBar (QWidget* parent= nullptr);

  /// 设置嵌入的 TeXmacs 搜索输入 widget。
  void setSearchInput (QWidget* input);
  /// 显示搜索栏并聚焦输入区。
  void activate ();
  /// 设置匹配信息（current=0, total=0 显示"无匹配"）。
  void setMatchInfo (int current, int total);

signals:
  void findNextRequested ();
  void findPreviousRequested ();
  void closeRequested ();

private:
  QHBoxLayout* rowLayout_= nullptr; ///< 上层水平布局（输入+按钮）
  QWidget*     inputQW_  = nullptr; ///< 嵌入的 TeXmacs 输入 QWidget
  QLabel*      infoLbl_  = nullptr; ///< 匹配计数标签
};

/// Scheme 胶水函数：显示 ("true"/"#t") 或隐藏悬浮搜索栏。
void qt_floating_search (string flag);

/// Scheme 胶水函数：传入 search-buffer URL，创建 texmacs-input
/// 并嵌入浮动搜索栏。
void qt_floating_search_init (string aux_url_str);

/// Scheme 胶水函数：更新浮动搜索栏的匹配计数显示。
void qt_floating_search_set_match_info (int current, int total);

#endif // QT_FLOATING_SEARCH_BAR_HPP
