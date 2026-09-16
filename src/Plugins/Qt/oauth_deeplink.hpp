/******************************************************************************
 * MODULE     : oauth_deeplink.hpp
 * DESCRIPTION: liiistem:// 自定义协议深链的接收、解析与实例路由
 * COPYRIGHT  : (C) 2026  MoonL79
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef OAUTH_DEEPLINK_HPP
#define OAUTH_DEEPLINK_HPP

#include <QString>
#include <QStringList>

#include <functional>

QT_FORWARD_DECLARE_CLASS (QObject)

/**
 * @file oauth_deeplink.hpp
 * @brief 浏览器完成 OAuth 授权后，经 liiistem:// 自定义协议把回调交回本进程。
 *
 * 回调可能落到一个新的进程（浏览器总是新拉起一个进程），而发起登录的实例可能
 * 仍在运行。路由依据是 state 里的实例标识：state 的格式为
 * `<instanceId>.<一次性随机数>`，回调 URL
 * 因此天然带着「我是从哪个实例发起的」。
 *
 * 纯解析部分不依赖平台与 I/O，便于单独测试；转发部分目前只在 Windows 生效
 * （见 oauth_deeplink.cpp）。
 */

namespace oauth_deeplink {

/// 协议 scheme（Windows 把 scheme 与 host 规范化为小写后再交给进程）
inline constexpr const char* kScheme= "liiistem";

/// 深链回调地址，同时用作 custom scheme 通道下的 redirect_uri
inline constexpr const char* kRedirectUri= "liiistem://callback";

/// 每实例接收转发 URL 的本地 socket 名前缀
inline constexpr const char* kUrlServerPrefix= "LiiiSTEM-URL-";

/**
 * @brief 判断 URL 是否 liiistem 协议下的一次 OAuth 回调
 *
 * Windows 会规范化 custom scheme URL（scheme/host 转小写、query 前补 `/`），
 * 因此这里 scheme 大小写不敏感，且只认 code 与 state 两个查询参数，
 * 不依赖 host 与 path。
 */
bool is_oauth_callback (const QString& url);

/// 从回调 URL 的 state 中取出实例标识；格式不符时返回空串
QString instance_id_from_url (const QString& url);

/// 在命令行参数中找出 liiistem:// URL；没有则返回空串
QString find_url (const QStringList& arguments);

/**
 * @brief 把 URL 转发给发起登录的那个实例，并结束本进程
 *
 * 连不上目标实例（例如用户登录途中关掉了软件）时返回 false，调用方继续正常
 * 启动即可——这个 code 换不出 token，本进程无从接手：PKCE 的 code_verifier
 * 只存在于发起实例的内存里，授权请求里的 state 也是它生成的。用户在新窗口
 * 重新登录一次即可，比让一个没跑过 login () 的进程硬换更省事也更安全。
 *
 * @return true 表示已转发并退出（调用方不应再继续启动）
 */
bool try_forward (const QString& url);

/// 本进程自己的 redirect_uri（深链通道下两处必须完全一致）
QString redirect_uri ();

/**
 * @brief 把主窗口提到前台
 *
 * 深链到达时前台焦点仍在浏览器上，不置前就是「浏览器亮着、软件在后台静默登录
 * 完成」。与 try_forward 里的 AllowSetForegroundWindow 是一次交接的两半：没有
 * 那次授权，这里的 activateWindow () 只会闪一下任务栏按钮。
 */
void bring_to_front ();

/// 收到一条深链 URL 时的处理入口
using UrlHandler= std::function<void (const QString&)>;

/// 运行期按实例标识起本地 socket，接收其它进程转发来的 URL
void start_receiver (QObject* owner, const QString& instance_id,
                     QObject* target, const UrlHandler& handler);

/// Windows 注册表补写协议注册；非 Windows 为空操作
void ensure_registered ();

/**
 * @brief 深链通道此刻是否可用——即浏览器点 liiistem:// 会不会拉起本进程
 *
 * 探测而非假定：只有注册表中存在完整可用的协议注册，且 shell\open\command
 * 指向的正是本进程的可执行文件时才算可用。指向别的 LiiiSTEM 副本时不算——
 * 那个副本未必带得动本版的路由逻辑（见 oauth_deeplink_registry.cpp）。
 *
 * 非 Windows 恒为 false。
 */
bool deep_link_usable ();

} // namespace oauth_deeplink

#endif // OAUTH_DEEPLINK_HPP
