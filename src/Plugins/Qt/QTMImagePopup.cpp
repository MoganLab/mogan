
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
#include "qt_renderer.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "server.hpp"
#include "tm_ostream.hpp"

#include <QButtonGroup>
#include <QHelpEvent>
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QToolTip>
#include <cmath>

// 悬浮菜单创建函数
QTMImagePopup::QTMImagePopup (QWidget* parent, qt_simple_widget_rep* owner)
    : QTMBasePopup (parent, owner), current_align ("") {
  Q_INIT_RESOURCE (images);

  leftBtn= new QToolButton ();
  leftBtn->setObjectName ("base_popup_button");
  leftBtn->setProperty ("icon-name", "left");
  leftBtn->setCheckable (true);
  middleBtn= new QToolButton ();
  middleBtn->setObjectName ("base_popup_button");
  middleBtn->setProperty ("icon-name", "center");
  middleBtn->setCheckable (true);
  rightBtn= new QToolButton ();
  rightBtn->setObjectName ("base_popup_button");
  rightBtn->setProperty ("icon-name", "right");
  rightBtn->setCheckable (true);
  ocrBtn= new QToolButton ();
  ocrBtn->setObjectName ("base_popup_button");
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
             else if (button == ocrBtn) {
               call ("ocr-to-latex-by-image", current_tree);
               eval ("(when (defined? 'tutorial-notify-action) "
                     "(tutorial-notify-action \"ocr-paste\"))");
             }
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
QTMImagePopup::showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                          int scroll_x, int scroll_y, int canvas_x,
                          int canvas_y) {
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  autoSize ();
  if (!selectionInView ()) {
    hide ();
    return;
  }
  updatePosition (ren);
  updateButtonStates ();
  show ();
  raise ();
}

void
QTMImagePopup::setImageTree (tree t) {
  this->current_tree= t;
}

void
QTMImagePopup::updateButtonStates () {
  current_align= as_string (call ("get-image-alignment", current_tree));
  leftBtn->setChecked (false);
  middleBtn->setChecked (false);
  rightBtn->setChecked (false);
  if (current_align == "left") leftBtn->setChecked (true);
  else if (current_align == "center") middleBtn->setChecked (true);
  else if (current_align == "right") rightBtn->setChecked (true);
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
