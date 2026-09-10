
/******************************************************************************
 * MODULE     : process_guard.hpp
 * DESCRIPTION: guard against applying updates while other instances run
 * COPYRIGHT  : (C) 2026 Mogan
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef PROCESS_GUARD_HPP
#define PROCESS_GUARD_HPP

/**
 * @brief 检测是否存在其他共享本安装目录的 Mogan 进程。
 *
 * Velopack 更新器只等待触发更新的进程退出即替换安装目录；若另一实例仍在
 * 运行，其占用的文件会使替换失败并损坏安装。启动钩子与手动应用更新前用
 * 本函数拦截（见 devel/0970.md）。
 *
 * 按安装目录键匹配而非进程名：不同安装位置（安装版 + 便携版并行）互不
 * 拦截，只有同目录的实例才共享即将被替换的文件。无状态实现（实时枚举
 * 进程），无崩溃残留问题。可在任何初始化之前调用（仅依赖系统 C API）。
 *
 * @return true 表示存在其他共享本安装目录的 Mogan 进程；非 Windows/macOS
 *         平台（无 Velopack 更新）恒为 false。
 */
bool has_other_mogan_instances ();

#endif // PROCESS_GUARD_HPP
