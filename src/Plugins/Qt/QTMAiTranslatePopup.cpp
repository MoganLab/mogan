/******************************************************************************
 * MODULE     : QTMAiTranslatePopup.cpp
 * DESCRIPTION: AI action bar (translate/polish/chat) shown below the selection
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMAiTranslatePopup.hpp"
#include "QTMStyle.hpp"
#include "edit_interface.hpp"
#include "qt_utilities.hpp"

#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QHoverEvent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <algorithm>
#include <cmath>

QTMAiTranslatePopup::QTMAiTranslatePopup (QWidget*              parent,
                                          qt_simple_widget_rep* owner)
    : QTMBasePopup (parent, owner) {
  // QML 自绘圆角底板与假阴影，宿主窗口需透明（子 widget 经顶层 backing
  // store 合成即可，勿设 WA_NativeWindow：macOS 上原生子窗口收不到无按键
  // mouseMoved，按钮 hover 会失效）；基类的 widget 阴影对 QQuickWidget
  // 离屏渲染不生效，关掉
  setAttribute (Qt::WA_TranslucentBackground);
  effect->setEnabled (false);

  // software 场景图后端与主题上下文（dpScale/isDark）与 QTMQmlDialog 同源，
  // 收敛在 qt_utilities 共享助手
  qt_use_software_scene_graph ();

  quick= new QQuickWidget (this);
  quick->setResizeMode (QQuickWidget::SizeViewToRootObject);
  quick->setClearColor (Qt::transparent);
  qt_inject_theme_context (quick);
  // 按钮文案（translate 只折叠首字符，"Ai translate" 命中词典键 "ai
  // translate"）
  quick->rootContext ()->setContextProperty ("labelTranslate",
                                             qt_translate ("Ai translate"));
  quick->rootContext ()->setContextProperty ("labelPolish",
                                             qt_translate ("Ai polish"));
  quick->rootContext ()->setContextProperty ("labelChat",
                                             qt_translate ("Ai chat"));
  quick->setSource (QUrl ("qrc:/qml/AiActionsBar.qml"));

  layout->setContentsMargins (0, 0, 0, 0);
  layout->addWidget (quick);

  // 第一步仅挂接显隐，点击后的翻译/润色/对话流程在后续任务接入
  if (QQuickItem* root= quick->rootObject ()) {
    QObject::connect (root, SIGNAL (triggered (QString)), this,
                      SLOT (onActionTriggered ()));
  }
}

void
QTMAiTranslatePopup::onActionTriggered () {
  if (edit_interface_rep* ed= dynamic_cast<edit_interface_rep*> (this->owner)) {
    ed->dismiss_translate_popup ();
  }
}

void
QTMAiTranslatePopup::syncHover () {
  // 弹窗可能在静止光标正下方弹出/重定位（滚动跟随选区时尤甚），这不会有
  // 任何鼠标事件到来，按当前光标位置向离屏 scene 发一次 HoverMove 同步
  // hover 态——光标在栏外时即为清空，顺带纠正上次隐藏前残留的 hover 态
  QQuickWindow* w= quick->quickWindow ();
  if (!w) return;
  QPointF     pos (quick->mapFromGlobal (QCursor::pos ()));
  QHoverEvent hover (QEvent::HoverMove, pos, pos);
  QCoreApplication::sendEvent (w, &hover);
}

void
QTMAiTranslatePopup::autoSize () {
  // 按钮字号略大于选区内最小文字的渲染高度（含文档缩放因子），整体尺寸随
  // QML 内边距/图标比例自适应；取不到时退回 mini 控件字号。showPopup 在
  // 选区存续期间被高频触发（apply_changes/鼠标移动/滚动），字号未变时直接
  // 跳过整套 QML 重排与定尺寸
  QObject* root= quick->rootObject ();
  if (!root) return;
  double inv_unit= 1.0 / 256.0;
  double text_px=
      sel_text_height > 0 ? sel_text_height * cached_magf * inv_unit : 0;
  int font_px= text_px > 0 ? std::max (9, int (std::round (text_px * 1.05)))
                           : std::max (10, qt_zoom (QTM_MINI_FONTSIZE) * 4 / 3);
  if (font_px == cached_font_px) return;
  cached_font_px= font_px;
  root->setProperty ("fontPixelSize", font_px);
  int w= int (std::round (root->property ("implicitWidth").toReal ()));
  int h= int (std::round (root->property ("implicitHeight").toReal ()));
  quick->setFixedSize (w, h);
  setFixedSize (w, h);
  cached_width = w;
  cached_height= h;
}

void
QTMAiTranslatePopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  (void) ren;
  rectangle selr    = cached_rect;
  double    inv_unit= 1.0 / 256.0;
  // 选区矩形不变式 y1 < y2：y2 为上缘、y1 为下缘（逻辑坐标 y 向上）
  double sel_top_logic   = selr->y2;
  double sel_bottom_logic= selr->y1;

  double left_px=
      ((selr->x1 - cached_scroll_x) * cached_magf + cached_canvas_x) * inv_unit;
  double top_px= -(sel_top_logic - cached_scroll_y) * cached_magf * inv_unit;
  double bottom_px=
      -(sel_bottom_logic - cached_scroll_y) * cached_magf * inv_unit;

  // 视口大于画布表面时顶部存在居中留白，需补偿（同基类算法）
  double blank_top= blank_top_offset ();
  top_px+= blank_top;
  bottom_px+= blank_top;

  const int gap= 4;
  // 始终显示在选中文字的下一行：与最末选区行左缘对齐、位于其下方
  x= int (std::round (left_px));
  y= int (std::round (bottom_px + gap));

  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_w= owner->scrollarea ()->viewport ()->width ();
    int vp_h= owner->scrollarea ()->viewport ()->height ();

    // 下方放不下时退到选区上方，最终裁剪到视口内
    if (y + cached_height > vp_h) {
      y= int (std::round (top_px - cached_height - gap));
    }
    if (x < 0) x= 0;
    if (x + cached_width > vp_w) x= vp_w - cached_width;
    if (y < 0) y= 0;
    if (y + cached_height > vp_h) y= vp_h - cached_height;
  }
}

void
QTMAiTranslatePopup::showPopup (qt_renderer_rep* ren, rectangle selr,
                                double magf, int scroll_x, int scroll_y,
                                int canvas_x, int canvas_y) {
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  autoSize ();
  if (!selectionInView ()) {
    hide ();
    return;
  }
  updatePosition (ren);
  show ();
  raise ();
  syncHover ();
}
