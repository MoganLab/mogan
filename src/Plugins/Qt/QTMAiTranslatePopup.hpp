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

// 选区下方的一行 AI 操作栏：龙虾标识 + 翻译 / 润色 / 对话（QML 渲染）
class QTMAiTranslatePopup : public QTMBasePopup {
  Q_OBJECT

public:
  QTMAiTranslatePopup (QWidget* parent, qt_simple_widget_rep* owner);

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override;
  void autoSize () override;

  // 选择方向（由下往上为 true）：与锚行同时由编辑器侧算出，须在 showPopup
  // 前调用——定位只读缓存，不回查编辑器活态
  void setUpward (bool upward) { cached_upward= upward; }

protected:
  // 水平居中于「最后选中文字」所在行，纵向按选择方向取该行下方/上方，
  // 首选侧放不下时退到另一侧
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y) override;

  // qApp 级截获无按键 move，持续同步 hover（见 cpp，悬浮可靠性关键）
  bool eventFilter (QObject* obj, QEvent* ev) override;
  // 过滤器仅在显示期间挂载，隐藏即卸载
  void showEvent (QShowEvent* ev) override;
  void hideEvent (QHideEvent* ev) override;

private slots:
  // QML 根信号 triggered(action) 的接收槽（translate/polish/chat）
  void onActionTriggered (const QString& action);

private:
  // 按当前光标位置向离屏 scene 发 HoverMove：激活 hover 上下文并纠正残留态
  void syncHover ();

  QQuickWidget* quick;
  // 上次显示时的选择方向：定位输出仅为缓存的函数
  bool cached_upward= false;
  // 上次同步时光标是否在栏内：决定「进入/栏内移动/首次离开」三态是否需要
  // 同步，栏外远处的 move 不再触发 Quick 场景命中测试
  bool hover_inside= false;
  // 上次 autoSize 使用的字号：DPI 不变时跳过 QML 写入与布局重算
  int cached_font_px= 0;
};

#endif // QT_AI_TRANSLATE_POPUP_HPP
