/******************************************************************************
 * MODULE     : qt_flow_layout.hpp
 * DESCRIPTION: 自动换行流式布局（仿 Qt widgets/flowlayout 示例，BSD）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_FLOW_LAYOUT_HPP
#define QT_FLOW_LAYOUT_HPP

#include <QLayout>
#include <QStyle>

/**
 * @brief 流式布局：子项从左到右排列，宽度不足时自动换行。
 *
 * 行内左对齐（不拉伸子项、不均摊剩余宽度），几何变化时由 Qt 布局系统
 * 自动重排，无需调用方干预。
 */
class QtFlowLayout : public QLayout {
public:
  explicit QtFlowLayout (QWidget* parent, int hSpacing= -1, int vSpacing= -1)
      : QLayout (parent), hSpace_ (hSpacing), vSpace_ (vSpacing) {}
  ~QtFlowLayout () override;

  void         addItem (QLayoutItem* item) override { items_.append (item); }
  int          count () const override { return items_.size (); }
  QLayoutItem* itemAt (int index) const override {
    return items_.value (index);
  }
  QLayoutItem*     takeAt (int index) override;
  Qt::Orientations expandingDirections () const override {
    return Qt::Orientations (0);
  }
  bool  hasHeightForWidth () const override { return true; }
  int   heightForWidth (int width) const override;
  QSize sizeHint () const override { return minimumSize (); }
  QSize minimumSize () const override;
  void  setGeometry (const QRect& rect) override;

private:
  int doLayout (const QRect& rect, bool testOnly) const;
  int smartSpacing (QStyle::PixelMetric pm) const;

  QList<QLayoutItem*> items_;
  int                 hSpace_;
  int                 vSpace_;
};

#endif // QT_FLOW_LAYOUT_HPP
