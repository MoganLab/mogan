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
 * @brief 浏览器经 liiistem:// 自定义协议把软件唤回前台。
 *
 * 登录职能仍归环回地址（见 QTMOAuth::login）：深链只做一件事——用户点完授权
 * 后把软件窗口提到前台。需要它的原因是环回回调发生在后台，回调页一跳走，用户
 * 眼前就只剩浏览器，软件是否登录成功没有任何可见反馈。
 *
 * 唤醒由官网成长激励页（回调页跳转过去后的稳定 origin）发起，URL 形如
 * `liiistem://wake?instance=<instanceId>`。浏览器总会新拉起一个进程，而发起
 * 登录的实例可能仍在运行，故带上实例标识做路由。
 *
 * 纯解析部分不依赖平台与 I/O，便于单独测试；转发部分目前只在 Windows 生效
 * （见 oauth_deeplink.cpp）。
 */

namespace oauth_deeplink {

/// 协议 scheme（Windows 把 scheme 与 host 规范化为小写后再交给进程）
inline constexpr const char* kScheme= "liiistem";

/// 每实例接收转发 URL 的本地 socket 名前缀
inline constexpr const char* kUrlServerPrefix= "LiiiSTEM-URL-";

/**
 * @brief 判断 URL 是否一次 liiistem:// 唤醒
 *
 * Windows 会规范化 custom scheme URL（scheme/host 转小写、query 前补 `/`），
 * 因此这里 scheme 大小写不敏感，且只认 instance 一个查询参数，
 * 不依赖 host 与 path。
 */
bool is_wake (const QString& url);

/// 从唤醒 URL 中取出实例标识；缺失时返回空串
QString instance_id_from_url (const QString& url);

/// 在命令行参数中找出 liiistem:// URL；没有则返回空串
QString find_url (const QStringList& arguments);

/**
 * @brief 把 URL 转发给发起登录的那个实例，并结束本进程
 *
 * 连不上目标实例（例如用户授权途中关掉了软件）时返回 false，调用方继续正常
 * 启动即可：目标窗口都没了，也就无所谓置前。登录本身走环回地址，与这条转发
 * 的成败无关。
 *
 * @return true 表示已转发并退出（调用方不应再继续启动）
 */
bool try_forward (const QString& url);

/**
 * @brief 把主窗口提到前台
 *
 * 深链到达时前台焦点仍在浏览器上，不置前就是「浏览器亮着、软件在后台悄无声息
 * 地完成了登录」。与 try_forward 里的 AllowSetForegroundWindow 是一次交接的
 * 两半：没有那次授权，这里的 activateWindow () 只会闪一下任务栏按钮。
 */
void bring_to_front ();

/// 收到一条深链 URL 时的处理入口
using UrlHandler= std::function<void (const QString&)>;

/// 运行期按实例标识起本地 socket，接收其它进程转发来的 URL
void start_receiver (QObject* owner, const QString& instance_id,
                     QObject* target, const UrlHandler& handler);

/// Windows 注册表补写协议注册；非 Windows 为空操作
void ensure_registered ();

} // namespace oauth_deeplink

#endif // OAUTH_DEEPLINK_HPP
