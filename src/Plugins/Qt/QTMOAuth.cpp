
/******************************************************************************
 * MODULE     : QTMOAuth.cpp
 * DESCRIPTION: Mogan OAuth Module impl
 * COPYRIGHT  : (C) 2025  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMOAuth.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "tm_sys_utils.hpp"

#include <QtGui/qdesktopservices.h>

#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qrestaccessmanager.h>
#include <QtNetwork/qrestreply.h>

#include <QtCore/qcryptographichash.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qrandom.h>
#include <QtCore/qurlquery.h>

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qtcpserver.h>

QTMOAuth::QTMOAuth (QObject* parent) {
  // 加载 OAuth2 配置
  eval ("(use-modules (liii account))");

  c_string clientIdentifier (
      as_string (call ("account-oauth2-config", "client-identifier")));
  // c_string clientSecret (
  //     as_string (call ("account-oauth2-config", "client-secret")));
  c_string scope (as_string (call ("account-oauth2-config", "scope")));
  c_string portListStr (
      as_string (call ("account-oauth2-config", "port-list")));

  // 解析端口列表字符串，格式如
  // "6029,8087,9256,7438,5173,6391,8642,9901,44118,55055,1895"
  QString    portListQStr= QString ((char*) portListStr).trimmed ();
  QList<int> portList;

  if (!portListQStr.isEmpty ()) {
    QStringList portStrs= portListQStr.split (',', Qt::SkipEmptyParts);
    for (const QString& portStr : portStrs) {
      bool ok;
      int  port= portStr.toInt (&ok);
      if (ok && port > 0 && port <= 65535) {
        portList.append (port);
      }
    }
  }

  // 如果解析失败，使用默认端口
  if (portList.isEmpty ()) {
    portList.append (6029);
  }

  // 找到第一个未被占用的端口
  m_port= -1;
  for (int port : portList) {
    QTcpServer testServer;
    if (testServer.listen (QHostAddress::LocalHost, port)) {
      m_port= port;
      testServer.close ();
      break;
    }
  }

  // 如果所有端口都被占用，使用第一个端口
  if (m_port == -1) {
    m_port= portList.first ();
    debug_boot << "All ports occupied, using:" << m_port << "\n";
  }
  else {
    debug_boot << "Using available port:" << m_port << "\n";
  }

  m_reply= new QOAuthHttpServerReplyHandler (
      QHostAddress (QString::fromUtf8 ("127.0.0.1")), m_port, this);
  m_reply->setCallbackPath ("/callback");

  // 生成PKCE参数
  m_codeVerifier = generateCodeVerifier ();
  m_codeChallenge= generateCodeChallenge (m_codeVerifier);

  // 设置自定义成功页面
  QString customHtml=
      "<!doctype html><html><head>"
      "<meta charset='utf-8'>"
      "<style>body{font-family:Arial;background:#f2f2f2;margin:0;padding:20px}"
      "h2{color:#000;margin:0}</style>"
      "</head><body>"
      "<h2>亲爱的三鲤用户，恭喜登录成功！您可以关闭此页面并返回应用。</h2>"
      "<script>window.setTimeout(function(){window.close()},3000);</script>"
      "</body></html>";
  m_reply->setCallbackText (customHtml);

  oauth2.setReplyHandler (m_reply);
  oauth2.setScope ((char*) scope);
  oauth2.setClientIdentifier ((char*) clientIdentifier);

  connect (&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this,
           &QDesktopServices::openUrl);

  // 连接回调URL捕获信号
  connect (m_reply, &QOAuthHttpServerReplyHandler::callbackReceived, this,
           [this] (const QVariantMap& values) {
             // 提取授权码
             if (values.contains ("code")) {
               QString code= values["code"].toString ();

               // 手动处理授权码交换
               handleAuthorizationCode (code);
             }
             else {
               //  debug_boot << "No authorization code found in callback" <<
               //  "\n";
             }
           });

  // 初始化定时器用于定期检查token状态
  m_tokenCheckTimer= new QTimer (this);
  connect (m_tokenCheckTimer, &QTimer::timeout, this,
           &QTMOAuth::checkTokenStatus);
  m_tokenCheckTimer->start (90000); // 每一分半检查一次

  // 加载现有的token信息
  loadExistingToken ();
}

void
QTMOAuth::login () {
  if (m_reply->isListening ()) {
    // 手动构建授权URL
    QUrl      authUrl (getAuthorizationUrl ());
    QUrlQuery query;
    query.addQueryItem ("response_type", "code");
    query.addQueryItem ("client_id", oauth2.clientIdentifier ());
    query.addQueryItem ("redirect_uri",
                        QString ("http://127.0.0.1:%1/callback").arg (m_port));
    query.addQueryItem ("scope", oauth2.scope ());
    query.addQueryItem ("code_challenge", m_codeChallenge);
    query.addQueryItem ("code_challenge_method", "S256");

    authUrl.setQuery (query);
    // 手动打开浏览器进行授权
    QDesktopServices::openUrl (authUrl);
  }
}

bool
QTMOAuth::isLoggedIn () {
  return m_isLoggedIn;
}

void
QTMOAuth::handleAuthorizationCode (const QString& code) {
  // 手动交换授权码为访问令牌
  QUrl      url (getAccessTokenUrl ());
  QUrlQuery query;
  query.addQueryItem ("grant_type", "authorization_code");
  query.addQueryItem ("code", code);
  query.addQueryItem ("redirect_uri",
                      QString ("http://127.0.0.1:%1/callback").arg (m_port));
  query.addQueryItem ("client_id", oauth2.clientIdentifier ());
  query.addQueryItem ("code_verifier", m_codeVerifier);

  QNetworkRequest request (url);
  request.setHeader (QNetworkRequest::ContentTypeHeader,
                     "application/x-www-form-urlencoded");
  request.setRawHeader ("User-Agent",
                        to_qstring (stem_user_agent ()).toUtf8 ());
  request.setRawHeader ("X-Device-Id",
                        to_qstring (stem_device_id ()).toUtf8 ());

  QNetworkAccessManager* manager= new QNetworkAccessManager (this);
  QNetworkReply*         reply=
      manager->post (request, query.toString (QUrl::FullyEncoded).toUtf8 ());

  connect (reply, &QNetworkReply::finished, this, [this, reply, manager] {
    if (reply->error () == QNetworkReply::NoError) {
      QByteArray response= reply->readAll ();

      QJsonDocument doc= QJsonDocument::fromJson (response);
      QJsonObject   obj= doc.object ();

      if (obj.contains ("access_token")) {
        QString accessToken = obj["access_token"].toString ();
        QString refreshToken= obj["refresh_token"].toString ();
        int     expiresIn   = obj["expires_in"].toInt ();

        // 设置token
        oauth2.setToken (accessToken);

        // 保存token信息
        eval ("(use-modules (liii account))");
        call ("account-save-token", from_qstring (accessToken));

        // 设置登录状态
        m_isLoggedIn= true;

        if (!refreshToken.isEmpty ()) {
          m_refreshToken= refreshToken;
          call ("account-save-refresh-token", from_qstring (refreshToken));
        }

        m_tokenExpiryTime= QDateTime::currentSecsSinceEpoch () + expiresIn;
        call ("account-save-token-expiry",
              from_qstring (QString::number (m_tokenExpiryTime)));

        // 发出登录状态变化信号
        emit loginStateChanged (true);

        debug_boot << "Token exchange successful" << "\n";
      }
      else {
        debug_boot << "Token exchange failed: Invalid response" << "\n";
      }
    }
    else {
      debug_boot << "Token exchange failed:"
                 << from_qstring (reply->errorString ()) << "\n";
    }

    reply->deleteLater ();
    manager->deleteLater ();
  });
}

void
QTMOAuth::refreshToken () {
  debug_std << "Start refresh token..." << "\n";
  if (m_refreshToken.isEmpty ()) {
    // 清除无效的token信息
    clearInvalidTokens ();
    return;
  }

  // 使用refresh_token刷新access_token
  QUrl      url (getAccessTokenUrl ());
  QUrlQuery query;
  query.addQueryItem ("grant_type", "refresh_token");
  query.addQueryItem ("refresh_token", m_refreshToken);
  query.addQueryItem ("client_id", oauth2.clientIdentifier ());

  QNetworkRequest request (url);
  request.setHeader (QNetworkRequest::ContentTypeHeader,
                     "application/x-www-form-urlencoded");
  request.setRawHeader ("User-Agent",
                        to_qstring (stem_user_agent ()).toUtf8 ());
  request.setRawHeader ("X-Device-Id",
                        to_qstring (stem_device_id ()).toUtf8 ());

  // 发送刷新请求
  QNetworkAccessManager* manager= new QNetworkAccessManager (this);
  QNetworkReply*         reply=
      manager->post (request, query.toString (QUrl::FullyEncoded).toUtf8 ());

  connect (reply, &QNetworkReply::finished, this, [this, reply, manager] {
    if (reply->error () == QNetworkReply::NoError) {
      QByteArray response= reply->readAll ();

      QJsonDocument doc= QJsonDocument::fromJson (response);
      QJsonObject   obj= doc.object ();

      if (obj.contains ("access_token")) {
        QString newAccessToken = obj["access_token"].toString ();
        QString newRefreshToken= obj["refresh_token"].toString ();
        int     expiresIn      = obj["expires_in"].toInt ();

        // 更新token
        oauth2.setToken (newAccessToken);

        // 保存新的token信息
        eval ("(use-modules (liii account))");
        call ("account-save-token", from_qstring (newAccessToken));

        if (!newRefreshToken.isEmpty ()) {
          m_refreshToken= newRefreshToken;
          call ("account-save-refresh-token", from_qstring (newRefreshToken));
          debug_boot << "Token refreshed successfully" << "\n";
        }
        else {
          debug_boot << "No new refresh token received, keeping existing one"
                     << "\n";
        }

        // 计算并保存新的过期时间
        m_tokenExpiryTime= QDateTime::currentSecsSinceEpoch () + expiresIn;
        call ("account-save-token-expiry",
              from_qstring (QString::number (m_tokenExpiryTime)));

        // 确保登录状态为true
        if (!m_isLoggedIn) {
          m_isLoggedIn= true;
          emit loginStateChanged (true);
        }
      }
      else {
        // 返回内容不存在accessToken，清除无效的token信息
        debug_boot << "The returned content does not contain an accessToken; "
                      "clearing invalid token information."
                   << "\n";
        clearInvalidTokens ();
      }
    }
    else {
      debug_boot << "ERROR: Network error during refresh:"
                 << from_qstring (reply->errorString ()) << "\n";
    }

    reply->deleteLater ();
    manager->deleteLater ();
  });
}

void
QTMOAuth::checkTokenStatus () {
  if (oauth2.token ().isEmpty ()) {
    if (m_isLoggedIn) {
      m_isLoggedIn= false;
      emit loginStateChanged (false);
    }
    return;
  }

  qint64 currentTime= QDateTime::currentSecsSinceEpoch ();

  // 检查token是否已过期
  if (m_tokenExpiryTime > 0 && m_tokenExpiryTime <= currentTime) {
    // Token已过期，需要刷新或清除
    refreshToken ();
    return;
  }

  // Token有效且未过期
  if (!m_isLoggedIn) {
    m_isLoggedIn= true;
    emit loginStateChanged (true);
  }

  // 如果token将在3分钟内过期，自动刷新
  if (m_tokenExpiryTime - currentTime <= 180) { // 3分钟
    refreshToken ();
  }
}

void
QTMOAuth::loadExistingToken () {
  eval ("(use-modules (liii account))");

  // 加载access_token
  c_string tokenStr (as_string (call ("account-load-token")));
  QString  token= QString ((char*) tokenStr);
  if (!token.isEmpty ()) {
    oauth2.setToken (token);
  }

  // 加载refresh_token
  c_string refreshTokenStr (as_string (call ("account-load-refresh-token")));
  m_refreshToken= QString ((char*) refreshTokenStr);

  // 加载token过期时间
  c_string expiryStr (as_string (call ("account-load-token-expiry")));
  QString  expiryTimeStr= QString ((char*) expiryStr);
  if (!expiryTimeStr.isEmpty ()) {
    m_tokenExpiryTime= expiryTimeStr.toLongLong ();
  }

  checkTokenStatus ();
}

void
QTMOAuth::clearInvalidTokens () {
  eval ("(use-modules (liii account))");
  call ("account-clear-tokens");

  // 清除内存中的token信息
  oauth2.setToken ("");
  m_refreshToken.clear ();
  m_tokenExpiryTime= 0;
  m_isLoggedIn     = false;

  // 发出登录状态变化信号
  emit loginStateChanged (false);
}

QString
QTMOAuth::generateCodeVerifier () {
  // 生成43-128个字符的随机字符串
  const QString possibleCharacters (
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~");
  const int length= 64; // 推荐长度

  QString randomString;
  for (int i= 0; i < length; ++i) {
    int index=
        QRandomGenerator::global ()->bounded (possibleCharacters.length ());
    QChar nextChar= possibleCharacters.at (index);
    randomString.append (nextChar);
  }

  return randomString;
}

QString
QTMOAuth::generateCodeChallenge (const QString& verifier) {
  // 使用SHA-256哈希code_verifier，然后进行base64url编码
  QByteArray verifierBytes= verifier.toUtf8 ();
  QByteArray hash=
      QCryptographicHash::hash (verifierBytes, QCryptographicHash::Sha256);

  // Base64 URL编码（替换+为-，/为_，移除=填充）
  QString base64= hash.toBase64 (QByteArray::Base64UrlEncoding |
                                 QByteArray::OmitTrailingEquals);

  return base64;
}

QUrl
QTMOAuth::getAuthorizationUrl () {
  eval ("(use-modules (liii account))");
  c_string authorizationUrl (
      as_string (call ("account-oauth2-config", "authorization-url")));
  return QUrl ((char*) authorizationUrl);
}

QUrl
QTMOAuth::getAccessTokenUrl () {
  eval ("(use-modules (liii account))");
  c_string accessTokenUrl (
      as_string (call ("account-oauth2-config", "access-token-url")));
  return QUrl ((char*) accessTokenUrl);
}
