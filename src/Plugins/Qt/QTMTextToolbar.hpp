
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

#include "QTMBasePopup.hpp"
#include "qt_simple_widget.hpp"
#include "rectangles.hpp"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>

class QTMTextToolbar : public QTMBasePopup {
protected:
  int       cached_selection_mid_x;
  int       cached_selection_mid_y;
  bool      painted;
  int       painted_count;
  qt_widget text_toolbar_widget;

public:
  QTMTextToolbar (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMTextToolbar ();

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override;
  void updatePosition (qt_renderer_rep* ren) override;
  void scrollBy (int x, int y) override;
  void autoSize () override;

protected:
  void cachePosition (rectangle selr, double magf, int scroll_x, int scroll_y,
                      int canvas_x, int canvas_y) override;
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y) override;
  bool selectionInView () const override;
  void rebuildButtonsFromScheme ();
  void clearButtons ();
  bool eventFilter (QObject* obj, QEvent* event) override;
};

#endif // QT_TEXT_TOOLBAR_HPP
