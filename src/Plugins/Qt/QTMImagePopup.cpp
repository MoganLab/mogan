
/******************************************************************************
 * MODULE     : QTMImagePopup.cpp
 * DESCRIPTION:
 * COPYRIGHT  : (C) 2025 MoonLL, Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMImagePopup.hpp"
#include "bitmap_font.hpp"
#include "qbuttongroup.h"
#include "qt_renderer.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "server.hpp"
#include "tm_ostream.hpp"

#include <QHelpEvent>
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QToolTip>
#include <cmath>

const string left_str = "\"left\"";
const string mid_str  = "\"center\"";
const string right_str= "\"right\"";

// 悬浮菜单创建函数
QTMImagePopup::QTMImagePopup (QWidget* parent, qt_simple_widget_rep* owner)
    : QWidget (parent), owner (owner), layout (nullptr), cached_image_mid_x (0),
      cached_image_mid_y (0), cached_scroll_x (0), cached_scroll_y (0),
      cached_canvas_x (0), cached_canvas_y (0), cached_magf (0.0),
      current_align (""), painted (false), painted_count (0) {
  Q_INIT_RESOURCE (images);
  setObjectName ("image_popup");
  setWindowFlags (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute (Qt::WA_ShowWithoutActivating);
  setMouseTracking (true);
  setFocusPolicy (Qt::NoFocus);
  layout= new QHBoxLayout (this);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  layout->setSpacing (1);
  setLayout (layout);

  QGraphicsDropShadowEffect* effect= new QGraphicsDropShadowEffect (this);
  effect->setBlurRadius (40);
  effect->setOffset (0, 4);
  effect->setColor (QColor (0, 0, 0, 120));
  this->setGraphicsEffect (effect);

  leftBtn= new QToolButton ();
  leftBtn->setObjectName ("image-align-button");
  leftBtn->setProperty ("icon-name", "left");
  leftBtn->setCheckable (true);
  middleBtn= new QToolButton ();
  middleBtn->setObjectName ("image-align-button");
  middleBtn->setProperty ("icon-name", "center");
  middleBtn->setCheckable (true);
  rightBtn= new QToolButton ();
  rightBtn->setObjectName ("image-align-button");
  rightBtn->setProperty ("icon-name", "right");
  rightBtn->setCheckable (true);
  ocrBtn= new QToolButton ();
  ocrBtn->setObjectName ("image-align-button");
  ocrBtn->setProperty ("icon-name", "ocr");
  // 设置tooltip - 由于tooltip会挡住图片，建议用户将鼠标移到按钮右侧查看完整提示
#if defined(Q_OS_MAC)
  ocrBtn->setToolTip (qt_translate ("Copy the image and press Command+Shift+v "
                                    "to paste the OCR recognition result"));
#else
  ocrBtn->setToolTip (qt_translate ("Copy the image and press Ctrl+Shift+v to "
                                    "paste the OCR recognition result"));
#endif
  QButtonGroup* alignGroup= new QButtonGroup (this);
  alignGroup->addButton (leftBtn);
  alignGroup->addButton (middleBtn);
  alignGroup->addButton (rightBtn);
  alignGroup->addButton (ocrBtn);
  alignGroup->setExclusive (true);
  eval ("(use-modules (liii ocr))");
  connect (alignGroup,
           QOverload<QAbstractButton*>::of (&QButtonGroup::buttonClicked), this,
           [=] (QAbstractButton* button) {
             if (button == leftBtn)
               call ("set-image-alignment", current_tree, "left");
             else if (button == middleBtn)
               call ("set-image-alignment", current_tree, "center");
             else if (button == rightBtn)
               call ("set-image-alignment", current_tree, "right");
             else if (button == ocrBtn)
               call ("ocr-to-latex-by-image", current_tree);
             current_align=
                 as_string (call ("get-image-alignment", current_tree));
           });
  layout->addWidget (leftBtn);
  layout->addWidget (middleBtn);
  layout->addWidget (rightBtn);
  layout->addWidget (ocrBtn);

  // 为OCR按钮安装事件过滤器，以便控制tooltip位置
  ocrBtn->installEventFilter (this);
}

QTMImagePopup::~QTMImagePopup () {}

// 显示图片悬浮菜单，根据缩放比例决定是否显示
void
QTMImagePopup::showImagePopup (qt_renderer_rep* ren, rectangle selr,
                               double magf, int scroll_x, int scroll_y,
                               int canvas_x, int canvas_y) {
  if (painted) return;
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  autoSize ();
  int x, y;
  getCachedPosition (ren, x, y);
  move (x, y);
  updateButtonStates ();
  if (painted_count == 2) {
    show ();
    painted= true;
  }
  else {
    painted_count++;
  }
}

void
QTMImagePopup::setImageTree (tree t) {
  this->current_tree= t;
}

void
QTMImagePopup::updateButtonStates () {
  if (current_align == "")
    current_align= as_string (call ("get-image-alignment", current_tree));
  if (current_align == "left") leftBtn->setChecked (true);
  else if (current_align == "center") middleBtn->setChecked (true);
  else if (current_align == "right") rightBtn->setChecked (true);
}

void
QTMImagePopup::scrollBy (int x, int y) {
  cached_scroll_x-= (int) (x / cached_magf);
  cached_scroll_y-= (int) (y / cached_magf);
}

void
QTMImagePopup::updatePosition (qt_renderer_rep* ren) {
  int pos_x, pos_y;
  getCachedPosition (ren, pos_x, pos_y);
  move (pos_x, pos_y);
}

// 根据DPI缩放和图片缩放比例自动调整按钮大小和窗口尺寸
void
QTMImagePopup::autoSize () {
  const double Scale= DpiUtils::scaleFactor ();
  // 基准窗口大小（逻辑像素）
  const int baseWidth = 200;
  const int baseHeight= 50;
  double    totalScale= Scale * cached_magf * 2.0; // 减小缩放因子
  int       IconSize  = int (40 * totalScale);
  if (cached_magf <= 0.16) {
    // 文档缩放很小时，使用固定大小（仅DPI缩放）
    cached_width = DpiUtils::scaled (169);
    cached_height= DpiUtils::scaled (42);
    IconSize     = 25;
    setFixedSize (cached_width, cached_height);
  }
  else {
    cached_width = int (baseWidth * totalScale);
    cached_height= int (baseHeight * totalScale);
    this->resize (cached_width, cached_height);
  }
  leftBtn->setIconSize (QSize (IconSize, IconSize));
  middleBtn->setIconSize (QSize (IconSize, IconSize));
  rightBtn->setIconSize (QSize (IconSize, IconSize));
  ocrBtn->setIconSize (QSize (IconSize, IconSize));
}

// 缓存菜单显示位置
void
QTMImagePopup::cachePosition (rectangle selr, double magf, int scroll_x,
                              int scroll_y, int canvas_x, int canvas_y) {
  cached_image_mid_x= (selr->x1 + selr->x2) / 2;
  cached_image_mid_y= selr->y2;
  cached_rect       = selr;
  cached_scroll_x   = scroll_x;
  cached_scroll_y   = scroll_y;
  cached_canvas_x   = canvas_x;
  cached_canvas_y   = canvas_y;
  cached_magf       = magf;
}

// 计算菜单显示位置 / 2
void
QTMImagePopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  rectangle selr     = cached_rect;
  double    inv_unit = 1.0 / 256.0;
  double    cx_logic = (selr->x1 + selr->x2) * 0.5;
  double    top_logic= selr->y2;

  // use the formula to calculate the QT coordinates
  double cx_px=
      ((cx_logic - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double top_px= -(top_logic - cached_scroll_y) * cached_magf * inv_unit;

  // fix: viewport > surface: blank_top exists
  double blank_top= 0.0;
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport () &&
      owner->scrollarea ()->surface ()) {
    int vp_h  = owner->scrollarea ()->viewport ()->height ();
    int surf_h= owner->scrollarea ()->surface ()->height ();
    if (vp_h > surf_h) blank_top= (vp_h - surf_h) * 0.5;
  }
  top_px+= blank_top;

  x= int (std::round (cx_px - cached_width * 0.5));
  y= int (std::round (top_px - cached_height));
}

// 事件过滤器，用于控制OCR按钮的tooltip位置
bool
QTMImagePopup::eventFilter (QObject* obj, QEvent* event) {
  if (obj == ocrBtn && event->type () == QEvent::ToolTip) {
    QHelpEvent* helpEvent= static_cast<QHelpEvent*> (event);
    // 获取按钮的全局位置
    QPoint globalPos= ocrBtn->mapToGlobal (QPoint (0, 0));
    // 计算tooltip应该显示的位置（按钮右侧，垂直居中）
    // 垂直方向需要根据平台调整，因为不同平台的QToolTip行为可能不同
#if defined(Q_OS_MAC)
    // macOS上使用正偏移
    QPoint tooltipPos=
        globalPos + QPoint (ocrBtn->width () + 10, -ocrBtn->height () * 3 / 4);
#else
    // 其他平台使用负偏移
    QPoint tooltipPos=
        globalPos + QPoint (ocrBtn->width () + 10, -ocrBtn->height () / 4);
#endif

    // 显示tooltip在按钮右侧
    QToolTip::showText (tooltipPos, ocrBtn->toolTip (), ocrBtn);
    return true; // 事件已处理
  }
  // 其他事件传递给基类处理
  return QWidget::eventFilter (obj, event);
}