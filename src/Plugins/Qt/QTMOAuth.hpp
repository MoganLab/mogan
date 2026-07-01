
/******************************************************************************
 * MODULE     : QTMOAuth.cpp
 * DESCRIPTION: Mogan OAuth Module headers
 * COPYRIGHT  : (C) 2025  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTMOAUTH_H
#define QTMOAUTH_H

#ifndef OS_WASM
#include <QtNetwork/qnetworkrequestfactory.h>
#include <QtNetworkAuth/qoauth2authorizationcodeflow.h>
#include <QtNetworkAuth/qoauthhttpserverreplyhandler.h>
#endif
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

QT_FORWARD_DECLARE_CLASS (QRestAccessManager)

class QTMOAuth : public QObject {
  Q_OBJECT

public:
  QTMOAuth (QObject* parent= nullptr);
  void login ();
  bool isLoggedIn ();
  void refreshToken ();
  void handleAuthorizationCode (const QString& code);

signals:
  void loginStateChanged (bool loggedIn);

private slots:
  void checkTokenStatus ();

private:
  void    loadExistingToken ();
  QString generateCodeVerifier ();
  QString generateCodeChallenge (const QString& verifier);
  QUrl    getAuthorizationUrl ();
  QUrl    getAccessTokenUrl ();
  QString getGrowthUrl ();
  void    refreshCallbackHtml ();

  bool m_isLoggedIn= false;

public:
  void clearInvalidTokens ();

#ifndef OS_WASM
  QRestAccessManager*           network= nullptr;
  QOAuth2AuthorizationCodeFlow  oauth2;
  QOAuthHttpServerReplyHandler* m_reply;
#endif
  QTimer* m_tokenCheckTimer;
  QString m_refreshToken;
  qint64  m_tokenExpiryTime= 0;
  QString m_codeVerifier;
  QString m_codeChallenge;
  int     m_port;
};

#endif
