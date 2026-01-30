/******************************************************************************
 * MODULE     : QTMCodePopup.cpp
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2026
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMCodePopup.hpp"
#include "qt_renderer.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"

#include <QAction>
#include <QGuiApplication>
#include <QScreen>
#include <cmath>

// 悬浮菜单创建函数
QTMCodePopup::QTMCodePopup (QWidget* parent, qt_simple_widget_rep* owner)
    : QWidget (parent), owner (owner), layout (nullptr), cached_scroll_x (0),
      cached_scroll_y (0), cached_canvas_x (0), cached_canvas_y (0),
      cached_width (0), cached_height (0), cached_magf (0.0), copyBtn (nullptr),
      langBtn (nullptr), langMenu (nullptr), langGroup (nullptr),
      painted (false), painted_count (0) {
  setObjectName ("code_popup");
  setWindowFlags (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute (Qt::WA_ShowWithoutActivating);
  setMouseTracking (true);
  setFocusPolicy (Qt::NoFocus);

  layout= new QHBoxLayout (this);
  layout->setContentsMargins (6, 6, 6, 6);
  layout->setSpacing (2);
  setLayout (layout);

  effect= new QGraphicsDropShadowEffect (this);
  effect->setBlurRadius (40);
  effect->setOffset (0, 4);
  effect->setColor (QColor (0, 0, 0, 120));
  this->setGraphicsEffect (effect);

  copyBtn= new QToolButton ();
  copyBtn->setObjectName ("code-popup-button");
  copyBtn->setText (qt_translate ("copy"));
  copyBtn->setCheckable (false);
  copyBtn->setToolTip (qt_translate ("copy"));
  layout->addWidget (copyBtn);

  langBtn= new QToolButton ();
  langBtn->setObjectName ("code-popup-button");
  langBtn->setText (qt_translate ("language"));
  langBtn->setCheckable (false);
  langBtn->setToolTip (qt_translate ("select a language"));
  layout->addWidget (langBtn);

  langMenu = new QMenu (this);
  langGroup= new QActionGroup (this);
  langGroup->setExclusive (true);
  langBtn->setMenu (langMenu);
  langBtn->setPopupMode (QToolButton::InstantPopup);

  eval ("(use-modules (prog prog-edit))");
  connect (copyBtn, &QToolButton::clicked, this,
           [=] () { call ("code-popup-copy", current_tree); });
}

QTMCodePopup::~QTMCodePopup () {}

void
QTMCodePopup::showCodePopup (qt_renderer_rep* ren, rectangle selr, double magf,
                             int scroll_x, int scroll_y, int canvas_x,
                             int canvas_y) {
  if (painted) return;
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  autoSize ();
  refreshLanguageMenu ();
  int x, y;
  getCachedPosition (ren, x, y);
  move (x, y);
  if (painted_count == 2) {
    show ();
    painted= true;
  }
  else {
    painted_count++;
  }
}

void
QTMCodePopup::setCodeTree (tree t) {
  current_tree= t;
}

void
QTMCodePopup::scrollBy (int x, int y) {
  cached_scroll_x-= (int) (x / cached_magf);
  cached_scroll_y-= (int) (y / cached_magf);
}

void
QTMCodePopup::updatePosition (qt_renderer_rep* ren) {
  int pos_x, pos_y;
  getCachedPosition (ren, pos_x, pos_y);
  move (pos_x, pos_y);
}

// 根据DPI缩放和缩放比例自动调整按钮大小和窗口尺寸
void
QTMCodePopup::autoSize () {
  QScreen*     Screen    = QGuiApplication::primaryScreen ();
  const double Dpi       = Screen ? Screen->logicalDotsPerInch () : 96.0;
  const double Scale     = Dpi / 96.0;
  const int    baseWidth = 180;
  const int    baseHeight= 36;
  double       totalScale= Scale * cached_magf * 3.0;
  if (cached_magf <= 0.16) {
    cached_width = baseWidth;
    cached_height= baseHeight;
  }
  else {
    cached_width = int (baseWidth * totalScale);
    cached_height= int (baseHeight * totalScale);
  }
  setFixedSize (cached_width, cached_height);
}

void
QTMCodePopup::refreshLanguageMenu () {
  if (!langMenu || !langGroup) return;
  langMenu->clear ();
  if (langGroup) {
    delete langGroup;
    langGroup= new QActionGroup (this);
    langGroup->setExclusive (true);
  }
  const list<string> entries=
      as_list_string (call ("code-popup-language-options", current_tree));
  const string current=
      as_string (call ("code-popup-language-current", current_tree));
  for (list<string> it= entries; !is_nil (it); it= it->next) {
    string entry= it->item;
    int    sep  = -1;
    for (int i= 0; i < N (entry); ++i) {
      if (entry[i] == '\t') {
        sep= i;
        break;
      }
    }
    if (sep < 0) continue;
    string   label= entry (0, sep);
    string   tag  = entry (sep + 1, N (entry));
    QAction* act  = new QAction (qt_translate (label), langMenu);
    act->setData (to_qstring (tag));
    act->setCheckable (true);
    if (tag == current) act->setChecked (true);
    langGroup->addAction (act);
    langMenu->addAction (act);
    connect (act, &QAction::triggered, this, [=] () {
      string target= from_qstring (act->data ().toString ());
      call ("code-popup-set-language", current_tree, target);
    });
  }
}

// 缓存菜单显示位置
void
QTMCodePopup::cachePosition (rectangle selr, double magf, int scroll_x,
                             int scroll_y, int canvas_x, int canvas_y) {
  cached_rect    = selr;
  cached_scroll_x= scroll_x;
  cached_scroll_y= scroll_y;
  cached_canvas_x= canvas_x;
  cached_canvas_y= canvas_y;
  cached_magf    = magf;
}

// 计算菜单显示位置
void
QTMCodePopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  rectangle selr       = cached_rect;
  double    inv_unit   = 1.0 / 256.0;
  double    right_logic= selr->x2;
  double    top_logic  = selr->y2;

  double right_px=
      ((right_logic - cached_scroll_x) * cached_magf + cached_canvas_x) *
      inv_unit;
  double top_px= -(top_logic - cached_scroll_y) * cached_magf * inv_unit;

  double blank_top= 0.0;
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport () &&
      owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  top_px+= blank_top;

  x= int (std::round (right_px - cached_width));
  y= int (std::round (top_px - cached_height));
  (void) ren;
}
