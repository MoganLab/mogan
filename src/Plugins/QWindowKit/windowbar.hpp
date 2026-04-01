// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef WINDOWBAR_H
#define WINDOWBAR_H

#include <QAbstractButton>
#include <QFrame>
#include <QLabel>
#include <QMenuBar>

namespace QWK {

class WindowBarPrivate;

class WindowBar : public QFrame {
  Q_OBJECT
  Q_DECLARE_PRIVATE (WindowBar)
public:
  explicit WindowBar (QWidget* parent= nullptr);
  ~WindowBar ();

public:
  QMenuBar*        menuBar () const;
  QLabel*          titleLabel () const;  // 保留的QLabel接口，可能返回nullptr
  QWidget*         titleWidget () const; // 新接口：获取任意类型的标题Widget
  QAbstractButton* iconButton () const;
  QAbstractButton* loginButton () const;
  QAbstractButton* pinButton () const;
  QAbstractButton* minButton () const;
  QAbstractButton* maxButton () const;
  QAbstractButton* closeButton () const;

  void setMenuBar (QMenuBar* menuBar);
  void setTitleLabel (QLabel* label);
  void setTitleWidget (QWidget* widget); // 新接口：支持任意QWidget
  void setIconButton (QAbstractButton* btn);
  void setLoginButton (QAbstractButton* btn);
  void setPinButton (QAbstractButton* btn);
  void setMinButton (QAbstractButton* btn);
  void setMaxButton (QAbstractButton* btn);
  void setCloseButton (QAbstractButton* btn);

  QMenuBar*        takeMenuBar ();
  QLabel*          takeTitleLabel ();
  QWidget*         takeTitleWidget (); // 新接口：获取标题Widget
  QAbstractButton* takeIconButton ();
  QAbstractButton* takeLoginButton ();
  QAbstractButton* takePinButton ();
  QAbstractButton* takeMinButton ();
  QAbstractButton* takeMaxButton ();
  QAbstractButton* takeCloseButton ();

  QWidget* hostWidget () const;
  void     setHostWidget (QWidget* w);

  bool titleFollowWindow () const;
  void setTitleFollowWindow (bool value);

  bool iconFollowWindow () const;
  void setIconFollowWindow (bool value);

Q_SIGNALS:
  void loginRequested ();
  void pinRequested (bool pin= false);
  void minimizeRequested ();
  void maximizeRequested (bool max= false);
  void closeRequested ();

protected:
  bool eventFilter (QObject* obj, QEvent* event) override;

  virtual void titleChanged (const QString& text);
  virtual void iconChanged (const QIcon& icon);

protected:
  WindowBar (WindowBarPrivate& d, QWidget* parent= nullptr);

  QScopedPointer<WindowBarPrivate> d_ptr;
};

} // namespace QWK

#endif // WINDOWBAR_H