// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#include "windowbar.hpp"
#include "windowbar_p.hpp"

#include <QWidget>
#include <QtCore/QDebug>
#include <QtCore/QLocale>
#include <QtGui/QtEvents>

namespace QWK {

WindowBarPrivate::WindowBarPrivate () {
  w        = nullptr;
  autoTitle= true;
  autoIcon = false;
}

WindowBarPrivate::~WindowBarPrivate ()= default;

void
WindowBarPrivate::init () {
  Q_Q (WindowBar);
  layout= new QHBoxLayout ();
  if (QLocale::system ().textDirection () == Qt::RightToLeft) {
    layout->setDirection (QBoxLayout::RightToLeft);
  }

  layout->setContentsMargins (QMargins ());
  layout->setSpacing (0);
  for (int i= IconButton; i <= CloseButton; ++i) {
    insertDefaultSpace (i);
  }
  q->setLayout (layout);
}

void
WindowBarPrivate::setWidgetAt (int index, QWidget* widget) {
  auto item     = layout->takeAt (index);
  auto orgWidget= item->widget ();
  if (orgWidget) {
    orgWidget->deleteLater ();
  }
  delete item;
  if (!widget) {
    insertDefaultSpace (index);
  }
  else {
    layout->insertWidget (index, widget);
  }
}

QWidget*
WindowBarPrivate::takeWidgetAt (int index) {
  auto item     = layout->itemAt (index);
  auto orgWidget= item->widget ();
  if (orgWidget) {
    item= layout->takeAt (index);
    delete item;
    insertDefaultSpace (index);
  }
  return orgWidget;
}

WindowBar::WindowBar (QWidget* parent)
    : WindowBar (*new WindowBarPrivate (), parent) {}

WindowBar::~WindowBar ()= default;

QMenuBar*
WindowBar::menuBar () const {
  Q_D (const WindowBar);
  return static_cast<QMenuBar*> (d->widgetAt (WindowBarPrivate::MenuWidget));
}

QLabel*
WindowBar::titleLabel () const {
  Q_D (const WindowBar);
  return static_cast<QLabel*> (d->widgetAt (WindowBarPrivate::TitleLabel));
}

QWidget*
WindowBar::titleWidget () const {
  Q_D (const WindowBar);
  return d->widgetAt (WindowBarPrivate::TitleLabel);
}

QAbstractButton*
WindowBar::iconButton () const {
  Q_D (const WindowBar);
  return static_cast<QAbstractButton*> (
      d->widgetAt (WindowBarPrivate::IconButton));
}

QAbstractButton*
WindowBar::loginButton () const {
  Q_D (const WindowBar);
  return static_cast<QAbstractButton*> (
      d->widgetAt (WindowBarPrivate::LoginButton));
}

QAbstractButton*
WindowBar::vipButton () const {
  Q_D (const WindowBar);
  return static_cast<QAbstractButton*> (
      d->widgetAt (WindowBarPrivate::VipButton));
}

QAbstractButton*
WindowBar::pinButton () const {
  Q_D (const WindowBar);
  return static_cast<QAbstractButton*> (
      d->widgetAt (WindowBarPrivate::PinButton));
}

QAbstractButton*
WindowBar::minButton () const {
  Q_D (const WindowBar);
  return static_cast<QAbstractButton*> (
      d->widgetAt (WindowBarPrivate::MinimizeButton));
}

QAbstractButton*
WindowBar::maxButton () const {
  Q_D (const WindowBar);
  return static_cast<QAbstractButton*> (
      d->widgetAt (WindowBarPrivate::MaximizeButton));
}

QAbstractButton*
WindowBar::closeButton () const {
  Q_D (const WindowBar);
  return static_cast<QAbstractButton*> (
      d->widgetAt (WindowBarPrivate::CloseButton));
}

void
WindowBar::setMenuBar (QMenuBar* menuBar) {
  Q_D (WindowBar);
  auto org= takeMenuBar ();
  if (org) org->deleteLater ();
  if (!menuBar) return;
  d->setWidgetAt (WindowBarPrivate::MenuWidget, menuBar);
  menuBar->setSizePolicy (QSizePolicy::Maximum, QSizePolicy::Minimum);
}

void
WindowBar::setTitleLabel (QLabel* label) {
  // 使用setTitleWidget来保持一致性
  setTitleWidget (label);
}

void
WindowBar::setTitleWidget (QWidget* widget) {
  Q_D (WindowBar);
  auto org= takeTitleWidget ();
  if (org) org->deleteLater ();
  if (!widget) return;
  d->setWidgetAt (WindowBarPrivate::TitleLabel, widget);
  widget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Minimum);

  // 如果设置的是QLabel，且开启了autoTitle，则设置文本
  if (QLabel* label= qobject_cast<QLabel*> (widget)) {
    if (d->autoTitle && d->w) label->setText (d->w->windowTitle ());
  }
}

void
WindowBar::setIconButton (QAbstractButton* btn) {
  Q_D (WindowBar);
  auto org= takeIconButton ();
  if (org) org->deleteLater ();
  if (!btn) return;
  d->setWidgetAt (WindowBarPrivate::IconButton, btn);
  if (d->autoIcon && d->w) btn->setIcon (d->w->windowIcon ());
  btn->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred);
}

void
WindowBar::setLoginButton (QAbstractButton* btn) {
  Q_D (WindowBar);
  auto org= takeLoginButton ();
  if (org) org->deleteLater ();
  if (!btn) return;
  d->setWidgetAt (WindowBarPrivate::LoginButton, btn);
  // 登录按钮的点击事件在qt_tm_widget.cpp中直接处理，这里不需要额外连接
}

void
WindowBar::setVipButton (QAbstractButton* btn) {
  Q_D (WindowBar);
  auto org= takeVipButton ();
  if (org) org->deleteLater ();
  if (!btn) return;
  d->setWidgetAt (WindowBarPrivate::VipButton, btn);
}

void
WindowBar::setPinButton (QAbstractButton* btn) {
  Q_D (WindowBar);
  auto org= takePinButton ();
  if (org) org->deleteLater ();
  if (!btn) return;
  d->setWidgetAt (WindowBarPrivate::PinButton, btn);
  connect (btn, &QAbstractButton::clicked, this, &WindowBar::pinRequested);
}

void
WindowBar::setMinButton (QAbstractButton* btn) {
  Q_D (WindowBar);
  auto org= takeMinButton ();
  if (org) org->deleteLater ();
  if (!btn) return;
  d->setWidgetAt (WindowBarPrivate::MinimizeButton, btn);
  connect (btn, &QAbstractButton::clicked, this, &WindowBar::minimizeRequested);
}

void
WindowBar::setMaxButton (QAbstractButton* btn) {
  Q_D (WindowBar);
  auto org= takeMaxButton ();
  if (org) org->deleteLater ();
  if (!btn) return;
  d->setWidgetAt (WindowBarPrivate::MaximizeButton, btn);
  connect (btn, &QAbstractButton::clicked, this, &WindowBar::maximizeRequested);
}

void
WindowBar::setCloseButton (QAbstractButton* btn) {
  Q_D (WindowBar);
  auto org= takeCloseButton ();
  if (org) org->deleteLater ();
  if (!btn) return;
  d->setWidgetAt (WindowBarPrivate::CloseButton, btn);
  connect (btn, &QAbstractButton::clicked, this, &WindowBar::closeRequested);
}

QMenuBar*
WindowBar::takeMenuBar () {
  Q_D (WindowBar);
  return static_cast<QMenuBar*> (
      d->takeWidgetAt (WindowBarPrivate::MenuWidget));
}

QLabel*
WindowBar::takeTitleLabel () {
  Q_D (WindowBar);
  return static_cast<QLabel*> (d->takeWidgetAt (WindowBarPrivate::TitleLabel));
}

QWidget*
WindowBar::takeTitleWidget () {
  Q_D (WindowBar);
  return d->takeWidgetAt (WindowBarPrivate::TitleLabel);
}

QAbstractButton*
WindowBar::takeIconButton () {
  Q_D (WindowBar);
  return static_cast<QAbstractButton*> (
      d->takeWidgetAt (WindowBarPrivate::IconButton));
}

QAbstractButton*
WindowBar::takeLoginButton () {
  Q_D (WindowBar);
  auto btn= static_cast<QAbstractButton*> (
      d->takeWidgetAt (WindowBarPrivate::LoginButton));
  if (!btn) {
    return nullptr;
  }
  // 登录按钮的点击事件在qt_tm_widget.cpp中直接处理，这里不需要额外断开连接
  return btn;
}

QAbstractButton*
WindowBar::takeVipButton () {
  Q_D (WindowBar);
  auto btn= static_cast<QAbstractButton*> (
      d->takeWidgetAt (WindowBarPrivate::VipButton));
  return btn;
}

QAbstractButton*
WindowBar::takePinButton () {
  Q_D (WindowBar);
  auto btn= static_cast<QAbstractButton*> (
      d->takeWidgetAt (WindowBarPrivate::PinButton));
  if (!btn) {
    return nullptr;
  }
  disconnect (btn, &QAbstractButton::clicked, this, &WindowBar::pinRequested);
  return btn;
}

QAbstractButton*
WindowBar::takeMinButton () {
  Q_D (WindowBar);
  auto btn= static_cast<QAbstractButton*> (
      d->takeWidgetAt (WindowBarPrivate::MinimizeButton));
  if (!btn) {
    return nullptr;
  }
  disconnect (btn, &QAbstractButton::clicked, this,
              &WindowBar::minimizeRequested);
  return btn;
}

QAbstractButton*
WindowBar::takeMaxButton () {
  Q_D (WindowBar);
  auto btn= static_cast<QAbstractButton*> (
      d->takeWidgetAt (WindowBarPrivate::MaximizeButton));
  if (!btn) {
    return nullptr;
  }
  disconnect (btn, &QAbstractButton::clicked, this,
              &WindowBar::maximizeRequested);
  return btn;
}

QAbstractButton*
WindowBar::takeCloseButton () {
  Q_D (WindowBar);
  auto btn= static_cast<QAbstractButton*> (
      d->takeWidgetAt (WindowBarPrivate::CloseButton));
  if (!btn) {
    return nullptr;
  }
  disconnect (btn, &QAbstractButton::clicked, this, &WindowBar::closeRequested);
  return btn;
}

QWidget*
WindowBar::hostWidget () const {
  Q_D (const WindowBar);
  return d->w;
}

void
WindowBar::setHostWidget (QWidget* w) {
  Q_D (WindowBar);

  QWidget* org= d->w;
  if (org) {
    org->removeEventFilter (this);
  }
  d_ptr->w= w;
  if (w) {
    w->installEventFilter (this);
  }
}

bool
WindowBar::titleFollowWindow () const {
  Q_D (const WindowBar);
  return d->autoTitle;
}

void
WindowBar::setTitleFollowWindow (bool value) {
  Q_D (WindowBar);
  d->autoTitle= value;
}

bool
WindowBar::iconFollowWindow () const {
  Q_D (const WindowBar);
  return d->autoIcon;
}

void
WindowBar::setIconFollowWindow (bool value) {
  Q_D (WindowBar);
  d->autoIcon= value;
}

bool
WindowBar::eventFilter (QObject* obj, QEvent* event) {
  Q_D (WindowBar);
  auto w= d->w;
  if (obj == w) {
    QAbstractButton* iconBtn    = iconButton ();
    QWidget*         titleWidget= this->titleWidget ();
    QLabel* label= qobject_cast<QLabel*> (titleWidget); // 检查是否为QLabel
    QAbstractButton* maxBtn= maxButton ();
    switch (event->type ()) {
    case QEvent::WindowIconChange: {
      if (d_ptr->autoIcon && iconBtn) {
        iconBtn->setIcon (w->windowIcon ());
        iconChanged (w->windowIcon ());
      }
      break;
    }
    case QEvent::WindowTitleChange: {
      if (d_ptr->autoTitle && label) { // 只对QLabel设置文本
        label->setText (w->windowTitle ());
        titleChanged (w->windowTitle ());
      }
      break;
    }
    case QEvent::WindowStateChange: {
      if (maxBtn) {
        maxBtn->setChecked (w->isMaximized ());
      }
      break;
    }
    default:
      break;
    }
  }
  return QWidget::eventFilter (obj, event);
}

void
WindowBar::titleChanged (const QString& text) {
  Q_UNUSED (text)
}

void
WindowBar::iconChanged (const QIcon& icon) {
  Q_UNUSED (icon)
}

WindowBar::WindowBar (WindowBarPrivate& d, QWidget* parent)
    : QFrame (parent), d_ptr (&d) {
  d.q_ptr= this;

  d.init ();
}

} // namespace QWK
