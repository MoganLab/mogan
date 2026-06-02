
/******************************************************************************
 * MODULE     : QTMMathCompletionPopup.hpp
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2025 JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_MATH_COMPLETION_POPUP_HPP
#define QT_MATH_COMPLETION_POPUP_HPP

#include "qt_simple_widget.hpp"

#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWidget>

class QTMMathCompletionPopup : public QWidget {
protected:
  qt_simple_widget_rep* owner;
  QVBoxLayout*          layout;
  int                   cached_cursor_x;
  int                   cached_cursor_y;
  int                   cached_scroll_x;
  int                   cached_scroll_y;
  int                   cached_canvas_x;
  double                cached_magf;

public:
  QTMMathCompletionPopup (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMMathCompletionPopup ();

  void showMathCompletions (struct cursor cu, double magf, int scroll_x,
                            int scroll_y, int canvas_x);
  void setWidget (QWidget* w);
  void updatePosition ();
  void scrollBy (int x, int y);

protected:
  void cleanLayout ();
  void cachePosition (struct cursor cu, double magf, int scroll_x, int scroll_y,
                      int canvas_x);
  void getCachedPosition (int& x, int& y);
  void installEventFilterRecursively (QWidget* widget, QObject* filterObj);
  void installTopLevelWindowFilter ();

protected:
  bool eventFilter (QObject* obj, QEvent* event) override;
};

#endif // QT_MATH_COMPLETION_POPUP_HPP
