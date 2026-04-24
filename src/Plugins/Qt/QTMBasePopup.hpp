
/******************************************************************************
 * MODULE     : QTMBasePopup.hpp
 * DESCRIPTION: Base class for popup widgets (image toolbar, text toolbar, etc.)
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_BASE_POPUP_HPP
#define QT_BASE_POPUP_HPP

#include "qt_simple_widget.hpp"
#include "rectangles.hpp"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QWidget>

class QTMBasePopup : public QWidget {
protected:
  qt_simple_widget_rep*      owner;
  QHBoxLayout*               layout;
  QGraphicsDropShadowEffect* effect;
  rectangle                  cached_rect;
  int                        cached_scroll_x; // 页面滚动位置x
  int                        cached_scroll_y; // 页面滚动位置y
  int                        cached_canvas_x;
  int                        cached_canvas_y;
  int                        cached_width;
  int                        cached_height;
  double                     cached_magf; // 缩放因子

public:
  QTMBasePopup (QWidget* parent, qt_simple_widget_rep* owner);
  virtual ~QTMBasePopup ();

  // 显示悬浮框（纯虚函数，子类实现具体显示逻辑）
  virtual void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                          int scroll_x, int scroll_y, int canvas_x,
                          int canvas_y)= 0;

  // 更新悬浮框位置（有默认实现）
  virtual void updatePosition (qt_renderer_rep* ren);

  // 滚动时调整位置
  virtual void scrollBy (int x, int y);

  // 自动调整大小（有默认实现，子类可重写）
  virtual void autoSize ();

protected:
  // 缓存位置信息
  virtual void cachePosition (rectangle selr, double magf, int scroll_x,
                              int scroll_y, int canvas_x, int canvas_y);

  // 计算显示位置（虚函数，子类可重写不同位置算法）
  virtual void getCachedPosition (qt_renderer_rep* ren, int& x, int& y);

  // 检查选区是否在视口内
  virtual bool selectionInView () const;

  // 初始化共同的UI元素（阴影效果等）
  void initCommonUI ();
};

#endif // QT_BASE_POPUP_HPP