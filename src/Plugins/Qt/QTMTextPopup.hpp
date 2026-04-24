
/******************************************************************************
 * MODULE     : QTMTextPopup.hpp
 * DESCRIPTION: Text selection toolbar popup widget
 * COPYRIGHT  : (C) 2025  Jie Chen
 *                  2026  Yifan Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_TEXT_POPUP_HPP
#define QT_TEXT_POPUP_HPP

#include "QTMBasePopup.hpp"
#include "qt_simple_widget.hpp"
#include "rectangles.hpp"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>

class QTMTextPopup : public QTMBasePopup {
protected:
  qt_widget text_popup_widget;

public:
  QTMTextPopup (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMTextPopup ();

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override;
  void updatePosition (qt_renderer_rep* ren) override;
  void scrollBy (int x, int y) override;

protected:
  void rebuildButtonsFromScheme ();
  void clearButtons ();
};

#endif // QT_TEXT_POPUP_HPP
