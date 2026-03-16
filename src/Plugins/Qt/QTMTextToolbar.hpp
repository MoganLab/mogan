
/******************************************************************************
 * MODULE     : QTMTextToolbar.hpp
 * DESCRIPTION: Text selection toolbar popup widget
 * COPYRIGHT  : (C) 2025  Jie Chen
 *                  2026  Yifan Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_TEXT_TOOLBAR_HPP
#define QT_TEXT_TOOLBAR_HPP

#include "qt_simple_widget.hpp"
#include "rectangles.hpp"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>

class QTMTextToolbar : public QWidget {
protected:
  qt_simple_widget_rep*      owner;
  QHBoxLayout*               layout;
  QGraphicsDropShadowEffect* effect;
  int                        cached_selection_mid_x;
  int                        cached_selection_mid_y;
  rectangle                  cached_rect;
  int                        cached_scroll_x; // 页面滚动位置x
  int                        cached_scroll_y; // 页面滚动位置y
  int                        cached_canvas_x;
  int                        cached_canvas_y;
  int                        cached_width;
  int                        cached_height;
  double                     cached_magf; // 缩放因子
  bool                       painted;
  int                        painted_count;
  qt_widget                  text_toolbar_widget;

public:
  QTMTextToolbar (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMTextToolbar ();

  void showTextToolbar (qt_renderer_rep* ren, rectangle selr, double magf,
                        int scroll_x, int scroll_y, int canvas_x, int canvas_y);
  void updatePosition (qt_renderer_rep* ren);
  void scrollBy (int x, int y);
  void autoSize ();

protected:
  void cachePosition (rectangle selr, double magf, int scroll_x, int scroll_y,
                      int canvas_x, int canvas_y);
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y);
  bool selectionInView () const;
  void rebuildButtonsFromScheme ();
  void clearButtons ();
  bool eventFilter (QObject* obj, QEvent* event) override;
};

#endif // QT_TEXT_TOOLBAR_HPP
