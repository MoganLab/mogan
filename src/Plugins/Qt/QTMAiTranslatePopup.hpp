/******************************************************************************
 * MODULE     : QTMAiTranslatePopup.hpp
 * DESCRIPTION: AI action bar (translate/polish/chat) shown below the selection
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_AI_TRANSLATE_POPUP_HPP
#define QT_AI_TRANSLATE_POPUP_HPP

#include "QTMBasePopup.hpp"
#include "rectangles.hpp"

class QQuickWidget;
class QTimer;

// 选区下方的一行 AI 操作栏：龙虾标识 + 翻译 / 润色 / 对话（QML 渲染）
class QTMAiTranslatePopup : public QTMBasePopup {
  Q_OBJECT

public:
  QTMAiTranslatePopup (QWidget* parent, qt_simple_widget_rep* owner);

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override;
  void autoSize () override;

  // 停止光标跟踪并隐藏：编辑器侧所有「不想显示」路径（选区取消/点击
  // 外部/dismiss/偏好关闭等）都经 hide_translate_popup 汇入此处；
  // showPopup 会重新开始跟踪
  void disarm ();

  // 选择方向（由下往上为 true）：与锚行同时由编辑器侧算出，须在 showPopup
  // 前调用——定位只读缓存，不回查编辑器活态
  void setUpward (bool upward) { cached_upward= upward; }

protected:
  // 水平居中于「最后选中文字」所在行，纵向按选择方向取该行下方/上方，
  // 首选侧放不下时退到另一侧
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y) override;

  // 悬停状态在每次显示时复位
  void showEvent (QShowEvent* ev) override;

private slots:
  // QML 根信号 triggered(action) 的接收槽（translate/polish/chat）
  void onActionTriggered (const QString& action);
  // 光标轮询：驱动显隐（离选区过远即隐藏，靠近重新显示——不依赖事件
  // 投递，覆盖事件流失效会话）与悬浮高亮同步
  void pollCursor ();

private:
  // 向离屏 scene 发 HoverMove（quick 本地坐标）：激活 hover 上下文并纠正
  // 残留态；坐标未变时由调用方跳过
  void syncHover (QPointF pos);

  // 光标是否在「锚行矩形（最后选中文字所在行）∪ 操作栏自身矩形」外扩
  // margin 的邻近区内：悬浮到操作栏上不算远离，避免自隐藏
  bool cursorNearSelection (const QPoint& global) const;

  // 统一显示闸门：光标远离锚行或选区移出视口时不显示（前者保持跟踪，
  // 靠近后由轮询拉起）；showPopup 与轮询复现都经此进入
  void present ();

  QQuickWidget* quick;
  // 光标跟踪轮询（显隐与悬浮的唯一驱动）：move 事件可能在 QPA 层被整体
  // 吞掉，轮询只读系统光标位置，不依赖事件投递；showPopup 启动、
  // disarm 停止，隐藏期间仍运行（靠近需重新显示）
  QTimer* hover_timer;
  // 上次显示时的选择方向：定位输出仅为缓存的函数
  bool cached_upward= false;
  // 上次同步时光标是否在栏内：决定「进入/栏内移动/首次离开」三态是否需要
  // 同步，栏外远处的 move 不再触发 Quick 场景命中测试
  bool hover_inside= false;
  // 上次实际发出的 HoverMove 坐标（quick 本地）：静止悬停不重发
  QPointF last_sync_pos;
  // 上次 autoSize 使用的字号：DPI 不变时跳过 QML 写入与布局重算
  int cached_font_px= 0;
};

#endif // QT_AI_TRANSLATE_POPUP_HPP
