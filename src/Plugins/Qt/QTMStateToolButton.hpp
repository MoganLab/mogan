
/******************************************************************************
 * MODULE     : QTMStateToolButton.hpp
 * DESCRIPTION: 支持通过 CSS qproperty-iconNormal/iconChecked/iconHovered
 *切换图标的 QToolButton COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTMSTATETOOLBUTTON_HPP
#define QTMSTATETOOLBUTTON_HPP

#include <QBasicTimer>
#include <QEvent>
#include <QToolButton>

class QTMStateToolButton : public QToolButton {
  Q_OBJECT
  Q_PROPERTY (QIcon iconNormal READ iconNormal WRITE setIconNormal FINAL)
  Q_PROPERTY (QIcon iconChecked READ iconChecked WRITE setIconChecked FINAL)
  Q_PROPERTY (QIcon iconHovered READ iconHovered WRITE setIconHovered FINAL)
public:
  explicit QTMStateToolButton (QWidget* parent= nullptr)
      : QToolButton (parent) {
    // toggled 在 setChecked() 完全结束后发射，此时 isChecked() 已稳定，
    // 比 checkStateSet() 更可靠地触发图标切换。
    connect (this, &QTMStateToolButton::toggled, this,
             &QTMStateToolButton::reloadIcon, Qt::DirectConnection);
  }

  QIcon iconNormal () const { return iconNormal_; }
  void  setIconNormal (const QIcon& icon) {
    iconNormal_= icon;
    reloadIcon ();
  }

  QIcon iconChecked () const { return iconChecked_; }
  void  setIconChecked (const QIcon& icon) {
    iconChecked_= icon;
    reloadIcon ();
  }

  QIcon iconHovered () const { return iconHovered_; }
  void  setIconHovered (const QIcon& icon) {
    iconHovered_= icon;
    reloadIcon ();
  }

  /**
   * @brief 切换处理中状态：on 时正常图标按 45° 步进旋转（处理动画），
   *        off 时停止并恢复常规图标。
   */
  void setProcessing (bool on) {
    if (processing_ == on) return;
    processing_= on;
    if (on) {
      spinAngle_= 0;
      spinTimer_.start (80, this);
    }
    else {
      spinTimer_.stop ();
      reloadIcon ();
    }
  }
  bool processing () const { return processing_; }

protected:
  bool event (QEvent* event) override {
    if (event->type () == QEvent::Enter || event->type () == QEvent::Leave) {
      reloadIcon ();
    }
    return QToolButton::event (event);
  }

  void timerEvent (QTimerEvent* event) override {
    if (event->timerId () != spinTimer_.timerId ()) {
      QToolButton::timerEvent (event);
      return;
    }
    // QPixmap::transformed 保持尺寸居中与原 DPR，直接作为下一帧图标
    if (iconNormal_.isNull ()) return;
    spinAngle_ = (spinAngle_ + 45) % 360;
    QPixmap src= iconNormal_.pixmap (iconSize ());
    setIcon (QIcon (src.transformed (QTransform ().rotate (spinAngle_),
                                     Qt::SmoothTransformation)));
  }

private:
  QIcon       iconNormal_;
  QIcon       iconChecked_;
  QIcon       iconHovered_;
  QBasicTimer spinTimer_;
  int         spinAngle_ = 0;
  bool        processing_= false;

  void reloadIcon () {
    // 处理中图标由旋转帧驱动，hover/checked 图标切换暂缓
    if (processing_) return;
    if (underMouse () && !iconHovered_.isNull ()) {
      setIcon (iconHovered_);
      return;
    }
    if (isChecked () && !iconChecked_.isNull ()) {
      setIcon (iconChecked_);
      return;
    }
    if (!iconNormal_.isNull ()) {
      setIcon (iconNormal_);
    }
  }
};

#endif // QTMSTATETOOLBUTTON_HPP
