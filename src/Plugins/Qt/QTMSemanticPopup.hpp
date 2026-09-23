/******************************************************************************
 * MODULE     : QTMSemanticPopup.hpp
 * DESCRIPTION: Floating popup for semantic blocks (equations, TOC, etc.)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_SEMANTIC_POPUP_HPP
#define QT_SEMANTIC_POPUP_HPP

#include "QTMBasePopup.hpp"
#include "rectangles.hpp"
#include "tree.hpp"

#include <QList>
#include <QPropertyAnimation>
#include <QTimer>
#include <QToolButton>

class QEnterEvent;

class QTMSemanticPopup : public QTMBasePopup {
  Q_OBJECT

protected:
  tree                current_tree;
  string              current_tag;
  QList<QToolButton*> buttons;
  QPropertyAnimation* fade_anim;
  QTimer*             hide_timer;

public:
  QTMSemanticPopup (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMSemanticPopup ();

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override;
  void setSemanticNode (string tag, tree t);
  void clearButtons ();
  void rebuildButtons ();

  void startFadeOut ();
  void cancelFadeOut ();

  void autoSize () override;

protected:
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y) override;
  void enterEvent (QEnterEvent* event) override;
  void leaveEvent (QEvent* event) override;

private slots:
  void handleAction (const string& action_id, QToolButton* btn);
};

#endif // QT_SEMANTIC_POPUP_HPP
