/******************************************************************************
 * MODULE     : QTMSingleInstance.hpp
 * DESCRIPTION: Linux single-instance support for double-clicking .tmu files.
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTM_SINGLE_INSTANCE_HPP
#define QTM_SINGLE_INSTANCE_HPP

// 双击 .tmu 时桌面环境会启动一个全新进程（MoganSTEM.desktop 用 Exec=moganstem
// %U，
// 无单实例封装）。本模块让第二个进程把待打开的文件路径转发给已运行实例，由后者在
// 当前窗口新开标签页，随后第二个进程自行退出。这样既复用已有窗口，又避免新进程命中
// acquire_boot_lock() 误删运行中实例的 settings 与缓存而导致崩溃。macOS 走
// QFileOpenEvent 路径，无需本模块。

#include <QObject>

class QLocalServer;

// 客户端：在 main() 早期（QApplication 构造之后、init_texmacs() 之前）调用。
// 若已有实例在运行，把 argv 中的位置参数文件（绝对路径）经 socket 转发过去，
// 等待 ACK 后返回 true——调用方应据此 return 0 退出，避免二次初始化。
// 若没有实例在运行（连不上 socket），返回 false，调用方继续正常启动。
// 非 Linux 平台恒返回 false（无单实例机制），保持既有行为。
bool mogan_forward_to_running_instance (int argc, char** argv);

// 服务端：在已运行实例侧（TeXmacs_main 里 gui_open/ensure_window 之后）调用
// 一次，开始监听单实例 socket。收到文件路径后用 load-buffer（默认
// :current-window 路径）在当前窗口新开标签页。非 Linux 平台为 no-op。
void mogan_start_single_instance_server ();

// 服务端对象（需 Q_OBJECT 以接收 QLocalServer::newConnection 信号）。声明不平台
// 条件化，确保 xmake 的 qt.moc 规则总能为它生成元对象代码；其实现仅在 Linux 上
// 存在，非 Linux 平台无人引用、链接器会丢弃无用符号。
class QTMSingleInstanceServer : public QObject {
  Q_OBJECT
public:
  explicit QTMSingleInstanceServer (QObject* parent= nullptr);
  // 创建监听 socket 并连接 newConnection 信号。监听失败（路径过长/权限等）返回
  // false，调用方据此退化为"每次双击开新进程"。
  bool start ();
private slots:
  void handle_connection ();

private:
  QLocalServer* server;
};

#endif // QTM_SINGLE_INSTANCE_HPP
