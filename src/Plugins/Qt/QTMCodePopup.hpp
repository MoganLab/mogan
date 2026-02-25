/******************************************************************************
 * MODULE     : QTMCodePopup.hpp
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2026
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CODE_POPUP_HPP
#define QT_CODE_POPUP_HPP

#include "qt_simple_widget.hpp"
#include "rectangles.hpp"

#include <QActionGroup>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMenu>
#include <QToolButton>
#include <QWidget>

class QTMCodePopup : public QWidget {
protected:
  qt_simple_widget_rep*      owner;
  QHBoxLayout*               layout;
  QGraphicsDropShadowEffect* effect;
  rectangle                  cached_rect;
  int                        cached_scroll_x;
  int                        cached_scroll_y;
  int                        cached_canvas_x;
  int                        cached_canvas_y;
  int                        cached_width;
  int                        cached_height;
  double                     cached_magf;
  tree                       current_tree;
  QToolButton*               copyBtn;
  QToolButton*               langBtn;
  QMenu*                     langMenu;
  QActionGroup*              langGroup;
  bool                       painted;
  int                        painted_count;

public:
  QTMCodePopup (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMCodePopup ();

  void showCodePopup (qt_renderer_rep* ren, rectangle selr, double magf,
                      int scroll_x, int scroll_y, int canvas_x, int canvas_y);
  void updatePosition (qt_renderer_rep* ren);
  void scrollBy (int x, int y);
  void setCodeTree (tree t);

protected:
  void autoSize ();
  void refreshLanguageMenu ();
  void cachePosition (rectangle selr, double magf, int scroll_x, int scroll_y,
                      int canvas_x, int canvas_y);
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y);
};

#endif // QT_CODE_POPUP_HPP
