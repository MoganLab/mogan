// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
// SPDX-License-Identifier: Apache-2.0

#ifndef GUESTNOTIFICATIONBAR_H
#define GUESTNOTIFICATIONBAR_H

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace QWK {

class GuestNotificationBar : public QFrame {
  Q_OBJECT
public:
  explicit GuestNotificationBar (QWidget* parent= nullptr);
  ~GuestNotificationBar ();

  void    setMessage (const QString& message);
  QString message () const;

  void setLoginButtonVisible (bool visible);
  bool isLoginButtonVisible () const;

  void setCloseButtonVisible (bool visible);
  bool isCloseButtonVisible () const;

  QPushButton* loginButton () const;
  QPushButton* closeButton () const;

Q_SIGNALS:
  void loginRequested ();
  void closeRequested ();

private:
  void setupUI ();
  void applyStyle ();

private:
  QHBoxLayout* m_layout;
  QLabel*      m_messageLabel;
  QPushButton* m_loginButton;
  QPushButton* m_closeButton;
};

} // namespace QWK

#endif // GUESTNOTIFICATIONBAR_H