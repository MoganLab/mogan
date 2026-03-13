
/******************************************************************************
 * MODULE     : QTMTextToolbar.cpp
 * DESCRIPTION: Text selection toolbar popup widget implementation
 * COPYRIGHT  : (C) 2025  Jie Chen
 *                  2026  Yifan Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMTextToolbar.hpp"
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
#include <QGuiApplication>
#include <QHelpEvent>
#include <QIcon>
#include <QLabel>
#include <QLayoutItem>
#include <QPainter>
#include <QPen>
#include <QScreen>
#include <QSizePolicy>
#include <QToolButton>
#include <QToolTip>
#include <QWidgetAction>
#include <algorithm>
#include <cmath>

// 悬浮工具栏创建函数
QTMTextToolbar::QTMTextToolbar (QWidget* parent, qt_simple_widget_rep* owner)
    : QWidget (parent), owner (owner), layout (nullptr),
      cached_selection_mid_x (0), cached_selection_mid_y (0),
      cached_scroll_x (0), cached_scroll_y (0), cached_canvas_x (0),
      cached_canvas_y (0), cached_magf (0.0), painted (false),
      painted_count (0) {
  setObjectName ("text_toolbar");
  setWindowFlags (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute (Qt::WA_ShowWithoutActivating);
  setMouseTracking (true);
  setFocusPolicy (Qt::NoFocus);
  layout= new QHBoxLayout (this);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  layout->setSpacing (1);
  setLayout (layout);

  // 添加阴影效果
  QGraphicsDropShadowEffect* effect= new QGraphicsDropShadowEffect (this);
  effect->setBlurRadius (40);
  effect->setOffset (0, 4);
  effect->setColor (QColor (0, 0, 0, 120));
  this->setGraphicsEffect (effect);

  rebuildButtonsFromScheme ();
}

QTMTextToolbar::~QTMTextToolbar () {}

void
QTMTextToolbar::clearButtons () {
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
QTMTextToolbar::rebuildButtonsFromScheme () {
  eval ("(use-modules (generic text-toolbar))");
  object menu= eval ("'(horizontal (link text-toolbar-icons))");
  object obj = call ("make-menu-widget", menu, 0);
  if (!is_widget (obj)) return;

  text_toolbar_widget  = concrete (as_widget (obj));
  QList<QAction*>* list= text_toolbar_widget->get_qactionlist ();
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
    button->setObjectName ("text-toolbar-button");
    button->setAutoRaise (true);
    button->setDefaultAction (action);
    button->setPopupMode (QToolButton::InstantPopup);
    if (tm_style_sheet == "") button->setStyle (qtmstyle ());
    layout->addWidget (button);
  }

  autoSize ();
}

void
QTMTextToolbar::showTextToolbar (qt_renderer_rep* ren, rectangle selr,
                                 double magf, int scroll_x, int scroll_y,
                                 int canvas_x, int canvas_y) {
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  if (!selectionInView ()) {
    hide ();
    return;
  }
  updatePosition (ren);
  show ();
  raise ();
}

void
QTMTextToolbar::updatePosition (qt_renderer_rep* ren) {
  if (!selectionInView ()) {
    hide ();
    return;
  }
  int x, y;
  getCachedPosition (ren, x, y);
  move (x, y);
}

void
QTMTextToolbar::scrollBy (int x, int y) {
  cached_scroll_x-= (int) (x / cached_magf);
  cached_scroll_y-= (int) (y / cached_magf);
}

void
QTMTextToolbar::cachePosition (rectangle selr, double magf, int scroll_x,
                               int scroll_y, int canvas_x, int canvas_y) {
  cached_rect    = selr;
  cached_magf    = magf;
  cached_scroll_x= scroll_x;
  cached_scroll_y= scroll_y;
  cached_canvas_x= canvas_x;
  cached_canvas_y= canvas_y;

  // 计算选区中心位置
  cached_selection_mid_x= (selr->x1 + selr->x2) * 0.5;
  cached_selection_mid_y=
      selr->y2; // 使用选区底部位置，使工具栏显示在选中文字正上方
}

void
QTMTextToolbar::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  rectangle selr            = cached_rect;
  double    inv_unit        = 1.0 / 256.0;
  double    cx_logic        = (selr->x1 + selr->x2) * 0.5;
  double    sel_top_logic   = (selr->y1 > selr->y2) ? selr->y1 : selr->y2;
  double    sel_bottom_logic= (selr->y1 > selr->y2) ? selr->y2 : selr->y1;

  // 使用公式计算QT坐标
  double cx_px=
      ((cx_logic - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double top_px= -(sel_top_logic - cached_scroll_y) * cached_magf * inv_unit;
  double bottom_px=
      -(sel_bottom_logic - cached_scroll_y) * cached_magf * inv_unit;

  // 修正：视口 > 表面：存在空白顶部
  double blank_top= 0.0;
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport () &&
      owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  top_px+= blank_top;
  bottom_px+= blank_top;

  const int above_y=
      int (std::round (top_px - cached_height - 10)); // 在选区顶部上方显示
  const int below_y=
      int (std::round (bottom_px + 10)); // 如果上面空间不够，显示在选区下方

  x= int (std::round (cx_px - cached_width * 0.5));
  y= above_y;

  // 确保工具栏在视口内
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();

    const bool above_fits= (above_y >= 0) && (above_y + cached_height <= vp_h);
    const bool below_fits= (below_y >= 0) && (below_y + cached_height <= vp_h);

    if (above_fits) y= above_y;
    else if (below_fits) y= below_y;
    else {
      x= std::max (0, (vp_w - cached_width) / 2);
      y= std::max (0, (vp_h - cached_height) / 2);
    }

    if (x < 0) x= 0;
    if (x + cached_width > vp_w) x= vp_w - cached_width;
    if (y < 0) y= 0;
    if (y + cached_height > vp_h) y= vp_h - cached_height;
  }
  else {
    if (y < 0) y= below_y;
  }
}

bool
QTMTextToolbar::selectionInView () const {
  if (!owner || !owner->scrollarea () || !owner->scrollarea ()->viewport ())
    return true;

  rectangle selr    = cached_rect;
  double    inv_unit= 1.0 / 256.0;

  double x1_px=
      ((selr->x1 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double x2_px=
      ((selr->x2 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double y1_px= -(selr->y1 - cached_scroll_y) * cached_magf * inv_unit;
  double y2_px= -(selr->y2 - cached_scroll_y) * cached_magf * inv_unit;

  double blank_top= 0.0;
  if (owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  y1_px+= blank_top;
  y2_px+= blank_top;

  double left  = std::min (x1_px, x2_px);
  double right = std::max (x1_px, x2_px);
  double top   = std::min (y1_px, y2_px);
  double bottom= std::max (y1_px, y2_px);

  int vp_w= owner->scrollarea ()->viewport ()->width ();
  int vp_h= owner->scrollarea ()->viewport ()->height ();

  if (right < 0.0 || left > vp_w) return false;
  if (bottom < 0.0 || top > vp_h) return false;
  return true;
}

void
QTMTextToolbar::autoSize () {
  // 根据DPI和缩放因子自动调整大小
  QScreen*     Screen= QGuiApplication::primaryScreen ();
  const double Dpi   = Screen ? Screen->logicalDotsPerInch () : 96.0;
  const double Scale = Dpi / 96.0;
  const double totalScale=
      Scale * cached_magf * 12.0; // 原始3.0倍，扩大4倍后为12.0倍
  int btn_size;

#if defined(Q_OS_MAC)
  btn_size= int (50 * totalScale);
#else
  btn_size= int (40 * totalScale);
#endif

  if (cached_magf <= 0.16) {
    btn_size= 25;
  }

  // 设置按钮大小
  QSize                     icon_size (btn_size, btn_size);
  QSize                     fixed_size (btn_size + 32,
                                        btn_size + 32); // 内边距也扩大4倍 (8 * 4.0 = 32)
  const QList<QToolButton*> buttons=
      findChildren<QToolButton*> (QString (), Qt::FindChildrenRecursively);
  for (QToolButton* button : buttons) {
    if (!button) continue;
    if (button->objectName ().isEmpty ())
      button->setObjectName ("text-toolbar-button");
    button->setIconSize (icon_size);
    button->setFixedSize (fixed_size);
  }

  // 调整窗口大小
  adjustSize ();
  cached_width = width ();
  cached_height= height ();
}

bool
QTMTextToolbar::eventFilter (QObject* obj, QEvent* event) {
  // 处理事件过滤
  return QWidget::eventFilter (obj, event);
}
