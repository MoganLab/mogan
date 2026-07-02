
/******************************************************************************
 * MODULE     : base.hpp
 * DESCRIPTION: header file for test purpose
 * COPYRIGHT  : (C) 2022  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TM_TEST_BASE_HPP
#define TM_TEST_BASE_HPP

#include "string.hpp"
#include "sys_utils.hpp"

void qcompare (string actual, string expected);
void init_lolly ();

// 隐藏并清理所有可见的顶层 Qt 窗口。
// 用在 QtTest 的 cleanup() 槽里，避免断言失败导致 widget 泄漏、窗口持续显示
// 而卡住整个测试套件（Windows 上表现为 0xC000013A DLL 初始化失败）。
void cleanup_qt_top_level_widgets ();

#endif
