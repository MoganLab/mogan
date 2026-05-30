
/******************************************************************************
 * MODULE     : qt_floating_search_bar.hpp
 * DESCRIPTION: A VSCode-style floating search bar widget for TeXmacs
 * COPYRIGHT  : (C) 2026  Yuki Lu
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_FLOATING_SEARCH_BAR_HPP
#define QT_FLOATING_SEARCH_BAR_HPP

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QWidget>

#include "string.hpp"

#include <functional>

class QAbstractScrollArea;

/**
 * VSCode 风格的悬浮搜索栏组件。
 *
 * 布局：
 *   左侧：嵌入的输入框（如 texmacs_input_widget）
 *   右侧：[上一个] [下一个] [关闭] 按钮 + 匹配计数
 */
class QTMFloatingSearchBar : public QWidget {
  Q_OBJECT

public:
  explicit QTMFloatingSearchBar (QWidget* parent= nullptr);
  ~QTMFloatingSearchBar () override;

  /// 设置嵌入的搜索输入框。旧的输入框（如有）会被移除并 deleteLater。
  void setSearchInput (QWidget* input);
  /// 显示搜索栏并聚焦输入框。
  void activate ();
  /// 设置匹配信息（current=0, total=0 时显示"无匹配"）。
  void setMatchInfo (int current, int total);

  /// 配置按钮点击时求值的 Scheme 命令。
  void setSchemeCallbacks (const string& next_cmd, const string& prev_cmd,
                           const string& close_cmd);
  void setModeIcon (bool mathMode);
  void toggleMode ();

signals:
  void findNextRequested ();
  void findPreviousRequested ();
  void closeRequested ();
  void modeToggled ();

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;
  void showEvent (QShowEvent* event) override;

private:
  void reposition ();
  void connectSignals ();

  QHBoxLayout*         rowLayout_      = nullptr;
  QWidget*             inputQW_        = nullptr;
  QAbstractScrollArea* inputScrollArea_= nullptr;
  QLabel*              infoLbl_        = nullptr;
  QToolButton*         modeBtn_        = nullptr;

  string next_cmd_;
  string prev_cmd_;
  string close_cmd_;
  bool   mathMode_          = false;
  bool   callbacksConnected_= false;
};

/******************************************************************************
 * 通用管理 API（基于 parent widget，不依赖 ChatController）
 ******************************************************************************/

/// 显示或隐藏 attach 到 \a parent 的悬浮搜索栏。
void qt_floating_search_bar_show (QWidget* parent, bool show);

/// 为 \a parent 创建/attach 搜索栏，并用绑定到 \a aux_url_str 的
/// texmacs 输入框初始化。\a mode 为 "text" 或 "math"，决定输入框的数学环境。
/// 失败时返回 false。
bool qt_floating_search_bar_init (QWidget* parent, const string& aux_url_str,
                                  const string& mode);

/// 更新 attach 到 \a parent 的搜索栏的匹配计数。
void qt_floating_search_bar_set_match_info (QWidget* parent, int current,
                                            int total);

/// 为 attach 到 \a parent 的搜索栏设置 Scheme 回调。
void qt_floating_search_bar_set_callbacks (QWidget*      parent,
                                           const string& next_cmd,
                                           const string& prev_cmd,
                                           const string& close_cmd);

/// 销毁 attach 到 \a parent 的搜索栏。
void qt_floating_search_bar_destroy (QWidget* parent);

/******************************************************************************
 * 兼容层胶水函数（保留向后兼容）。
 * 通过注册的 parent provider 代理到上面的通用 API。
 ******************************************************************************/

using qt_floating_search_parent_provider= std::function<QWidget*()>;

/// 注册一个返回默认 parent widget 的函数，供兼容层胶水函数使用。
/// 通常在 chat controller 初始化时调用。
void qt_floating_search_set_parent_provider (
    qt_floating_search_parent_provider provider);

/// Scheme 胶水函数：显示 ("true"/"#t") 或隐藏悬浮搜索栏。
void qt_floating_search (string flag);

/// Scheme 胶水函数：传入 search-buffer URL 和 mode ("text"/"math")，
/// 创建 texmacs-input 并嵌入浮动搜索栏。
void qt_floating_search_init (string aux_url_str, string mode);

/// Scheme 胶水函数：更新浮动搜索栏的匹配计数显示。
void qt_floating_search_set_match_info (int current, int total);

/// Scheme 胶水函数：设置搜索栏按钮点击时求值的 Scheme 回调命令。
void qt_floating_search_set_callbacks (string next_cmd, string prev_cmd,
                                       string close_cmd);

#endif // QT_FLOATING_SEARCH_BAR_HPP
