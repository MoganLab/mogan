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

// 选区下方的一行 AI 操作栏：🦞 标识 + 翻译 / 润色 / 对话（QML 渲染）
class QTMAiTranslatePopup : public QTMBasePopup {
  Q_OBJECT

public:
  QTMAiTranslatePopup (QWidget* parent, qt_simple_widget_rep* owner);

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override;
  void autoSize () override;

  /**
   * @brief 设置选区内最小文字的渲染高度（编辑器逻辑单位），
   * 按钮字号与整体尺寸据此缩放
   */
  void setTextHeight (SI h) { sel_text_height= h; }

protected:
  // 定位到选区最末行的下一行（左缘对齐），下方放不下时退到选区上方
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y) override;

private slots:
  // QML 根信号 triggered(action) 的接收槽（translate/polish/chat；字符串式
  // connect 允许槽省略信号参数，后续接动作流程时再带上）
  void onActionTriggered ();

private:
  // 显示/重定位后按当前光标位置向离屏 scene 发 HoverMove：弹窗可能在静止
  // 光标正下方出现（滚动跟随选区时尤甚），不会有鼠标事件到来，须主动同步
  // （在栏外即为清空，顺带纠正上次隐藏前残留的 hover 态）
  void syncHover ();

  QQuickWidget* quick;
  SI            sel_text_height= 0;
  int           cached_font_px = -1;
};

#endif // QT_AI_TRANSLATE_POPUP_HPP
