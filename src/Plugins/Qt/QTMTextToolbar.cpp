/******************************************************************************
 * MODULE     : QTMTextToolbar.cpp
 * DESCRIPTION: Text selection toolbar popup widget implementation
 * COPYRIGHT  : (C) 2025
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMTextToolbar.hpp"
#include "bitmap_font.hpp"
#include "moebius/data/scheme.hpp"
#include "qbuttongroup.h"
#include "qt_renderer.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "server.hpp"
#include "tm_ostream.hpp"

#include <QGuiApplication>
#include <QHelpEvent>
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QScreen>
#include <QToolTip>
#include <cmath>

// 悬浮工具栏创建函数
QTMTextToolbar::QTMTextToolbar (QWidget* parent, qt_simple_widget_rep* owner)
    : QWidget (parent), owner (owner), layout (nullptr),
      cached_selection_mid_x (0), cached_selection_mid_y (0),
      cached_scroll_x (0), cached_scroll_y (0), cached_canvas_x (0),
      cached_canvas_y (0), cached_magf (0.0), painted (false),
      painted_count (0) {
  Q_INIT_RESOURCE (images);
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

  // 创建按钮
  boldBtn= new QToolButton ();
  boldBtn->setObjectName ("text-toolbar-button");
  boldBtn->setProperty ("icon-name", "bold");
  boldBtn->setCheckable (true);
  boldBtn->setToolTip (qt_translate ("Bold"));

  italicBtn= new QToolButton ();
  italicBtn->setObjectName ("text-toolbar-button");
  italicBtn->setProperty ("icon-name", "italic");
  italicBtn->setCheckable (true);
  italicBtn->setToolTip (qt_translate ("Italic"));

  underlineBtn= new QToolButton ();
  underlineBtn->setObjectName ("text-toolbar-button");
  underlineBtn->setProperty ("icon-name", "underline");
  underlineBtn->setCheckable (true);
  underlineBtn->setToolTip (qt_translate ("Underline"));

  highlightBtn= new QToolButton ();
  highlightBtn->setObjectName ("text-toolbar-button");
  highlightBtn->setProperty ("icon-name", "highlight");
  highlightBtn->setCheckable (true);
  highlightBtn->setToolTip (qt_translate ("Highlight"));

  colorBtn= new QToolButton ();
  colorBtn->setObjectName ("text-toolbar-button");
  colorBtn->setProperty ("icon-name", "color");
  colorBtn->setCheckable (false);
  colorBtn->setToolTip (qt_translate ("Text color"));

  alignLeftBtn= new QToolButton ();
  alignLeftBtn->setObjectName ("text-toolbar-button");
  alignLeftBtn->setProperty ("icon-name", "left-align");
  alignLeftBtn->setCheckable (true);
  alignLeftBtn->setToolTip (qt_translate ("Align left"));

  alignCenterBtn= new QToolButton ();
  alignCenterBtn->setObjectName ("text-toolbar-button");
  alignCenterBtn->setProperty ("icon-name", "middle-align");
  alignCenterBtn->setCheckable (true);
  alignCenterBtn->setToolTip (qt_translate ("Align center"));

  alignRightBtn= new QToolButton ();
  alignRightBtn->setObjectName ("text-toolbar-button");
  alignRightBtn->setProperty ("icon-name", "right-align");
  alignRightBtn->setCheckable (true);
  alignRightBtn->setToolTip (qt_translate ("Align right"));

  // 添加按钮到布局
  layout->addWidget (boldBtn);
  layout->addWidget (italicBtn);
  layout->addWidget (underlineBtn);
  layout->addWidget (highlightBtn);
  layout->addWidget (colorBtn);
  layout->addWidget (alignLeftBtn);
  layout->addWidget (alignCenterBtn);
  layout->addWidget (alignRightBtn);

  // 连接按钮点击事件
  connect (boldBtn, &QToolButton::clicked, this, [this] () {
    if (this->owner) {
      call ("toggle-bold");
    }
  });
  connect (italicBtn, &QToolButton::clicked, this, [this] () {
    if (this->owner) {
      call ("toggle-italic");
    }
  });
  connect (underlineBtn, &QToolButton::clicked, this, [this] () {
    if (this->owner) {
      call ("toggle-underlined");
    }
  });
  connect (highlightBtn, &QToolButton::clicked, this, [this] () {
    if (this->owner) {
      // 高亮功能
      call ("mark-text");
    }
  });
  connect (colorBtn, &QToolButton::clicked, this, [this] () {
    if (this->owner) {
      // 文字颜色功能 - 打开颜色选择器
      call ("interactive", object ("open-color-selector"));
    }
  });

  // 对齐按钮分组
  QButtonGroup* alignGroup= new QButtonGroup (this);
  alignGroup->addButton (alignLeftBtn);
  alignGroup->addButton (alignCenterBtn);
  alignGroup->addButton (alignRightBtn);
  alignGroup->setExclusive (true);

  connect (alignGroup,
           QOverload<QAbstractButton*>::of (&QButtonGroup::buttonClicked), this,
           [=] (QAbstractButton* button) {
             if (this->owner) {
               if (button == alignLeftBtn) {
                 call ("make-line-with", object ("par-mode"), object ("left"));
               }
               else if (button == alignCenterBtn) {
                 call ("make-line-with", object ("par-mode"),
                       object ("center"));
               }
               else if (button == alignRightBtn) {
                 call ("make-line-with", object ("par-mode"), object ("right"));
               }
             }
           });

  autoSize ();
}

QTMTextToolbar::~QTMTextToolbar () {}

void
QTMTextToolbar::showTextToolbar (qt_renderer_rep* ren, rectangle selr,
                                 double magf, int scroll_x, int scroll_y,
                                 int canvas_x, int canvas_y) {
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  updatePosition (ren);
  show ();
  raise ();
}

void
QTMTextToolbar::updatePosition (qt_renderer_rep* ren) {
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
  rectangle selr    = cached_rect;
  double    inv_unit= 1.0 / 256.0;
  double    cx_logic= (selr->x1 + selr->x2) * 0.5;
  double top_logic= selr->y2; // 使用选区底部位置，使工具栏显示在选中文字正上方

  // 使用公式计算QT坐标
  double cx_px=
      ((cx_logic - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double top_px= -(top_logic - cached_scroll_y) * cached_magf * inv_unit;

  // 修正：视口 > 表面：存在空白顶部
  double blank_top= 0.0;
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport () &&
      owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  top_px+= blank_top;

  x= int (std::round (cx_px - cached_width * 0.5));
  y= int (std::round (top_px -
                      cached_height)); // 在选区底部上方显示，与图片悬浮窗口一致

  // 确保工具栏在视口内
  if (x < 0) x= 0;
  if (y < 0)
    y= int (std::round (top_px + 10)); // 如果上面空间不够，显示在选区下方
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();
    if (x + cached_width > vp_w) x= vp_w - cached_width;
    if (y + cached_height > vp_h) y= vp_h - cached_height;
  }
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
  QSize icon_size (btn_size, btn_size);
  boldBtn->setIconSize (icon_size);
  italicBtn->setIconSize (icon_size);
  underlineBtn->setIconSize (icon_size);
  highlightBtn->setIconSize (icon_size);
  colorBtn->setIconSize (icon_size);
  alignLeftBtn->setIconSize (icon_size);
  alignCenterBtn->setIconSize (icon_size);
  alignRightBtn->setIconSize (icon_size);

  // 设置按钮固定大小
  QSize fixed_size (btn_size + 32,
                    btn_size + 32); // 内边距也扩大4倍 (8 * 4.0 = 32)
  boldBtn->setFixedSize (fixed_size);
  italicBtn->setFixedSize (fixed_size);
  underlineBtn->setFixedSize (fixed_size);
  highlightBtn->setFixedSize (fixed_size);
  colorBtn->setFixedSize (fixed_size);
  alignLeftBtn->setFixedSize (fixed_size);
  alignCenterBtn->setFixedSize (fixed_size);
  alignRightBtn->setFixedSize (fixed_size);

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
