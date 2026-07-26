/******************************************************************************
 * MODULE     : QTMSingleInstance.hpp
 * DESCRIPTION: Single-instance support for double-clicking .tmu files.
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTM_SINGLE_INSTANCE_HPP
#define QTM_SINGLE_INSTANCE_HPP

// 双击 .tmu 时桌面环境会启动一个全新进程（Linux 的 MoganSTEM.desktop 用
// Exec=moganstem %U；Windows 的注册表 Assoc 也启动新进程）。本模块让第二个
// 进程把待打开的文件路径转发给已运行实例，由后者在当前窗口新开标签页，随后
// 第二个进程自行退出。这样既复用已有窗口，又避免新进程命中 acquire_boot_lock()
// 误删运行中实例的 settings 与缓存而导致崩溃。macOS 走 QFileOpenEvent 路径，
// 不需要本模块。
//
// 平台覆盖：Linux 与 Windows。两者 QLocalServer/QLocalSocket 均为 Qt 原生支持
// （Linux 走 Unix domain socket，Windows 走命名管道）。非上述平台为空桩。

#include <QObject>

class QLocalServer;

// 客户端：在 main() 早期（QApplication 构造之后、init_texmacs() 之前）调用。
// 若已有实例在运行，把 argv 中的位置参数文件（绝对路径）经 socket 转发过去，
// 等待 ACK 后返回 true——调用方应据此 return 0 退出，避免二次初始化。
// 若没有实例在运行（连不上 socket），返回 false，调用方继续正常启动。
// 不支持单实例的平台恒返回 false，保持既有行为。
bool mogan_forward_to_running_instance (int argc, char** argv);

// 服务端：在已运行实例侧（TeXmacs_main 里 gui_open/ensure_window 之后）调用
// 一次，开始监听单实例 socket。收到文件路径后用 load-buffer（默认
// :current-window 路径）在当前窗口新开标签页。不支持的平台为 no-op。
void mogan_start_single_instance_server ();

// 协议：客户端把每条待打开文件的绝对路径作为一行 UTF-8 文本（行尾 '\n'）写入
// socket，全部写完后 flush 并立即关闭连接。服务端累加缓冲、按 '\n' 切行，以
// "读到 EOF（客户端关闭连接）" 作为一条完整消息的边界；处理完即可清理。
// 不使用 ACK——客户端进程本来就要退出，ACK 在此场景下是冗余的，去掉它同时也
// 消除了"客户端等 ACK 不主动断 / 服务端等 disconnect 才发 ACK"的潜在死锁。

// 服务端对象（需 Q_OBJECT 以接收 QLocalServer::newConnection 信号）。声明不平台
// 条件化，确保 xmake 的 qt.moc 规则总能为它生成元对象代码；其实现仅在支持平台
// 上有实质逻辑，其它平台为空桩以满足链接。
class QTMSingleInstanceServer : public QObject {
  Q_OBJECT
public:
  explicit QTMSingleInstanceServer (QObject* parent= nullptr);
  // 创建监听 socket 并连接 newConnection 信号。监听失败（路径过长/权限等）返回
  // false，调用方据此退化为"每次双击开新进程"。
  bool start ();
private slots:
  // 新连接到达：取下一个待处理连接，挂上 readyRead/disconnected 信号驱动读取
  // （不阻塞 GUI 主线程）。
  void handle_connection ();
  // 缓冲区有新数据可读：累加到内部缓冲，并在缓冲里按 '\n' 切行处理。
  void read_from_client ();
  // 客户端断开或兜底超时：清理 pending 连接。
  void discard_client ();

protected:
  // 兜底定时器：客户端崩溃不 disconnect 时强制回收悬挂连接。
  void timerEvent (QTimerEvent* e) override;

private:
  QLocalServer*       server;
  class QLocalSocket* pending; // 当前正在读取的客户端连接
  class QByteArray*   buffer;  // pending 对应的累加缓冲（跨 readyRead 片段）
  int                 timer_id; // 兜底定时器 ID，discard 时 killTimer
};

#endif // QTM_SINGLE_INSTANCE_HPP
