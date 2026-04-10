
/******************************************************************************
 * MODULE     : QTMTextPopup.cpp
 * DESCRIPTION: Text selection toolbar popup widget implementation
 * COPYRIGHT  : (C) 2025  Jie Chen
 *                  2026  Yifan Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMTextPopup.hpp"
#include "QTMStyle.hpp"
#include "bitmap_font.hpp"
#include "moebius/data/scheme.hpp"
#include "object_l5.hpp"
#include "qt_renderer.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "server.hpp"
#include "tm_ostream.hpp"

#include <QFrame>
#include <QHelpEvent>
#include <QIcon>
#include <QLabel>
#include <QLayoutItem>
#include <QPainter>
#include <QPen>
#include <QSizePolicy>
#include <QToolButton>
#include <QToolTip>
#include <QWidgetAction>
#include <algorithm>
#include <cmath>

// 悬浮工具栏创建函数
QTMTextPopup::QTMTextPopup (QWidget* parent, qt_simple_widget_rep* owner)
    : QTMBasePopup (parent, owner) {
  rebuildButtonsFromScheme ();
}

QTMTextPopup::~QTMTextPopup () {}

void
QTMTextPopup::clearButtons () {
  if (!layout) return;
  QLayoutItem* item= nullptr;
  while ((item= layout->takeAt (0)) != nullptr) {
    if (QWidget* w= item->widget ()) {
      w->setParent (nullptr);
      delete w;
    }
    else if (QLayout* l= item->layout ()) {
      delete l;
    }
    delete item;
  }
}

void
QTMTextPopup::rebuildButtonsFromScheme () {
  eval ("(use-modules (generic text-toolbar))");
  object menu= eval ("'(horizontal (link text-toolbar-icons))");
  object obj = call ("make-menu-widget", menu, 0);
  if (!is_widget (obj)) return;

  text_popup_widget    = concrete (as_widget (obj));
  QList<QAction*>* list= text_popup_widget->get_qactionlist ();
  if (!list) return;

  clearButtons ();

  for (int i= 0; i < list->count (); ++i) {
    QAction* action= list->at (i);
    if (!action) continue;

    if (action->isSeparator ()) {
      QFrame* sep= new QFrame (this);
      sep->setFrameShape (QFrame::VLine);
      sep->setFrameShadow (QFrame::Plain);
      sep->setFixedWidth (1);
      sep->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Expanding);
      layout->addWidget (sep);
      continue;
    }

    if (action->text ().isNull () && action->icon ().isNull ()) {
      layout->addSpacing (8);
      continue;
    }

    if (QWidgetAction* wa= qobject_cast<QWidgetAction*> (action)) {
      QWidget* w= wa->requestWidget (this);
      if (w) layout->addWidget (w);
      continue;
    }

    QToolButton* button= new QToolButton (this);
    button->setObjectName ("base_popup_button");
    button->setAutoRaise (true);
    button->setDefaultAction (action);
    button->setPopupMode (QToolButton::InstantPopup);
    if (tm_style_sheet == "") button->setStyle (qtmstyle ());
    layout->addWidget (button);
  }
}

void
QTMTextPopup::showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                         int scroll_x, int scroll_y, int canvas_x,
                         int canvas_y) {
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  autoSize ();
  if (!selectionInView ()) {
    hide ();
    return;
  }
  updatePosition (ren);
  show ();
  raise ();
}

void
QTMTextPopup::updatePosition (qt_renderer_rep* ren) {
  if (!selectionInView ()) {
    hide ();
    return;
  }
  int x, y;
  getCachedPosition (ren, x, y);
  move (x, y);
}

void
QTMTextPopup::scrollBy (int x, int y) {
  QTMBasePopup::scrollBy (x, y);
}
