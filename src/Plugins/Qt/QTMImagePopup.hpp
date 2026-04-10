
/******************************************************************************
 * MODULE     : QTMImagePopup.hpp
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2025 MoonLL, Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_IMAGE_POPUP_HPP
#define QT_IMAGE_POPUP_HPP

#include "qt_simple_widget.hpp"
#include "rectangles.hpp"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QToolButton>
#include <QWidget>

#include "QTMBasePopup.hpp"

class QTMImagePopup : public QTMBasePopup {
protected:
  tree         current_tree;
  string       current_align;
  QToolButton* leftBtn;
  QToolButton* middleBtn;
  QToolButton* rightBtn;
  QToolButton* ocrBtn;

public:
  QTMImagePopup (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMImagePopup ();

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override;
  void setImageTree (tree t);
  void updateButtonStates ();

protected:
  bool eventFilter (QObject* obj, QEvent* event) override;
};

#endif // QT_IMAGE_POPUP_HPP
