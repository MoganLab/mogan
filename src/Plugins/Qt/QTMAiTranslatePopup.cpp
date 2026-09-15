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
#include "qt_utilities.hpp"

#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QHideEvent>
#include <QHoverEvent>
#include <QPixmap>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QShowEvent>
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

  // 动作经根信号回传后由 edit_interface_rep::ai_action 统一处理（引用选区
  // 到 AI 侧边栏，翻译自动发送），润色后续接入
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
QTMAiTranslatePopup::syncHover () {
  // QQuickWidget 不为无按键的 move 合成 hover：hover 上下文需先由一次显式
  // HoverMove 激活（见 qml_load_test 的 test_ai_actions_bar_hover）。每次
  // 显示/重定位后按当前光标位置同步一次——光标在栏外时即为清空，顺带纠正
  // 上次会话残留的 hover 态
  QQuickWindow* w= quick->quickWindow ();
  if (!w) return;
  QPointF     pos (quick->mapFromGlobal (QCursor::pos ()));
  QHoverEvent hover (QEvent::HoverMove, pos, pos);
  QCoreApplication::sendEvent (w, &hover);
}

bool
QTMAiTranslatePopup::eventFilter (QObject* obj, QEvent* ev) {
  // 无按键 move 依赖「macOS → 顶层窗口 → 半透明子 widget → QQuickWidget」
  // 的逐级投递，该链路在部分会话下整体失效（悬浮点亮失灵，点击因 grab
  // 语义仍可用）。挂在 qApp 上的过滤器能看到投递给任意对象的原始 move
  // 流，不受子 widget 路由成败影响：据此直接同步 hover，绕开失效环节。
  // 栏外远处的 move 只在「刚离开栏内」时同步一次以清空高亮，避免每次
  // move 都触发 Quick 场景命中测试
  if (ev->type () == QEvent::MouseMove && isVisible ()) {
    bool inside= rect ().contains (mapFromGlobal (QCursor::pos ()));
    if (inside || hover_inside) syncHover ();
    hover_inside= inside;
  }
  return QObject::eventFilter (obj, ev);
}

void
QTMAiTranslatePopup::showEvent (QShowEvent* ev) {
  QTMBasePopup::showEvent (ev);
  hover_inside= false;
  QCoreApplication::instance ()->installEventFilter (this);
}

void
QTMAiTranslatePopup::hideEvent (QHideEvent* ev) {
  QTMBasePopup::hideEvent (ev);
  QCoreApplication::instance ()->removeEventFilter (this);
}

void
QTMAiTranslatePopup::autoSize () {
  // 尺寸按屏幕 DPI 缩放（与 QML 弹窗的 dpScale 同源），不跟随文档字体；
  // 整体尺寸随 QML 内边距/图标比例自适应。字号与会话内 DPI 绑定，鼠标
  // 移动会高频重入此处，字号未变时跳过 QML 写入与布局重算
  QQuickItem* root= quick->rootObject ();
  if (!root) return;
  int font_px= std::max (10, DpiUtils::scaled (12));
  if (font_px == cached_font_px && cached_width > 0) return;
  cached_font_px= font_px;
  root->setProperty ("fontPixelSize", font_px);
  // Text 宽度与 Row 布局的 polish 挂在渲染周期上，要到渲染一帧才结算：
  // 改完字号立即读到的 implicitWidth/Height 仍是旧字号布局的值（只有
  // 非定位器绑定同步生效），setFixedSize 会把操作栏钉在过小尺寸上、
  // 右侧按钮整块被裁。先 grab() 同步渲染一帧借道结算布局，再取尺寸。
  // Windows 缩放 125%/150%/200% 时字号必变必现；macOS 逻辑 DPI 恒为
  // 72、字号不变，加载时的初始布局即最终布局，旧代码侥幸不触发
  quick->grab ();
  // implicit 尺寸常带小数，向上取整避免削掉尾部不足 1px 的内容
  int w= int (std::ceil (root->implicitWidth ()));
  int h= int (std::ceil (root->implicitHeight ()));
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
