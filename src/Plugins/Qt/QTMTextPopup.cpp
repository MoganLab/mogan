
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
QTMTextPopup::updateButtonsFromScheme () {
  QSize old_icon_size;
  QSize old_button_size;
  int   old_height= (cached_height > 0) ? cached_height : height ();

  const QList<QToolButton*> old_buttons=
      findChildren<QToolButton*> (QString (), Qt::FindChildrenRecursively);
  if (!old_buttons.isEmpty ()) {
    QToolButton* button= old_buttons.first ();
    if (button) {
      old_icon_size  = button->iconSize ();
      old_button_size= button->minimumSize ();
    }
  }

  object menu= eval ("'(horizontal (link text-toolbar-icons))");
  object obj = call ("make-menu-widget", menu, 0);
  if (!is_widget (obj)) return;

  qt_widget        new_popup_widget= concrete (as_widget (obj));
  QList<QAction*>* list            = new_popup_widget->get_qactionlist ();
  if (!list) return;

  bool can_update_in_place= (layout && layout->count () == list->count ());
  if (can_update_in_place) {
    for (int i= 0; i < list->count (); ++i) {
      QAction* action= list->at (i);
      if (!action) {
        can_update_in_place= false;
        break;
      }

      QLayoutItem* item= layout->itemAt (i);
      if (!item) {
        can_update_in_place= false;
        break;
      }

      if (action->isSeparator ()) {
        if (!qobject_cast<QFrame*> (item->widget ())) {
          can_update_in_place= false;
          break;
        }
        continue;
      }

      if (action->text ().isNull () && action->icon ().isNull ()) {
        if (!item->spacerItem ()) {
          can_update_in_place= false;
          break;
        }
        continue;
      }

      if (qobject_cast<QWidgetAction*> (action)) {
        can_update_in_place= false;
        break;
      }

      QToolButton* button= qobject_cast<QToolButton*> (item->widget ());
      if (!button) {
        can_update_in_place= false;
        break;
      }

      QAction* current_action= button->defaultAction ();
      bool     current_has_menu=
          (current_action != nullptr && current_action->menu () != nullptr);
      bool next_has_menu= (action->menu () != nullptr);
      if (current_has_menu != next_has_menu) {
        can_update_in_place= false;
        break;
      }
    }
  }

  if (can_update_in_place) {
    text_popup_widget= new_popup_widget;
    for (int i= 0; i < list->count (); ++i) {
      QAction* action= list->at (i);
      if (!action || action->isSeparator ()) continue;
      if (action->text ().isNull () && action->icon ().isNull ()) continue;

      QToolButton* button=
          qobject_cast<QToolButton*> (layout->itemAt (i)->widget ());
      if (!button) continue;

      const QList<QAction*> stale_actions= button->actions ();
      for (QAction* stale_action : stale_actions) {
        if (stale_action) button->removeAction (stale_action);
      }
      button->setMenu (nullptr);
      button->setDefaultAction (action);
      button->setPopupMode (QToolButton::InstantPopup);
      if (tm_style_sheet == "") button->setStyle (qtmstyle ());
    }
    cached_width = width ();
    cached_height= height ();
    return;
  }

  text_popup_widget= new_popup_widget;
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
    if (old_icon_size.isValid ()) button->setIconSize (old_icon_size);
    if (old_button_size.isValid ()) button->setFixedSize (old_button_size);
    layout->addWidget (button);
  }

  if (layout) {
    layout->invalidate ();
    layout->activate ();
  }
  adjustSize ();
  if (old_height > 0) resize (width (), old_height);
  cached_width = width ();
  cached_height= (old_height > 0) ? old_height : height ();
}

void
QTMTextPopup::showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                         int scroll_x, int scroll_y, int canvas_x,
                         int canvas_y) {
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  updateButtonsFromScheme ();
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
