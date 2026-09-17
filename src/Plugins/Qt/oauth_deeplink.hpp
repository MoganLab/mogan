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
QT_FORWARD_DECLARE_CLASS (QUrl)

/**
 * @file oauth_deeplink.hpp
 * @brief 浏览器经 liiistem:// 自定义协议把软件唤回前台。
 *
 * 登录职能仍归环回地址（见 QTMOAuth::login）：深链只做一件事——用户点完授权
 * 后把软件窗口提到前台。需要它的原因是环回回调发生在后台，回调页一跳走，用户
 * 眼前就只剩浏览器，软件是否登录成功没有任何可见反馈。
 *
 * 唤醒由官网成长激励页（回调页跳转过去后的稳定 origin）发起，URL 形如
 * `liiistem://wake?instance=<instanceId>`，其中 `instance` 是发起登录那个
 * 进程的标识。两个平台的投递方式不同，差异收在本模块内：
 *
 * - Windows：浏览器按注册表里那行命令行新起一个进程，URL 在 argv 中同步
 *   可得，该进程只做转发随后退出（handle_launch / try_forward）；
 * - macOS：LaunchServices 把 URL 投给已运行的实例，没有则先把 App 拉起
 *   再以 QFileOpenEvent 异步投递（handle_open_url）。因此 macOS 上不需要
 *   转发，但只要出现「新进程被拉起」就说明发起登录的实例已经没了。
 *
 * 纯解析部分不依赖平台与 I/O，便于单独测试。
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
 * @brief 判断一次唤醒是否指向给定实例
 *
 * 实例标识为空（本进程尚未登记，例如深链在启动期就到达）时恒为 false：
 * 「不知道自己是哪个实例」不能当成「就是给本进程的」。
 */
bool is_wake_for (const QString& url, const QString& instance_id);

/// 登记本进程的实例标识（QTMOAuth 构造时调用；仅 macOS 使用，其它平台空操作）
void set_local_instance_id (const QString& instance_id);

/**
 * @brief 事件循环即将开始（GUI 就绪后调用一次）
 *
 * handle_open_url 用它区分「本进程刚被深链拉起」与「用户正在用的实例收到了发给
 * 别的实例的唤醒」——前者要退出，后者绝不能退出。
 */
void mark_loop_started ();

/**
 * @brief 处理系统投递的「用本应用打开一条 URL」事件（macOS 的 QFileOpenEvent）
 *
 * macOS 不新起转发进程：URL 要么投给已运行的实例，要么由 LaunchServices 先把本
 * 进程拉起、再把事件异步投进来（实测事件比主窗口晚约 0.3 秒到达）。因此这里的
 * 判据只能是 instance：
 *
 * - 指向本实例 → 一次正常唤醒，把窗口提到前台；
 * - 不指向本实例，且本进程刚起来（事件循环才开始几百毫秒）→ 本进程就是被
 *   这条深链拉起的：发起登录的实例已经没了，用户没要这个窗口，记日志后退出；
 * - 不指向本实例，但本进程已经跑了一阵 → 用户正在用的实例收到了别人的唤醒
 *   （多开，或有人直接打开了一条唤醒链接）。既不置前也不退出：退出会连带
 *   杀掉用户手里未保存的工作。
 *
 * 非 macOS 平台恒返回 false：那些平台的 URL 不走 FileOpen 事件。
 *
 * @return true 表示这条 URL 已由深链接管，调用方不要再按普通文件处理
 */
bool handle_open_url (const QUrl& url);

/**
 * @brief 启动期接入深链：写协议注册，并处理「本进程由深链拉起」的情况
 *
 * 注册、转发、接收三处平台差异都收在本文件内，调用方不含平台宏。
 * 非 Windows 上注册是空操作，命令行里也不会有深链 URL：macOS 的 URL 以
 * QFileOpenEvent 异步到达，接入点在 QTMGuiHelper，不在这里。
 *
 * 命令行里带深链 URL，就说明本进程是被浏览器拉起来送唤醒的，它只做转发，不承担
 * 启动职责；转发不成（发起实例已退出）也只是静默退出，不开窗口。
 *
 * @return true 表示本进程由深链拉起、调用方应立即退出（无论转发是否成功）
 */
bool handle_launch ();

/**
 * @brief 把 URL 转发给发起登录的那个实例
 *
 * 连不上目标实例（例如用户授权途中关掉了软件）时返回 false，此时唤醒丢失、没有
 * 补救动作。登录本身走环回地址，与这条转发的成败无关。
 *
 * @return true 表示已送达目标实例
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

/// Windows 注册表补写协议注册；非 Windows 与社区版为空操作
void ensure_registered ();

} // namespace oauth_deeplink

#endif // OAUTH_DEEPLINK_HPP
