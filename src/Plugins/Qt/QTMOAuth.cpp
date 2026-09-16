
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
#include "oauth_deeplink.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "telemetry.hpp"
#include "tm_sys_utils.hpp"

#include <QtGui/qdesktopservices.h>

#include <QtNetwork/qlocalserver.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qrestaccessmanager.h>
#include <QtNetwork/qrestreply.h>

#include <QtCore/qcryptographichash.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qrandom.h>
#include <QtCore/qurlquery.h>

#include <QtNetwork/qhostaddress.h>

QTMOAuth::QTMOAuth (QObject* parent) {
  // 加载 OAuth2 配置
  eval ("(use-modules (account liii))");

  c_string clientIdentifier (
      as_string (call ("account-oauth2-config", "client-identifier")));
  // c_string clientSecret (
  //     as_string (call ("account-oauth2-config", "client-secret")));
  c_string scope (as_string (call ("account-oauth2-config", "scope")));

  // 回调服务器监听端口 0：由系统分配临时端口（RFC 8252 §7.3），多实例天然互不
  // 冲突，实际端口经 m_reply->callback () 反映到 redirect_uri
  m_reply= new QOAuthHttpServerReplyHandler (
      QHostAddress (QString::fromUtf8 ("127.0.0.1")), 0, this);
  m_reply->setCallbackPath ("/callback");
  // 构造即监听是该 handler 的默认行为；这里立刻关掉，改为 login () 时按需监听、
  // 收到回调或超时后关闭（见 closeCallbackServer），避免没有登录需求时端口
  // 一直敞着
  m_reply->close ();

  // 生成PKCE参数
  m_codeVerifier = generateCodeVerifier ();
  m_codeChallenge= generateCodeChallenge (m_codeVerifier);

  // 本进程的唯一标识：随 state 发给授权服务器并由回调原样带回，用于把回调
  // 路由回发起登录的那个实例（多实例场景，见 handleCallback）
  m_instanceId= generateRandomString (16);

  // 登录回调的 HTML 内容由 refreshCallbackHtml() 生成，会在登录时按需读取
  // account.scm 的 growth-url，跟随 stem-profile 在 production/staging/local
  // 之间切换。这里首次生成一份作为启动默认值。
  refreshCallbackHtml ();

  oauth2.setReplyHandler (m_reply);
  oauth2.setScope ((char*) scope);
  oauth2.setClientIdentifier ((char*) clientIdentifier);

  connect (&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this,
           &QDesktopServices::openUrl);

  // 连接回调URL捕获信号
  connect (m_reply, &QOAuthHttpServerReplyHandler::callbackReceived, this,
           [this] (const QVariantMap& values) {
             // 回调已经到手，但浏览器还在等回调页写回（redirect 到成长激励页的
             // HTML）：延迟 3 秒再关监听，给系统代理或杀软包检测留足时间，
             // 否则响应可能还没发出去端口就没了。
             // 使用成员定时器以便在用户快速再次登录时能被取消
             m_callbackCloseTimer->start (3000);
             handleCallback (values);
           });

  // 初始化定时器用于定期检查token状态
  m_tokenCheckTimer= new QTimer (this);
  connect (m_tokenCheckTimer, &QTimer::timeout, this,
           &QTMOAuth::checkTokenStatus);
  m_tokenCheckTimer->start (90000); // 每一分半检查一次

  // 登录超时：login () 发起后 5 分钟还没等到回调（用户一直没完成授权）就关闭
  // 回调服务器；下一次 login () 会重新监听
  m_loginTimer= new QTimer (this);
  m_loginTimer->setSingleShot (true);
  connect (m_loginTimer, &QTimer::timeout, this,
           &QTMOAuth::closeCallbackServer);

  // 延迟关闭定时器：收到回调后延迟 3 秒关闭回调服务器，留足写回响应的时间
  m_callbackCloseTimer= new QTimer (this);
  m_callbackCloseTimer->setSingleShot (true);
  connect (m_callbackCloseTimer, &QTimer::timeout, this,
           &QTMOAuth::closeCallbackServer);

  // 接收深链转发：浏览器总会新拉起一个进程，由它把回调 URL 送到发起登录的
  // 实例（见 oauth_deeplink.hpp 的实例路由）
  startUrlRouter ();

  // 加载现有的token信息
  loadExistingToken ();
}

// 按本实例的标识起本地 socket，接收其它进程（浏览器拉起的转发进程）送来的
// 深链 URL。名字含实例标识，多开时天然不撞，因此不需要「主实例」概念，
// 也不改变现有允许多开的策略。
void
QTMOAuth::startUrlRouter () {
  oauth_deeplink::start_receiver (
      this, m_instanceId, this,
      [this] (const QString& url) { handleDeepLink (url); });
}

// 深链回调的唯一入口：与环回回调共用 handleCallback，因此 state 校验、
// 一次性清空、延迟关闭等逻辑全部沿用，不新增第二套校验
void
QTMOAuth::handleDeepLink (const QString& url) {
  if (!oauth_deeplink::is_oauth_callback (url)) return;

  // 先置前再换 token：用户刚在浏览器里点完授权，视线该落回软件上
  oauth_deeplink::bring_to_front ();

  QUrl            parsed (url);
  const QUrlQuery query (parsed);
  QVariantMap     values;
  values["code"] = query.queryItemValue ("code");
  values["state"]= query.queryItemValue ("state");

  handleCallback (values);
}

void
QTMOAuth::login () {
  // 如果上一轮登录已收到回调且处于 3 秒延迟关闭等待中，立即中止延迟关闭并
  // 关掉旧监听，避免上一轮的延迟关闭在当前新流程中意外触发并掐断新端口
  if (m_callbackCloseTimer->isActive ()) {
    m_callbackCloseTimer->stop ();
    if (m_reply->isListening ()) m_reply->close ();
  }

  // 探测协议注册，可用则优先走深链：浏览器直接拉起软件，用户不必盯着「正在
  // 等待回调」的页面，也不必担心防火墙拦掉本地端口。探测的时机在建注册之后
  // ——research.cpp 启动时已补写过，故自动更新上来的老用户同样能立刻用上
  m_deepLinkChosen= oauth_deeplink::deep_link_usable ();

  // 环回服务器照旧备着：深链不可用时它是唯一出路。监听失败不再中断整个流程，
  // 剩下的那条通道仍可能打通
  if (!m_reply->isListening () &&
      !m_reply->listen (QHostAddress (QString::fromUtf8 ("127.0.0.1")), 0))
    debug_boot << "OAuth callback server failed to listen" << "\n";

  // 固化本次登录的 redirect_uri：授权请求与令牌交换两处必须逐字节一致
  // （RFC 6749 §4.1.3），故一次性确定，之后不再随运行时状态变化。
  // 选定的通道收不到码时另一条也收不到——服务端只认请求里那一个地址
  m_redirectUri= (m_deepLinkChosen || !m_reply->isListening ())
                     ? oauth_deeplink::redirect_uri ()
                     : m_reply->callback ();

  // 按当前 stem-profile 刷新回调页（让 profile 切换在下次登录立即生效）
  refreshCallbackHtml ();
  // 手动构建授权URL
  QUrl      authUrl (getAuthorizationUrl ());
  QUrlQuery query;
  query.addQueryItem ("response_type", "code");
  query.addQueryItem ("client_id", oauth2.clientIdentifier ());
  query.addQueryItem ("redirect_uri", getRedirectUri ());
  query.addQueryItem ("scope", oauth2.scope ());
  query.addQueryItem ("code_challenge", m_codeChallenge);
  query.addQueryItem ("code_challenge_method", "S256");
  // 每次登录重新生成 state：实例标识 + 一次性随机数。随机数做 CSRF 防护
  // （回调必须原样带回，见 handleCallback），实例标识供后续 liiistem://
  // 深链把回调路由回发起登录的那个实例
  m_state= m_instanceId + "." + generateRandomString (32);
  query.addQueryItem ("state", m_state);

  authUrl.setQuery (query);
  // 手动打开浏览器进行授权
  QDesktopServices::openUrl (authUrl);
  m_loginTimer->start (5 * 60 * 1000); // 5 分钟没有回调就收摊
}

// 关闭回调服务器并停掉登录超时：登录流程结束（收到回调或超时）后不再监听，
// 端口立即释放
void
QTMOAuth::closeCallbackServer () {
  m_callbackCloseTimer->stop ();
  m_loginTimer->stop ();
  if (m_reply->isListening ()) m_reply->close ();
}

bool
QTMOAuth::isLoggedIn () {
  return m_isLoggedIn;
}

// 回调的唯一入口：环回回调与（后续的）liiistem:// 深链都走这里。
// 先校验 state 再取 code：回调带回的 state 与本次登录发出的不一致，说明这个
// 回调不是本次登录发起的（CSRF 或重放），直接丢弃
void
QTMOAuth::handleCallback (const QVariantMap& values) {
  QString state= values.value ("state").toString ();
  if (m_state.isEmpty () || state != m_state) {
    debug_boot << "OAuth callback rejected: state mismatch" << "\n";
    return;
  }
  m_state.clear (); // 一次性：同一个 state 只接受一次回调

  if (values.contains ("code")) {
    // 手动处理授权码交换
    handleAuthorizationCode (values["code"].toString ());
  }
}

void
QTMOAuth::handleAuthorizationCode (const QString& code) {
  // 手动交换授权码为访问令牌
  QUrl      url (getAccessTokenUrl ());
  QUrlQuery query;
  query.addQueryItem ("grant_type", "authorization_code");
  query.addQueryItem ("code", code);
  query.addQueryItem ("redirect_uri", getRedirectUri ());
  query.addQueryItem ("client_id", oauth2.clientIdentifier ());
  query.addQueryItem ("code_verifier", m_codeVerifier);

  QNetworkRequest request (url);
  request.setHeader (QNetworkRequest::ContentTypeHeader,
                     "application/x-www-form-urlencoded");
  request.setRawHeader ("User-Agent",
                        to_qstring (stem_user_agent ()).toUtf8 ());
  request.setRawHeader ("X-Device-Id",
                        to_qstring (stem_device_id ()).toUtf8 ());
  QByteArray previewCookie= getPreviewCookieHeader ();
  if (!previewCookie.isEmpty ()) request.setRawHeader ("Cookie", previewCookie);

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
        eval ("(use-modules (account liii))");
        call ("account-save-token", from_qstring (accessToken));

        // 设置登录状态
        m_isLoggedIn= true;

        // 记录 LOGIN 事件
        telemetry_track ("LOGIN");

        // 记录 OAUTH 事件
        telemetry_track ("OAUTH");

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
  QByteArray previewCookie= getPreviewCookieHeader ();
  if (!previewCookie.isEmpty ()) request.setRawHeader ("Cookie", previewCookie);

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
        eval ("(use-modules (account liii))");
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
          telemetry_track ("LOGIN");
          emit loginStateChanged (true);
        }
        else {
          // 记录 HEART_BEAT 事件
          telemetry_track ("HEART_BEAT");
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
    telemetry_track ("LOGIN");
    emit loginStateChanged (true);
  }

  // 如果token将在3分钟内过期，自动刷新
  if (m_tokenExpiryTime - currentTime <= 180) { // 3分钟
    refreshToken ();
  }
}

void
QTMOAuth::loadExistingToken () {
  eval ("(use-modules (account liii))");

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
  eval ("(use-modules (account liii))");
  call ("account-clear-tokens");

  // 清除内存中的token信息
  oauth2.setToken ("");
  m_refreshToken.clear ();
  m_tokenExpiryTime= 0;
  m_isLoggedIn     = false;

  // 发出登录状态变化信号
  emit loginStateChanged (false);
}

// 字母数字随机串（URL 安全）。刻意不含 "."：state 用
// "<实例标识>.<一次性随机数>" 拼接，"." 可无歧义地拆回两段
QString
QTMOAuth::generateRandomString (int length) {
  static const QString possibleCharacters (
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

  QString result;
  for (int i= 0; i < length; ++i) {
    int index=
        QRandomGenerator::global ()->bounded (possibleCharacters.length ());
    result.append (possibleCharacters.at (index));
  }

  return result;
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
  eval ("(use-modules (account liii))");
  c_string authorizationUrl (
      as_string (call ("account-oauth2-config", "authorization-url")));
  return QUrl ((char*) authorizationUrl);
}

QUrl
QTMOAuth::getAccessTokenUrl () {
  eval ("(use-modules (account liii))");
  c_string accessTokenUrl (
      as_string (call ("account-oauth2-config", "access-token-url")));
  return QUrl ((char*) accessTokenUrl);
}

// redirect_uri：授权请求与令牌交换两处必须使用完全一致的值（OAuth 2.0 规范）。
// 值在 login () 中一次性固化，此处不再读取回调服务器的实时状态——服务器会在
// 收到回调后延迟关闭，届时 callback () 返回空串。
//
// 兜底一句：环回回调只在 login () 起监听之后才可能到达、深链回调又先过 state
// 校验，两条路都走不到「m_redirectUri 为空」。留着只是不让空串流进令牌交换，
// 真流进去了服务端会明确报错，比静默换个地址安全
QString
QTMOAuth::getRedirectUri () {
  if (!m_redirectUri.isEmpty ()) return m_redirectUri;
  return m_reply->callback ();
}

QString
QTMOAuth::getGrowthUrl () {
  eval ("(use-modules (account liii))");
  c_string growthUrl (as_string (call ("account-oauth2-config", "growth-url")));
  return QString::fromUtf8 ((const char*) growthUrl);
}

QByteArray
QTMOAuth::getPreviewCookieHeader () {
  eval ("(use-modules (account liii))");
  c_string previewCookie (
      as_string (call ("account-oauth2-config", "preview-cookie-header")));
  return QByteArray ((const char*) previewCookie);
}

void
QTMOAuth::refreshCallbackHtml () {
  // 每次调用都按当前 stem-profile 现场读取 growth-url，避免启动时固化导致
  // profile 切换后回调仍跳到旧环境
  QString redirectUrl= getGrowthUrl ();
  QString customHtml = "<!doctype html><html><head>"
                       "<meta charset='utf-8'>"
                       "<title>登录成功</title>"
                       "</head><body>"
                       "<script>window.location.replace(\"" +
                      redirectUrl +
                      "\");</script>"
                      "<noscript><meta http-equiv='refresh' content='0;url=" +
                      redirectUrl +
                      "'></noscript>"
                      "</body></html>";
  m_reply->setCallbackText (customHtml);
}
