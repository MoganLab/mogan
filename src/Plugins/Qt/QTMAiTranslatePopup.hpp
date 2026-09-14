/******************************************************************************
 * MODULE     : QTMAiTranslatePopup.hpp
 * DESCRIPTION: AI translate button popup shown next to the text selection
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

class QPushButton;

class QTMAiTranslatePopup : public QTMBasePopup {
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
  // 定位到选区最末文字的右方（垂直居中），越界时退到左侧并裁剪到视口内
  void getCachedPosition (qt_renderer_rep* ren, int& x, int& y) override;

private:
  QPushButton* translateButton;
  SI           sel_text_height= 0;
};

#endif // QT_AI_TRANSLATE_POPUP_HPP
