
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

protected:
  bool event (QEvent* event) override {
    if (event->type () == QEvent::Enter || event->type () == QEvent::Leave) {
      reloadIcon ();
    }
    return QToolButton::event (event);
  }

private:
  QIcon iconNormal_;
  QIcon iconChecked_;
  QIcon iconHovered_;

  void reloadIcon () {
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
