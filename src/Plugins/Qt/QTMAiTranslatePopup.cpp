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
#include "edit_interface.hpp"
#include "qt_chat_controller.hpp"
#include "qt_renderer.hpp"
#include "qt_utilities.hpp"

#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QHoverEvent>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QShowEvent>
#include <QTimer>
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

  // scene graph 固定 software 后端：与 QTMQmlDialog 一致——Metal/RHI 后端下
  // 透明 clear color 会被合成成黑色方块；图形 API 是进程级全局选择，须赶在
  // 首个 QQuickWidget 构造前设定（本类与 QTMQmlDialog 是仅有的两个构造点，
  // 两处都设、值相同，谁先生效都一样）
  static const bool sgApiInitialized= [] () {
    QQuickWindow::setGraphicsApi (QSGRendererInterface::Software);
    return true;
  }();
  (void) sgApiInitialized;

  quick= new QQuickWidget (this);
  quick->setResizeMode (QQuickWidget::SizeViewToRootObject);
  quick->setClearColor (Qt::transparent);
  quick->setStyleSheet ("background: transparent;");
  // 与模态弹窗一致的主题上下文（Theme 单例读取 dpScale/isDark）
  quick->rootContext ()->setContextProperty ("dpScale",
                                             DpiUtils::scaleFactor ());
  bool isDark=
      occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet);
  quick->rootContext ()->setContextProperty ("isDark", isDark);
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

  // 光标跟踪轮询（显隐与悬浮的唯一驱动）：无按键 move 可能断在 QPA 层
  // （macOS 上 s_windowUnderMouse 陈旧等会话态会让 QNSView 整体吞掉 move，
  // 事件不会成为 QMouseEvent，qApp 过滤器同样看不到）。轮询读系统级光标
  // 位置，不依赖事件投递，任何会话下都能正常工作
  hover_timer= new QTimer (this);
  // 16ms ≈ 60Hz 一帧：响应延迟上界，低于感知阈值，再快无收益
  hover_timer->setInterval (16);
  QObject::connect (hover_timer, SIGNAL (timeout ()), this,
                    SLOT (pollCursor ()));

  // 动作经根信号回传后由 edit_interface_rep::ai_action 统一处理（翻译/对话
  // 引用选区到 AI 侧边栏，润色走 trigger-diff-text，等同 Tab 快捷键）
  if (QQuickItem* root= quick->rootObject ()) {
    QObject::connect (root, SIGNAL (triggered (QString)), this,
                      SLOT (onActionTriggered (QString)));
  }
}

void
QTMAiTranslatePopup::onActionTriggered (const QString& action) {
  if (edit_interface_rep* ed= dynamic_cast<edit_interface_rep*> (this->owner)) {
    // 动作统一入口（引用选区到 AI 侧边栏的流程与快捷键共用），见
    // edit_interface_rep::ai_action
    ed->ai_action (from_qstring_utf8 (action));
  }
}

void
QTMAiTranslatePopup::syncHover (QPointF pos) {
  // QQuickWidget 不为无按键的 move 合成 hover：hover 上下文需先由一次显式
  // HoverMove 激活（见 qml_load_test 的 test_ai_actions_bar_hover）。每次
  // 显示/重定位后按当前光标位置同步一次——光标在栏外时即为清空，顺带纠正
  // 上次会话残留的 hover 态
  QQuickWindow* w= quick->quickWindow ();
  if (!w) return;
  QHoverEvent hover (QEvent::HoverMove, pos, pos);
  QCoreApplication::sendEvent (w, &hover);
  last_sync_pos= pos;
}

void
QTMAiTranslatePopup::pollCursor () {
  // 一轮双责：显隐（光标离选区过远即隐藏，靠近重新显示）与悬浮同步。
  // 隐藏但仍在跟踪（光标过远）时只判显隐
  QPoint g (QCursor::pos ());
  if (!isVisible ()) {
    if (cursorNearSelection (g)) present ();
    return;
  }
  if (!cursorNearSelection (g)) {
    hide ();
    return;
  }
  // 三态（进入/栏内/刚离开）叠加坐标未变跳过：静止悬停不重发
  bool inside= rect ().contains (mapFromGlobal (g));
  if (inside || hover_inside) {
    QPointF pos (quick->mapFromGlobal (g));
    if (pos != last_sync_pos) syncHover (pos);
  }
  hover_inside= inside;
}

bool
QTMAiTranslatePopup::cursorNearSelection (const QPoint& global) const {
  // 邻近区 = 锚行矩形（最后选中文字所在行）∪ 操作栏自身矩形（画布像素），
  // 四周外扩 80px：操作栏可能宽于锚行，悬浮在栏上不算「离选中文字过远」。
  // 注意锚行只是选区的一行，高于约两个 margin 的选区中部会被判远——
  // 如需全选区邻近再从编辑器侧传入包围盒
  if (!parentWidget ()) return true;
  double x1, x2, top, bottom;
  selectionRectPixels (x1, x2, top, bottom);
  const double margin= 80;
  QRectF       sel (QPointF (x1, top), QPointF (x2, bottom));
  return sel.united (geometry ())
      .adjusted (-margin, -margin, margin, margin)
      .contains (parentWidget ()->mapFromGlobal (global));
}

void
QTMAiTranslatePopup::present () {
  // 统一显示闸门：光标远离锚行（保持跟踪，靠近后由轮询拉起）或选区移出
  // 视口（updatePosition 已隐藏）都不显示
  if (!cursorNearSelection (QCursor::pos ())) {
    hide ();
    return;
  }
  if (!updatePosition (the_qt_renderer ())) return;
  show ();
  raise ();
  syncHover (quick->mapFromGlobal (QCursor::pos ()));
}

void
QTMAiTranslatePopup::disarm () {
  hover_timer->stop ();
  hide ();
}

void
QTMAiTranslatePopup::showEvent (QShowEvent* ev) {
  QTMBasePopup::showEvent (ev);
  hover_inside= false;
}

void
QTMAiTranslatePopup::autoSize () {
  // 尺寸按屏幕 DPI 缩放（与 QML 弹窗的 dpScale 同源），不跟随文档字体；
  // 整体尺寸随 QML 内边距/图标比例自适应。字号与会话内 DPI 绑定，鼠标
  // 移动会高频重入此处，字号未变时跳过 QML 写入与布局重算
  QObject* root= quick->rootObject ();
  if (!root) return;
  int font_px= std::max (10, DpiUtils::scaled (12));
  if (font_px == cached_font_px && cached_width > 0) return;
  cached_font_px= font_px;
  root->setProperty ("fontPixelSize", font_px);
  int w=
      int (std::round (QQmlProperty::read (root, "implicitWidth").toReal ()));
  int h=
      int (std::round (QQmlProperty::read (root, "implicitHeight").toReal ()));
  quick->setFixedSize (w, h);
  setFixedSize (w, h);
  cached_width = w;
  cached_height= h;
}

void
QTMAiTranslatePopup::getCachedPosition (qt_renderer_rep* ren, int& x, int& y) {
  (void) ren;
  double center_px, top_px, bottom_px;
  selectionEdgePixels (center_px, top_px, bottom_px);

  const int gap= 4;
  // 水平居中于「最后选中文字」所在行
  x= int (std::round (center_px - cached_width * 0.5));

  // 向下选择显示在锚行下方、向上选择显示在锚行上方；首选侧放不下退到
  // 另一侧（两侧均放不下时由 clampToViewport 收敛到视口内，结果一致）
  int first = cached_upward ? int (std::round (top_px - cached_height - gap))
                            : int (std::round (bottom_px + gap));
  int backup= cached_upward ? int (std::round (bottom_px + gap))
                            : int (std::round (top_px - cached_height - gap));
  y         = first;
  if (owner && owner->scrollarea () && owner->scrollarea ()->viewport ()) {
    int vp_h= owner->scrollarea ()->viewport ()->height ();
    if (y < 0 || y + cached_height > vp_h) y= backup;
  }
  clampToViewport (x, y);
}

void
QTMAiTranslatePopup::showPopup (qt_renderer_rep* ren, rectangle selr,
                                double magf, int scroll_x, int scroll_y,
                                int canvas_x, int canvas_y) {
  (void) ren;
  cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  autoSize ();
  // 编辑器 show 调用即开始光标跟踪；仅未运行时启动——update 随鼠标移动
  // 高频重入，反复 start() 会重置间隔把轮询饿死。显示闸门（光标远近/
  // 选区出视口）统一在 present
  if (!hover_timer->isActive ()) hover_timer->start ();
  present ();
}
