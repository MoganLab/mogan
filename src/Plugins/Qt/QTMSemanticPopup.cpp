/******************************************************************************
 * MODULE     : QTMSemanticPopup.cpp
 * DESCRIPTION: Implementation of semantic popup
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMSemanticPopup.hpp"
#include "QTMStyle.hpp"
#include "bitmap_font.hpp"
#include "qt_picture.hpp"
#include "qt_renderer.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "server.hpp"
#include "tm_ostream.hpp"

#include <QEnterEvent>
#include <QIcon>
#include <QLayoutItem>
#include <QSize>
#include <cmath>

extern s7_scheme* tm_s7;

QTMSemanticPopup::QTMSemanticPopup (QWidget* parent, qt_simple_widget_rep* owner)
    : QTMBasePopup (parent, owner), current_tag (""), fade_anim (nullptr),
      hide_timer (nullptr) {
  setObjectName ("base_popup");

  layout->setContentsMargins (4, 3, 4, 3);
  layout->setSpacing (2);

  fade_anim= new QPropertyAnimation (this, "windowOpacity", this);
  fade_anim->setDuration (150);

  hide_timer= new QTimer (this);
  hide_timer->setSingleShot (true);
  hide_timer->setInterval (250);
  connect (hide_timer, &QTimer::timeout, this, [this] () {
    startFadeOut ();
  });
}

QTMSemanticPopup::~QTMSemanticPopup () {}

void
QTMSemanticPopup::clearButtons () {
  for (QToolButton* btn : buttons) {
    if (btn) {
      layout->removeWidget (btn);
      btn->deleteLater ();
    }
  }
  buttons.clear ();
}

void
QTMSemanticPopup::rebuildButtons () {
  clearButtons ();
  if (current_tag == "" || is_nil (current_tree)) return;

  tree acts;
  if (tm_s7 != nullptr) {
    eval ("(use-modules (generic semantic-popup))");
    acts= as_tree (call ("semantic-popup-actions", current_tag, current_tree));
  }
  else {
    if (current_tag == "equation*") {
      acts= compound (
          "actions", compound ("action", "copy-latex", "Copy LaTeX", "tm_copy"),
          compound ("action", "toggle-number", "Add Number", "tm_numbered"));
    }
    else if (current_tag == "equation") {
      acts= compound (
          "actions", compound ("action", "copy-latex", "Copy LaTeX", "tm_copy"),
          compound ("action", "toggle-number", "Hide Number", "tm_numbered"));
    }
    else if (current_tag == "table-of-contents" ||
             current_tag == "table-of-contents*") {
      acts= compound (
          "actions",
          compound ("action", "refresh-toc", "Refresh TOC", "tm_reload"));
    }
  }
  if (!is_compound (acts, "actions")) return;

  for (int i= 0; i < N (acts); ++i) {
    tree act= acts[i];
    if (is_compound (act, "action") && N (act) >= 3) {
      string act_id   = act[0]->label;
      string label_str= act[1]->label;
      string icon_name= act[2]->label;

      QToolButton* btn= new QToolButton (this);
      btn->setObjectName ("base_popup_button");
      btn->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);

      QString q_label= qt_translate (label_str);
      btn->setText (q_label);
      btn->setToolTip (q_label);

      if (icon_name != "") {
        QIcon ico= qt_load_icon (icon_name);
        if (!ico.isNull ()) {
          btn->setIcon (ico);
          int icon_s= DpiUtils::scaled (16);
          btn->setIconSize (QSize (icon_s, icon_s));
        }
      }

      int btn_h= DpiUtils::scaled (28);
      btn->setFixedHeight (btn_h);
      if (tm_style_sheet == "") {
        btn->setStyle (qtmstyle ());
      }

      connect (btn, &QToolButton::clicked, this, [this, act_id, btn] () {
        handleAction (act_id, btn);
      });

      layout->addWidget (btn);
      buttons.append (btn);
    }
  }
}

void
QTMSemanticPopup::setSemanticNode (string tag, tree t) {
  if (current_tag != tag || current_tree != t) {
    current_tag = tag;
    current_tree= t;
    rebuildButtons ();
  }
}

void
QTMSemanticPopup::showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                             int scroll_x, int scroll_y, int canvas_x,
                             int canvas_y) {
  cancelFadeOut ();
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
QTMSemanticPopup::autoSize () {
  int icon_s= DpiUtils::scaled (16);
  int btn_h = DpiUtils::scaled (28);
  for (QToolButton* btn : buttons) {
    if (btn) {
      btn->setIconSize (QSize (icon_s, icon_s));
      btn->setFixedHeight (btn_h);
    }
  }
  adjustSize ();
  QSize sz     = layout ? layout->sizeHint () : sizeHint ();
  setFixedSize (sz);
  cached_width = sz.width ();
  cached_height= sz.height ();
}

void
QTMSemanticPopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  (void) ren;
  double x1_px, x2_px, top_px, bottom_px;
  selectionRectPixels (x1_px, x2_px, top_px, bottom_px);

  double    left_px = std::min (x1_px, x2_px);
  const int margin_x= 0;
  const int margin_y= 6;

  int target_x    = int (std::round (left_px + margin_x));
  int above_y     = int (std::round (top_px - cached_height - margin_y));
  int inside_top_y= int (std::round (top_px + margin_y));
  int below_y     = int (std::round (bottom_px + margin_y));

  x= target_x;
  y= above_y;

  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();

    const bool above_fits = (above_y >= 0) && (above_y + cached_height <= vp_h);
    const bool inside_fits=
        (inside_top_y >= 0) && (inside_top_y + cached_height <= vp_h);
    const bool below_fits= (below_y >= 0) && (below_y + cached_height <= vp_h);

    if (above_fits) y= above_y;
    else if (inside_fits) y= inside_top_y;
    else if (below_fits) y= below_y;
    else {
      x= std::max (0, (vp_w - cached_width) / 2);
      y= std::max (0, (vp_h - cached_height) / 2);
    }

    clampToViewport (x, y);
  }
  else {
    if (y < 0) y= below_y;
  }
}

void
QTMSemanticPopup::startFadeOut () {
  if (!isVisible ()) return;
  fade_anim->stop ();
  fade_anim->setStartValue (windowOpacity ());
  fade_anim->setEndValue (0.0);
  disconnect (fade_anim, &QPropertyAnimation::finished, nullptr, nullptr);
  connect (fade_anim, &QPropertyAnimation::finished, this, [this] () {
    hide ();
    setWindowOpacity (1.0);
  });
  fade_anim->start ();
}

void
QTMSemanticPopup::cancelFadeOut () {
  hide_timer->stop ();
  fade_anim->stop ();
  setWindowOpacity (1.0);
}

void
QTMSemanticPopup::enterEvent (QEnterEvent* event) {
  cancelFadeOut ();
  QWidget::enterEvent (event);
}

void
QTMSemanticPopup::leaveEvent (QEvent* event) {
  hide_timer->start ();
  QWidget::leaveEvent (event);
}

void
QTMSemanticPopup::handleAction (const string& action_id, QToolButton* btn) {
  if (tm_s7 == nullptr) return;

  if (action_id == "copy-latex") {
    call ("semantic-copy-latex", current_tree);
    if (btn) {
      QString original_text= btn->text ();
      btn->setText (qt_translate ("Copied!"));
      QTimer::singleShot (1200, btn, [btn, original_text] () {
        if (btn) btn->setText (original_text);
      });
    }
  }
  else if (action_id == "toggle-number") {
    call ("semantic-toggle-equation-number", current_tree);
    if (current_tag == "equation*") {
      current_tag= "equation";
    }
    else if (current_tag == "equation") {
      current_tag= "equation*";
    }
    rebuildButtons ();
    autoSize ();
    if (owner) {
      qt_renderer_rep* ren= the_qt_renderer ();
      updatePosition (ren);
    }
  }
  else if (action_id == "refresh-toc") {
    call ("update-document", "table-of-contents");
  }
  else {
    call ("semantic-popup-action-trigger", current_tag, action_id,
          current_tree);
  }
}
